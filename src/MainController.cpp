#include "MainController.h"
#include <Arduino.h>
#include "config.h" // Added as per instruction

MainController::MainController(ScaleComponent& scale, DisplayComponent& display, 
                               CommComponent& comm, int buttonPin, int servoPin)
    : _scale(scale), _display(display), _comm(comm), 
      _buttonPin(buttonPin), _servoPin(servoPin),
      _currentState(STATE_RUN), _calWeightIndex(3), _currentId(1),
      _ztrThreshold(2), _ztrIndex(1),
      _btnPressStart(0), _lastBtnRelease(0), _btnPressed(false), 
      _longPressHandled(false), _servoIsOpen(false),
      _lastSendTime(0), _txCount(0), _rxData("") {}

void MainController::begin() {
    pinMode(_buttonPin, INPUT_PULLUP);
    _servo.attach(_servoPin);
    _servo.write(0); // Close by default
    
    _prefs.begin("weight_unit", false);
    _currentId = _prefs.getInt("slave_id", 1); // Default to 1
    
    _ztrThreshold = _prefs.getInt("ztr_thresh", 2); // Default to 2g
    if (_ztrThreshold == 0) _ztrIndex = 0;
    else if (_ztrThreshold <= 2) _ztrIndex = 1;
    else if (_ztrThreshold <= 5) _ztrIndex = 2;
    else _ztrIndex = 3;
    
    _scale.begin();
    _display.begin();
    _comm.begin(_currentId); // 初始化 Modbus Slave
    _comm.setZtrThreshold(_ztrThreshold); // 同步初始门限到 Modbus
    
    _display.showMessage("System Ready!", 1000);
}

void MainController::loop() {
    _comm.task(); // 维持 Modbus 协议栈运行
    
    handleButton();
    if (_currentState == STATE_COMM_TEST) {
        handleCommTest();
    } else if (_currentState != STATE_CONFIG_ID) {
        handleComm(); // 处理寄存器指令
    }
    unsigned long now = millis();
    if (now - _lastSendTime > 100) {
        long rawADC = _scale.getRawValue(); // 堵塞拿最新的 1 个采样，避免多次采样挂起超时
        float scaleF = _scale.getScaleFactor();
        float currentWeight = 0.0;
        
        // 核心修复：在这里统一用最新读到的 ADC 和比例尺演算重量，防止底层库干扰
        if (abs(scaleF) > 0.001) {
            currentWeight = (float)(rawADC - _scale.getOffset()) / scaleF;
            
            // 防零飘死区 (Zero Tracking Deadband)
            if (abs(currentWeight) <= (float)_ztrThreshold) {
                currentWeight = 0.0;
            }
        }

        _lastSendTime = now;
        bool commActive = (_comm.getControlCommand() != 0); // 若有指令则闪烁
        
        // 同步实时重量到 Modbus 寄存器
        _comm.updateWeight(currentWeight);
        _comm.updateStatus((uint16_t)_currentState);
        
        if (_currentState == STATE_VIEW_CALIB) {
            _display.updateViewCalib(scaleF, _scale.getOffset());
        } else {
            int displayParam = (_currentState == STATE_CONFIG_ZTR) ? _ztrOptions[_ztrIndex] : _calWeights[_calWeightIndex];
            _display.update(_currentState, currentWeight, rawADC, _currentId, commActive, displayParam);
        }
    }
}

void MainController::handleButton() {
    bool currentBtnState = (digitalRead(_buttonPin) == LOW);
    unsigned long now = millis();

    if (currentBtnState && !_btnPressed) {
        _btnPressed = true;
        _btnPressStart = now;
        _longPressHandled = false;
        
        // 判断双击：如果距离上次松开时间不到 400ms，判定为发起了双击的第二下
        if (_currentState == STATE_RUN && _lastBtnRelease > 0 && (now - _lastBtnRelease < 400)) {
            _servoIsOpen = !_servoIsOpen;
            _servo.write(_servoIsOpen ? 90 : 0);
            _display.showLargeMessage(_servoIsOpen ? "SRV ON" : "SRV OFF", 1000);
            
            _lastBtnRelease = 0;      // 消费掉这次点击记录
            _longPressHandled = true; // 拦截长按和松开逻辑
        }
    } else if (currentBtnState && _btnPressed) {
        if (!_longPressHandled && (now - _btnPressStart > 2000)) {
            _longPressHandled = true;
            _lastBtnRelease = 0; // 取消未决的短按
            if (_currentState == STATE_RUN) {
                _currentState = STATE_CONFIG_ID;
            } else if (_currentState == STATE_CONFIG_ID) {
                _prefs.putInt("slave_id", _currentId);
                _comm.begin(_currentId);
                _currentState = STATE_CONFIG_ZTR; // 转入零飘防抖配置
            } else if (_currentState == STATE_CONFIG_ZTR) {
                _ztrThreshold = _ztrOptions[_ztrIndex];
                _prefs.putInt("ztr_thresh", _ztrThreshold);
                _comm.setZtrThreshold(_ztrThreshold);
                _currentState = STATE_MENU_SELECT_WEIGHT;
                _calWeightIndex = 0; // 默认停在 EXIT
            } else if (_currentState == STATE_MENU_SELECT_WEIGHT) {
                int targetW = _calWeights[_calWeightIndex];
                if (targetW == 0) {
                    _currentState = STATE_VIEW_CALIB; // 退出并查看参数
                } else {
                    performCalibration(targetW); // 标定完成后会自动进入 VIEW_CALIB
                }
            } else if (_currentState == STATE_VIEW_CALIB) {
                _currentState = STATE_RUN; // 看完参数长按退回主界面
            }
        }
    } else if (!currentBtnState && _btnPressed) {
        _btnPressed = false;
        if (!_longPressHandled && (now - _btnPressStart > 20)) { // Debounce
            if (_currentState == STATE_RUN) {
                _lastBtnRelease = now; // 记录松开的时间，启动双击等待
            } else {
                // 菜单状态下直接响应短按，无需防双击
                if (_currentState == STATE_CONFIG_ID) {
                    _currentId++;
                    if (_currentId > 20) _currentId = 1;
                } else if (_currentState == STATE_CONFIG_ZTR) {
                    _ztrIndex++;
                    if (_ztrIndex > 3) _ztrIndex = 0;
                } else if (_currentState == STATE_MENU_SELECT_WEIGHT) {
                    _calWeightIndex++;
                    if (_calWeightIndex > 4) _calWeightIndex = 0;
                }
            }
        }
    } else if (!currentBtnState && !_btnPressed) {
        // 处理 RUN 模式下被延迟确认的单击事件 (去皮)
        if (_lastBtnRelease > 0 && (now - _lastBtnRelease > 400)) {
            if (_currentState == STATE_RUN) {
                _scale.tare();
            }
            _lastBtnRelease = 0;
        }
    }
}

void MainController::handleComm() {
    // 监听主站是否通过 Modbus 修改了零飘阈值
    uint16_t modbusZTR = _comm.getZtrThreshold();
    if (modbusZTR != _ztrThreshold) {
        _ztrThreshold = modbusZTR;
        _prefs.putInt("ztr_thresh", _ztrThreshold);
        // 同步改变本地 UI 配置索引
        if (_ztrThreshold == 0) _ztrIndex = 0;
        else if (_ztrThreshold <= 2) _ztrIndex = 1;
        else if (_ztrThreshold <= 5) _ztrIndex = 2;
        else _ztrIndex = 3;
    }

    // 轮询控制寄存器
    uint16_t cmd = _comm.getControlCommand();
    if (cmd == 0) return;

    if (cmd == 1) { // OPEN
        _servo.write(90);
    } else if (cmd == 2) { // CLOSE
        _servo.write(0);
    } else if (cmd == 3) { // TARE
        _scale.tare();
    } else if (cmd == 4) { // CALIBRATE (可通过主控触发)
        performCalibration(_calWeights[_calWeightIndex]);
    }

    _comm.clearControlCommand(); // 指令已处理，回馈 (清零)
}

void MainController::handleCommTest() {
    // 通信测试模式下，Modbus 仍在运行
    _rxData = "MDBS RUNNING";
    _txCount = 0; // 这里的计数器意义已变，可留空
}

void MainController::performCalibration(int weight) {
    _currentState = STATE_CALIBRATING;
    _display.update(_currentState, 0, _scale.getRawValue(), _currentId, false, weight);
    
    unsigned long start = millis();
    while (millis() - start < 1000) {
        _comm.task();
    }

    _scale.calibrate(weight);
    _currentState = STATE_VIEW_CALIB; // 校准终了，自动跳转进参显示界面
}
