cmake_minimum_required(VERSION 3.16)

project(can_cluster_gui VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Quick QuickControls2 SerialPort)

qt_standard_project_setup()

qt_add_executable(can_cluster_gui
    main.cpp
    core/SerialDevice.cpp
    core/SerialDevice.h
    models/AppLogModel.cpp
    models/AppLogModel.h
    controllers/EngineController.cpp
    controllers/EngineController.h
    controllers/GatewayController.cpp
    controllers/GatewayController.h
    controllers/BodyController.cpp
    controllers/BodyController.h
    controllers/UdsController.cpp
    controllers/UdsController.h
)

qt_add_qml_module(can_cluster_gui
    URI CanClusterGui
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/DashboardPage.qml
        qml/EnginePage.qml
        qml/BodyPage.qml
        qml/GatewayPage.qml
        qml/UdsPage.qml
        qml/CanLogPage.qml
        qml/components/Panel.qml
        qml/components/MetricCard.qml
        qml/components/StatusPill.qml
        qml/components/UiButton.qml
        qml/components/SerialPanel.qml
        qml/components/GaugeCard.qml
        qml/components/CanByteRow.qml
)

target_include_directories(can_cluster_gui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/models
    ${CMAKE_CURRENT_SOURCE_DIR}/controllers
)

target_link_libraries(can_cluster_gui
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::SerialPort
)

if(WIN32)
    set_target_properties(can_cluster_gui PROPERTIES WIN32_EXECUTABLE TRUE)
endif()
