cmake_minimum_required(VERSION 3.24)
project(Focusboard VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

include(GNUInstallDirs)
include(CTest)
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets)
find_package(LayerShellQt REQUIRED)
qt_standard_project_setup(REQUIRES 6.5)
qt_policy(SET QTP0004 NEW)

qt_add_executable(focusboard
    src/main.cpp
    src/core/DocumentStore.cpp src/core/DocumentStore.hpp
    src/core/SettingsManager.cpp src/core/SettingsManager.hpp
    src/core/CommandController.cpp src/core/CommandController.hpp
    src/core/MarkdownPreview.cpp src/core/MarkdownPreview.hpp
    src/core/LayerShellWindowController.cpp src/core/LayerShellWindowController.hpp
    src/integration/AutostartManager.cpp src/integration/AutostartManager.hpp
)
qt_add_qml_module(focusboard URI Focusboard VERSION 1.0 QML_FILES
    qml/Main.qml
    qml/components/CommandPalette.qml
    qml/components/ResizeHandle.qml
)
target_link_libraries(focusboard PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::Widgets LayerShellQt::Interface)
target_include_directories(focusboard PRIVATE src/core)
target_compile_definitions(focusboard PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
    FOCUSBOARD_VERSION="${PROJECT_VERSION}")

install(TARGETS focusboard RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
set_target_properties(focusboard PROPERTIES INSTALL_RPATH "")
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
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/focusboard)
configure_file(src/integration/InstallPaths.hpp.in generated/InstallPaths.hpp @ONLY)
target_include_directories(focusboard PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)

if(BUILD_TESTING)
find_package(Qt6 REQUIRED COMPONENTS Test)
add_executable(focusboard-tests tests/tst_core.cpp src/core/DocumentStore.cpp src/core/DocumentStore.hpp src/core/SettingsManager.cpp src/core/SettingsManager.hpp src/core/LayerShellWindowController.cpp src/core/LayerShellWindowController.hpp src/integration/AutostartManager.cpp src/integration/AutostartManager.hpp)
target_link_libraries(focusboard-tests PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::Test LayerShellQt::Interface)
add_test(NAME focusboard-core COMMAND focusboard-tests)
target_sources(focusboard-tests PRIVATE src/core/CommandController.cpp src/core/CommandController.hpp)
qt_add_executable(focusboard-ui-tests tests/tst_presentation.cpp
    src/core/DocumentStore.cpp src/core/SettingsManager.cpp src/core/CommandController.cpp
    src/core/LayerShellWindowController.cpp src/core/MarkdownPreview.cpp)
target_link_libraries(focusboard-ui-tests PRIVATE Qt6::Quick Qt6::QuickControls2 Qt6::Test LayerShellQt::Interface)
target_compile_definitions(focusboard-ui-tests PRIVATE FOCUSBOARD_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME focusboard-presentation COMMAND focusboard-ui-tests)
set_tests_properties(focusboard-presentation PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
target_include_directories(focusboard-tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
endif()
