cmake_minimum_required(VERSION 3.24)
project(Focusboard VERSION 1.0.1 LANGUAGES CXX)

if(WIN32)
    set(CMAKE_INSTALL_BINDIR "." CACHE PATH "" FORCE)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)
include(CTest)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets)
if(WIN32)
    set(platform_sources src/platform/windows/WindowsDesktopBackend.cpp src/platform/windows/WindowsAutostart.cpp)
    set(platform_libraries user32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(LayerShellQt REQUIRED)
    set(platform_sources src/platform/linux/LinuxDesktopBackend.cpp src/integration/AutostartManager.cpp)
    set(platform_libraries LayerShellQt::Interface)
else()
    message(FATAL_ERROR "Focusboard supports Linux and Windows")
endif()
qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(focusboard
    src/main.cpp
    src/core/DocumentStore.cpp src/core/DocumentStore.hpp
    src/core/SettingsManager.cpp src/core/SettingsManager.hpp
    src/core/CommandController.cpp src/core/CommandController.hpp
    src/core/MarkdownPreview.cpp src/core/MarkdownPreview.hpp
    ${platform_sources} src/core/DesktopWindowController.hpp
    src/core/MultiMonitorController.cpp src/core/MultiMonitorController.hpp
    src/integration/AutostartManager.hpp
)
qt_add_qml_module(focusboard URI Focusboard VERSION 1.0 QML_FILES
    qml/Main.qml
    qml/components/CommandPalette.qml
    qml/components/ResizeHandle.qml
)
target_link_libraries(focusboard PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets ${platform_libraries})
target_include_directories(focusboard PRIVATE src/core src/integration)
target_compile_definitions(focusboard PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
    FOCUSBOARD_VERSION="${PROJECT_VERSION}")

install(TARGETS focusboard RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(focusboard PROPERTIES INSTALL_RPATH "")
if(NOT WIN32)
configure_file(packaging/focusboard.desktop.in focusboard.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/focusboard.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES assets/icons/focusboard.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
foreach(size IN ITEMS 16 32 48 64 128 256)
    install(FILES assets/icons/${size}x${size}.png
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps
        RENAME focusboard.png)
endforeach()
install(FILES README.md CHANGELOG.md CONTRIBUTING.md PERFORMANCE.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/focusboard)
install(FILES packaging/README.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/focusboard/packaging)
install(FILES packaging/windows/README.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/focusboard/packaging/windows)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/focusboard)
endif()
configure_file(src/integration/InstallPaths.hpp.in generated/InstallPaths.hpp @ONLY)
target_include_directories(focusboard PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

if(WIN32)
    set_target_properties(focusboard PROPERTIES OUTPUT_NAME Focusboard WIN32_EXECUTABLE TRUE)
    configure_file(packaging/windows/focusboard.rc.in generated/focusboard.rc @ONLY)
    target_sources(focusboard PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated/focusboard.rc)
    qt_add_resources(focusboard branding PREFIX "/icons" BASE assets/icons FILES assets/icons/256x256.png)
    qt_generate_deploy_qml_app_script(TARGET focusboard OUTPUT_SCRIPT deploy_script
        NO_TRANSLATIONS NO_COMPILER_RUNTIME DEPLOY_TOOL_OPTIONS --release)
    install(SCRIPT ${deploy_script})
    # App-local redistributable CRT DLLs: no elevated vc_redist installer.
    # Windows 10/11 already provide the Universal CRT.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
    if(MSVC AND NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        message(FATAL_ERROR "MSVC redistributable runtime was not found")
    endif()
    install(FILES LICENSE README.md CHANGELOG.md CONTRIBUTING.md PERFORMANCE.md DESTINATION .)
    install(FILES packaging/README.md DESTINATION packaging)
    install(FILES packaging/windows/README.md DESTINATION packaging/windows)
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/version.txt" CONTENT "${PROJECT_VERSION}")
endif()

if(BUILD_TESTING)
find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(focusboard-tests tests/tst_core.cpp src/core/DocumentStore.cpp src/core/DocumentStore.hpp src/core/SettingsManager.cpp src/core/SettingsManager.hpp ${platform_sources} src/core/DesktopWindowController.hpp src/core/MultiMonitorController.cpp src/core/MultiMonitorController.hpp src/integration/AutostartManager.hpp)
target_link_libraries(focusboard-tests PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::Test ${platform_libraries})
target_include_directories(focusboard-tests PRIVATE src/core src/integration ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_test(NAME focusboard-core COMMAND focusboard-tests)
if(WIN32)
    qt_add_executable(focusboard-windows-tests tests/tst_windows.cpp
        src/core/SettingsManager.cpp src/core/SettingsManager.hpp
        ${platform_sources} src/core/DesktopWindowController.hpp src/integration/AutostartManager.hpp)
    target_include_directories(focusboard-windows-tests PRIVATE src/core src/integration)
    target_link_libraries(focusboard-windows-tests PRIVATE Qt6::Quick Qt6::Test ${platform_libraries})
    add_test(NAME focusboard-windows-native COMMAND focusboard-windows-tests)
    set_tests_properties(focusboard-windows-native PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=windows")
endif()
target_sources(focusboard-tests PRIVATE src/core/CommandController.cpp src/core/CommandController.hpp)
qt_add_executable(focusboard-ui-tests tests/tst_presentation.cpp
    src/core/DocumentStore.cpp src/core/SettingsManager.cpp src/core/CommandController.cpp
    ${platform_sources} src/core/MarkdownPreview.cpp
    src/core/DesktopWindowController.hpp src/integration/AutostartManager.hpp)
target_link_libraries(focusboard-ui-tests PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Test ${platform_libraries})
target_compile_definitions(focusboard-ui-tests PRIVATE FOCUSBOARD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(focusboard-ui-tests PRIVATE src/core src/integration ${CMAKE_CURRENT_BINARY_DIR}/generated)
add_test(NAME focusboard-presentation COMMAND focusboard-ui-tests)
if(NOT WIN32)
add_test(NAME focusboard-installer COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/tst_autostart_installer.sh)
add_test(NAME focusboard-autostart-cli COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/tst_autostart_cli.sh $<TARGET_FILE:focusboard>)
endif()
add_test(NAME focusboard-qml-resource COMMAND focusboard --verify-qml-resource)
set_tests_properties(focusboard-presentation focusboard-qml-resource PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
target_include_directories(focusboard-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
endif()

# Keep MSVC source encoding flags off the Windows resource compiler.
foreach(target IN ITEMS focusboard focusboard-tests focusboard-ui-tests focusboard-windows-tests)
    if(TARGET ${target})
        target_compile_options(${target} PRIVATE
            "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/utf-8;/Zc:__cplusplus>")
    endif()
endforeach()
