/**
 *******************************************************************************
 * @file    video-link-remote.h
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


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_VIDEO_LINK_REMOTE_H
#define INFANTRY_VIDEO_LINK_REMOTE_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "Adapter/adapter-remote.h"
#include "FreeRTOS.h"
#include "System/Input/ControlImpl/control-impl-axis.h"
#include "System/Input/ControlImpl/control-impl-switch.h"
#include "System/crtp.h"


#include <cstdint>


/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/
/**
 * @brief 图传链路遥控器原始数据帧结构
 * @note 采用 GCC packed 属性保证内存紧凑排列，直接映射底层 DMA 接收的字节流
 */
typedef struct __attribute__((packed)) {
    uint8_t sof1;
    uint8_t sof2;

    // 遥控器通道 11-bit 位域
    uint64_t ch0     : 11;
    uint64_t ch1     : 11;
    uint64_t ch2     : 11;
    uint64_t ch3     : 11;

    // 开关与按键位域
    uint64_t modeSw  : 2;
    uint64_t pause   : 1;
    uint64_t fn1     : 1;
    uint64_t fn2     : 1;
    uint64_t wheel   : 11;
    uint64_t trigger : 1;

    // 鼠标数据
    int16_t mouseX;
    int16_t mouseY;
    int16_t mouseZ;
    uint8_t mouseLeft   : 2;
    uint8_t mouseRight  : 2;
    uint8_t mouseMiddle : 2;

    // 键盘掩码与校验
    uint16_t keyMask;
    uint16_t crc16;
} VideoLinkRawData;

enum class VideoLinkKeyBit : uint8_t {
    W = 0,
    S = 1,
    A = 2,
    D = 3,
    Shift = 4,
    Ctrl = 5,
    Q = 6,
    E = 7,
    R = 8,
    F = 9,
    G = 10,
    Z = 11,
    X = 12,
    C = 13,
    V = 14,
    B = 15
};

/**
 * @brief 图传链路遥控器驱动类
 */
class VideoLinkRemote : public RemoteBase, public Singleton<VideoLinkRemote> {
  public:
    VideoLinkRemote();

    // 核心数据更新接口
    void updateRaw(const VideoLinkRawData& raw);

    // =============================================================
    // 实现 RemoteBase 接口：语义轴映射
    // =============================================================
    IInputControl* getAxis(AxisID id) override;
    IInputControl* getButton(ButtonID id) override;

    [[nodiscard]] bool isConnected() const override;
    void onDataReceived() override;

    // =============================================================
    // 暴露特定于图传链路遥控器的独立控件接口
    // =============================================================
    // 3. 暴露控件接口 (供绑定层使用)
    // 架构原则: 返回基类指针 IInputControl*，隐藏具体实现
    IInputControl* getLeftX() { return &_axisLeftX; }
    IInputControl* getLeftY() { return &_axisLeftY; }
    IInputControl* getRightX() { return &_axisRightX; }
    IInputControl* getRightY() { return &_axisRightY; }
    IInputControl* getModeSw() { return &_modeSw; }
    IInputControl* getPause() { return &_pause; }
    IInputControl* getFn1() { return &_fn1; }
    IInputControl* getFn2() { return &_fn2; }
    IInputControl* getTrigger() { return &_trigger; }
    IInputControl* getWheel() { return &_wheel; }
    IInputControl* getMouseX() { return &_mouseX; }
    IInputControl* getMouseY() { return &_mouseY; }
    IInputControl* getMouseZ() { return &_mouseZ; }
    IInputControl* getMouseLeft() { return &_mouseLeft; }
    IInputControl* getMouseRight() { return &_mouseRight; }
    IInputControl* getMouseMiddle() { return &_mouseMiddle; }

    IInputControl* getAxisKeyWS() { return &_axisKeyWS; }
    IInputControl* getAxisKeyAD() { return &_axisKeyAD; }

    IInputControl* getKeyW() { return &_keyW; }
    IInputControl* getKeyS() { return &_keyS; }
    IInputControl* getKeyA() { return &_keyA; }
    IInputControl* getKeyD() { return &_keyD; }
    IInputControl* getKeyShift() { return &_keyShift; }
    IInputControl* getKeyCtrl() { return &_keyCtrl; }
    IInputControl* getKeyQ() { return &_keyQ; }
    IInputControl* getKeyE() { return &_keyE; }
    IInputControl* getKeyR() { return &_keyR; }
    IInputControl* getKeyF() { return &_keyF; }
    IInputControl* getKeyG() { return &_keyG; }
    IInputControl* getKeyZ() { return &_keyZ; }
    IInputControl* getKeyX() { return &_keyX; }
    IInputControl* getKeyC() { return &_keyC; }
    IInputControl* getKeyV() { return &_keyV; }
    IInputControl* getKeyB() { return &_keyB; }

    IInputControl* getKeyControl(uint16_t keyBitMask);

  private:
    friend class Singleton<VideoLinkRemote>;

    // 实体化控件 (映射图传链路数据结构)
    ControlAxis _axisRightX; // ch0
    ControlAxis _axisRightY; // ch1
    ControlAxis _axisLeftX;  // ch2
    ControlAxis _axisLeftY;  // ch3
    ControlAxis _wheel;      // wheel

    ControlSwitch _modeSw;  // mode_sw
    ControlSwitch _pause;   // pause
    ControlSwitch _fn1;     // fn_1
    ControlSwitch _fn2;     // fn_2
    ControlSwitch _trigger; // trigger

    // 鼠标控件映射 (假设使用类似摇杆的归一化)
    ControlAxis _mouseX;
    ControlAxis _mouseY;
    ControlAxis _mouseZ;
    ControlSwitch _mouseLeft;
    ControlSwitch _mouseRight;
    ControlSwitch _mouseMiddle;

    // 键盘虚拟轴
    ControlMaskAxis _axisKeyWS;
    ControlMaskAxis _axisKeyAD;

    // 键盘按键 bit0~bit15
    ControlSwitch _keyW;
    ControlSwitch _keyS;
    ControlSwitch _keyA;
    ControlSwitch _keyD;
    ControlSwitch _keyShift;
    ControlSwitch _keyCtrl;
    ControlSwitch _keyQ;
    ControlSwitch _keyE;
    ControlSwitch _keyR;
    ControlSwitch _keyF;
    ControlSwitch _keyG;
    ControlSwitch _keyZ;
    ControlSwitch _keyX;
    ControlSwitch _keyC;
    ControlSwitch _keyV;
    ControlSwitch _keyB;

    TickType_t _lastUpdateTick;
};

/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_VIDEO_LINK_REMOTE_H*/
