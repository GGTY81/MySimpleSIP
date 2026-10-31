# Camada de compatibilidade MFC/Win32 (sem dependência de Qt)
add_library(msip_compat STATIC
    CString.cpp
    CTime.cpp
    IniFile.cpp
    PrivateProfile.cpp
    StdioFileEx.cpp
    Win32Window.cpp
)

target_include_directories(msip_compat PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(msip_compat PRIVATE -Wall -Wextra)
