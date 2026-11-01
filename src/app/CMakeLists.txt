# Integração do núcleo com o laço de eventos do Qt (usado pela CLI e pela janela)
add_library(msip_appqt STATIC
    QtMessageBackend.cpp
    KeychainPasswordStore.cpp
    SystemMonitors.cpp
)
set_target_properties(msip_appqt PROPERTIES AUTOMOC ON)
target_include_directories(msip_appqt PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(Qt6Keychain REQUIRED)
target_link_libraries(msip_appqt PUBLIC msip_compat msip_settings Qt6::Core Qt6::DBus qt6keychain)
target_compile_options(msip_appqt PRIVATE -Wall -Wextra)
