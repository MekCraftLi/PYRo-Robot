/**
 *******************************************************************************
 * @file    trigger-impl-linear.h
 * @brief   线性触发器
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * 触发条件： 线性值大于死区
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_TRIGGER_IMPL_LINEAR_H
#define INFANTRY_CHASSIS_TRIGGER_IMPL_LINEAR_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../triggers.h"
#include <cmath>





/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class TriggerLinear : public InputTrigger {
public:
    explicit TriggerLinear(float threshold = 0.01f): _threshold(threshold){}

    TriggerState update(float value, float dt) override {
        if (std::abs(value) > _threshold) {
            return TriggerState::Triggered;
        }

        return TriggerState::None;
    }

private:
    float _threshold;
};




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_TRIGGER_IMPL_LINEAR_H*/
