/**
 * BurstFire State
 *
 * Role: 连发态 — 拨弹盘纯速度环全速连发, 由 HeatController 动态调节安全射频
 *
 * Entry actions:
 *   - 清除校准标志 (下次单发前需重新校准)
 *   - 清零堵转计时器
 *   - 切换到纯速度环模式
 *
 * Exit actions:
 *   - 清空速度环积分项
 *   - 向上取整对齐到最近的拨弹槽位 (targetTriggerEcd)
 *   - 切回位置环模式
 *
 * Exit condition (transitions OUT):
 *   - BURST_STOP / SINGLE_FIRE / burstShot==0 -> Ready
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - 堵转超时 2000 ms -> CaliReverse (记录来源为 BurstFire)
 *
 * Context modifications:
 *   - Writes: isCalibrated, blockStartTick, useTriggerSpeedLoopOnly,
 *             targetTriggerSpeed, targetTriggerEcd, jamSourceState, state
 *   - Clears: _triggerSpdPid (in exit)
 */

#include "fire-ctrl-app.h"
#include "Config/Gimbal/hw-config.h"

void FireCtrlApp::StateBurstFire::enter(FireCtrlCtx* ctx) {
    // --- 初始化 ---
    ctx->isCalibrated            = false;
    ctx->blockStartTick          = 0;
    ctx->useTriggerSpeedLoopOnly = true;
    ctx->state                   = FireState::BurstFire;
}

void FireCtrlApp::StateBurstFire::execute(FireCtrlCtx* ctx) {
    // --- 停止条件 ---
    if (ctx->transientEvent == ShootEvent::BURST_STOP ||
        ctx->transientEvent == ShootEvent::SINGLE_FIRE ||
        ctx->cmd.state.burstShot == 0) {
        request_switch(&instance()._stateReady);
        return;
    }

    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 热控器动态调节安全射频 ---
    ctx->targetTriggerSpeed =
        ctx->heatController.getSafeBurstRpm(Config::Hardware::MotorTopo::TRIGGER_SPEED, 36.0f);

    // --- 堵转检测: 目标速度大但实际极低 ---
    float speedErr = std::abs(ctx->targetTriggerSpeed) - std::abs(ctx->fdb.trigger.vel);
    if (speedErr > 50.0f && std::abs(ctx->fdb.trigger.vel) < 10.0f) {
        if (ctx->blockStartTick == 0)
            ctx->blockStartTick = xTaskGetTickCount();
        else if (xTaskGetTickCount() - ctx->blockStartTick >= pdMS_TO_TICKS(2000)) {
            ctx->jamSourceState = FireState::BurstFire;
            request_switch(&instance()._stateCaliReverse);
            return;
        }
    } else {
        ctx->blockStartTick = 0;
    }
}

void FireCtrlApp::StateBurstFire::exit(FireCtrlCtx* ctx) {
    // --- 清空速度环积分 ---
    instance()._triggerSpdPid.clear();

    // --- 对齐到前方最近的槽位, 为位置环锁位做准备 ---
    int32_t currentEcd   = ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;
    int32_t ecdPerBullet = 8192 * 36 / 8;
    ctx->targetTriggerEcd = ((currentEcd + ecdPerBullet - 1) / ecdPerBullet) * ecdPerBullet;

    ctx->useTriggerSpeedLoopOnly = false;
}
