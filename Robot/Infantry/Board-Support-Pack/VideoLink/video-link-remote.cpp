/**
 *******************************************************************************
 * @file    video-link-remote.cpp
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
 * @date    2026/3/4
 * @version 1.0
 *******************************************************************************
 */


/* ------- define ----------------------------------------------------------------------------------------------------*/





/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "video-link-remote.h"

#include "task.h"
#include "projdefs.h"



/* ------- class prototypes-------------------------------------------------------------------------------------------*/





/* ------- macro -----------------------------------------------------------------------------------------------------*/





/* ------- variables -------------------------------------------------------------------------------------------------*/





/* ------- function implement ----------------------------------------------------------------------------------------*/

// 构造函数：初始化所有原子控件及其硬件参数映射
VideoLinkRemote::VideoLinkRemote()
    : // --- 1. 初始化 11-bit 模拟摇杆与拨轮 (参数依据典型大疆协议) ---
      // Min=364, Max=1684, Center=1024, Deadzone=20
      _axisRightX({364, 1684, 1024, 20, false}),
      _axisRightY({364, 1684, 1024, 20, false}),
      _axisLeftX({364, 1684, 1024, 20, false}),
      _axisLeftY({364, 1684, 1024, 20, false}),
      _wheel({364, 1684, 1024, 20, false}),

      // --- 2. 初始化开关 (离散控制) ---
      // 假设 modeSw 为三档开关：1->Up(0), 3->Mid(1), 2->Down(2)
      _modeSw({{0, 0},{1, 1}, {2, 2}}, 0),

      // 单 Bit 按键，通常为 0->松开(0), 1->按下(1)
      _pause({{0, 0}, {1, 1}}, 0),
      _fn1({{0, 0}, {1, 1}}, 0),
      _fn2({{0, 0}, {1, 1}}, 0),
      _trigger({{0, 0}, {1, 1}}, 0),

      // --- 3. 初始化鼠标映射 ---
      // 鼠标通常以增量形式发送，将其中心设为 0，最大值根据实际情况标定 (假定32768)
      _mouseX({-32768, 32767, 0, 5, false}),
      _mouseY({-32768, 32767, 0, 5, false}),
      _mouseZ({-32768, 32767, 0, 5, false}),

      // 鼠标按键通常为 0->松开，1->按下
      _mouseLeft({{0, 0}, {1, 1}}, 0),
      _mouseRight({{0, 0}, {1, 1}}, 0),
      _mouseMiddle({{0, 0}, {1, 1}}, 0),

      _axisKeyWS(static_cast<uint8_t>(VideoLinkKeyBit::W), static_cast<uint8_t>(VideoLinkKeyBit::S)),
      _axisKeyAD(static_cast<uint8_t>(VideoLinkKeyBit::D), static_cast<uint8_t>(VideoLinkKeyBit::A)),

      _keyW({{0, 0}, {1, 1}}, 0),
      _keyS({{0, 0}, {1, 1}}, 0),
      _keyA({{0, 0}, {1, 1}}, 0),
      _keyD({{0, 0}, {1, 1}}, 0),
      _keyShift({{0, 0}, {1, 1}}, 0),
      _keyCtrl({{0, 0}, {1, 1}}, 0),
      _keyQ({{0, 0}, {1, 1}}, 0),
      _keyE({{0, 0}, {1, 1}}, 0),
      _keyR({{0, 0}, {1, 1}}, 0),
      _keyF({{0, 0}, {1, 1}}, 0),
      _keyG({{0, 0}, {1, 1}}, 0),
      _keyZ({{0, 0}, {1, 1}}, 0),
      _keyX({{0, 0}, {1, 1}}, 0),
      _keyC({{0, 0}, {1, 1}}, 0),
      _keyV({{0, 0}, {1, 1}}, 0),
      _keyB({{0, 0}, {1, 1}}, 0),

      _lastUpdateTick(0)
{}

// 数据更新：将物理帧分配给各个独立控件
void VideoLinkRemote::updateRaw(const VideoLinkRawData& raw) {
    _axisRightX.updateRaw(raw.ch0);
    _axisRightY.updateRaw(raw.ch1);
    _axisLeftX.updateRaw(raw.ch2);
    _axisLeftY.updateRaw(raw.ch3);

    _modeSw.updateRaw(raw.modeSw);
    _pause.updateRaw(raw.pause);
    _fn1.updateRaw(raw.fn1);
    _fn2.updateRaw(raw.fn2);
    _wheel.updateRaw(raw.wheel);
    _trigger.updateRaw(raw.trigger);

    _mouseX.updateRaw(raw.mouseX);
    _mouseY.updateRaw(raw.mouseY);
    _mouseZ.updateRaw(raw.mouseZ);

    _mouseLeft.updateRaw(raw.mouseLeft);
    _mouseRight.updateRaw(raw.mouseRight);
    _mouseMiddle.updateRaw(raw.mouseMiddle);

    _axisKeyWS.updateRaw(raw.keyMask);
    _axisKeyAD.updateRaw(raw.keyMask);

    _keyW.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::W)) & 0x01);
    _keyS.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::S)) & 0x01);
    _keyA.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::A)) & 0x01);
    _keyD.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::D)) & 0x01);
    _keyShift.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::Shift)) & 0x01);
    _keyCtrl.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::Ctrl)) & 0x01);
    _keyQ.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::Q)) & 0x01);
    _keyE.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::E)) & 0x01);
    _keyR.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::R)) & 0x01);
    _keyF.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::F)) & 0x01);
    _keyG.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::G)) & 0x01);
    _keyZ.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::Z)) & 0x01);
    _keyX.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::X)) & 0x01);
    _keyC.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::C)) & 0x01);
    _keyV.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::V)) & 0x01);
    _keyB.updateRaw((raw.keyMask >> static_cast<uint8_t>(VideoLinkKeyBit::B)) & 0x01);
}

IInputControl* VideoLinkRemote::getKeyControl(uint16_t keyBitMask) {
    switch (keyBitMask) {
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::W)):
            return &_keyW;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::S)):
            return &_keyS;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::A)):
            return &_keyA;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::D)):
            return &_keyD;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::Shift)):
            return &_keyShift;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::Ctrl)):
            return &_keyCtrl;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::Q)):
            return &_keyQ;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::E)):
            return &_keyE;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::R)):
            return &_keyR;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::F)):
            return &_keyF;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::G)):
            return &_keyG;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::Z)):
            return &_keyZ;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::X)):
            return &_keyX;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::C)):
            return &_keyC;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::V)):
            return &_keyV;
        case (1u << static_cast<uint8_t>(VideoLinkKeyBit::B)):
            return &_keyB;
        default:
            return nullptr;
    }
}

// 统一路由映射 (通过枚举获取对应的控件对象指针)
IInputControl* VideoLinkRemote::getAxis(AxisID id) {
    switch (id) {
        case AxisID::MoveX:
            return &_axisRightY;   // 左摇杆Y轴控制底盘前后
        case AxisID::MoveY:
            return &_axisRightX;   // 左摇杆X轴控制底盘左右
        case AxisID::ViewYaw:
            return &_axisLeftY;  // 右摇杆X轴控制云台Yaw
        case AxisID::ViewPitch:
            return &_axisLeftX;  // 右摇杆Y轴控制云台Pitch
        case AxisID::Aux1:
            return &_wheel;       // 辅助拨轮
        default:
            return nullptr;
    }
}

IInputControl* VideoLinkRemote::getButton(ButtonID id) {
    switch (id) {
        case ButtonID::ModeSwitch:
            return &_modeSw;      // 主模式切换
        case ButtonID::FuncSwitch:
            return &_trigger;     // 发射触发
        case ButtonID::KeyA:
            return &_fn1;         // 自定义功能键 A
        case ButtonID::KeyB:
            return &_fn2;         // 自定义功能键 B
        default:
            return nullptr;
    }
}

bool VideoLinkRemote::isConnected() const {
    // 50ms 超时判定看门狗
    return (xTaskGetTickCount() - _lastUpdateTick) <= pdMS_TO_TICKS(50);
}

void VideoLinkRemote::onDataReceived() {
    _lastUpdateTick = xTaskGetTickCount();
}