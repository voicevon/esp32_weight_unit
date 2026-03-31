# ESP32 从机 (Weight Slave) 固件重构技术报告

**版本**: 2.5
**日期**: 2026-03-31
**开发者**: Antigravity

---

## 1. 重构背景 (Background)
为了解决工业组合秤生产环境中的“陈旧数据干扰”和“状态机死锁”问题，本次对从机固件进行了系统性重构。目标是提升系统在动态环境下的称重精度与实时响应。

## 2. 核心架构变更 (Architectural Changes)

### 2.1 引入统一有限状态机 (Unified FSM)
- **旧版**: 混合使用 `_currentState` 和 `_doorPhase` 及 `_servoIsOpen` 变量，逻辑散布。
- **新版**: 引入 `SlaveState` 枚举，严格控制状态迁移：
  - `SLAVE_STANDBY`: 空闲监控模式。
  - `SLAVE_LOCKED`: 节点被主机锁定（参与组合）。
  - `SLAVE_DISCHARGING`: 执行下料动作（舵机动作）。
  - `SLAVE_RECOVERY`: 机械缓冲等待期。
- **优势**: 彻底杜绝了物料未卸载完就判定为“稳定”导致的二次误差。

### 2.2 数据新鲜度跟踪 (Data Freshness Tag)
- **新增寄存器**: `REG_DATA_ID` (0x0103)。
- **逻辑**: 每当从机收到 `OPEN` 指令时，`DataID` 会自增并发布。
- **目的**: 配合主机的“逻辑失效”算法，保证只有产生新一轮数据的节点才能进入下一次组合。

## 3. 处理核心优化 (Processing Optimizations)

### 3.1 动态指数移动平均滤波 (EMA Filter)
- **实现**: 在 `ScaleComponent` 中引入 `_emaAlpha`。
- **优点**: 不同于简单的历史平均，EMA 能够对近期变化的权重给予更高的灵敏度，同时有效抑制 100ms 内的高频震动。

### 3.2 任务隔离 (Task Isolation)
- **Core 0**: 负责 Modbus 解析，避免通讯突发导致采样丢失。
- **Core 1**: 负责 `AnalogRead` 与高频逻辑循环，保证数据响应在毫秒级。

## 4. UI 与交互提升 (UI & UX)
- **UI 模式分离**: 引入 `UIMode`，明确区分“实时运行模式”与“配置菜单模式”。
- **双击去皮 (Double Click)**: 优化了按钮逻辑，支持运行时双击快速去皮。
- **高刷新率显示**: 提升 I2C 时钟至 400kHz，更新 UI 耗时减少 30%。

---

## 5. 通讯协议说明 (Protocol)
| 寄存器地址 | 定义 | 含义 |
| :--- | :--- | :--- |
| 0x0002 | `REG_STATUS` | 低 8 位存储 `SlaveState`，第 8 位标识稳定性。 |
| 0x0103 | `REG_DATA_ID` | 下料计数标识位。 |

---
> [!TIP]
> 建议在每次机械维护（更换舵机或支架）后，重新进行一次单点标定以确保 `ScaleFactor` 处于最优状态。
