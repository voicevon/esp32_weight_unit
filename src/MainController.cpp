#include "MainController.h"
#include <Arduino.h>

MainController::MainController(ScaleComponent& scale, DisplayComponent& display, 
                               CommComponent& comm, int buttonPin, int servoPin)
    : _scale(scale), _display(display), _comm(comm), 
      _buttonPin(buttonPin), _servoPin(servoPin),
      _state(SLAVE_INIT), _uiMode(UI_RUN), _currentId(1),
      _ztrThreshold(2), _doorWaitTime(1000), _stateTimer(0),
      _pendingTare(false), _calTargetWeight(0), _lastUpdateMillis(0),
      _btnPressed(false), _btnPressStart(0), _lastBtnRelease(0),
      _longPressHandled(false), _ztrIndex(1), _calWeightIndex(0) {}

void MainController::begin() {
    pinMode(_buttonPin, INPUT_PULLUP);
    _servo.attach(_servoPin);
    _servo.write(0);

    _prefs.begin("weight_unit", false);
    _currentId = _prefs.getInt("slave_id", 1);
    _ztrThreshold = _prefs.getInt("ztr_thresh", 2);
    _doorWaitTime = _prefs.getInt("door_time", 1000);
    _prefs.end();

    _display.begin();
    _scale.begin();
    _comm.begin(_currentId);
    _comm.setZtrThreshold(_ztrThreshold);
    _comm.setDoorTime(_doorWaitTime);

    _mutexComm = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(commTask, "ModbusTask", 4096, this, 10, &_commTaskHandle, 0);

    updateState(SLAVE_STANDBY, "Boot Complete");
}

void MainController::commTask(void* pvParameters) {
    MainController* instance = (MainController*)pvParameters;
    for(;;) {
        if (xSemaphoreTake(instance->_mutexComm, pdMS_TO_TICKS(20)) == pdTRUE) {
            instance->_comm.task();
            xSemaphoreGive(instance->_mutexComm);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void MainController::loop() {
    handleSampling();
    handleLogic();
    handleComm();
    handleUI();
}

void MainController::handleSampling() {
    if (_scale.isReady()) {
        long raw = _scale.getRawValue();
        _scale.update(raw);
    }
}

void MainController::handleLogic() {
    unsigned long now = millis();

    switch (_state) {
        case SLAVE_STANDBY:
            // 等待 Modbus 指令或按钮触发
            break;

        case SLAVE_LOCKED:
            // 已被主站锁定，等待下料指令转换
            break;

        case SLAVE_DISCHARGING:
            if (now - _stateTimer >= (unsigned long)_doorWaitTime) {
                _servo.write(0);
                updateState(SLAVE_RECOVERY, "Wait for mechanical settle");
            }
            break;

        case SLAVE_RECOVERY:
            // 额外给 200ms 的震动消除期
            if (now - _stateTimer >= 200) {
                updateState(SLAVE_STANDBY, "Ready for next cycle");
            }
            break;

        case SLAVE_ZERO_TRACKING:
            break;

        case SLAVE_CALIBRATING:
            if (now - _stateTimer >= 1500) { // 增加标定前置抖动等待
                _scale.calibrate(_calTargetWeight);
                updateState(SLAVE_STANDBY, "Calibration Success");
                _uiMode = UI_RUN;
                _display.showMessage("SUCCESS", 1000);
            }
            break;

        default:
            break;
    }

    // 独立处理异步去皮请求
    if (_pendingTare) {
        _scale.tare();
        _pendingTare = false;
        _display.showMessage("TARE OK", 800);
    }
}

void MainController::handleComm() {
    if (xSemaphoreTake(_mutexComm, pdMS_TO_TICKS(5)) != pdTRUE) return;

    // 同步主机配置
    uint16_t mZtr = _comm.getZtrThreshold();
    if (mZtr != _ztrThreshold) {
        _ztrThreshold = mZtr;
        _prefs.begin("weight_unit", false);
        _prefs.putInt("ztr_thresh", _ztrThreshold);
        _prefs.end();
    }

    uint16_t mDoor = _comm.getDoorTime();
    if (mDoor != _doorWaitTime) {
        _doorWaitTime = mDoor;
        _prefs.begin("weight_unit", false);
        _prefs.putInt("door_time", _doorWaitTime);
        _prefs.end();
    }

    // 处理控制指令
    uint16_t cmd = _comm.getControlCommand();
    if (cmd != 0) {
        if (cmd == 1 || cmd == 5) { // OPEN / OPEN_1S
            if (_state == SLAVE_STANDBY || _state == SLAVE_LOCKED) {
                _servo.write(90);
                _comm.incrementDataId(); // 标记新一次下料动作，触发主站新鲜度检测
                updateState(SLAVE_DISCHARGING, "Modbus OPEN");
            }
        } else if (cmd == 2) { // CLOSE
            _servo.write(0);
            updateState(SLAVE_RECOVERY, "Modbus CLOSE");
        } else if (cmd == 3) { // TARE
            _pendingTare = true;
        } else if (cmd == 4) { // CALIBRATE
            performCalibration(_calWeights[_calWeightIndex]);
        }
        _comm.clearControlCommand();
    }

    // 同步状态到 Modbus
    float weight = _scale.getFilteredWeight();
    // 零点死区掩盖
    if (abs(weight) < (float)_ztrThreshold) weight = 0.0f;

    _comm.updateWeight(weight);
    _comm.updateStatus(_state, _scale.isStable(2.0f)); // 2.0g 容忍度
    _comm.updateRawADC(_scale.getRawValue());

    xSemaphoreGive(_mutexComm);
}

void MainController::handleUI() {
    unsigned long now = millis();
    
    // 限制刷新率
    if (now - _lastUpdateMillis >= 100) {
        _lastUpdateMillis = now;
        
        // 按钮扫描逻辑
        bool currentBtn = (digitalRead(_buttonPin) == LOW);
        if (currentBtn && !_btnPressed) {
            _btnPressed = true;
            _btnPressStart = now;
            _longPressHandled = false;
        } else if (currentBtn && _btnPressed) {
            if (!_longPressHandled && (now - _btnPressStart > 2000)) {
                _longPressHandled = true;
                _lastBtnRelease = 0;
                // 长按切换模式
                if (_uiMode == UI_RUN) _uiMode = UI_CONFIG_ID;
                else if (_uiMode == UI_CONFIG_ID) {
                    _prefs.begin("weight_unit", false);
                    _prefs.putInt("slave_id", _currentId);
                    _prefs.end();
                    _comm.begin(_currentId);
                    _uiMode = UI_CONFIG_ZTR;
                } else if (_uiMode == UI_CONFIG_ZTR) {
                    _uiMode = UI_MENU_CALIB;
                } else if (_uiMode == UI_MENU_CALIB) {
                    int w = _calWeights[_calWeightIndex];
                    if (w == 0) _uiMode = UI_VERSION;
                    else performCalibration(w);
                } else {
                    _uiMode = UI_RUN;
                }
            }
        } else if (!currentBtn && _btnPressed) {
            _btnPressed = false;
            if (!_longPressHandled && (now - _btnPressStart > 30)) {
                if (_uiMode == UI_RUN) {
                    _lastBtnRelease = now;
                } else {
                    // 短按调整参数
                    if (_uiMode == UI_CONFIG_ID) {
                        if (++_currentId > 20) _currentId = 1;
                    } else if (_uiMode == UI_CONFIG_ZTR) {
                        _ztrIndex = (_ztrIndex + 1) % 4;
                        _ztrThreshold = _ztrOptions[_ztrIndex];
                    } else if (_uiMode == UI_MENU_CALIB) {
                        _calWeightIndex = (_calWeightIndex + 1) % 5;
                    }
                }
            }
        } else if (!currentBtn && !_btnPressed) {
            if (_lastBtnRelease > 0 && (now - _lastBtnRelease > 400)) {
                _pendingTare = true; // 双击去皮
                _lastBtnRelease = 0;
            }
        }

        // 刷新显示
        int displayParam = 0;
        if (_uiMode == UI_CONFIG_ZTR) displayParam = _ztrThreshold;
        else if (_uiMode == UI_MENU_CALIB) displayParam = _calWeights[_calWeightIndex];
        
        _display.update(_uiMode, _state, _scale.getFilteredWeight(), _scale.getRawValue(), 
                        _currentId, false, displayParam, _scale.isStable(2.0f));
    }
}

void MainController::updateState(SlaveState newState, const char* reason) {
    if (_state == newState) return;
    _state = newState;
    _stateTimer = millis();
    Serial.printf("[FSM] State -> %d (%s)\n", (int)newState, reason);
}

void MainController::performCalibration(int weight) {
    _calTargetWeight = weight;
    updateState(SLAVE_CALIBRATING, "User Action");
}
