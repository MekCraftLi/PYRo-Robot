/**
 *******************************************************************************
 * @file    commander.cpp
 * @brief   输入指挥服务 — 4 层输入管线实现
 *
 * 管线流程 (每 tick):
 *   1. 读取遥控器硬件缓存 (DMA → updateRaw)
 *   2. 驱动所有 Action 更新 (控件值 → 触发器判定)
 *   3. 仲裁控制源 (SAFE_STOP / REMOTE / VISION)
 *   4. 根据控制源填充指令 (云台 / 底盘 / 射击)
 *   5. 写入黑板 (Blackboard)
 *
 * 仲裁规则:
 *   - 遥控器离线 → SAFE_STOP (急停)
 *   - DR16:  右开关上/中 → REMOTE, 下 → VISION
 *   - 图传:  开关 < 0 → SAFE_STOP, 鼠标右键 → VISION,
 *            其余 → REMOTE
 *******************************************************************************
 * @author  MekLi
 * @date    2026/2/27
 * @version 2.0
 *******************************************************************************
 */

#include "commander.h"
#include "Config/config.h"
#include "System/DataHub/blackboard.h"
#include "pyro_bsp_uart.h"

#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_VIDEO_LINK
#include "video-link-remote.h"
#elif REMOTE_DEVICE == REMOTE_GAMEPAD
#include "Board-Support-Pack/GamePad/bluetooth-gamepad.h"
#endif
#endif

#include <algorithm>

/* -------- 应用属性 -------------------------------------------------------------------------------------------------
 */

#define APPLICATION_ENABLE     true
#define APPLICATION_NAME       "Input"
#define APPLICATION_STACK_SIZE 512
#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

[[maybe_unused]] static auto& forceInit = CommanderSrvc::instance();

/* -------- 遥控器 DMA 缓存 -------------------------------------------------------------------------------------------
 */

#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_DR16
static Dr16Data _dr16Data;
#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
static VideoLinkRawData _videoLinkRawData;
#elif REMOTE_DEVICE == REMOTE_GAMEPAD
static GamepadRawData _gamepadRawData;
#endif
#endif

/* -------- 辅助函数 -------------------------------------------------------------------------------------------------
 */

namespace {
inline float clampUnit(float v) { return std::max(-1.0f, std::min(1.0f, v)); }
inline float composeAxis(float a, float b) { return clampUnit(a + b); }
} // namespace

/* -------- 构造 -----------------------------------------------------------------------------------------------------
 */

CommanderSrvc::CommanderSrvc()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 10) {
    // 所有触发器已在头文件中通过 in-class 初始化完成, 无需此处重复.
}

/* -------- 初始化: 绑定物理控件 → Action -------------------------------------------------- */

void CommanderSrvc::init() {
    // ── 1. Action 映射绑定 (物理控件 + 触发器 → Action) ──

#if REMOTE_DEVICE == REMOTE_DR16
    //
    //  DR16 遥控器布局:
    //    左摇杆 Y → 前后 (MoveX)      左摇杆 X → 左右 (MoveY)
    //    右摇杆 X → 偏航 (Yaw)        右摇杆 Y → 俯仰 (Pitch)
    //    右开关   → 控制源仲裁         左开关   → 射击系统
    //    拨轮     → 小陀螺切换
    //
    actionMoveX.bind(RemoteDR16::instance().getLeftY(), &_joystickDeadzone);
    actionMoveY.bind(RemoteDR16::instance().getLeftX(), &_joystickDeadzone);
    actionYaw.bind(RemoteDR16::instance().getRightX(), &_joystickDeadzone);
    actionPitch.bind(RemoteDR16::instance().getRightY(), &_joystickDeadzone);

    // 控制源仲裁: 右开关
    actionCtrlMode.bind(RemoteDR16::instance().getSwRight(), &_work);

    // 射击: 左开关 → 摩擦轮 / 连发 / 单发
    actionFricToggle.bind(RemoteDR16::instance().getSwLeft(), &_trigFricToggle);
    actionShootBurst.bind(RemoteDR16::instance().getSwLeft(), &_triggerBurst);
    actionShootSingle.bind(RemoteDR16::instance().getSwLeft(), &_trigSingleRelease);

    // 小陀螺: 拨轮 toggle
    actionSpinMode.bind(RemoteDR16::instance().getWheel(), &_trigSpin);

#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
#ifdef GIMBAL
    //
    //  图传链路布局:
    //    右摇杆 Y/X → 底盘前后/左右     W/S, A/D → 键盘辅助
    //    摇杆偏航/俯仰 → 云台            鼠标 X/Y → 云台辅助
    //    鼠标左键 → 连发/单发            鼠标右键 → 视觉瞄准
    //    模式开关 → 控制源仲裁           Fn2 / Q → 摩擦轮切换
    //    Pause → 小陀螺                 Shift → 键盘小陀螺
    //
    auto& vr = VideoLinkRemote::instance();

    // 底盘: 摇杆 + 键盘
    actionMoveX.bind(vr.getRightY(), &_joystickDeadzone);
    actionMoveY.bind(vr.getRightX(), &_joystickDeadzone);
    actionMoveXKey.bind(vr.getAxisKeyWS());
    actionMoveYKey.bind(vr.getAxisKeyAD());

    // 云台: 摇杆 + 鼠标
    actionYaw.bind(vr.getAxis(AxisID::ViewYaw), &_joystickDeadzone);
    actionPitch.bind(vr.getAxis(AxisID::ViewPitch), &_joystickDeadzone);
    actionMouseYaw.bind(vr.getMouseX(), &_joystickDeadzone);
    actionMousePitch.bind(vr.getMouseY(), &_joystickDeadzone);

    // 射击: 扳机 + 鼠标
    actionMouseBurst.bind(vr.getMouseLeft(), &_triggerMouseBurst);
    actionMouseSingle.bind(vr.getMouseLeft(), &_trigSingleRelease);
    actionShootBurst.bind(vr.getTrigger(), &_triggerBurst);
    actionShootSingle.bind(vr.getTrigger(), &_trigSingleRelease);
    actionMouseVision.bind(vr.getMouseRight(), &_trigVision);

    // 控制源 + 摩擦轮
    actionCtrlMode.bind(vr.getModeSw(), &_work);
    actionFricToggle.bind(vr.getFn2(), &_trigFricToggle);
    actionKeyboardFric.bind(vr.getKeyQ(), &_trigFricToggle);

    // 运动/电容
    actionSpinMode.bind(vr.getPause(), &_trigSpin);
    actionKeySpin.bind(vr.getKeyShift(), &_trigSpinKey);
    actionCapSwitch.bind(vr.getKeyC(), &_baseContinuousTrigger);
#endif

#elif REMOTE_DEVICE == REMOTE_GAMEPAD
    //
    //  蓝牙手柄布局:
    //    右扳机 → 前进     左扳机 → 刹车
    //    摇杆偏航 → 转向   A 键 → 放松    B 键 → 手刹
    //
    actionMoveX.bind(BluetoothGamepad::instance().getRightTrigger(), &_joystickDeadzone);
    actionBreak.bind(BluetoothGamepad::instance().getLeftTrigger(), &_joystickDeadzone);
    actionYaw.bind(BluetoothGamepad::instance().getAxis(AxisID::ViewYaw), &_joystickDeadzone);
    actionRelax.bind(BluetoothGamepad::instance().getButtonA(), &_relax);
    actionHandbrakeDepth.bind(BluetoothGamepad::instance().getButtonB(), &_handbreak);
#endif

    // ── 2. 初始化 UART DMA 接收 ──

#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_DR16
    pyro::bsp_uart::get_uart5().add_rx_event_callback(
        [](uint8_t* p, uint16_t size, BaseType_t&) -> bool {
            if (size <= sizeof(Dr16Data)) {
                memcpy(&_dr16Data, p, size);
                RemoteDR16::instance().onDataReceived();
            }
            // Restart DMA reception for the next incoming packet.
            // 为下一个传入的数据包重新启动 DMA 接收。
            pyro::bsp_uart::get_uart5().enable_rx_dma();
            return true;
        },
        reinterpret_cast<uint32_t>(this));
    pyro::bsp_uart::get_uart5().enable_rx_dma();

#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
    pyro::bsp_uart::get_uart1().add_rx_event_callback(
        [](uint8_t* p, uint16_t size, BaseType_t&) -> bool {
            if (size <= sizeof(VideoLinkRawData)) {
                memcpy(&_videoLinkRawData, p, size);
                VideoLinkRemote::instance().onDataReceived();
            }
            return true;
        },
        reinterpret_cast<uint32_t>(this));
    pyro::bsp_uart::get_uart1().enable_rx_dma();

#elif REMOTE_DEVICE == REMOTE_GAMEPAD
    pyro::bsp_uart::get_uart7().add_rx_event_callback(
        [](uint8_t* p, uint16_t size, BaseType_t&) -> bool {
            if (size <= sizeof(GamepadRawData)) {
                memcpy(&_gamepadRawData, p, size);
                BluetoothGamepad::instance().onDataReceived();
            }
            return true;
        },
        reinterpret_cast<uint32_t>(this));
    pyro::bsp_uart::get_uart7().enable_rx_dma();
#endif
#endif
}

/* -------- 私有辅助方法 ----------------------------------------------------------------------------------------------
 */

void CommanderSrvc::resolveChassisMode(bool spinRequested, bool capRequested, GimbalToChassisComm& comm) {
    comm.msg.mode = spinRequested ? CHASSIS_SPIN : CHASSIS_NORMAL;

    // 电容开关标志 (bit 2)
    if (capRequested)
        comm.msg.mode |= 0x04;
    else
        comm.msg.mode &= ~0x04;
}

void CommanderSrvc::resolveMovement(GimbalCmd& gCmd, GimbalToChassisComm& comm) {
#if REMOTE_DEVICE == REMOTE_VIDEO_LINK
    // 复合轴: 摇杆 + 键盘 → 底盘速度
    const float moveXInput = composeAxis(actionMoveX.getValue(), actionMoveXKey.getValue());
    const float moveYInput = composeAxis(actionMoveY.getValue(), actionMoveYKey.getValue());
    // 复合轴: 鼠标 × 灵敏度 + 摇杆 → 云台角速度
    const float yawInput =
        composeAxis(actionMouseYaw.getValue() * Config::Algorithm::Input::Y_SENSITIVITY, actionYaw.getValue());
    const float pitchInput =
        composeAxis(actionMousePitch.getValue() * Config::Algorithm::Input::X_SENSITIVITY, actionPitch.getValue());
#else
    // DR16: 直接摇杆映射
    const float moveXInput = actionMoveX.getValue();
    const float moveYInput = actionMoveY.getValue();
    const float yawInput   = actionYaw.getValue();
    const float pitchInput = actionPitch.getValue();
#endif

    // 遥控器方向与运动坐标系部分相反, 取负补偿
    comm.msg.vx   = moveXInput * Config::Algorithm::Chassis::MAX_VX * 10;
    comm.msg.vy   = -moveYInput * Config::Algorithm::Chassis::MAX_VY * 10;
    gCmd.yawVel   = -yawInput * Config::Algorithm::Gimbal::MAX_YAW_SPEED;
    gCmd.pitchVel = -pitchInput * Config::Algorithm::Gimbal::MAX_PITCH_SPEED;
}

void CommanderSrvc::resolveShootEvents(ShootCmd& sCmd, bool burstAllowed) {
    // 摩擦轮切换 (遥控器开关 或 键盘 Q)
    if (actionFricToggle.isTriggered() || actionKeyboardFric.isTriggered()) {
        sCmd.event = ShootEvent::FRIC_TOGGLE;
    }

    // 连发: 遥控器扳机 或 鼠标左键
    if (burstAllowed && (actionShootBurst.isTriggered() || actionMouseBurst.isTriggered())) {
        sCmd.state.burstShot = 1;
    } else {
        sCmd.state.burstShot = 0;
    }

    // 单发 (优先级高于连发)
    if (actionShootSingle.isTriggered() || actionMouseSingle.isTriggered()) {
        sCmd.event           = ShootEvent::SINGLE_FIRE;
        sCmd.state.burstShot = 0;
    }
}

/* -------- 主循环 ----------------------------------------------------------------------------------------------------
 */

void CommanderSrvc::run() {
    // ── 0. 计算时间步长 ──
    static uint32_t lastTick = xTaskGetTickCount();
    uint32_t nowTick         = xTaskGetTickCount();
    float dt                 = (float)(nowTick - lastTick) / 1000.0f;
    if (dt <= 0.0f)
        dt = 0.001f;
    lastTick = nowTick;

    // ── 1. 更新遥控器原始数据 ──
#ifdef GIMBAL
#if REMOTE_DEVICE == REMOTE_DR16
    RemoteDR16::instance().updateRaw(_dr16Data);
#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
    VideoLinkRemote::instance().updateRaw(_videoLinkRawData);
#elif REMOTE_DEVICE == REMOTE_GAMEPAD
    BluetoothGamepad::instance().updateRaw(_gamepadRawData);
#endif
#endif

    // ── 2. 驱动所有 Action 更新 ──
    for (auto& action : _actions)
        action.update(dt);

    // ── 3~5. 仲裁 → 填充指令 → 写入黑板 ──

#ifdef GIMBAL

#if REMOTE_DEVICE != REMOTE_GAMEPAD
    // ========================================
    //  DR16 / VideoLink 共用仲裁逻辑
    // ========================================

    GimbalCmd gCmd{};
    ShootCmd sCmd{};
    GimbalToChassisComm comm{};

    // ── 3. 仲裁控制源 ──
    ControlSource currentSource = ControlSource::SAFE_STOP;

    if (remote.isConnected()) {
        float swState = actionCtrlMode.getValue();

#if REMOTE_DEVICE == REMOTE_DR16
        // DR16: 右开关上方/中位 → REMOTE, 下方 → VISION
        if (actionCtrlMode.isTriggered()) {
            currentSource = (swState > -0.5f) ? ControlSource::REMOTE : ControlSource::VISION;
        }
#elif REMOTE_DEVICE == REMOTE_VIDEO_LINK
        // VideoLink: 模式开关 < 0 → SAFE_STOP, 鼠标右键或开关 > 0 → VISION
        if (swState < 0) {
            currentSource = ControlSource::SAFE_STOP;
        } else if (swState > 0 || actionMouseVision.isTriggered()) {
            currentSource = ControlSource::VISION;
        } else {
            currentSource = ControlSource::REMOTE;
        }
#endif
    }

    // ── 4. 读取黑板历史值 (未修改字段天然保留) ──
    ImuState imuState{};
    Blackboard::instance().gimbalCmd.read(gCmd);
    Blackboard::instance().shootCmd.read(sCmd);
    Blackboard::instance().imuState.read(imuState);
    sCmd.event               = ShootEvent::NONE;

    // 预计算运动意图 (REMOTE / VISION 共用)
    const bool spinRequested = actionSpinMode.isTriggered() || actionKeySpin.isTriggered();
    const bool capRequested  = actionCapSwitch.isTriggered();

    switch (currentSource) {
        case ControlSource::SAFE_STOP: {
            comm.msg.mode        = (uint8_t)CHASSIS_RELAX;
            sCmd.event           = ShootEvent::EMERGENCY_STOP;
            sCmd.state.burstShot = 0;
            gCmd.mode            = GIMBAL_RELAX;
            gCmd.yawVel          = 0;
            gCmd.pitchVel        = 0;
        } break;

        case ControlSource::REMOTE: {
            resolveChassisMode(spinRequested, capRequested, comm);
            resolveMovement(gCmd, comm);

            gCmd.mode           = GIMBAL_NORMAL;
            gCmd.targetYawSpeed = 0;

            resolveShootEvents(sCmd);
        } break;

        case ControlSource::VISION: {
            resolveChassisMode(spinRequested, capRequested, comm);
            resolveMovement(gCmd, comm);

            gCmd.mode = GIMBAL_AUTO;

            // 视觉指令覆盖云台目标
            VisionCommand vCmd{};
            Blackboard::instance().visionCmd.read(vCmd);
            gCmd.targetYaw             = vCmd.targetYaw;
            gCmd.targetPitch           = -vCmd.targetPitch;
            gCmd.targetYawSpeed        = vCmd.targetYawSpeed;
            gCmd.pitchVel              = -vCmd.targetPitchSpeed;
            gCmd.targetYawAcceleration = vCmd.targetYawAcceleration;

            // 连发需视觉允许开火
            resolveShootEvents(sCmd, vCmd.fireCommand);
        } break;

        default:
            break;
    }

    gCmd.timestamp = nowTick;

#else
    // ========================================
    //  Gamepad 专用逻辑
    // ========================================

    GimbalCmd gCmd{};
    GimbalToChassisComm comm{};
    Blackboard::instance().gimbalCmd.read(gCmd);

    if (actionRelax.isTriggered()) {
        gCmd.mode     = GIMBAL_RELAX;
        comm.msg.mode = CHASSIS_RELAX;
        gCmd.yawVel   = 0;
        gCmd.pitchVel = 0;
    } else {
        gCmd.mode     = GIMBAL_NORMAL;
        comm.msg.mode = CHASSIS_NORMAL;
        gCmd.yawVel   = actionYaw.getValue();

        float pureVel = actionMoveX.getValue() - actionBreak.getValue();
        if (pureVel < 0)
            pureVel = 0;
        comm.msg.vx = pureVel * Config::Algorithm::Chassis::MAX_VX * 10;
    }
#endif

    // ── 5. 写入黑板 ──
    Blackboard::instance().g2cOutput.write(comm);
    Blackboard::instance().gimbalCmd.write(gCmd);
    Blackboard::instance().shootCmd.write(sCmd);

#elif defined(CHASSIS)
    // ========================================
    //  底盘板逻辑: 转发云台指令 → 底盘指令
    // ========================================

    GimbalToChassisComm comm{};
    ChassisCmd cmd{};
    Blackboard::instance().rComm.read(comm);

    cmd.mode = comm.msg.mode;
    cmd.vx   = (float)comm.msg.vx / 10;
    cmd.vy   = (float)comm.msg.vy / 10;

    if ((cmd.mode & 0x03) == CHASSIS_NORMAL)
        cmd.vw = 0;
    else if ((cmd.mode & 0x03) == CHASSIS_SPIN)
        cmd.vw = Config::Algorithm::Chassis::MAX_VW;

    Blackboard::instance().chassisCmd.write(cmd);

#endif
}
