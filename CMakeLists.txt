cmake_minimum_required(VERSION 3.25)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MYSIMPLESIP_VERSION LIMIT_COUNT 1)
string(STRIP "${MYSIMPLESIP_VERSION}" MYSIMPLESIP_VERSION)
# reconfigura quando o arquivo VERSION muda
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

project(MySimpleSIP
    VERSION ${MYSIMPLESIP_VERSION}
    DESCRIPTION "Softphone SIP para Linux baseado no MicroSIP"
    LANGUAGES C CXX)

# Versão do MicroSIP usada como referência do port
set(MSIP_BASE_VERSION "3.22.16")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Tipo de build" FORCE)
endif()

include(GNUInstallDirs)
# pastas instaladas com 0755 independentemente do umask (pacote .deb)
set(CMAKE_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Widgets Network DBus)
find_package(PJPROJECT REQUIRED)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)   # .qrc listados nos fontes (qt_standard_project_setup só liga MOC/UIC)

add_subdirectory(src)
include(Packaging)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
