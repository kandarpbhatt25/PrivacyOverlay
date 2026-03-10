cmake_minimum_required(VERSION 3.16)
project(PrivacyOverlay VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake to automatically handle Qt's special macros (MOC)
set(CMAKE_AUTOMOC ON)

# 1. Add Widgets and Gui components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Group our source files
set(SOURCES
    main.cpp
    platform/WinApiWrapper.cpp
    platform/WindowEnumerator.cpp
    platform/CaptureMonitor.cpp
    models/AppRule.cpp
    models/Profile.cpp
    persistence/SettingsManager.cpp
    core/CaptureDetectionEngine.cpp
    
    # UI Files
    ui/MainWindow.h
    ui/MainWindow.cpp
    ui/AppListItem.h
    ui/AppListItem.cpp 
)

# Create the executable
add_executable(PrivacyOverlay ${SOURCES})

target_include_directories(PrivacyOverlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# Link the Windows APIs and Qt6 Core to our executable
target_link_libraries(PrivacyOverlay PRIVATE 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Gui
    user32 
    kernel32
    dwmapi
    advapi32
)