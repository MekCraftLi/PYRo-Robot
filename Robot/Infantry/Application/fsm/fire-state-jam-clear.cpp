/**
 * DEPRECATED: JamClear State
 *
 * 此状态已废弃, 不再从任何其他状态进入.
 * 卡弹清除功能已由 CaliReverse → CaliForward 校准流程替代.
 * 保留此文件仅供参考, 未来可安全删除.
 *
 * 原始逻辑: 强制反转 150ms 后回到 Ready.
 */

#include "fire-ctrl-app.h"

constexpr float ANGLE_PER_BULLET = 45.0f; // 拨弹盘单发角度 (8发/圈)

void FireCtrlApp::StateJamClear::enter(FireCtrlCtx* ctx) {
    ctx->stateStartTick          = xTaskGetTickCount();
    ctx->useTriggerSpeedLoopOnly = true;
    ctx->state                   = FireState::JamClear;
}

void FireCtrlApp::StateJamClear::execute(FireCtrlCtx* ctx) {
    ctx->targetTriggerSpeed = -300.0f; // 强行反转退弹
    if (xTaskGetTickCount() - ctx->stateStartTick >= pdMS_TO_TICKS(150)) {
        request_switch(&instance()._stateReady);
    }
}

void FireCtrlApp::StateJamClear::exit(FireCtrlCtx* ctx) {
    ctx->targetTriggerEcd        = std::round(ctx->fdb.trigger.pos / ANGLE_PER_BULLET) * ANGLE_PER_BULLET;
    ctx->isCalibrated            = false;
    ctx->useTriggerSpeedLoopOnly = false;
}
