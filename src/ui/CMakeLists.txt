# Interface Qt Widgets (port das janelas do MicroSIP)
add_library(msip_ui STATIC
    UiCommon.cpp
    mainDlg.cpp
    DialogLayout.cpp
    AccountDlg.cpp
    DialerControls.cpp
    Dialer.cpp
    MessagesDlg.cpp
    Calls.cpp
    Contacts.cpp
    AddDlg.cpp
    Transfer.cpp
    SettingsDlg.cpp
    AAOptionsDlg.cpp
    FeatureCodesDlg.cpp
    ShortcutsDlg.cpp
    AccountsListsDlg.cpp
    ExportDlg.cpp
    ImportDlg.cpp
    RinginDlg.cpp
)
target_include_directories(msip_ui PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/resources"   # icon_table.inc
)
target_link_libraries(msip_ui PUBLIC msip_data msip_core msip_appqt Qt6::Widgets)
target_compile_options(msip_ui PRIVATE -Wall -Wextra -Wno-reorder)
