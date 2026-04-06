/**
 *******************************************************************************
 * @file    trigger-impl-hold.h
 * @brief   长按触发器
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_TRIGGER_IMPL_HOLD_H
#define INFANTRY_CHASSIS_TRIGGER_IMPL_HOLD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../triggers.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

enum class HoldCondition {
    GreaterOrEqual,
    LessOrEqual,
};


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/


class TriggerHold : public InputTrigger {
public:
    // hold_time: 需要按住多少秒才触发
    // one_shot: true=只触发一次; false=时间到了之后每帧都触发
    TriggerHold(float threshold, float hold_time, bool one_shot = false, HoldCondition condition = HoldCondition::GreaterOrEqual)
        : _threshold(threshold), _holdTime(hold_time), _oneShot(one_shot), _timer(0.0f), _cond(condition) {}

    TriggerState update(float value, float dt) override {
        bool isActive = (_cond == HoldCondition::GreaterOrEqual) ? (value >= _threshold) : (value <= _threshold);

        if (!isActive) {
            _timer = 0.0f;
            _hasTriggered = false;   // 松开时重置触发锁
            return TriggerState::None;
        }

        // 正在按住
        if (_timer < _holdTime) {
            _timer += dt;
            return TriggerState::Started;
        }


        // 时间已到！
        if (_oneShot) {
            // 单次触发模式
            if (!_hasTriggered) {
                _hasTriggered = true;           // 关门上锁
                return TriggerState::Triggered; // 仅在时间刚到的那一帧触发
            }
            return TriggerState::Ongoing; // 已经触发过了，保持按压状态但不重复触发
        }
        // 持续触发模式
        return TriggerState::Triggered; // 只要不松手，帧帧都触发
    }

private:
    float _threshold;
    float _holdTime;
    bool _oneShot;
    float _timer;
    HoldCondition _cond = HoldCondition::GreaterOrEqual;
    bool _hasTriggered = false;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_TRIGGER_IMPL_HOLD_H*/
