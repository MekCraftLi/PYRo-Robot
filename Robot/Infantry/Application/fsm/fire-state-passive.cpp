/**
 * Passive State
 *
 * Role: 休眠态 — 摩擦轮停转, 拨弹盘位置环锁位防止溜弹
 *
 * Entry actions:
 *   - 清除校准标志 (isCalibrated = false)
 *   - 摩擦轮目标转速归零
 *   - 拨弹盘切回位置环模式
 *   - 重置弹速补偿器
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE -> SpinUp (启动摩擦轮)
 *
 * Context modifications:
 *   - Writes: isCalibrated, targetFricSpeed, useTriggerSpeedLoopOnly,
 *             targetTriggerEcd, state
 *   - Calls:  speedCompensator.reset()
 */

#include "fire-ctrl-app.h"

void FireCtrlApp::StatePassive::enter(FireCtrlCtx* ctx) {
    // --- 初始化状态 ---
    ctx->isCalibrated            = false;
    ctx->targetFricSpeed         = 0.0f;
    ctx->useTriggerSpeedLoopOnly = false;
    ctx->speedCompensator.reset();
    ctx->state                   = FireState::Passive;
}

void FireCtrlApp::StatePassive::execute(FireCtrlCtx* ctx) {
    // --- 位置环锁位: 每拍跟踪当前编码器, 防止断电滑转 ---
    ctx->targetTriggerEcd =
        ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;

    // --- 转移条件 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE) {
        request_switch(&instance()._stateSpinUp);
    }
}
