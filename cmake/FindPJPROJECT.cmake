# Localiza o pjproject estático compilado por scripts/build-pjproject.sh
# e cria o alvo importado PJPROJECT::pjproject.
#
# Variável de entrada opcional: PJPROJECT_PREFIX
#   (padrão: <raiz>/third_party/pjproject/install)

set(PJPROJECT_PREFIX "${PROJECT_SOURCE_DIR}/third_party/pjproject/install"
    CACHE PATH "Prefixo de instalação do pjproject")

find_package(PkgConfig REQUIRED)

set(_pj_saved_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
set(ENV{PKG_CONFIG_PATH} "${PJPROJECT_PREFIX}/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
pkg_check_modules(_PJ QUIET libpjproject)
set(ENV{PKG_CONFIG_PATH} "${_pj_saved_pkg_config_path}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PJPROJECT
    REQUIRED_VARS _PJ_FOUND _PJ_STATIC_LDFLAGS
    VERSION_VAR _PJ_VERSION
    FAIL_MESSAGE "pjproject não encontrado em ${PJPROJECT_PREFIX}. Rode ./scripts/build-pjproject.sh")

if(PJPROJECT_FOUND AND NOT TARGET PJPROJECT::pjproject)
    set(PJPROJECT_VERSION "${_PJ_VERSION}")
    add_library(PJPROJECT::pjproject INTERFACE IMPORTED)
    set_target_properties(PJPROJECT::pjproject PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${_PJ_STATIC_INCLUDE_DIRS}"
        INTERFACE_COMPILE_OPTIONS "${_PJ_STATIC_CFLAGS_OTHER}"
        # Em LINK_LIBRARIES (e não LINK_OPTIONS) para que as -l fiquem depois
        # dos objetos na linha de ligação, como exigem bibliotecas estáticas.
        INTERFACE_LINK_LIBRARIES "${_PJ_STATIC_LDFLAGS}")
endif()
