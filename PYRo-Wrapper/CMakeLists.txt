# PYRo-Wrapper/CMakeLists.txt
# Wrapper that builds the PYRo submodule as an OBJECT library target.
# This file exists because the PYRo git submodule does not contain its own CMakeLists.txt.

set(_PYRO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../PYRo")

add_library(PYRo OBJECT)

# ── Algorithm ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Algorithm/Common/pyro_algo_common.cpp
    ${_PYRO_DIR}/Algorithm/EKF/QuaternionEKF.cpp
    ${_PYRO_DIR}/Algorithm/KF/kalman_filter.cpp
    ${_PYRO_DIR}/Algorithm/Kinematics/pyro_kin_hybrid.cpp
    ${_PYRO_DIR}/Algorithm/Kinematics/pyro_kin_mec.cpp
    ${_PYRO_DIR}/Algorithm/Kinematics/pyro_kin_rudder.cpp
    ${_PYRO_DIR}/Algorithm/Kinematics/pyro_kin_wl.cpp
    ${_PYRO_DIR}/Algorithm/OLS/pyro_algo_ols.cpp
    ${_PYRO_DIR}/Algorithm/PID/pyro_algo_pid.cpp
)

# ── Communication ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Communication/pyro_com_canrx.cpp
    ${_PYRO_DIR}/Communication/pyro_com_cantx.cpp
    ${_PYRO_DIR}/Communication/pyro_uart_comm.cpp
)

# ── Component ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Component/CRC/pyro_crc.cpp
    ${_PYRO_DIR}/Component/INS/pyro_ins.cpp
    ${_PYRO_DIR}/Component/Measure/pyro_ms53l0m_drv.cpp
    ${_PYRO_DIR}/Component/Motor/pyro_motor_base.cpp
    ${_PYRO_DIR}/Component/Motor/pyro_dji_motor_drv.cpp
    ${_PYRO_DIR}/Component/Motor/pyro_dm_motor_drv.cpp
    ${_PYRO_DIR}/Component/Powercontrol/pyro_power_control_drv.cpp
    ${_PYRO_DIR}/Component/Powermeter/pyro_powermeter.cpp
    ${_PYRO_DIR}/Component/RC/pyro_dr16_rc_drv.cpp
    ${_PYRO_DIR}/Component/RC/pyro_vt03_rc_drv.cpp
    ${_PYRO_DIR}/Component/RC/pyro_rc_base_drv.cpp
    ${_PYRO_DIR}/Component/Referee/pyro_referee.cpp
    ${_PYRO_DIR}/Component/Referee/UI/pyro_ui_drv.cpp
    ${_PYRO_DIR}/Component/Supercap/pyro_supercap_drv.cpp
)

# ── Core ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Core/DataBoard/pyro_databoard.cpp
    ${_PYRO_DIR}/Core/ETL/map.cpp
    ${_PYRO_DIR}/Core/FSM/pyro_core_fsm.tpp
    ${_PYRO_DIR}/Core/Lock/pyro_mutex.cpp
    ${_PYRO_DIR}/Core/Lock/pyro_rw_lock.cpp
    ${_PYRO_DIR}/Core/Memory/pyro_core_dma_heap.c
    ${_PYRO_DIR}/Core/Memory/pyro_core_mem.cpp
    ${_PYRO_DIR}/Core/Task/pyro_task.cpp
)

# ── Debug ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Debug/Debug_task.cpp
    ${_PYRO_DIR}/Debug/JCOM/pyro_jcom.cpp
    ${_PYRO_DIR}/Debug/VOFA/pyro_vofa.cpp
)

# ── Device ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Device/BMI088/BMI088_driver.cpp
    ${_PYRO_DIR}/Device/BMI088/BMI088_middleware.cpp
)

# ── Peripheral ──
target_sources(PYRo PRIVATE
    ${_PYRO_DIR}/Peripheral/CAN/pyro_can_drv.cpp
    ${_PYRO_DIR}/Peripheral/DWT/pyro_dwt_drv.cpp
    ${_PYRO_DIR}/Peripheral/UART/pyro_bsp_uart.cpp
    ${_PYRO_DIR}/Peripheral/UART/pyro_uart_drv.cpp
)

# ── Include directories ──
target_include_directories(PYRo PUBLIC
    ${_PYRO_DIR}
    ${_PYRO_DIR}/Core/Config
    ${_PYRO_DIR}/Core/Def
    ${_PYRO_DIR}/Core/ETL
    ${_PYRO_DIR}/Core/FSM
    ${_PYRO_DIR}/Core/Lock
    ${_PYRO_DIR}/Core/Memory
    ${_PYRO_DIR}/Core/Task
    ${_PYRO_DIR}/Core/DataBoard
    ${_PYRO_DIR}/Peripheral/CAN
    ${_PYRO_DIR}/Peripheral/DWT
    ${_PYRO_DIR}/Peripheral/UART
    ${_PYRO_DIR}/Device/BMI088
    ${_PYRO_DIR}/Algorithm/Common
    ${_PYRO_DIR}/Algorithm/EKF
    ${_PYRO_DIR}/Algorithm/KF
    ${_PYRO_DIR}/Algorithm/Kinematics
    ${_PYRO_DIR}/Algorithm/OLS
    ${_PYRO_DIR}/Algorithm/PID
    ${_PYRO_DIR}/Component/CRC
    ${_PYRO_DIR}/Component/IMU
    ${_PYRO_DIR}/Component/INS
    ${_PYRO_DIR}/Component/Measure
    ${_PYRO_DIR}/Component/Motor
    ${_PYRO_DIR}/Component/Powercontrol
    ${_PYRO_DIR}/Component/Powermeter
    ${_PYRO_DIR}/Component/RC
    ${_PYRO_DIR}/Component/Referee
    ${_PYRO_DIR}/Component/Referee/UI
    ${_PYRO_DIR}/Component/Supercap
    ${_PYRO_DIR}/Communication
    ${_PYRO_DIR}/Debug/JCOM
    ${_PYRO_DIR}/Debug/VOFA
)

target_link_libraries(PYRo PUBLIC stm32cubemx dsppp_lib)
