# Constantes globais portadas de define.h/const.h do MicroSIP (somente cabeçalhos)
add_library(msip_common INTERFACE)
target_include_directories(msip_common INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_BINARY_DIR}/src"   # version.h gerado
)
target_link_libraries(msip_common INTERFACE msip_compat)
