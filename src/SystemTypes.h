#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

/**
 * @brief 从机统一运行状态机 (Slave Unit State Machine)
 * 相比之前的多状态混合，统一状态机能有效保证逻辑一致性
 */
enum SlaveState {
    SLAVE_INIT,         // 初始化
    SLAVE_STANDBY,      // 待机监控 (正常采样)
    SLAVE_LOCKED,       // 已被选中 (等待物理动作)
    SLAVE_DISCHARGING,  // 正在出料 (舵机开启)
    SLAVE_RECOVERY,     // 物理回位中 (舵机关闭，等待机械稳定)
    SLAVE_ZERO_TRACKING,// 自动零位追踪中
    SLAVE_CALIBRATING,  // 标定流程中
    SLAVE_ERROR         // 故障状态 (硬件异常)
};

/**
 * @brief 菜单层级状态 (UI 模式)
 */
enum UIMode {
    UI_RUN,             // 正常运行显示
    UI_CONFIG_ID,       // 配置 ID
    UI_CONFIG_ZTR,      // 配置零漂
    UI_MENU_CALIB,      // 标定选择
    UI_RS485_DIAG,      // 连接诊断
    UI_VERSION          // 版本查看
};

#endif
