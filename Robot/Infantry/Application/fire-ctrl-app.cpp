/**
 *******************************************************************************
 * @file    fire-ctrl-app.cpp
 * @brief   拨弹摩擦火控 FSM 应用 — 主循环、PID 运算、物理发弹检测
 *
 * 主循环 (run) 执行顺序:
 *   1. 读取黑板输入 (指令、反馈、裁判数据)
 *   2. 热量/弹速同步 & 物理发弹检测
 *   3. 边沿事件提取
 *   4. 驱动 FSM (仅设定目标, 不做 PID)
 *   5. 统一 PID 计算输出电流
 *   6. 写入黑板输出
 *******************************************************************************
 */

#include "fire-ctrl-app.h"
#include "Config/Gimbal/hw-config.h"
#include "System/DataHub/blackboard.h"
#include "pyro_dwt_drv.h"
#include "referee-data-hub.h"

/* -------- Ozone 调试探针 (仅用于实时波形观测) -------------------------------------------------- */

struct HeatDebugOzone {
    float local_heat;       // 算法预估的本地实时热量
    float referee_heat;     // 裁判系统真实下发的热量 (带延迟)
    float heat_limit;       // 热量上限
    float safe_margin_line; // 安全警戒线 (limit - safe_margin)
    float target_rpm;       // 拨弹电机目标转速
    uint8_t physical_shot;  // 物理发弹脉冲 (每发一次跳变)
} g_heat_debug;

volatile struct SpeedDebugOzone {
    float ref_bullet_speed;    // 裁判系统回传的真实弹速 (m/s)
    float target_bullet_speed; // 期望压制弹速 (m/s)
    float base_fric_target;    // 基础摩擦轮设定转速
    float final_fric_target;   // 经闭环补偿后的最终目标转速
    float fric_left_real;     // 左摩擦轮实际反馈转速
    float comp_integration;   // 补偿增量
    uint8_t physical_shot;    // 物理发弹脉冲
} g_speed_debug;
debug_trigger_t debug_trigger;

// 弹速闭环补偿开关 (Ozone/调试器运行时可写，置 false 直接旁路 SpeedCompensator)
volatile bool g_enable_speed_compensation = false;
/* -------- 应用属性 -------------------------------------------------------------------------------------------------- */

#define APPLICATION_ENABLE     true
#define APPLICATION_NAME       "FireCtrl"
#define APPLICATION_STACK_SIZE 512
#define APPLICATION_PRIORITY   4

static StackType_t appStack[APPLICATION_STACK_SIZE];

[[maybe_unused]] static auto& forceInit = FireCtrlApp::instance();

/* -------- 构造 & 生命周期 ------------------------------------------------------------------------------------------- */

FireCtrlApp::FireCtrlApp()
    : PeriodicApp(APPLICATION_ENABLE, APPLICATION_NAME, APPLICATION_STACK_SIZE, appStack, APPLICATION_PRIORITY, 1),
      _ctx() {}

void FireCtrlApp::init() {
    _fsm.change_state(&_statePassive);
    _fsm.enter(&_ctx);
}

/* -------- 主循环 ---------------------------------------------------------------------------------------------------- */

void FireCtrlApp::run() {
    static uint32_t dwtCnt = 0;
    float dt = pyro::dwt_drv_t::get_delta_t(&dwtCnt);

    // ── 1. 读取黑板输入 ──
    ChassisToGimbalComm c2gData{};
    Blackboard::instance().shootCmd.read(_ctx.cmd);
    Blackboard::instance().boosterState.read(_ctx.fdb);
    Blackboard::instance().c2gComm.read(c2gData);
    RMShootData shootData{};

    RefereeDataHub::instance().shootData.read(shootData);

    uint32_t nowMs = xTaskGetTickCount();

    // ── 2a. 同步裁判系统 → 热量控制器 ──
    _ctx.heatController.syncWithReferee(
        c2gData.msg.shooter17mmBarrelHeat, c2gData.msg.heatLimit,
        c2gData.msg.coolingRate, nowMs);

    // ── 2b. 喂弹速补偿器 (关闭时跳过，防止后台累积) ──
    if (g_enable_speed_compensation) {
        _ctx.speedCompensator.update(shootData.initialSpeed);
    }

    // ── 2c. 本地冷却推演 ──
    _ctx.heatController.tickCooling(dt);

    // ── 2d. 物理发弹检测 (编码器跨越一发跨度 → 注册热量) ──
    const int32_t ECD_PER_BULLET = 8192 * 36 / 8; // M2006 单发编码器跨度 (36864)

    _ctx.rawTriggerEcd = _ctx.fdb.triggerEcd + _ctx.fdb.triggerRound * 8192;
    _ctx.currentTriggerEcd = (_ctx.rawTriggerEcd - _ctx.triggerOffset + 8192 * 36) % (8192 * 36); // 当前编码器位置 (连续值 - 校准偏移, 模拟 36 周)
    static int32_t lastShotContinuousEcd;
    lastShotContinuousEcd = _ctx.rawTriggerEcd;

    debug_trigger.currentTrigger = _ctx.currentTriggerEcd;
    debug_trigger.targetTrigger = _ctx.targetTriggerEcd;
    debug_trigger.state = (uint8_t)_ctx.state;
    debug_trigger.offset = _ctx.triggerOffset;


    if (_ctx.rawTriggerEcd - lastShotContinuousEcd >= ECD_PER_BULLET) {
        _ctx.heatController.recordBulletShot(nowMs);
        lastShotContinuousEcd += ECD_PER_BULLET;
        g_heat_debug.physical_shot  = 50;
        g_speed_debug.physical_shot = 50;
    } else {
        g_heat_debug.physical_shot  = 0;
        g_speed_debug.physical_shot = 0;
    }

    // ── 3. 边沿事件提取 ──
    updateTransientEvent();

    // ── 4. 驱动 FSM (状态机内部仅设定目标角度/速度, 不涉及 PID) ──
    _fsm.execute(&_ctx);

    // ── 5. 统一 PID 计算输出电流 ──
    BoosterOutput finalOut{};
    calculateCurrents(finalOut);

    // ── 6a. Ozone 探针赋值 ──
    g_heat_debug.local_heat       = _ctx.heatController.getLocalHeat();
    g_heat_debug.referee_heat     = c2gData.msg.shooter17mmBarrelHeat;
    g_heat_debug.heat_limit       = c2gData.msg.heatLimit;
    g_heat_debug.safe_margin_line = c2gData.msg.heatLimit - HeatController::SAFE_MARGIN;
    g_heat_debug.target_rpm       = _ctx.targetTriggerSpeed;
    g_speed_debug.ref_bullet_speed    = (float)c2gData.msg.initialSpeedX100 / 100.0f;
    g_speed_debug.target_bullet_speed = 23.5f;

    // ── 6b. 写入黑板输出 ──
    Blackboard::instance().boosterOut.write(finalOut);
}

/* -------- 内部方法 -------------------------------------------------------------------------------------------------- */

/**
 * @brief  边沿检测: cmd.event 发生变化时, 将其作为瞬态事件传递给 FSM.
 *         瞬态事件仅存活 1 tick, 下次调用自动归零.
 */
void FireCtrlApp::updateTransientEvent() {
    _ctx.transientEvent = ShootEvent::NONE;
    if (_ctx.cmd.event != _lastEvent) {
        _ctx.transientEvent = _ctx.cmd.event;
        _lastEvent          = _ctx.cmd.event;
    }
}

/**
 * @brief  统一 PID 电流计算.
 *
 * 摩擦轮: 双速度环 (左正右反).
 * 拨弹盘: 根据 useTriggerSpeedLoopOnly 选择:
 *   true  → 纯速度环 (连发/校准反转)
 *   false → 位置外环 + 速度内环 (单发/就绪锁位)
 *
 * 安全锁: 摩擦轮目标 < 10 rad/s 时, 强制拨弹电流归零并清空 PID 积分.
 */
void FireCtrlApp::calculateCurrents(BoosterOutput& out) {
    // ── 摩擦轮: 弹速闭环补偿 ──
    float finalFricTargetSpeed = g_enable_speed_compensation
        ? _ctx.speedCompensator.getCompensatedRadPerSec(_ctx.targetFricSpeed)
        : _ctx.targetFricSpeed;

    g_speed_debug.base_fric_target  = _ctx.targetFricSpeed;
    g_speed_debug.final_fric_target = finalFricTargetSpeed;
    g_speed_debug.comp_integration  = finalFricTargetSpeed - _ctx.targetFricSpeed;
    g_speed_debug.fric_left_real    = _ctx.fdb.fric[(uint8_t)Config::Hardware::MotorTopo::FRIC_LEFT_ID].vel;

    // 左摩擦轮
    out.fricLeftCurrent = _fricLeftSpdPid.calculate(
        finalFricTargetSpeed,
        _ctx.fdb.fric[(uint8_t)Config::Hardware::MotorTopo::FRIC_LEFT_ID].vel);

    // 右摩擦轮 (反向安装, 目标取反)
    out.fricRightCurrent = _fricRightSpdPid.calculate(
        -finalFricTargetSpeed,
        _ctx.fdb.fric[(uint8_t)Config::Hardware::MotorTopo::FRIC_RIGHT_ID].vel);

    // ── 拨弹盘 ──
    if (_ctx.targetFricSpeed < 10.0f) {
        // 安全模式: 摩擦轮未启动 → 彻底断开拨弹盘动力
        out.triggerCurrent = 0.0f;
        _triggerPosPid.clear();
        _triggerSpdPid.clear();
        return;
    }

    float spdTarget = 0.0f;

    if (_ctx.useTriggerSpeedLoopOnly) {
        // 纯速度环 (连发 / 校准反转)
        spdTarget = _ctx.targetTriggerSpeed;
    } else {
        // 位置外环 → 速度内环 (单发 / 就绪锁位)
        float targetTriggerAngle = (float)(_ctx.targetTriggerEcd) / (float)(8192 * 36) * 2 * M_PI;


        float realTriggerAngle = (float)(_ctx.currentTriggerEcd) / (float)(8192 * 36) * 2 * M_PI;

        float err = targetTriggerAngle - realTriggerAngle;
        while (err >  M_PI) err -= 2.0f * M_PI;
        while (err < -M_PI) err += 2.0f * M_PI;

        spdTarget = _triggerPosPid.calculate(realTriggerAngle + err, realTriggerAngle);
    }

    // 速度内环 → 电流
    out.triggerCurrent = _triggerSpdPid.calculate(spdTarget, _ctx.fdb.trigger.vel);
}

/* -------- 访问器 ---------------------------------------------------------------------------------------------------- */

FireCtrlApp::FireState FireCtrlApp::getFireState() {
    return _ctx.state;
}
