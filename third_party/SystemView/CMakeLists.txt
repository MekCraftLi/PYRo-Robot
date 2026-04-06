# third_party/SystemView/CMakeLists.txt
# Builds SEGGER SystemView + RTT as an OBJECT library.

set(_SV_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_library(SystemView OBJECT)

target_sources(SystemView PRIVATE
    # RTT core
    ${_SV_DIR}/SEGGER_RTT.c
    # SystemView core
    ${_SV_DIR}/SEGGER_SYSVIEW.c
    # FreeRTOS integration
    ${_SV_DIR}/SEGGER_SYSVIEW_FreeRTOS.c
    # Application-provided SystemView configuration
    ${_SV_DIR}/SEGGER_SYSVIEW_Config_FreeRTOS.c
)

target_compile_definitions(SystemView PRIVATE
    RTT_USE_ASM=0
)

target_include_directories(SystemView PUBLIC
    ${_SV_DIR}
)

# FreeRTOS headers are exposed via the stm32cubemx INTERFACE library
target_link_libraries(SystemView PUBLIC stm32cubemx)