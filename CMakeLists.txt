cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER "C:/msys64/ucrt64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/msys64/ucrt64/bin/g++.exe")

project(PrivacyOverlay VERSION 1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDEs (clangd, etc.) to resolve include errors
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)

# Tell CMake to automatically handle Qt's special macros (MOC)
set(CMAKE_AUTOMOC ON)

# 1. Add Widgets and Gui components
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# --- TARGET 1: The Injection Payload (DLL) ---
# This is compiled as a SHARED library so it can be injected
add_library(InjectionPayload SHARED 
    platform/InjectionPayload.h
    platform/InjectionPayload.cpp
)

# Define a macro so the header knows we are EXPORTING functions
target_compile_definitions(InjectionPayload PRIVATE INJECTIONPAYLOAD_EXPORTS)
set_target_properties(InjectionPayload PROPERTIES PREFIX "")

# Link basic Windows libraries needed by the DLL
target_link_libraries(InjectionPayload PRIVATE user32 dwmapi)

# --- TARGET 2: The Main Executable ---
# Added MaskEngine and Injector to the source list
set(SOURCES
    main.cpp
    platform/WinApiWrapper.cpp
    platform/WindowEnumerator.cpp
    platform/AppDiscoveryEngine.cpp
    platform/Injector.cpp            
    models/AppRule.cpp
    models/Profile.cpp
    models/CachedApp.h
    persistence/SettingsManager.cpp
    persistence/AppCacheManager.cpp
    core/MaskEngine.cpp
    core/IconManager.cpp               
    platform/ActiveProcessMonitor.cpp
    
    # UI Files
    ui/MainWindow.h
    ui/MainWindow.cpp
    ui/DashboardWidget.h
    ui/DashboardWidget.cpp
    ui/AppListItem.h
    ui/AppListItem.cpp 
    ui/ToastWidget.h
    ui/ToastWidget.cpp
    ui/NotificationCenterWidget.h
    ui/NotificationCenterWidget.cpp
    ui/PresetsWidget.h
    ui/PresetsWidget.cpp 
    ui/SettingsWidget.h
    ui/SettingsWidget.cpp
    ui/TrayManager.h
    ui/TrayManager.cpp
)

# Create the executable
add_executable(PrivacyOverlay WIN32 ${SOURCES})

target_include_directories(PrivacyOverlay PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link the Windows APIs and Qt6 to our executable
# Note: advapi32 is required for SetDebugPrivilege in Injector.cpp
target_link_libraries(PrivacyOverlay PRIVATE 
    Qt6::Core 
    Qt6::Widgets 
    Qt6::Gui
    user32 
    kernel32
    dwmapi
    advapi32
    psapi
    version
    shell32
)

# Ensure the DLL is placed in the same directory as the EXE for MaskEngine to find it
add_dependencies(PrivacyOverlay InjectionPayload)