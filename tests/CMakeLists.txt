find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(test_compat
    test_main.cpp
    compat/test_cstring.cpp
    compat/test_ctime.cpp
    compat/test_collections.cpp
    compat/test_inifile.cpp
    compat/test_win32window.cpp
    compat/test_stdiofile.cpp
    settings/test_paths.cpp
    settings/test_settings.cpp
    i18n/test_langpack.cpp
)
target_include_directories(test_compat PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_compat PRIVATE msip_compat msip_settings msip_i18n GTest::gtest)
target_compile_definitions(test_compat PRIVATE
    MSIP_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    MSIP_RESOURCES_DIR="${PROJECT_SOURCE_DIR}/resources")
target_compile_options(test_compat PRIVATE -Wall -Wextra)
gtest_discover_tests(test_compat)

# Núcleo (liga pjproject e Qt Core/Network)
add_executable(test_core
    test_main.cpp
    core/test_core.cpp
    core/test_mp3.cpp
    core/test_pulse_audio.cpp
)
target_link_libraries(test_core PRIVATE msip_core GTest::gtest)
target_include_directories(test_core PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(test_core PRIVATE -Wall -Wextra -Wno-reorder)
gtest_discover_tests(test_core)

# Dados: histórico (SQLite) e CSV
add_executable(test_storage
    test_main.cpp
    storage/test_calls_service.cpp
    storage/test_contacts_service.cpp
)
target_link_libraries(test_storage PRIVATE msip_data GTest::gtest)
target_include_directories(test_storage PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(test_storage PRIVATE -Wall -Wextra -Wno-reorder)
gtest_discover_tests(test_storage)

# Integração com Qt (chaveiro); o teste do chaveiro real só roda com MSIP_TEST_KEYRING=1
add_executable(test_app
    test_main.cpp
    app/test_keychain.cpp
)
target_include_directories(test_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(test_app PRIVATE msip_appqt GTest::gtest)
target_compile_options(test_app PRIVATE -Wall -Wextra)
gtest_discover_tests(test_app)

# Interface Qt (offscreen)
qt_add_executable(test_ui
    test_main.cpp
    ui/test_accountdlg.cpp
    ui/test_dialer.cpp
    ui/test_messagesdlg.cpp
    ui/test_calls.cpp
    ui/test_contacts.cpp
    ui/test_presence.cpp
    ui/test_transfer.cpp
    ui/test_settings_dlg.cpp
    ui/test_shortcuts.cpp
    ui/test_export_import.cpp
    ui/test_chat.cpp
    ui/test_command_line.cpp
    ui/test_system_monitors.cpp
    "${PROJECT_SOURCE_DIR}/resources/resources.qrc"
    "${PROJECT_SOURCE_DIR}/resources/layout.qrc"
)
target_include_directories(test_ui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Qt6 REQUIRED COMPONENTS Test)
target_link_libraries(test_ui PRIVATE msip_ui Qt6::Test GTest::gtest)
target_compile_options(test_ui PRIVATE -Wall -Wextra -Wno-reorder)
target_compile_definitions(test_ui PRIVATE MSIP_CLI_PATH="$<TARGET_FILE:msip-cli>" MSIP_APP_PATH="$<TARGET_FILE:mysimplesip>")
add_dependencies(test_ui msip-cli mysimplesip)
gtest_discover_tests(test_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
