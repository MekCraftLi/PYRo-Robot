/**
 *******************************************************************************
 * @file    trigger-impl-click.h
 * @brief   click trigger
 *******************************************************************************
 */

#ifndef INFANTRY_CHASSIS_TRIGGER_IMPL_CLICK_H
#define INFANTRY_CHASSIS_TRIGGER_IMPL_CLICK_H

#include "../triggers.h"

class TriggerClick : public InputTrigger {
public:
    TriggerClick(float threshold, float max_duration, bool is_greater_equal = true)
        : _threshold(threshold), _maxDuration(max_duration), _isGreaterEqual(is_greater_equal), _timer(0.0f), _lastState(false) {}

    TriggerState update(float value, float dt) override {
        bool currentState = _isGreaterEqual ? (value >= _threshold) : (value <= _threshold);
        TriggerState result = TriggerState::None;

        if (currentState) {
            _timer += dt;
        } else if (_lastState) {
            if (_timer > 0.0f && _timer <= _maxDuration) {
                result = TriggerState::Triggered;
            }
            _timer = 0.0f;
        } else {
            _timer = 0.0f;
        }

        _lastState = currentState;
        return result;
    }

    void reset() override {
        InputTrigger::reset();
        _timer = 0.0f;
        _lastState = false;
    }

private:
    float _threshold;
    float _maxDuration;
    bool _isGreaterEqual;
    float _timer;
    bool _lastState;
};

#endif /*INFANTRY_CHASSIS_TRIGGER_IMPL_CLICK_H*/
