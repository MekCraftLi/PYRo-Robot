/**
 *******************************************************************************
 * @file    trigger-decorator-toggle.h
 * @brief   状态翻转触发器装饰器 (Toggle Decorator)
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 效果：当被装饰的原 Trigger 产生一次有效的 Triggered 状态时，
 * 翻转自身的布尔状态。
 * 状态为 True 时返回 Triggered，False 时返回 None。
 *
 *******************************************************************************
 * @author  Gemini
 * @date    2026/3/4
 * @version 1.0
 *******************************************************************************
 */

#ifndef INFANTRY_CHASSIS_TRIGGER_DECORATOR_TOGGLE_H
#define INFANTRY_CHASSIS_TRIGGER_DECORATOR_TOGGLE_H

/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../triggers.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/





/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class TriggerToggle : public InputTrigger {
public:
    /**
     * @brief 构造函数
     * @param base_trigger 需要被装饰的原触发器实例（强制传入引用）
     * @param default_state 初始的开关状态 (默认 false 为关闭)
     */
    explicit TriggerToggle(InputTrigger& base_trigger, bool default_state = false)
        : _baseTrigger(base_trigger), // 引用必须在初始化列表中绑定
          _isToggledOn(default_state),
          _wasTriggeredLastFrame(false) {}

  TriggerState update(float value, float dt) override {
        // 【变动】不需要判空了，因为 C++ 语法保证引用必定指向一个有效对象

        // 1. 获取底层触发器的当前状态 (使用 . 而不是 ->)
        TriggerState baseState = _baseTrigger.update(value, dt);

        // 2. 边缘检测：只有当底层触发器从 非Triggered 变为 Triggered 的瞬间，才执行翻转
        bool isTriggeredNow = (baseState == TriggerState::Triggered);

        if (isTriggeredNow && !_wasTriggeredLastFrame) {
            _isToggledOn = !_isToggledOn; // 翻转内部状态
        }

        // 3. 记录这一帧的状态，供下一帧边缘检测使用
        _wasTriggeredLastFrame = isTriggeredNow;

        // 4. 根据当前的内部开关状态输出最终结果
        if (_isToggledOn) {
            return TriggerState::Triggered;
        } else {
            return TriggerState::None;
        }
    }

    // 重写 reset，确保底层触发器和自身状态一并重置
    void reset() override {
        InputTrigger::reset();
        _baseTrigger.reset(); // 使用 . 而不是 ->
        _isToggledOn = false;
        _wasTriggeredLastFrame = false;
    }

private:
    InputTrigger& _baseTrigger; // 【变动】存储为引用
    bool _isToggledOn;
    bool _wasTriggeredLastFrame;
};
#endif /*INFANTRY_CHASSIS_TRIGGER_DECORATOR_TOGGLE_H*/