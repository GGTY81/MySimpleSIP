configure_file(version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

add_subdirectory(compat)
add_subdirectory(common)
add_subdirectory(settings)
add_subdirectory(i18n)
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(app)
add_subdirectory(cli)
add_subdirectory(ui)
add_subdirectory(tools)

qt_add_executable(mysimplesip
    main.cpp
    "${PROJECT_SOURCE_DIR}/resources/resources.qrc"
    "${PROJECT_SOURCE_DIR}/resources/layout.qrc"
    "${PROJECT_SOURCE_DIR}/resources/brand.qrc"
)

target_include_directories(mysimplesip PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(mysimplesip PRIVATE
    msip_ui
    Qt6::DBus
)

target_compile_options(mysimplesip PRIVATE -Wall -Wextra -Wno-reorder)

# Execução a partir de build/: langpacks e sons ao lado do executável (como no modo
# portátil do MicroSIP), até o empacotamento definir os caminhos de instalação.
add_custom_command(TARGET mysimplesip POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/langpacks" "$<TARGET_FILE_DIR:mysimplesip>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/resources/sounds" "$<TARGET_FILE_DIR:mysimplesip>"
    VERBATIM)

install(TARGETS mysimplesip RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
# Sons e langpacks são lidos do disco (pjsua_player_create precisa de arquivo)
install(DIRECTORY "${PROJECT_SOURCE_DIR}/resources/sounds"
                  "${PROJECT_SOURCE_DIR}/resources/langpacks"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/mysimplesip")
