#ifndef I_MODE_HANDLER_H
#define I_MODE_HANDLER_H

#include <Arduino.h>
#include "../common/SystemTypes.h"
#include "../hal/Button.h"

/**
 * @class IModeHandler
 * @brief 模式处理器接口：定义所有顶层模式的生命周期。
 */
class IModeHandler {
public:
    virtual ~IModeHandler() {}

    // 进入模式：进行资源锁、初始化显示等
    virtual void enter() = 0;

    // 每一帧的逻辑更新。返回 true 表示事件已被处理，不需要全局逻辑再次处理。
    virtual bool update(ButtonEvent event) = 0;

    // 退出模式：释放资源、清理缓冲区
    virtual void exit() = 0;
};

#endif
