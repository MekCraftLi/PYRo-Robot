/**
 *******************************************************************************
 * @file    control.h
 * @brief   控件的抽象描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * level II
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_CONTROL_H
#define INFANTRY_CHASSIS_CONTROL_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cmath>
#include <stdint.h>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

// 控件类型
enum class ControlType {
    Axis,       // 摇杆 (连续值 -1.0 ~ 1.0)
    Switch      // 离散开关
};


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/



/**
 * @brief 输入控件抽象基类 (Interface)
 * 符合架构规范中 "层级1: 原子化控件" 的定义 [cite: 23]
 */
class IInputControl {
public:
    virtual ~IInputControl() = default;

    /**
     * @brief 获取归一化后的浮点值
     * @return float 范围通常为 -1.0 到 1.0
     * 摇杆: -1.0(左/下) ~ 1.0(右/上)
     * 开关: 这里的定义由具体实现决定 (例如: Up=1.0, Down=-1.0)
     */
    virtual float get() const = 0;

    /**
     * @brief 控件是否处于“被操作”状态
     * 用于休眠检测或边缘触发
     */
    virtual bool isActive() const = 0;

    /**
     * @brief 注入原始数据 (依赖倒置：数据由驱动层推入)
     * @param raw_value 硬件层的原始数值
     */
    virtual void updateRaw(int raw_value) = 0;
};


class InputControl {
public:
    InputControl(ControlType type) : _type(type) {}

    // --- 核心操作 ---

    // 输入原始值 (由底层驱动调用)
    void updateRaw(float raw_val) {
        _prev_val = _val; // 记录上一帧，用于边缘检测
        _val = raw_val;
    }

    // --- 统一输出接口 ---

    // 获取归一化值 (-1.0 ~ 1.0)
    float get() const { return applyDeadzone(_val); }

    // 获取布尔值 (用于按钮)
    bool isPressed() const { return get() > 0.5f; }

    // --- 高级特性封装 ---

    // 上升沿检测 (刚刚按下)
    bool isDown() const {
        return (_val > 0.5f) && (_prev_val <= 0.5f);
    }

    // 下降沿检测 (刚刚松开)
    bool isUp() const {
        return (_val <= 0.5f) && (_prev_val > 0.5f);
    }

    // 设置死区 (比如摇杆中心 5% 的漂移忽略)
    void setDeadzone(float dz) { _deadzone = dz; }

private:
    ControlType _type;
    float _val = 0.0f;
    float _prev_val = 0.0f;
    float _deadzone = 0.05f; // 默认 5% 死区

    float applyDeadzone(float input) const {
        if (std::abs(input) < _deadzone) return 0.0f;
        return input;
    }
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_CONTROL_H*/
