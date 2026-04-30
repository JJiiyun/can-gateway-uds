cmake_minimum_required(VERSION 3.22)

project(can_gateway_uds_workspace NONE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(FIRMWARE_BOARDS
    board_a_engine
    board_b_gateway
    board_c_uds
    board_d_body
    board_e_safety
)

foreach(board IN LISTS FIRMWARE_BOARDS)
    set(board_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/firmware/${board})
    set(board_binary_dir ${board_source_dir}/build/${CMAKE_BUILD_TYPE})

    add_custom_target(configure_${board}
        COMMAND ${CMAKE_COMMAND} --preset ${CMAKE_BUILD_TYPE} --fresh -S ${board_source_dir}
        COMMENT "Configure ${board}"
        VERBATIM
    )

    add_custom_target(build_${board}
        COMMAND ${CMAKE_COMMAND} --build ${board_binary_dir} --parallel
        DEPENDS configure_${board}
        COMMENT "Build ${board}"
        VERBATIM
    )
endforeach()

add_custom_target(configure_firmware
    DEPENDS
        configure_board_a_engine
        configure_board_b_gateway
        configure_board_c_uds
        configure_board_d_body
        configure_board_e_safety
)

add_custom_target(build_firmware
    DEPENDS
        build_board_a_engine
        build_board_b_gateway
        build_board_c_uds
        build_board_d_body
        build_board_e_safety
)
