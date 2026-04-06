/**
 *******************************************************************************
 * @file    s-curve-planner.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2026/3/5
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_S_CURVE_PLANNER_H
#define INFANTRY_S_CURVE_PLANNER_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <algorithm>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class SCurveVelocityPlanner {
public:
    /**
     * @param max_v    最大允许绝对速度 (m/s)
     * @param max_a    最大允许绝对加速度 (m/s^2)
     * @param max_jerk 最大允许加加速度 (m/s^3)
     */
    SCurveVelocityPlanner(float max_v, float max_a, float max_jerk)
        : _max_v(max_v), _max_a(max_a), _max_jerk(max_jerk),
          _current_v(0.0f), _current_a(0.0f) {}

    /**
     * @brief 计算当前物理帧的平滑目标速度
     * @param target_v 期望的最终目标速度
     * @param dt       控制周期 (秒)
     * @return float   当前应下发的平滑速度
     */
    float calculate(float target_v, float dt) {
        if (dt <= 0.0f) return _current_v;

        // 1. 速度物理限幅
        target_v = std::clamp(target_v, -_max_v, _max_v);
        float v_err = target_v - _current_v;

        // 2. 计算最优目标加速度 (逼近目标速度且不超调的最优解)
        // 基于运动学公式：v^2 = 2 * a * s 推导而来的近似最优曲线
        float a_req = std::copysign(std::sqrt(2.0f * _max_jerk * std::abs(v_err)), v_err);

        // 【核心防抖】：消除离散时间下开根号带来的高增益震荡 (Chattering)
        // 当误差极小时，切换为线性逼近
        float a_linear = v_err / dt;
        if (std::abs(a_req) > std::abs(a_linear)) {
            a_req = a_linear;
        }

        // 3. 加速度物理限幅
        float a_target = std::clamp(a_req, -_max_a, _max_a);

        // 4. 加加速度 (Jerk) 限制：让加速度平滑变化
        float a_err = a_target - _current_a;
        float max_delta_a = _max_jerk * dt;

        _current_a += std::clamp(a_err, -max_delta_a, max_delta_a);

        // 5. 积分得到最终下发速度
        _current_v += _current_a * dt;

        return _current_v;
    }

    /**
     * @brief 强制重置状态 (用于急停或模式切换时)
     */
    void reset(float current_real_v = 0.0f) {
        _current_v = current_real_v;
        _current_a = 0.0f;
    }
    // 【新增】：暴露内部状态供 Ozone 示波器读取
    float getCurrentV() const { return _current_v; }
    float getCurrentA() const { return _current_a; }

private:
    float _max_v;
    float _max_a;
    float _max_jerk;

    float _current_v;
    float _current_a;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_S_CURVE_PLANNER_H*/
