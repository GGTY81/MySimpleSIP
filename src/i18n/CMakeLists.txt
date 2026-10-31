# Pacotes de idioma (port de lib/langpack.* do MicroSIP)
add_library(msip_i18n STATIC
    langpack.cpp
)
target_include_directories(msip_i18n PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(msip_i18n PUBLIC msip_compat)
target_compile_definitions(msip_i18n PRIVATE
    MSIP_DATA_DIR="${CMAKE_INSTALL_FULL_DATADIR}/mysimplesip")
target_compile_options(msip_i18n PRIVATE -Wall -Wextra)
