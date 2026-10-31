# Configurações: caminhos, senhas e AccountSettings (port de settings.cpp)
add_library(msip_settings STATIC
    Paths.cpp
    PasswordStore.cpp
    settings.cpp
)

target_include_directories(msip_settings PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/domain"
)
target_link_libraries(msip_settings PUBLIC msip_compat msip_common)
target_compile_options(msip_settings PRIVATE -Wall -Wextra)
