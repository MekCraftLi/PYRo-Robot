# Robot/Infantry/CMakeLists.txt
add_library(Infantry OBJECT)

# ── Board selection ──
set(INFANTRY_BOARD "GIMBAL" CACHE STRING "Select board firmware: CHASSIS or GIMBAL")
set_property(CACHE INFANTRY_BOARD PROPERTY STRINGS "CHASSIS" "GIMBAL")

if(INFANTRY_BOARD STREQUAL "CHASSIS")
    target_compile_definitions(Infantry PUBLIC CHASSIS=1)
elseif(INFANTRY_BOARD STREQUAL "GIMBAL")
    target_compile_definitions(Infantry PUBLIC GIMBAL=1)
else()
    message(FATAL_ERROR "INFANTRY_BOARD must be CHASSIS or GIMBAL")
endif()

# ── Remote device selection ──
set(REMOTE_DEVICE "VideoLink" CACHE STRING "Remote device: DR16, VideoLink, GamePad")
set_property(CACHE REMOTE_DEVICE PROPERTY STRINGS "DR16" "VideoLink" "GamePad")

if(REMOTE_DEVICE STREQUAL "DR16")
    set(_REMOTE 1)
elseif(REMOTE_DEVICE STREQUAL "VideoLink")
    set(_REMOTE 2)
elseif(REMOTE_DEVICE STREQUAL "GamePad")
    set(_REMOTE 3)
endif()

target_compile_definitions(Infantry PUBLIC
    REMOTE_DR16=1
    REMOTE_VIDEO_LINK=2
    REMOTE_GAMEPAD=3
    REMOTE_DEVICE=${_REMOTE}
)

set(_S ${CMAKE_CURRENT_SOURCE_DIR})

# ── Source files (common to both boards) ──
target_sources(Infantry PRIVATE
    ${_S}/app-main.cpp

    # Tools
    ${_S}/tools/crc.cpp

    # Algorithm
    ${_S}/Algorithm/Power/power-limiter.cpp
    ${_S}/Algorithm/Shoot/speed-compensater.cpp

    # Application
    ${_S}/Application/movtion-ctrl-app.cpp

    # Board Support Pack (common)
    ${_S}/Board-Support-Pack/BMI088/dev-bmi.cpp
    ${_S}/Board-Support-Pack/DR16/dr16.cpp


    # System / Service
    ${_S}/System/Service/commander.cpp
    ${_S}/System/Service/state-estimator.cpp
    ${_S}/System/Service/motor-actuator.cpp
    ${_S}/System/Service/heart-beat.cpp
    ${_S}/System/Service/real-time-comm.cpp
    ${_S}/System/Service/interrupt-callback.cpp
)

# ── Board-conditional sources ──
if(INFANTRY_BOARD STREQUAL "CHASSIS")
    target_sources(Infantry PRIVATE
        ${_S}/Application/ui-maker-app.cpp
        ${_S}/System/Service/referee.cpp
        ${_S}/System/Service/super-cap-comm.cpp
        ${_S}/System/Service/ui-renderer-srvc.cpp
    )
elseif(INFANTRY_BOARD STREQUAL "GIMBAL")
    target_sources(Infantry PRIVATE
        ${_S}/Application/fire-ctrl-app.cpp
        ${_S}/Application/fsm/fire-state-passive.cpp
        ${_S}/Application/fsm/fire-state-spin-up.cpp
        ${_S}/Application/fsm/fire-state-ready.cpp
        ${_S}/Application/fsm/fire-state-cali-reverse.cpp
        ${_S}/Application/fsm/fire-state-cali-forward.cpp
        ${_S}/Application/fsm/fire-state-single-fire.cpp
        ${_S}/Application/fsm/fire-state-burst-fire.cpp
        ${_S}/Application/fsm/fire-state-safe-burst.cpp
        ${_S}/Application/fsm/fire-state-jam-clear.cpp
        ${_S}/Board-Support-Pack/GamePad/bluetooth-gamepad.cpp
        ${_S}/Board-Support-Pack/USB/usb-descriptors.c
        ${_S}/Board-Support-Pack/USB/dev-usb.cpp
        ${_S}/Board-Support-Pack/USB/class-cdc.cpp
        ${_S}/Board-Support-Pack/VideoLink/video-link-remote.cpp
        ${_S}/System/Service/vision-comm.cpp
        ${_S}/System/Service/usb-device.cpp
    )

    # TinyUSB sources (GIMBAL only)
    set(_TINYUSB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/TinyUSB")

    target_sources(Infantry PRIVATE
        ${_TINYUSB_DIR}/src/tusb.c
        ${_TINYUSB_DIR}/src/common/tusb_fifo.c
        ${_TINYUSB_DIR}/src/device/usbd.c
        ${_TINYUSB_DIR}/src/device/usbd_control.c
        ${_TINYUSB_DIR}/src/class/cdc/cdc_device.c
        ${_TINYUSB_DIR}/src/portable/synopsys/dwc2/dcd_dwc2.c
        ${_TINYUSB_DIR}/src/portable/synopsys/dwc2/dwc2_common.c
    )

    target_compile_definitions(Infantry PUBLIC
        CFG_TUSB_MCU=OPT_MCU_STM32H7
        CFG_TUSB_OS=OPT_OS_FREERTOS
    )

    # SystemView (GIMBAL only)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/SystemView ${CMAKE_BINARY_DIR}/build_systemview)
    target_link_libraries(Infantry PUBLIC SystemView)
endif()

# ── Include directories ──
target_include_directories(Infantry PUBLIC
    ${_S}
    ${_S}/Config
    ${_S}/Config/Chassis
    ${_S}/Config/Gimbal
    ${_S}/Adapter
    ${_S}/Algorithm
    ${_S}/Algorithm/Motion
    ${_S}/Algorithm/Power
    ${_S}/Algorithm/Shoot
    ${_S}/Application
    ${_S}/Board-Support-Pack/BMI088
    ${_S}/Board-Support-Pack/DR16
    ${_S}/Board-Support-Pack/VideoLink
    ${_S}/Board-Support-Pack/GamePad
    ${_S}/Board-Support-Pack/USB
    ${_S}/System
    ${_S}/System/Thread
    ${_S}/System/DataHub
    ${_S}/System/Input
    ${_S}/System/Input/ControlImpl
    ${_S}/System/Input/TriggerImpl
    ${_S}/System/Service
    ${_S}/tools
    ${_S}/compat
)

# TinyUSB include (GIMBAL)
if(INFANTRY_BOARD STREQUAL "GIMBAL")
    target_include_directories(Infantry PUBLIC
        ${_TINYUSB_DIR}/src
    )
endif()

target_link_libraries(Infantry PUBLIC stm32cubemx dsppp_lib PYRo)
