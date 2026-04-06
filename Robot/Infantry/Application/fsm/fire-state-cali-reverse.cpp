/**
 * CaliReverse State
 *
 * Role: 校准反转态 — 拨弹盘高速反转, 直到碰到机械死区 (堵转) 后切换到 CaliForward
 *
 * Entry actions:
 *   - 清零堵转计时器
 *   - 切换到纯速度环模式
 *
 * Exit actions:
 *   - 清空速度环积分项 (避免残留积分驱动电机)
 *   - 记录当前编码器位置为零点偏移 (triggerOffset)
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - 堵转检测超时 30 ticks (速度误差 > 150) -> CaliForward
 *
 * Context modifications:
 *   - Writes: blockTimer, useTriggerSpeedLoopOnly, targetTriggerSpeed,
 *             triggerOffset, state
 *   - Clears: _triggerSpdPid (in exit)
 */

#include "fire-ctrl-app.h"
#include "Config/Gimbal/hw-config.h"

void FireCtrlApp::StateCaliReverse::enter(FireCtrlCtx* ctx) {
    // --- 初始化 ---
    ctx->blockTimer              = 0;
    ctx->useTriggerSpeedLoopOnly = true;
    ctx->state                   = FireState::CaliReverse;
}

void FireCtrlApp::StateCaliReverse::execute(FireCtrlCtx* ctx) {
    // --- 高速反转寻找机械死区 ---
    ctx->targetTriggerSpeed = -Config::Hardware::MotorTopo::TRIGGER_SPEED;

    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 堵转检测: 实际速度远小于目标 → 碰到机械限位 ---
    if (std::abs(ctx->fdb.trigger.vel - ctx->targetTriggerSpeed) > 150.0f) {
        ctx->blockTimer++;
        if (ctx->blockTimer > 30) {
            request_switch(&instance()._stateCaliForward);
        }
    } else {
        ctx->blockTimer = 0;
    }
}

void FireCtrlApp::StateCaliReverse::exit(FireCtrlCtx* ctx) {
    // --- 清空速度环积分, 防止残留积分继续驱动电机 ---
    instance()._triggerSpdPid.clear();

    // --- 以当前编码器位置作为新的零点 ---
    ctx->triggerOffset =
        (ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 + 8192 * 2) % (8192 * 36);
}
