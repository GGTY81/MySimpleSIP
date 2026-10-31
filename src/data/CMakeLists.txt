# Dados (port de Data/, Service/, Domain/ e lib/CSVFile do MicroSIP): histórico de chamadas
# em SQLite (call_log.db), contatos e importação/exportação CSV. Sem Qt Widgets.
find_package(SQLiteCpp REQUIRED)
add_library(msip_data STATIC
    Database.cpp
    CallsRepository.cpp
    ContactsRepository.cpp
    ../service/CallsService.cpp
    ../service/ContactsService.cpp
    ../utils/Markup.cpp
    ../utils/CSVFile.cpp
)
target_include_directories(msip_data PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/domain"
    "${PROJECT_SOURCE_DIR}/src/service"
    "${PROJECT_SOURCE_DIR}/src/utils"
)
target_link_libraries(msip_data PUBLIC msip_core SQLiteCpp)
target_compile_options(msip_data PRIVATE -Wall -Wextra -Wno-reorder)
# Código portado mantido como no original: silencia avisos de estilo (variáveis sem uso...).
set_source_files_properties(ContactsRepository.cpp ../service/CallsService.cpp ../service/ContactsService.cpp ../utils/CSVFile.cpp PROPERTIES COMPILE_OPTIONS
    "-Wno-unused-parameter;-Wno-unused-variable")
