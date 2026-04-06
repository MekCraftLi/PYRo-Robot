/**
 *******************************************************************************
 * @file    trigger-impl-edge.h
 * @brief   边沿触发器
 *******************************************************************************
 */

#ifndef INFANTRY_CHASSIS_TRIGGER_IMPL_EDGE_H
#define INFANTRY_CHASSIS_TRIGGER_IMPL_EDGE_H

#include "../triggers.h"

enum class EdgeType {
    Rising,
    Falling,
    Both
};

class TriggerEdge : public InputTrigger {
public:
    TriggerEdge(float threshold, EdgeType type, bool is_greater_equal = true)
        : _threshold(threshold), _type(type), _isGreaterEqual(is_greater_equal), _lastState(false) {}

    TriggerState update(float value, float dt) override {
        (void)dt;
        bool currentState = _isGreaterEqual ? (value >= _threshold) : (value <= _threshold);
        TriggerState result = TriggerState::None;

        if (currentState && !_lastState && (_type == EdgeType::Rising || _type == EdgeType::Both)) {
            result = TriggerState::Triggered;
        } else if (!currentState && _lastState && (_type == EdgeType::Falling || _type == EdgeType::Both)) {
            result = TriggerState::Triggered;
        }

        _lastState = currentState;
        return result;
    }

    void reset() override {
        InputTrigger::reset();
        _lastState = false;
    }

private:
    float _threshold;
    EdgeType _type;
    bool _isGreaterEqual;
    bool _lastState;
};

#endif /*INFANTRY_CHASSIS_TRIGGER_IMPL_EDGE_H*/

