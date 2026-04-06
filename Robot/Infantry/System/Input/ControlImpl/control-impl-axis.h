/**
 *******************************************************************************
 * @file    control-impl-axis.h
 * @brief   AXIS，负责将线性控件进行归一化
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
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_CONTROL_IMPL_AXIS_H
#define INFANTRY_CHASSIS_CONTROL_IMPL_AXIS_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../control.h"

#include <cmath>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 模拟轴控件 (针对 DR16 摇杆)
 * 职责: 归一化 (Normalize)、死区处理 (Deadzone) [cite: 26]
 */
class ControlAxis : public IInputControl {
public:
    // 配置参数结构体
    struct Config {
        int minRaw = 364;
        int maxRaw = 1684;
        int centerRaw = 1024;
        int deadzone = 20;    // 死区阈值
        bool reverse = false; // 是否反转轴
    };

    explicit ControlAxis(Config config) : _cfg(config), _val(0.0f) {}

    float get() const override { return _val; }

    bool isActive() const override {
        return std::abs(_val) > 0.0f;
    }

    void updateRaw(int raw) override {
        // 1. 中心偏移
        int offset = raw - _cfg.centerRaw;

        if (raw == 0) {
            // raw = 0说明数据无效，摇杆在中间状态
            _val = 0;
            return;
        }

        // 2. 死区过滤
        if (std::abs(offset) < _cfg.deadzone) {
            _val = 0.0f;
            return;
        }

        // 3. 归一化计算 (映射到 -1.0 ~ 1.0)
        float range = (float)(_cfg.maxRaw - _cfg.minRaw) / 2.0f;
        _val = (float)offset / range;

        // 4. 限幅与反转
        if (_val > 1.0f) _val = 1.0f;
        if (_val < -1.0f) _val = -1.0f;
        if (_cfg.reverse) _val = -_val;
    }

private:
    Config _cfg;
    float _val;
};

class ControlMaskAxis : public IInputControl {
public:
    ControlMaskAxis(uint8_t posBitOffset, uint8_t negBitOffset)
        : _posBit(posBitOffset), _negBit(negBitOffset), _val(0.0f) {}

    float get() const override { return _val; }

    bool isActive() const override { return std::abs(_val) > 0.0f; }

    void updateRaw(int rawMask) override {
        const int pos = (rawMask >> _posBit) & 0x01;
        const int neg = (rawMask >> _negBit) & 0x01;
        _val = static_cast<float>(pos - neg);
    }

private:
    uint8_t _posBit;
    uint8_t _negBit;
    float _val;
};


/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_CONTROL_IMPL_AXIS_H*/
