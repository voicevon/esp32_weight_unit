#include "MainController.h"
#include <Arduino.h>

MainController::MainController(WeighingScale& scale, TinyScreen& oled, 
                               ModbusSlave& modbus, int buttonPin, int servoPin)
    : _scale(scale), _oled(oled), _modbus(modbus), _btn(buttonPin),
      _buttonPin(buttonPin), _servoPin(servoPin),
      _state(SLAVE_INIT), _uiMode(UI_RUN), _currentId(1),
      _ztrThreshold(2), _doorWaitTime(1000), _stateTimer(0),
      _pendingTare(false), _calTargetWeight(0),
      _ztrIndex(1), _calWeightIndex(0) {}

void MainController::begin() {
    _btn.begin();
    _servo.attach(_servoPin);
    _servo.write(0);

    _prefs.begin("weight_unit", false);
    _currentId = _prefs.getInt("slave_id", 1);
    _ztrThreshold = _prefs.getInt("ztr_thresh", 2);
    _doorWaitTime = _prefs.getInt("door_time", 1000);

    _ztrIndex = 1; // Default to 2g
    for (int i = 0; i < 4; i++) {
        if (_ztrOptions[i] == _ztrThreshold) {
            _ztrIndex = i;
            break;
        }
    }
    _prefs.end();

    _oled.begin();
    _scale.begin();
    _modbus.begin(_currentId);
    _modbus.setZtrThreshold(_ztrThreshold);
    _modbus.setDoorTime(_doorWaitTime);

    _mutexComm = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(commTask, "ModbusTask", 4096, this, 10, &_commTaskHandle, 0);

    updateState(SLAVE_STANDBY, "Boot Complete");
}

void MainController::commTask(void* pvParameters) {
    MainController* instance = (MainController*)pvParameters;
    for(;;) {
        if (xSemaphoreTake(instance->_mutexComm, pdMS_TO_TICKS(20)) == pdTRUE) {
            // 诊模式下暂停 Modbus 协议栈处理，改为手动读写
            if (instance->_uiMode != UI_RS485_DIAG) {
                instance->_modbus.task();
            }
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
    bool ready = _scale.isReady();
    static unsigned long lastReadyTime = millis();
    
    if (ready) {
        lastReadyTime = millis();
        long raw = _scale.getRawValue();
        _scale.update(raw);
    } else if (millis() - lastReadyTime > 3000) {
        // 超过 3s 不 Ready，强制阻塞读取一次以尝试唤醒/同步硬件
        lastReadyTime = millis();
        long raw = _scale.getRawValue(); // 这是一个阻塞调用
        _scale.update(raw);
        Serial.println("[DIAG] Force-read triggered (Ready Timeout)");
    }

    static unsigned long lastLog = 0;
    if (millis() - lastLog > 500) {
        lastLog = millis();
        Serial.printf("[DIAG] Ready=%d, W=%.1fg, K=%.4f, Off=%ld\n", 
                      ready, _scale.getFilteredWeight(), _scale.getScaleFactor(), _scale.getOffset());
    }
}

void MainController::handleLogic() {
    unsigned long now = millis();

    switch (_state) {
        case SLAVE_DISCHARGING:
            if (now - _stateTimer >= (unsigned long)_doorWaitTime) {
                _servo.write(0);
                updateState(SLAVE_RECOVERY);
            }
            break;
        case SLAVE_RECOVERY:
            if (now - _stateTimer >= 200) updateState(SLAVE_STANDBY);
            break;
        case SLAVE_CALIBRATING:
            if (now - _stateTimer >= 1500) {
                _scale.calibrate(_calTargetWeight);
                updateState(SLAVE_STANDBY);
                _uiMode = UI_CALIB_RESULT; // 标定完成后显示结果页面，等待人工确认
            }
            break;
        default: break;
    }

    if (_pendingTare) {
        _scale.tare();
        _pendingTare = false;
        _oled.showMessage("TARE OK", 800);
    }
}

void MainController::handleComm() {
    if (xSemaphoreTake(_mutexComm, pdMS_TO_TICKS(5)) != pdTRUE) return;

    unsigned long now = millis();
    static unsigned long lastDiagUpdate = 0;

    // 链路统计逻辑：仅在诊断模式下读取真实字节，防止干扰正常 Modbus 协议栈
    if (_uiMode == UI_RS485_DIAG) {
        while (_modbus.availableRaw()) {
            _diagRxByte = _modbus.readRawByte(); 
            if (now - lastDiagUpdate > 50) { 
                _diagRxCount = (_diagRxCount + 1) % 1000;
                lastDiagUpdate = now; 
            }
        }
    }



    // 链路诊断模式逻辑
    if (_uiMode == UI_RS485_DIAG) {

        // 发送逻辑：1Hz 递增字节
        if (now - _lastDiagTime >= 1000) {
            _lastDiagTime = now;
            _diagTxCounter++;
            _modbus.sendRawByte(_diagTxCounter);
        }

        xSemaphoreGive(_mutexComm);
        return; 
    }

    uint16_t mZtr = _modbus.getZtrThreshold();
    if (mZtr != _ztrThreshold) {
        _ztrThreshold = mZtr;
        _prefs.begin("weight_unit", false);
        _prefs.putInt("ztr_thresh", _ztrThreshold);
        _prefs.end();
    }
    uint16_t mDoor = _modbus.getDoorTime();
    if (mDoor != _doorWaitTime) {
        _doorWaitTime = mDoor;
        _prefs.begin("weight_unit", false);
        _prefs.putInt("door_time", _doorWaitTime);
        _prefs.end();
    }

    uint16_t cmd = _modbus.getControlCommand();
    if (cmd != 0) {
        if (cmd == 1 || cmd == 5) {
            if (_state == SLAVE_STANDBY || _state == SLAVE_LOCKED) {
                _servo.write(90);
                _modbus.incrementDataId();
                updateState(SLAVE_DISCHARGING);
            }
        } else if (cmd == 2) {
            _servo.write(0);
            updateState(SLAVE_RECOVERY);
        } else if (cmd == 3) {
            _pendingTare = true;
        } else if (cmd == 4) {
            performCalibration(_calWeights[_calWeightIndex]);
        }
        _modbus.clearControlCommand();
    }

    float weight = _scale.getFilteredWeight();
    if (abs(weight) < (float)_ztrThreshold) weight = 0.0f;

    _modbus.updateWeight(weight);
    _modbus.updateStatus(_state, _scale.isStable(2.0f));
    _modbus.updateRawADC(_scale.getLastRaw());

    xSemaphoreGive(_mutexComm);
}

void MainController::handleUI() {
    ButtonEvent event = _btn.scan();
    
    if (event != BTN_NONE) {
        switch (event) {
            case BTN_CLICK:
                if (_uiMode == UI_RUN) _pendingTare = true;
                else if (_uiMode == UI_CONFIG_ID) _currentId = (_currentId % 20) + 1;
                else if (_uiMode == UI_CONFIG_ZTR) {
                    _ztrIndex = (_ztrIndex + 1) % 4;
                    _ztrThreshold = _ztrOptions[_ztrIndex];
                    
                    // 同步到 Modbus 和 Preferences，防止被 handleComm 覆盖
                    if (xSemaphoreTake(_mutexComm, pdMS_TO_TICKS(10)) == pdTRUE) {
                        _modbus.setZtrThreshold(_ztrThreshold);
                        xSemaphoreGive(_mutexComm);
                    }
                    _prefs.begin("weight_unit", false);
                    _prefs.putInt("ztr_thresh", _ztrThreshold);
                    _prefs.end();
                } else if (_uiMode == UI_MENU_CALIB) {
                    _calWeightIndex = (_calWeightIndex + 1) % 5;
                } else if (_uiMode == UI_CALIB_RESULT) {
                    _uiMode = UI_RUN; // 结果确认后回到主运行界面
                }
                break;
                
            case BTN_DOUBLE_CLICK:
                if (_uiMode == UI_RUN) {
                    _servo.write(90);
                    updateState(SLAVE_DISCHARGING);
                }
                break;
                
            case BTN_LONG_PRESS:
                if (_uiMode == UI_RUN) _uiMode = UI_CONFIG_ID;
                else if (_uiMode == UI_CONFIG_ID) {
                    _prefs.begin("weight_unit", false);
                    _prefs.putInt("slave_id", _currentId);
                    _prefs.end();
                    _modbus.begin(_currentId);
                    _uiMode = UI_CONFIG_ZTR;
                } else if (_uiMode == UI_CONFIG_ZTR) _uiMode = UI_MENU_CALIB;
                else if (_uiMode == UI_MENU_CALIB) {
                    int w = _calWeights[_calWeightIndex];
                    if (w == 0) _uiMode = UI_RS485_DIAG; // 标定退出后进入诊断
                    else performCalibration(w);
                } else if (_uiMode == UI_RS485_DIAG) {
                    _uiMode = UI_RUN; // 诊断退出回主界面
                }
                break;
            default: break;
        }
    }

    unsigned long now = millis();
    // 刷新频率提高：一秒约 30 帧 (33ms)
    if (now - _lastUIUpdateMillis >= 33) {
        _lastUIUpdateMillis = now;
        
        int displayParam = 0;
        float displayWeight = _scale.getFilteredWeight();
        int32_t displayADC = _scale.getLastRaw();

        if (_uiMode == UI_CONFIG_ZTR) displayParam = _ztrThreshold;
        else if (_uiMode == UI_MENU_CALIB) displayParam = _calWeights[_calWeightIndex];
        else if (_uiMode == UI_RS485_DIAG) displayParam = (_diagTxCounter << 8) | _diagRxByte;
        else if (_uiMode == UI_CALIB_RESULT) {
            displayWeight = _scale.getScaleFactor(); // 结果页显示斜率
            displayADC = _scale.getOffset();        // 结果页显示偏移
        }

        _oled.update(_uiMode, _state, displayWeight, displayADC, 
                        _currentId, false, displayParam, _scale.isStable(2.0f), _diagRxByte, _diagRxCount);
    }
}

void MainController::updateState(SlaveState newState, const char* reason) {
    if (_state == newState) return;
    _state = newState;
    _stateTimer = millis();
}

void MainController::performCalibration(int weight) {
    _calTargetWeight = weight;
    updateState(SLAVE_CALIBRATING);
}
