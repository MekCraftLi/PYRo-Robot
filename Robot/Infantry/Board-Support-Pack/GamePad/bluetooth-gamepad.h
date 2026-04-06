/**
 *******************************************************************************
 * @file    bluetooth-gamepad.h
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
 * @date    2026/3/8
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_BLUETOOTH_GAMEPAD_H
#define INFANTRY_BLUETOOTH_GAMEPAD_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "Adapter/adapter-remote.h"
#include "System/Input/ControlImpl/control-impl-axis.h"
#include "System/Input/ControlImpl/control-impl-switch.h"
#include "tools/crtp.h"
#include "FreeRTOS.h"



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/

#pragma pack(push, 1)

class IInputControl;
/**
 * @brief 蓝牙手柄底层串口数据帧
 * @note 严格使用 1 字节对齐，位域按实际字节流拆分，防止 Padding 错位
 */
struct GamepadRawData {
    // 摇杆数据 (16-bit)
    uint16_t leftStickX;
    uint16_t leftStickY;
    uint16_t rightStickX;
    uint16_t rightStickY;

    // 线性扳机 (10-bit 有效，最大 0x03FF)
    uint16_t leftTrigger;
    uint16_t rightTrigger;

    // 方向键 (通常为按键掩码或 POV Hat 值)
    uint8_t dpad;

    // --- 按键位域 (严格按字节拆分，共 3 字节) ---
    // Byte 1
    uint8_t buttonL1       : 1; // Left Bumper
    uint8_t buttonR1       : 1; // Right Bumper
    uint8_t reserved1      : 1;
    uint8_t buttonY        : 1;
    uint8_t buttonX        : 1;
    uint8_t reserved2      : 1;
    uint8_t buttonB        : 1;
    uint8_t buttonA        : 1;

    // Byte 2
    uint8_t reserved3      : 1;
    uint8_t rightStickClick: 1; // R3 键
    uint8_t leftStickClick : 1; // L3 键
    uint8_t reserved4      : 4;
    uint8_t buttonHome     : 1; // Home/Guide 键

    // Byte 3
    uint8_t buttonMenu     : 1; // Select/Back 键
    uint8_t buttonCapture  : 1; // Share/Capture 键
    uint8_t reserved5      : 5;
    uint8_t buttonStart    : 1; // Start/Options 键
};


/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 蓝牙游戏手柄驱动类
 */
class BluetoothGamepad : public RemoteBase, public Singleton<BluetoothGamepad> {
public:
    BluetoothGamepad();

    // 核心数据更新接口 (串口收到数据后调用)
    void updateRaw(const GamepadRawData& raw);

    // =============================================================
    // 实现 RemoteBase 接口：方便接入现有的 CommanderSrvc 仲裁体系
    // =============================================================
    IInputControl* getAxis(AxisID id) override;
    IInputControl* getButton(ButtonID id) override;

    [[nodiscard]] bool isConnected() const override;
    void onDataReceived() override;

    // =============================================================
    // 手柄特有控件暴露接口 (供高阶 Action 绑定使用)
    // =============================================================
    IInputControl* getLeftTrigger()  { return &_leftTrigger; }
    IInputControl* getRightTrigger() { return &_rightTrigger; }
    IInputControl* getButtonA()      { return &_buttonA; }
    IInputControl* getButtonB()      { return &_buttonB; }
    IInputControl* getButtonX()      { return &_buttonX; }
    IInputControl* getButtonY()      { return &_buttonY; }
    IInputControl* getButtonL1()     { return &_buttonL1; }
    IInputControl* getButtonR1()     { return &_buttonR1; }

private:
    friend class Singleton<BluetoothGamepad>;

    // 实体化控件：摇杆与线性扳机
    ControlAxis _leftStickX;
    ControlAxis _leftStickY;
    ControlAxis _rightStickX;
    ControlAxis _rightStickY;
    ControlAxis _leftTrigger;
    ControlAxis _rightTrigger;

    // 实体化控件：动作按键
    ControlSwitch _buttonA;
    ControlSwitch _buttonB;
    ControlSwitch _buttonX;
    ControlSwitch _buttonY;
    ControlSwitch _buttonL1;
    ControlSwitch _buttonR1;
    ControlSwitch _leftStickClick;
    ControlSwitch _rightStickClick;

    // 实体化控件：系统按键
    ControlSwitch _buttonStart;
    ControlSwitch _buttonMenu;
    ControlSwitch _buttonHome;
    ControlSwitch _buttonCapture;

    TickType_t _lastUpdateTick;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_BLUETOOTH_GAMEPAD_H*/
