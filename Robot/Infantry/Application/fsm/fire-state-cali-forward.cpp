/**
 * CaliForward State
 *
 * Role: 校准正转态 — 从机械死区正转回到零点 (triggerOffset), 完成后根据堵转来源恢复
 *
 * Entry actions:
 *   - 切回位置环模式
 *   - 设定目标编码器为零点 (targetTriggerEcd = 0)
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - 编码器到达零点附近 (< 1000 计数) -> 根据来源恢复:
 *       SingleFire 来源 -> Ready
 *       BurstFire 来源 -> BurstFire / SafeBurst / Ready (根据热量)
 *       SafeBurst 来源 -> SafeBurst / Ready (根据热量)
 *       默认 (首次校准) -> SingleFire (立刻打一发)
 *
 * Context modifications:
 *   - Writes: useTriggerSpeedLoopOnly, targetTriggerEcd, isCalibrated,
 *             jamSourceState, state
 */

#include "fire-ctrl-app.h"

void FireCtrlApp::StateCaliForward::enter(FireCtrlCtx* ctx) {
    // --- 切回位置环, 目标为零点 ---
    ctx->useTriggerSpeedLoopOnly = false;
    ctx->targetTriggerEcd        = 5000;
    ctx->state                   = FireState::CaliForward;
}

void FireCtrlApp::StateCaliForward::execute(FireCtrlCtx* ctx) {
    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 到达零点检测 ---
    int32_t currentEcd = ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;
    if (std::abs(currentEcd) > 1000)
        return;

    // --- 校准完成, 根据堵转来源恢复 ---
    ctx->isCalibrated = true;

    switch (ctx->jamSourceState) {
        case FireState::SingleFire:
            // 单发堵转 → 回到 Ready 等待下次指令
            request_switch(&instance()._stateReady);
            break;

        case FireState::BurstFire:
            // 连发堵转 → 根据热量恢复
            if (ctx->heatController.isApproachingHeatLimit()) {
                if (ctx->heatController.canShootSingle()) {
                    request_switch(&instance()._stateSafeBurst);
                } else {
                    request_switch(&instance()._stateReady);
                }
            } else {
                request_switch(&instance()._stateBurstFire);
            }
            break;

        case FireState::SafeBurst:
            // 安全连发堵转 → 根据热量恢复
            if (ctx->heatController.canShootSingle()) {
                request_switch(&instance()._stateSafeBurst);
            } else {
                request_switch(&instance()._stateReady);
            }
            break;

        default:
            // 首次校准 → 立刻打一发
            request_switch(&instance()._stateSingleFire);
            break;
    }
}
