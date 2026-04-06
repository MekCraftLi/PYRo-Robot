/**
 *******************************************************************************
 * @file    action.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * level IV
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_ACTION_H
#define INFANTRY_CHASSIS_ACTION_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "triggers.h"
#include "control.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

struct ActionBinding {
    IInputControl* control;  // Level 1: 物理控件 (数据源)
    InputTrigger* trigger;   // Level 3: 触发逻辑 (状态机)
};

class InputAction {
public:
    InputAction() : m_binding{nullptr, nullptr}, m_state(TriggerState::None), m_value(0.0f) {}

    // 一个 Action 只绑定一个输入源和一个触发器
    void bind(IInputControl* input_source, InputTrigger* trigger = nullptr) {
        m_binding.control = input_source;
        m_binding.trigger = trigger;
    }

    // 更新绑定 (由 Input_Task 调用)
    void update(float dt) {
        if (m_binding.control == nullptr) {
            m_value = 0.0f;
            m_state = TriggerState::None;
            return;
        }

        m_value = m_binding.control->get();
        m_state = (m_binding.trigger != nullptr) ? m_binding.trigger->update(m_value, dt) : TriggerState::None;
    }

    bool isTriggered() const { return m_state == TriggerState::Triggered; }
    float getValue() const { return m_value; }

private:
    ActionBinding m_binding;
    TriggerState m_state;
    float m_value;
};


// --- 模式切换意图 ---
extern InputAction Action_CtrlMode;   // 决定系统控制权 (映射到右侧开关)
extern InputAction Action_FuncMode;   // 决定附加功能 (映射到左侧开关)

// --- 底盘运动意图 ---
extern InputAction Action_MoveX;      // 前后移动
extern InputAction Action_MoveY;      // 左右移动
extern InputAction Action_Spin;       // 底盘自旋

// --- 云台/发射意图 ---
extern InputAction Action_GimbalYaw;
extern InputAction Action_GimbalPitch;

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_ACTION_H*/
