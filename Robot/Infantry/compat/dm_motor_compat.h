#ifndef INFANTRY_DM_MOTOR_COMPAT_H
#define INFANTRY_DM_MOTOR_COMPAT_H

#include "pyro_dm_motor_drv.h"
#include <array>
#include <cmath>

namespace compat {

// Replicate float_to_uint from pyro_dm_motor_drv.cpp (file-static, not accessible externally)
inline int float_to_uint(float x, float x_min, float x_max, int bits) {
    float span   = x_max - x_min;
    float offset = x_min;
    return (int)((x - offset) * ((float)((1 << bits) - 1)) / span);
}

/**
 * DM motor MIT mode compatibility wrapper.
 * Extends dm_motor_drv_t with send_mit_ctrl(pos, vel, t_ff) that the infantry code expects.
 * Stores copies of range values since the base class stores them as private members.
 */
class DmMotorCompat : public pyro::dm_motor_drv_t {
  public:
    DmMotorCompat(uint32_t can_id, uint32_t master_id, pyro::can_hub_t::which_can which)
        : dm_motor_drv_t(can_id, master_id, which), _my_can_id(can_id) {}

    // MIT mode full-parameter control: position, velocity, feedforward torque
    // kp and kd use values set via set_runtime_kp/kd
    void send_mit_ctrl(float pos, float vel, float t_ff) {
        uint16_t pos_int = float_to_uint(pos, _p_min, _p_max, 16);
        uint16_t vel_int = float_to_uint(vel, _v_min, _v_max, 12);
        uint16_t kp_int  = float_to_uint(_kp, 0.0f, 500.0f, 12);
        uint16_t kd_int  = float_to_uint(_kd, 0.0f, 5.0f, 12);
        uint16_t t_int   = float_to_uint(t_ff, _t_min, _t_max, 12);

        std::array<uint8_t, 8> data;
        data.fill(0);
        data[0] = (pos_int >> 8);
        data[1] = pos_int & 0xff;
        data[2] = (vel_int >> 4);
        data[3] = ((vel_int & 0x0f) << 4) | (kp_int >> 8);
        data[4] = kp_int;
        data[5] = kd_int >> 4;
        data[6] = ((kd_int & 0x0f) << 4) | (t_int >> 8);
        data[7] = t_int;

        _can_drv->send_msg(_my_can_id, data.data());
    }

    // Override range setters to keep our copies
    void set_position_range(float min, float max) {
        pyro::dm_motor_drv_t::set_position_range(min, max);
        _p_min = min;
        _p_max = max;
    }
    void set_rotate_range(float min, float max) {
        pyro::dm_motor_drv_t::set_rotate_range(min, max);
        _v_min = min;
        _v_max = max;
    }
    void set_torque_range(float min, float max) {
        pyro::dm_motor_drv_t::set_torque_range(min, max);
        _t_min = min;
        _t_max = max;
    }
    void set_runtime_kp(float kp) {
        pyro::dm_motor_drv_t::set_runtime_kp(kp);
        _kp = kp;
    }
    void set_runtime_kd(float kd) {
        pyro::dm_motor_drv_t::set_runtime_kd(kd);
        _kd = kd;
    }

  private:
    uint32_t _my_can_id;
    float _p_min = -12.5f, _p_max = 12.5f;
    float _v_min = -30.0f, _v_max = 30.0f;
    float _t_min = -10.0f, _t_max = 10.0f;
    float _kp = 0.0f, _kd = 0.0f;
};

// Apply encoder offset correction to GM6020 position reading
// offset_ecd: raw encoder ticks at the desired zero position
// Returns corrected angle in (-PI, PI] radians
inline float apply_ecd_offset(float position_rad, uint16_t offset_ecd) {
    float offset_rad = (float)offset_ecd / 8192.0f * 2.0f * M_PI;
    float corrected  = position_rad - offset_rad;
    while (corrected > M_PI) corrected -= 2.0f * M_PI;
    while (corrected < -M_PI) corrected += 2.0f * M_PI;
    return corrected;
}

// Convert motor shaft angle (-PI, PI] to raw encoder ticks (0-8191)
// For DJI motors with 14-bit encoder (8192 ticks/rev)
inline int32_t position_to_raw_ecd(float position_rad) {
    if (position_rad < 0) position_rad += 2.0f * M_PI;
    int32_t ecd = (int32_t)(position_rad / (2.0f * M_PI) * 8192.0f + 0.5f);
    return ecd % 8192;
}

} // namespace compat

#endif
