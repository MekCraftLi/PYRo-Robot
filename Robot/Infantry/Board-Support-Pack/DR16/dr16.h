/**
 *******************************************************************************
 * @file    dr16.h
 * @brief   简要描述
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

#ifndef INFANTRY_CHASSIS_DR16_H
#define INFANTRY_CHASSIS_DR16_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../../System/Input/ControlImpl/control-impl-axis.h"
#include "../../System/Input/ControlImpl/control-impl-switch.h"

#include "../../Adapter/adapter-remote.h"

#include "FreeRTOS.h"
#include "System/crtp.h"
#include "task.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/





/*-------- 3. interface ----------------------------------------------------------------------------------------------*/



typedef struct __attribute__((packed)) {
    /* =================================================================
           Part 1: 遥控器通道 (Remote Controller) - 6 Bytes (48 bits)
           原理: 利用位域直接映射 bit 流，利用小端序特性自动解包
           ================================================================= */

    uint64_t ch0 : 11; // Right Stick Horizontal (364-1684)
    uint64_t ch1 : 11; // Right Stick Vertical
    uint64_t ch2 : 11; // Left Stick Horizontal
    uint64_t ch3 : 11; // Left Stick Vertical
    uint64_t s2  : 2;  // Left Switch (1:Up, 3:Mid, 2:Down)
    uint64_t s1  : 2;  // Right Switch

    // 注意：这里刚好 48 bits = 6 Bytes，GCC packed 属性会保证
    // 下面的 standard types 紧接着第 7 个字节开始，无 Padding。

    /* =================================================================
       Part 2: 鼠标数据 (Mouse) - 8 Bytes
       ================================================================= */
    int16_t mouse_x; // Mouse X axis
    int16_t mouse_y; // Mouse Y axis
    int16_t mouse_z; // Mouse Z axis
    uint8_t mouse_l; // Mouse Left Button
    uint8_t mouse_r; // Mouse Right Button

    /* =================================================================
       Part 3: 键盘数据 (Keyboard) - 2 Bytes
       ================================================================= */
    uint16_t key_code; // Keyboard Key mask (W,A,S,D...)

    /* =================================================================
       Part 4: 拨轮 (Reserve / Wheel) - 2 Bytes
       ================================================================= */
    uint16_t wheel; // Side Wheel (364-1684)
} Dr16Data;




class RemoteDR16 : public RemoteBase, public Singleton<RemoteDR16> {
  public:
    // ... 构造函数和 updateRaw 同前 ...

    // 1. 构造函数：在这里配置具体的硬件参数 (死区、映射表)
    RemoteDR16();

    // 2. 数据更新接口 (被 BSP 驱动调用)
    void updateRaw(const Dr16Data& raw);

    // 3. 暴露控件接口 (供绑定层使用)
    // 架构原则: 返回基类指针 IInputControl*，隐藏具体实现
    IInputControl* getLeftX() { return &_axislx; }
    IInputControl* getLeftY() { return &_axisly; }
    IInputControl* getRightX() { return &_axisrx; }
    IInputControl* getRightY() { return &_axisry; }
    IInputControl* getSwLeft() { return &_swleft; }
    IInputControl* getSwRight() { return &_swright; }
    IInputControl* getWheel() { return &_wheel; }
    // --- 实现基类接口 ---

    IInputControl* getAxis(AxisID id) override {
        switch (id) {
            case AxisID::MoveX:
                return &_axislx; // 左横 -> 平移X
            case AxisID::MoveY:
                return &_axisly; // 左纵 -> 平移Y
            case AxisID::ViewYaw:
                return &_axisrx; // 右横 -> 云台Yaw
            case AxisID::ViewPitch:
                return &_axisry; // 右纵 -> 云台Pitch
            default:
                return nullptr;
        }
    }

    IInputControl* getButton(ButtonID id) override {
        switch (id) {
            case ButtonID::ModeSwitch:
                return &_swleft; // 左开关 -> 模式
            case ButtonID::FuncSwitch:
                return &_swright; // 右开关 -> 功能
            default:
                return nullptr;
        }
    }

    [[nodiscard]] bool isConnected() const override;
    void onDataReceived() override;

  private:
    // 实体化控件 (内存直接分配在 Remote 对象内)
    ControlAxis _axislx, _axisly;
    ControlAxis _axisrx, _axisry;
    ControlAxis _wheel;
    ControlSwitch _swleft;
    ControlSwitch _swright;
    TickType_t _lastUpdateTick;
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_DR16_H*/
