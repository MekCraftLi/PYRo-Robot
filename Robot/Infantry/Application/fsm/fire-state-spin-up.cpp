/**
 * SpinUp State
 *
 * Role: 摩擦轮启动态 — 设定摩擦轮目标转速, 等待双轮达到目标后进入 Ready
 *
 * Entry actions:
 *   - 设定摩擦轮目标转速 (来自硬件配置)
 *   - 拨弹盘切回位置环模式 (为 Ready 做准备)
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - 双摩擦轮速度均接近目标 -> Ready
 *
 * Context modifications:
 *   - Writes: targetFricSpeed, useTriggerSpeedLoopOnly, state
 */

#include "fire-ctrl-app.h"
#include "Config/Gimbal/hw-config.h"

void FireCtrlApp::StateSpinUp::enter(FireCtrlCtx* ctx) {
    // --- 设定摩擦轮目标 ---
    ctx->targetFricSpeed         = Config::Hardware::MotorTopo::FRIC_TARGET_SPEED;
    ctx->useTriggerSpeedLoopOnly = false;
    ctx->state                   = FireState::SpinUp;
}

void FireCtrlApp::StateSpinUp::execute(FireCtrlCtx* ctx) {
    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 启动完成检测: 双摩擦轮速度均接近目标 ---
    if (std::abs(ctx->fdb.fric[0].vel - ctx->targetFricSpeed) < 0.1f &&
        std::abs(ctx->fdb.fric[1].vel + ctx->targetFricSpeed) < 0.1f) {
        request_switch(&instance()._stateReady);
    }
}
