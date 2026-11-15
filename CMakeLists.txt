cmake_minimum_required(VERSION 3.21)
project(SerialMonitor VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_add_executable(SerialMonitor WIN32
    main.cpp
    mainwindow.cpp
    mainwindow.h
    serialworker.cpp
    serialworker.h
    plotwidget.cpp
    plotwidget.h
    theme.cpp
    theme.h
    app.rc
)
qt_add_resources(SerialMonitor "assets"
    PREFIX "/"
    BASE "${CMAKE_CURRENT_SOURCE_DIR}/assets"
    FILES
        assets/favicon.ico
        assets/check.svg
        assets/fonts/PlusJakartaSans.ttf
        assets/fonts/JetBrainsMono.ttf
)
target_link_libraries(SerialMonitor PRIVATE Qt6::Widgets setupapi)
target_compile_definitions(SerialMonitor PRIVATE APP_VERSION="${PROJECT_VERSION}")
