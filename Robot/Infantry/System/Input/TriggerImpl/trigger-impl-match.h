/**
 *******************************************************************************
 * @file    trigger-impl-match.h
 * @brief   状态匹配触发器
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 触发条件：与阈值匹配
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_TRIGGER_IMPL_MATCH_H
#define INFANTRY_CHASSIS_TRIGGER_IMPL_MATCH_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../triggers.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class TriggerMatch : public InputTrigger {
public:
    // threshold: 触发阈值。
    // 例如对于 SwitchN:
    //   检测 "上" (1.0): threshold = 0.9
    //   检测 "中" (0.0): 需要配合下面的 Interval 实现，或简单判断 > -0.1 && < 0.1
    //   检测 "下" (-1.0): threshold = -0.9 (且 actuates_on_low = true)
    explicit TriggerMatch(float threshold) : _threshold(threshold) {}

    TriggerState update(float value, float dt) override {
        // 简单阈值判断 (假设检测正向值，如 1.0)
        if (value == _threshold) {
            return TriggerState::Triggered;
        }
        return TriggerState::None;
    }

private:
    float _threshold;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_TRIGGER_IMPL_DOWN_H*/
