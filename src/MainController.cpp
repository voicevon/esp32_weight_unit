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
      _lastUpdateMillis(0), _lastDiagMillis(0), _diagTxValue(0), _diagRxValue(0),
      _txCount(0), _rxData(""),
      _doorPhase(0), _doorTimer(0), _doorWaitTime(1000),
      _calTimer(0), _calTargetWeight(0) {}

void MainController::begin() {
    pinMode(_buttonPin, INPUT_PULLUP);
    _servo.attach(_servoPin);
    _servo.write(0); // Close by default
    
    _prefs.begin("weight_unit", false);
    _currentId = _prefs.getInt("slave_id", 1); // Default to 1
    
    _ztrThreshold = _prefs.getInt("ztr_thresh", 2); // Default to 2g
    _doorWaitTime = _prefs.getInt("door_time", 1000); // Default to 1s
    if (_ztrThreshold == 0) _ztrIndex = 0;
    else if (_ztrThreshold <= 2) _ztrIndex = 1;
    else if (_ztrThreshold <= 5) _ztrIndex = 2;
    else _ztrIndex = 3;
    
    _display.begin();
    _display.showMessage("Wait HX711...", 0);
    
    _scale.begin();
    
    _mutexComm = xSemaphoreCreateMutex();
    
    xSemaphoreTake(_mutexComm, portMAX_DELAY);
    _comm.begin(_currentId); // 初始化 Modbus Slave
    _comm.setZtrThreshold(_ztrThreshold); // 同步初始门限到 Modbus
    _comm.setDoorTime(_doorWaitTime);     // 同步初始开门时间
    xSemaphoreGive(_mutexComm);
    
    _display.showMessage("System Ready!", 1000);

    // Start Modbus task on Core 0 with high priority to ensure responsiveness
    xTaskCreatePinnedToCore(
        commTask,
        "ModbusTask",
        4096,
        this,
        10, // High priority
        &_commTaskHandle,
        0 // Core 0
    );
}

void MainController::commTask(void* pvParameters) {
    MainController* controller = (MainController*)pvParameters;
    for(;;) {
        if (xSemaphoreTake(controller->_mutexComm, pdMS_TO_TICKS(20)) == pdTRUE) {
            controller->_comm.task();
            xSemaphoreGive(controller->_mutexComm);
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Yield to keep system stable
    }
}

void MainController::loop() {
    // _comm.task() is now handled by a dedicated background task on Core 0
    
    handleButton();
    if (_currentState == STATE_COMM_TEST) {
        handleCommTest();
    } else if (_currentState != STATE_CONFIG_ID) {
        handleComm(); // 处理寄存器指令
    }
    unsigned long now = millis();
    if (now - _lastUpdateMillis > 100) {
        static long lastRawADC = 0;
        static float lastWeight = 0.0;
        
        long rawADC = lastRawADC;
        float currentWeight = lastWeight;
        float scaleF = _scale.getScaleFactor();

        if (_scale.isReady()) {
            rawADC = _scale.getRawValue();
            lastRawADC = rawADC;
            
            // 重要修复：通过原始 ADC 直接换算重量并填入历史记录队列，彻底避免重复拉高引脚导致的 Race Condition 阻塞
            currentWeight = _scale.calculateWeight(rawADC);
            _scale.updateHistory(currentWeight);
            
            // 防零飘死区 (Zero Tracking Deadband)
            if (abs(currentWeight) <= (float)_ztrThreshold) {
                currentWeight = 0.0;
            }
            lastWeight = currentWeight;
        } else {
            // 增加硬件诊断
            static unsigned long lastWarn = 0;
            if (millis() - lastWarn > 5000) {
                lastWarn = millis();
                Serial.println("[ERROR] LoadCell DISCONNECTED or NOT READY!");
            }
        }

        _lastUpdateMillis = now;
        
        uint32_t rxBytes = 0;
        uint8_t lastByte = 0;
        bool commActive = false;
        
        // isStable 现在内部会处理 Ready 检查并打印诊断日志
        // 动态容忍阈值：让判断条件与零点防抖一致 (例如 2.0g)，最低保证 1.5g 容忍度避免原生噪声永远报警
        float stableThresh = (float)_ztrThreshold > 1.5f ? (float)_ztrThreshold : 1.5f;
        bool stable = _scale.isStable(stableThresh);

        // Synchronize Modbus access
        if (xSemaphoreTake(_mutexComm, pdMS_TO_TICKS(10)) == pdTRUE) {
            commActive = (_comm.getControlCommand() != 0); 
            _comm.updateWeight(currentWeight);
            _comm.updateStatus((uint8_t)_currentState, stable, _doorPhase);
            rxBytes = _comm.getRxByteCount();
            lastByte = _comm.getLastRxByte();
            xSemaphoreGive(_mutexComm);
        }
        
        // --- 门逻辑状态机 (Door State Machine) ---
        if (_doorPhase == 1) { // OPENING
            _servo.write(90);
            _doorTimer = now;
            _doorPhase = 2; // Transition to WAITING
        } else if (_doorPhase == 2) { // WAITING
            if (now - _doorTimer >= _doorWaitTime) {
                _doorPhase = 3; // DONE
            }
        }
        
        if (_currentState == STATE_RS485_DIAG) {
            // 485 诊断逻辑：使用独立定时器每秒发送一个自增字节
            if (millis() - _lastDiagMillis > 1000) {
                _lastDiagMillis = millis();
                _diagTxValue++;
                _comm.sendRawByte(_diagTxValue);
            }
            // 接收原始字节
            int r = _comm.readRawByte();
            if (r != -1) _diagRxValue = (uint8_t)r;
            
            // 复用 update 的参数传递 TX/RX 供 UI 绘制
            _display.update(_currentState, 0, 0, _currentId, false, (int)_diagTxValue, (uint32_t)_diagRxValue, 0);
        } else if (_currentState == STATE_VIEW_CALIB) {
            _display.updateViewCalib(scaleF, _scale.getOffset());
        } else if (_currentState == STATE_CALIBRATING) {
            // 异步标定逻辑：等待 1 秒稳定时间
            if (millis() - _calTimer >= 1000) {
                _scale.calibrate(_calTargetWeight);
                _currentState = STATE_VIEW_CALIB;
                _display.showMessage("Calib Success", 1000);
            } else {
                _display.update(_currentState, currentWeight, rawADC, _currentId, commActive, _calTargetWeight, rxBytes, lastByte, stable, _doorPhase);
            }
        } else {
            int displayParam = (_currentState == STATE_CONFIG_ZTR) ? _ztrOptions[_ztrIndex] : _calWeights[_calWeightIndex];
            _display.update(_currentState, currentWeight, rawADC, _currentId, commActive, displayParam, rxBytes, lastByte, stable, _doorPhase);
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
            _display.showLargeMessage(_servoIsOpen ? "S-OPEN" : "S-CLOSE", 1000);
            
            _lastBtnRelease = 0;      // 消费掉这次点击记录
            _longPressHandled = true; // 拦截长按和松开逻辑
        }
    } else if (currentBtnState && _btnPressed) {
        if (!_longPressHandled && (now - _btnPressStart > 2000)) {
            _longPressHandled = true;
            _lastBtnRelease = 0; // 取消未决的短按
            if (_currentState == STATE_RUN) {
                _currentState = STATE_RS485_DIAG;
                _diagTxValue = 0;
                _diagRxValue = 0;
                _lastDiagMillis = millis();
                _display.showMessage("485 DIAG MODE");
            } else if (_currentState == STATE_RS485_DIAG) {
                _currentState = STATE_CONFIG_ID;
                _display.showMessage("CONFIG ID");
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
                _currentState = STATE_RUN; // 循环回到 RUN
                _display.showMessage("RUN MODE", 1000);
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
    if (xSemaphoreTake(_mutexComm, pdMS_TO_TICKS(10)) != pdTRUE) return;

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

    // 监听主站是否修改了开门时间
    uint16_t modbusDoorTime = _comm.getDoorTime();
    if (modbusDoorTime != _doorWaitTime) {
        _doorWaitTime = modbusDoorTime;
        _prefs.putInt("door_time", _doorWaitTime);
    }

    // 轮询控制寄存器
    uint16_t cmd = _comm.getControlCommand();
    if (cmd == 0) {
        xSemaphoreGive(_mutexComm);
        return;
    }

    if (cmd == 1) { // OPEN (Starts Sequence)
        if (_doorPhase == 0 || _doorPhase == 3) { // Only start if idle or previous done
            _doorPhase = 1; // Trigger state machine
        }
    } else if (cmd == 2) { // CLOSE
        _servo.write(0);
        _doorPhase = 0; // Reset to IDLE
    } else if (cmd == 3) { // TARE
        _scale.tare();
    } else if (cmd == 4) { // CALIBRATE (可通过主控触发)
        performCalibration(_calWeights[_calWeightIndex]);
    }

    _comm.clearControlCommand(); // 指令已处理，回馈 (清零)
    xSemaphoreGive(_mutexComm);
}

void MainController::handleCommTest() {
    // 通信测试模式下，Modbus 仍在运行 (由后台任务处理)
    _rxData = "MDBS RUNNING";
    _txCount = 0; 
}

void MainController::performCalibration(int weight) {
    _currentState = STATE_CALIBRATING;
    _calTargetWeight = weight;
    _calTimer = millis();
    // 立即返回，不在互斥锁内等待。实际校验动作由 loop() 核心 1 完成。
}
