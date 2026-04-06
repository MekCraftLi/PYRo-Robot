/**
 *******************************************************************************
 * @file    control-impl-switch.h
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
 * @date    2026/2/9
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef INFANTRY_CHASSIS_CONTROL_IMPL_SWITCH_H
#define INFANTRY_CHASSIS_CONTROL_IMPL_SWITCH_H



/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include "../control.h"
#include <vector>
#include <map>
#include <cmath>




/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

/**
 * @brief 通用 N 档开关控件
 * * 特性:
 * 1. 支持任意数量档位 (2, 3, N...)
 * 2. 支持乱序硬件值映射 (通过 Map 配置)
 * 3. 自动计算归一化浮点值 (-1.0 ~ 1.0)
 */
class ControlSwitch : public IInputControl {
public:
    // 配置结构：定义每个档位对应的硬件原始值
    // 例如 DR16: { {1, 0}, {3, 1}, {2, 2} }  (Raw -> Logical Index)
    using StateMap = std::map<int, int>;

    /**
     * @param raw_map 硬件值到逻辑索引的映射表。逻辑索引必须从 0 开始连续。
     * @param default_idx 默认初始位置索引
     */
    ControlSwitch(StateMap raw_map, int default_idx = 0)
        : _map(raw_map), _totalStates(raw_map.size()), _currentIdx(default_idx)
    {
        // 预计算每个档位的 float 值，避免运行时除法
        // 逻辑索引 0 -> -1.0
        // 逻辑索引 N-1 -> 1.0
        if (_totalStates <= 1) {
            // 异常保护：单状态开关无意义，默认 0.0
            _stepVal = 0.0f;
        } else {
            _stepVal = 2.0f / (_totalStates - 1);
        }
    }

    // 1. 实现标准接口 (返回 -1.0 ~ 1.0)
    float get() const override {
        // Index 0 -> -1.0
        // Index 1 -> -1.0 + step
        // ...
        return -1.0f + (_currentIdx * _stepVal);
    }

    // 开关总是有效的
    bool isActive() const override { return true; }

    // 2. 核心更新逻辑
    void updateRaw(int raw) override {
        // 查找硬件值对应的逻辑索引
        auto it = _map.find(raw);
        if (it != _map.end()) {
            _currentIdx = it->second;
        }
        // 如果收到未知硬件值，保持上一次状态 (或者可以设计为切到默认值)
    }

    // ==========================================
    // 扩展功能：提供给业务层的强类型接口
    // ==========================================

    // 获取当前逻辑索引 (0, 1, 2...) - 比 float 更适合状态机判断
    int getIndex() const { return _currentIdx; }

    // 获取总档位数
    int getStateCount() const { return _totalStates; }

    // 判断是否在某个逻辑档位
    bool isAt(int logical_index) const { return _currentIdx == logical_index; }

private:
    StateMap _map;
    int _totalStates;
    int _currentIdx;
    float _stepVal;
};




/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*INFANTRY_CHASSIS_CONTROL_IMPL_SWITCH_H*/
