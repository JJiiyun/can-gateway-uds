cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

set(CMAKE_PROJECT_NAME board_e_safety)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)

add_executable(${CMAKE_PROJECT_NAME})

set(REPO_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(COMMON_DIR ${REPO_ROOT}/common)
set(BOARD_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CUBE_BASE ${CMAKE_CURRENT_SOURCE_DIR}/../board_d_body)

# Board E uses the same STM32F429ZI peripheral scaffold as Board D:
# CAN1/CAN2, ADC1, USART3, FreeRTOS, and the weak EngineSim_Task slot.
add_subdirectory(${CUBE_BASE}/cmake/stm32cubemx stm32cubemx)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    ${COMMON_DIR}/can_bsp.c
    ${COMMON_DIR}/uart.c
    ${BOARD_SRC_DIR}/adas_can.c
    ${BOARD_SRC_DIR}/adas_input.c
    ${BOARD_SRC_DIR}/adas_main.c
    ${BOARD_SRC_DIR}/adas_signal.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${COMMON_DIR}
    ${BOARD_SRC_DIR}
    ${CUBE_BASE}/Core/Inc
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    BOARD_E_SAFETY
)

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)
