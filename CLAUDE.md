# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

PYRo-Robot is embedded firmware for RoboMaster competition robots, targeting STM32H723 (Cortex-M7, hard float, fpv5-d16) with FreeRTOS. Written in C++17/C17. The shared hardware abstraction library **PYRo** is a git submodule (`https://github.com/PeiYangRobot/PYRo-uCtrl-Unity`) built via `PYRo-Wrapper/`.

## Build Commands

**Toolchain**: `arm-none-eabi-gcc`, configured in `CubeMX/cmake/gcc-arm-none-eabi.cmake`. No RTTI, no exceptions (`-fno-rtti -fno-exceptions -fno-threadsafe-statics`).

**ROBOT_ID** is mandatory — build fails without it. Defined in `CMake/config/pyro_robot_id_config.cmake`:
- 0=TEST_ROBOT, 1=HERO, 2=ENGINEER, 3=INFANTRY1, 4=INFANTRY2, 5=SENTRY, 6=UAV, 7=DARTS, 8=RADAR

```bash
# Infantry Gimbal board
mkdir -p build-gimbal && cd build-gimbal
cmake .. -DROBOT_ID=3 -DINFANTRY_BOARD=GIMBAL
cmake --build .

# Infantry Chassis board
mkdir -p build-chassis && cd build-chassis
cmake .. -DROBOT_ID=3 -DINFANTRY_BOARD=CHASSIS
cmake --build .

# Hero (single-board, no INFANTRY_BOARD needed)
mkdir -p build-hero && cd build-hero
cmake .. -DROBOT_ID=1
cmake --build .
```

Optional cache variables: `-DDEMO_MODE=0`, `-DDEBUG_MODE=0`, `-DIMU_CALIBRATION_EN=1`.

## Architecture

### Robot Variants

**Infantry** (`Robot/Infantry/`) — dual-board architecture split by `INFANTRY_BOARD`:
- **GIMBAL**: yaw/pitch gimbal, friction wheels, trigger motor, vision comm, fire control FSM, USB CDC, SystemView
- **CHASSIS**: 4-wheel swerve drive (M3508 drive + GM6020 steer), supercap, referee, UI renderer

Board selection is via compile define (`-DGIMBAL` or `-DCHASSIS`). Config headers and service code use `#ifdef` guards to include board-specific hardware mappings and modules.

**Hero** (`Robot/Hero/`) — single-board with booster (quad-barrel) and mecanum chassis. All modules compiled into one firmware. FSM pattern with active/passive states.

### Threading Framework

All services run as FreeRTOS static tasks. Base classes in `Robot/Infantry/System/Thread/application-base.h`:
- `StaticAppBase` — CRTP base, auto-registers into global vector, creates `xTaskCreateStatic`
- `PeriodicApp` — fixed-period loop via `vTaskDelayUntil`
- `ContinuousApp` — tight spin loop
- `NotifyApp` — task-notification driven
- `QueueApp` — FreeRTOS queue driven

Entry point: `app-main.cpp` → `StaticAppBase::startApplications()` creates all registered tasks.

### Data Bus

`Blackboard` (Singleton) in `System/DataHub/` is the central inter-task data exchange using `SeqVariable<T>` (seqlock pattern — lock-free reads, critical-section writes). All shared data structures defined in `data-def.h`. Board-specific members selected by `#ifdef`.

### Infantry Services (`System/Service/`)

| Service | Role |
|---|---|
| `CommanderSrvc` | 4-layer input pipeline: Controls → Triggers → Actions → Blackboard commands |
| `MotActSrvc` | DJI (M3508, GM6020, M2006) and DM motor management |
| `MovtionCtrlApp` | Motion control: swerve kinematics (chassis), yaw/pitch cascade PID (gimbal) |
| `FireCtrlApp` | Fire control FSM (gimbal only): Passive→SpinUp→Ready→Single/BurstFire |
| `StateEstimatorSrvc` | IMU state estimation |
| `RefereeSrvc` | Referee system protocol |
| `VisionCommSrvc` | Vision UART comm (gimbal only) |
| `RealTimeCommSrvc` | Board-to-board CAN |
| `SuperCapCommSrvc` | Supercapacitor UART (chassis only) |

### Input System (`System/Input/`)

`IInputControl` → `InputTrigger` (state machine: click, hold, edge, toggle) → `InputAction` (binds control to trigger). `RemoteBase` adapter pattern with DR16, VideoLink, GamePad implementations in `Board-Support-Pack/`.

### Config Pattern (`Config/`)

`config.h` selects `Chassis/` or `Gimbal/` sub-configs based on board define:
- `hw-config.h` — motor CAN topology, UART mappings, mechanical dimensions
- `algo-config.h` — PID tuning, algorithm parameters

All config lives in `Config::Hardware::` and `Config::Algorithm::` namespaces.

## Coding Conventions

- **Language**: Comments and build messages are in Chinese. Code identifiers in English.
- **Naming**: PascalCase classes, camelCase methods/variables, UPPER_SNAKE_CASE macros
- **File sections**: Numbered comment blocks: `/*-------- 1. includes ---*/`, `/*-------- 2. enum and define ---*/`, `/*-------- 3. interface ---*/`
- **Singletons**: Services use CRTP `Singleton<T>` from `tools/crtp.h`
- **Static allocation**: All FreeRTOS tasks use `xTaskCreateStatic` with pre-allocated stacks
- **Lock-free**: `SeqVariable<T>` for hot-path inter-task sharing; no dynamic allocation in RTOS tasks
- **Format**: See `.clang-format` — LLVM based, Attach braces, 120 col, 4-space indent

## Key Dependencies

| Library | Location | Purpose |
|---|---|---|
| PYRo | `PYRo/` (submodule) | Shared HAL: CAN, UART, motors, PID, IMU, referee, RC |
| CMSIS-DSP | `third_party/CMSIS-DSP/` | ARM DSP + dsppp linear algebra |
| TinyUSB | `third_party/TinyUSB/` (submodule) | USB device stack (gimbal board only) |
| SystemView | `third_party/SystemView/` | SEGGER FreeRTOS tracing (gimbal board only) |

## Git

- **Submodules**: `PYRo` → `github.com/PeiYangRobot/PYRo-uCtrl-Unity`, `third_party/TinyUSB` → `github.com/hathach/tinyusb`
- **Main branch**: `master`
