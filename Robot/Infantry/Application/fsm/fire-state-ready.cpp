/**
 * Ready State
 *
 * Role: 就绪态 — 摩擦轮已达标速, 拨弹盘位置环锁位, 等待射击指令
 *
 * Entry actions:
 *   - 拨弹盘切回位置环
 *   - 将当前编码器位置四舍五入对齐到最近的物理槽位 (防止半发偏移)
 *
 * Exit condition (transitions OUT):
 *   - FRIC_TOGGLE / EMERGENCY_STOP -> Passive
 *   - SINGLE_FIRE + !isCalibrated  -> CaliReverse (先校准再打)
 *   - SINGLE_FIRE + isCalibrated + canShootSingle -> SingleFire
 *   - burstShot (持续按住)         -> BurstFire
 *   - burstShot + heat warning     -> SafeBurst (当前未启用, 预留)
 *
 * Context modifications:
 *   - Writes: useTriggerSpeedLoopOnly, targetTriggerEcd, state
 */

#include "fire-ctrl-app.h"

void FireCtrlApp::StateReady::enter(FireCtrlCtx* ctx) {
    // --- 切回位置环, 提供物理刚性防止溜弹 ---
    ctx->useTriggerSpeedLoopOnly = false;

    // --- 将编码器四舍五入对齐到最近的拨弹槽位 ---
    int32_t currentEcd   = ctx->fdb.triggerEcd + ctx->fdb.triggerRound * 8192 - ctx->triggerOffset;
    int32_t ecdPerBullet = 8192 * 36 / 8; // M2006 单发跨度 = 36864
    ctx->targetTriggerEcd = ((currentEcd + ecdPerBullet / 2) / ecdPerBullet) * ecdPerBullet;

    ctx->state = FireState::Ready;
}

void FireCtrlApp::StateReady::execute(FireCtrlCtx* ctx) {
    // --- 紧急退出 ---
    if (ctx->transientEvent == ShootEvent::FRIC_TOGGLE ||
        ctx->transientEvent == ShootEvent::EMERGENCY_STOP) {
        request_switch(&instance()._statePassive);
        return;
    }

    // --- 单发指令 ---
    if (ctx->transientEvent == ShootEvent::SINGLE_FIRE) {
        if (!ctx->isCalibrated) {
            // 未校准 → 先进入校准流程
            request_switch(&instance()._stateCaliReverse);
        } else if (ctx->heatController.canShootSingle()) {
            // 已校准 + 热量允许 → 进入单发
            request_switch(&instance()._stateSingleFire);
        }
        // else: 热量不足, 留在 Ready 忽略本次指令
        return;
    }

    // --- 连发指令 (持续按住) ---
    if (ctx->cmd.state.burstShot) {
        // TODO: 热量警戒时切换到 SafeBurst
        request_switch(&instance()._stateBurstFire);
    }
}
