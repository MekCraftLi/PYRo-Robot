/**
 *******************************************************************************
 * @file    commander.h
 * @brief   输入指挥服务 — 桥接物理控件 → 触发器 → Action → 黑板指令
 *
 * 架构层次 (4 层输入管线):
 *
 *   Layer I:   IInputControl   物理控件 (摇杆/开关/鼠标/键盘)
 *                                 ↓ get() 归一化值 [-1.0, 1.0]
 *   Layer II:  InputTrigger    触发器 (死区/保持/边沿/点击/切换)
 *                                 ↓ update(value, dt) → TriggerState
 *   Layer III: InputAction     绑定层 (控件 + 可选触发器)
 *                                 ↓ isTriggered() / getValue()
 *   Layer IV:  CommanderSrvc   仲裁器 (控制源裁决 → 指令填充 → 写黑板)
 *
 * 数据流:
 *   Remote → CommanderSrvc::run()
 *          → Blackboard (gimbalCmd / shootCmd / g2cOutput)
 *
 * 控件映射 (编译时由 REMOTE_DEVICE 宏选择):
 *   DR16:       左右摇杆 + 左右开关 + 拨轮
 *   VideoLink:  摇杆 + 鼠标 + 键盘 + 扳机
 *   Gamepad:    扳机 + 摇杆 + 按键
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/27
 * @version 2.0
 *******************************************************************************
 */

#ifndef INFANTRY_COMMANDER_H
#define INFANTRY_COMMANDER_H

/*-------- includes
 * ---------------------------------------------------------------------------------------------------*/

#ifdef __cplusplus

#include "../DataHub/blackboard.h"
#include "System/Thread/application-base.h"
#include "tools/crtp.h"

#include "Board-Support-Pack/DR16/dr16.h"
#include "System/Input/ControlImpl/control-impl-axis.h"
#include "System/Input/ControlImpl/control-impl-switch.h"
#include "System/Input/TriggerImpl/trigger-decorator-toggle.h"
#include "System/Input/TriggerImpl/trigger-impl-click.h"
#include "System/Input/TriggerImpl/trigger-impl-edge.h"
#include "System/Input/TriggerImpl/trigger-impl-hold.h"
#include "System/Input/TriggerImpl/trigger-impl-linear.h"
#include "System/Input/TriggerImpl/trigger-impl-match.h"
#include "System/Input/action.h"

#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_VIDEO_LINK
#include "Board-Support-Pack/VideoLink/video-link-remote.h"
#endif
#endif

#include "usart.h"

/*-------- class
 * ------------------------------------------------------------------------------------------------------*/

class CommanderSrvc final : public PeriodicApp,
                            public Singleton<CommanderSrvc> {
public:
  CommanderSrvc();

  void init() override;
  void run() override;

private:
  // --- 远程设备引用 ---
#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_DR16
  RemoteBase &remote = RemoteDR16::instance();
#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
  RemoteBase &remote = VideoLinkRemote::instance();
#endif
#endif

#if REMOTE_DEVICE != REMOTE_GAMEPAD || defined(CHASSIS)

  // ── Action 槽位索引 ──────────────────────────────────────────
  //   使用 enum 替代魔法数字, 便于维护和自动迭代.
  // ──────────────────────────────────────────────────────────────
  enum ActionSlot : uint8_t {
    CTRL_MODE,     // 控制源切换开关
    MOVE_X,        // 底盘前后平移 (摇杆/扳机)
    MOVE_Y,        // 底盘左右平移
    MOVE_X_KEY,    // 底盘前后平移 (键盘 W/S)
    MOVE_Y_KEY,    // 底盘左右平移 (键盘 A/D)
    YAW,           // 云台偏航 (摇杆)
    PITCH,         // 云台俯仰 (摇杆)
    MOUSE_YAW,     // 云台偏航 (鼠标 X)
    MOUSE_PITCH,   // 云台俯仰 (鼠标 Y)
    MOUSE_BURST,   // 鼠标左键连发
    MOUSE_SINGLE,  // 鼠标左键单发
    MOUSE_VISION,  // 鼠标右键视觉瞄准
    FRIC_TOGGLE,   // 摩擦轮开关切换
    SHOOT_BURST,   // 连发触发
    SHOOT_SINGLE,  // 单发触发
    SPIN_MODE,     // 小陀螺模式切换
    KEY_SPIN,      // 键盘 Shift 旋转
    CAP_SWITCH,    // 超级电容开关
    KEYBOARD_FRIC, // 键盘 Q 摩擦轮切换
    ACTION_COUNT
  };

  InputAction _actions[ACTION_COUNT];

  // --- 便捷别名 (指向 _actions 槽位) ---
  InputAction &actionCtrlMode = _actions[CTRL_MODE];
  InputAction &actionMoveX = _actions[MOVE_X];
  InputAction &actionMoveY = _actions[MOVE_Y];
  InputAction &actionMoveXKey = _actions[MOVE_X_KEY];
  InputAction &actionMoveYKey = _actions[MOVE_Y_KEY];
  InputAction &actionYaw = _actions[YAW];
  InputAction &actionPitch = _actions[PITCH];
  InputAction &actionMouseYaw = _actions[MOUSE_YAW];
  InputAction &actionMousePitch = _actions[MOUSE_PITCH];
  InputAction &actionMouseBurst = _actions[MOUSE_BURST];
  InputAction &actionMouseSingle = _actions[MOUSE_SINGLE];
  InputAction &actionMouseVision = _actions[MOUSE_VISION];
  InputAction &actionFricToggle = _actions[FRIC_TOGGLE];
  InputAction &actionShootBurst = _actions[SHOOT_BURST];
  InputAction &actionShootSingle = _actions[SHOOT_SINGLE];
  InputAction &actionSpinMode = _actions[SPIN_MODE];
  InputAction &actionKeySpin = _actions[KEY_SPIN];
  InputAction &actionCapSwitch = _actions[CAP_SWITCH];
  InputAction &actionKeyboardFric = _actions[KEYBOARD_FRIC];

  // ── 触发器 ────────────────────────────────────────────────────
  // ──────────────────────────────────────────────────────────────

  // --- 通用 ---
  TriggerLinear _joystickDeadzone{0.02f}; // 摇杆死区过滤器

  // --- 模式仲裁 ---
  TriggerHold _work{-0.25f, 0.5f, false,
                    HoldCondition::LessOrEqual}; // 控制源开关判定

  // --- 射击系统 ---
  TriggerEdge _trigFricToggle{-0.5f, EdgeType::Rising}; // 摩擦轮切换 (上升沿)
  TriggerHold _triggerBurst{0.5f, 0.8f, false,
                            HoldCondition::GreaterOrEqual}; // 连发保持判定
  TriggerHold _triggerMouseBurst{0.5f, 0.3f, false,
                                 HoldCondition::GreaterOrEqual};
  TriggerEdge _trigSingleRelease{0.23f, EdgeType::Falling}; // 单发释放触发
  TriggerHold _trigVision{0.5f, 0.001f, false,
                          HoldCondition::GreaterOrEqual}; // 视觉瞄准保持

  // --- 运动/切换 ---
  TriggerHold _trigShiftHold{0.5f, 0.001f, true,
                             HoldCondition::GreaterOrEqual}; // Shift 键保持
  TriggerHold _trigKeyboardFric{0.5f, 0.001f, true,
                                HoldCondition::GreaterOrEqual}; // Q 键触发

  // --- Toggle 装饰器 (包装基础触发器) ---
  TriggerHold _baseInstantTrigger{
      0.5f, 0.001f, true, HoldCondition::GreaterOrEqual}; // 即时单次触发基座
  TriggerHold _baseContinuousTrigger{
      0.5f, 0.001f, false, HoldCondition::GreaterOrEqual}; // 持续触发基座
  TriggerToggle _trigSpin{_baseInstantTrigger, false};     // 小陀螺切换
  TriggerToggle _trigSpinKey{_trigShiftHold, false};       // Shift 小陀螺切换

#else
  // ── Gamepad 专用 ──────────────────────────────────────────────
  InputAction _actions[5];

  enum ActionSlot : uint8_t {
    RELAX,
    HANDBRAKE,
    MOVE_X,
    YAW,
    BRAKE,
    ACTION_COUNT
  };

  TriggerLinear _joystickDeadzone{0.02f};
  TriggerHold _handbreak{0.0f, 0.001f, false, HoldCondition::GreaterOrEqual};
  TriggerHold _aTest{0.0f, 0.001f, true, HoldCondition::GreaterOrEqual};
  TriggerToggle _relax{_aTest, true};

  InputAction &actionRelax = _actions[RELAX];
  InputAction &actionHandbrakeDepth = _actions[HANDBRAKE];
  InputAction &actionMoveX = _actions[MOVE_X];
  InputAction &actionYaw = _actions[YAW];
  InputAction &actionBreak = _actions[BRAKE];
#endif

  // --- 私有辅助方法 ---
  void resolveChassisMode(bool spinRequested, bool capRequested,
                          GimbalToChassisComm &comm);
  void resolveMovement(GimbalCmd &gCmd, GimbalToChassisComm &comm);
  void resolveShootEvents(ShootCmd &sCmd, bool burstAllowed = true);
};

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif
