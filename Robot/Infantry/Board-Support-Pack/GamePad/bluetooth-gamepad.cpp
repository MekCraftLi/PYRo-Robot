/**
 *******************************************************************************
 * @file    bluetooth-gamepad.cpp
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


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "bluetooth-gamepad.h"
#include "task.h"


/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/
// 构造函数：初始化所有原子控件的映射参数
BluetoothGamepad::BluetoothGamepad()
    : // 1. 摇杆初始化：假定 16-bit 范围为 0~65535，中心点 32768，死区 2000
      _leftStickX({0, 65535, 32768, 2000, false}),
      _leftStickY({0, 65535, 32768, 2000, false}),
      _rightStickX({0, 65535, 32768, 2000, false}),
      _rightStickY({0, 65535, 32768, 2000, false}),

      // 2. 线性扳机初始化：最大 0x03FF (1023)，默认松开是 0
      // 设置为：最小 0，最大 1023，中心点 0 (单向轴)
      _leftTrigger({0, 1023, 0, 10, false}),
      _rightTrigger({0, 1023, 0, 10, false}),

      // 3. 按键初始化：通用的 0->松开，1->按下
      _buttonA({{0, 0}, {1, 1}}, 0),
      _buttonB({{0, 0}, {1, 1}}, 0),
      _buttonX({{0, 0}, {1, 1}}, 0),
      _buttonY({{0, 0}, {1, 1}}, 0),
      _buttonL1({{0, 0}, {1, 1}}, 0),
      _buttonR1({{0, 0}, {1, 1}}, 0),
      _leftStickClick({{0, 0}, {1, 1}}, 0),
      _rightStickClick({{0, 0}, {1, 1}}, 0),
      _buttonStart({{0, 0}, {1, 1}}, 0),
      _buttonMenu({{0, 0}, {1, 1}}, 0),
      _buttonHome({{0, 0}, {1, 1}}, 0),
      _buttonCapture({{0, 0}, {1, 1}}, 0),

      _lastUpdateTick(0)
{}

// 数据更新：将串口帧分配给独立的控件
void BluetoothGamepad::updateRaw(const GamepadRawData& raw) {
    // 更新摇杆与扳机
    _leftStickX.updateRaw(raw.leftStickX);
    _leftStickY.updateRaw(raw.leftStickY);
    _rightStickX.updateRaw(raw.rightStickX);
    _rightStickY.updateRaw(raw.rightStickY);
    _leftTrigger.updateRaw(raw.leftTrigger);
    _rightTrigger.updateRaw(raw.rightTrigger);

    // 更新按键
    _buttonA.updateRaw(raw.buttonA);
    _buttonB.updateRaw(raw.buttonB);
    _buttonX.updateRaw(raw.buttonX);
    _buttonY.updateRaw(raw.buttonY);
    _buttonL1.updateRaw(raw.buttonL1);
    _buttonR1.updateRaw(raw.buttonR1);
    _leftStickClick.updateRaw(raw.leftStickClick);
    _rightStickClick.updateRaw(raw.rightStickClick);

    _buttonStart.updateRaw(raw.buttonStart);
    _buttonMenu.updateRaw(raw.buttonMenu);
    _buttonHome.updateRaw(raw.buttonHome);
    _buttonCapture.updateRaw(raw.buttonCapture);

    // 注: raw.dpad (方向键) 的解析取决于具体手柄固件。
    // 如果是 Bitmask: (raw.dpad & 0x01) == 上
    // 如果是 POV Hat: 0=上, 1=右上, 2=右, 8=居中等。
    // 这里保留接口，你可根据实际打印的 D-Pad 裸数据追加 ControlSwitch 映射。
}

// =========================================================
// 统一路由：将手柄抽象为标准的 RemoteBase 接口
// =========================================================
IInputControl* BluetoothGamepad::getAxis(AxisID id) {
    switch (id) {
        case AxisID::MoveX:
            return &_rightTrigger;   // 左摇杆推拉 -> 底盘前后
        case AxisID::Aux1:
            return &_leftTrigger;
        case AxisID::ViewYaw:
            return &_leftStickX;   // 左摇杆左右 -> 底盘平移
        default:
            return nullptr;
    }
}

IInputControl* BluetoothGamepad::getButton(ButtonID id) {
    switch (id) {
        case ButtonID::ModeSwitch:
            return &_buttonStart;  // 用 Start 键切换机器人大模式
        case ButtonID::FuncSwitch:
            return &_buttonR1;     // 用 R1 肩键作为通用功能键(或射击)
        case ButtonID::KeyA:
            return &_buttonA;      // 映射物理 A 键
        case ButtonID::KeyB:
            return &_buttonB;      // 映射物理 B 键
        default:
            return nullptr;
    }
}

bool BluetoothGamepad::isConnected() const {
    // 100ms 离线超时判定 (蓝牙可能有一定延迟，可适当放宽)
    return (xTaskGetTickCount() - _lastUpdateTick) <= pdMS_TO_TICKS(100);
}

void BluetoothGamepad::onDataReceived() {
    _lastUpdateTick = xTaskGetTickCount();
}