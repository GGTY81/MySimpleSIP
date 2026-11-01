# Núcleo de telefonia: port de global.*, lib/MSIP.* e (próximas etapas) da parte pjsua de mainDlg.cpp.
# Usa Qt Core/Network (HTTP), nunca Qt Widgets.
add_library(msip_core STATIC
    MSIP.cpp
    global.cpp
    DialPlanRegex.cpp
    http_qt.cpp
    mainDlgCore.cpp
    callbacks.cpp
    CMask.cpp
    Mp3Encode.cpp
    PulseAudioDev.cpp
    HidParser.cpp
)
target_include_directories(msip_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED IMPORTED_TARGET jsoncpp)
pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse libpulse-simple)
target_link_libraries(msip_core PUBLIC
    msip_settings
    msip_i18n
    PJPROJECT::pjproject
    Qt6::Core
    Qt6::Network
    PkgConfig::JSONCPP
    mp3lame
    PkgConfig::PULSE
)
target_compile_options(msip_core PRIVATE -Wall -Wextra -Wno-reorder) # -Wno-reorder: global.h original
# Código portado do MicroSIP mantido como no original: silencia só os avisos de estilo
# típicos do MSVC (NULL como inteiro, parâmetros sem uso, literais em char*...).
set_source_files_properties(global.cpp MSIP.cpp mainDlgCore.cpp callbacks.cpp CMask.cpp PROPERTIES COMPILE_OPTIONS
    "-Wno-unused-parameter;-Wno-conversion-null;-Wno-write-strings;-Wno-reorder;-Wno-sign-compare;-Wno-unused-variable;-Wno-switch;-Wno-unused-but-set-variable;-Wno-maybe-uninitialized")
