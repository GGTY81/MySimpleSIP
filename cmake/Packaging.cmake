# Instalação da integração com o sistema e pacote .deb (CPack).
# Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
#
# Pacote: ./scripts/build-deb.sh (Release, prefixo /usr) → dist/mysimplesip_X.Y.Z_amd64.deb

set(MSIP_LINUX_DIR "${PROJECT_SOURCE_DIR}/resources/linux")
set(MSIP_BRAND_DIR "${PROJECT_SOURCE_DIR}/resources/brand")

# .desktop (abre os links sip:, tel: e callto:), ícones hicolor e metainfo do AppStream
install(FILES "${MSIP_LINUX_DIR}/mysimplesip.desktop" DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
foreach(size 16 24 32 48 64 128 256)
    install(FILES "${MSIP_BRAND_DIR}/png/${size}/mysimplesip.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps")
endforeach()
install(FILES "${MSIP_BRAND_DIR}/mysimplesip.svg" DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps")
string(TIMESTAMP MSIP_RELEASE_DATE "%Y-%m-%d" UTC)
configure_file("${MSIP_LINUX_DIR}/io.github.ggty81.MySimpleSIP.metainfo.xml.in"
               "${PROJECT_BINARY_DIR}/io.github.ggty81.MySimpleSIP.metainfo.xml" @ONLY)
install(FILES "${PROJECT_BINARY_DIR}/io.github.ggty81.MySimpleSIP.metainfo.xml" DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")

# Regra do udev dos fones USB HID: o udev só lê /usr/lib/udev e /etc/udev, então a regra só
# é instalada com prefixo /usr (o pacote); em /usr/local use scripts/install-hid-rules.sh
if(CMAKE_INSTALL_PREFIX STREQUAL "/usr")
    set(MSIP_INSTALL_UDEV_DEFAULT ON)
else()
    set(MSIP_INSTALL_UDEV_DEFAULT OFF)
endif()
option(MSIP_INSTALL_UDEV "Instala a regra do udev dos fones USB HID" ${MSIP_INSTALL_UDEV_DEFAULT})
if(MSIP_INSTALL_UDEV)
    set(HID_CHECK "${CMAKE_INSTALL_FULL_LIBEXECDIR}/mysimplesip/mysimplesip-hid-check")
    configure_file("${PROJECT_SOURCE_DIR}/resources/udev/70-mysimplesip-headset.rules.in"
                   "${PROJECT_BINARY_DIR}/70-mysimplesip-headset.rules" @ONLY)
    install(FILES "${PROJECT_BINARY_DIR}/70-mysimplesip-headset.rules" DESTINATION "lib/udev/rules.d")
endif()

# documentação em /usr/share/doc/<pacote>, como no Debian
install(FILES "${PROJECT_SOURCE_DIR}/COPYING" "${PROJECT_SOURCE_DIR}/README.md"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/doc/mysimplesip")

# ---------------------------------------------------------------- CPack (.deb)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "mysimplesip")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VENDOR "Giovanni Grimaldi Torelly")
set(CPACK_PACKAGE_CONTACT "Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "SIP softphone for Linux based on MicroSIP")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/GGTY81/MySimpleSIP")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
set(CPACK_STRIP_FILES ON)
set(CPACK_PACKAGE_DIRECTORY "${PROJECT_SOURCE_DIR}/dist")

set(CPACK_DEBIAN_FILE_NAME "DEB-DEFAULT") # mysimplesip_X.Y.Z_amd64.deb
set(CPACK_DEBIAN_PACKAGE_SECTION "comm")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
# descrição longa (o CPack põe o resumo como primeira linha e indenta o resto)
set(CPACK_PACKAGE_DESCRIPTION "MySimpleSIP is a native Linux port of the MicroSIP 3.22 softphone (Windows), \
built with Qt 6 and PJSIP, with the same features, layout and configuration format: SIP \
accounts, audio and video calls, transfer, conference, call recording, contacts, call history, \
presence (BLF), SIP messages, media keys and USB HID headsets.")
# bibliotecas calculadas pelo dpkg-shlibdeps (o pjproject é estático); o plugin de SVG do
# Qt não é ligado ao executável, mas o ícone é carregado por ele no tema do sistema
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-svg-plugins, pipewire-pulse | pulseaudio")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "gnome-keyring")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${MSIP_LINUX_DIR}/debian/postinst;${MSIP_LINUX_DIR}/debian/postrm")
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION ON)

include(CPack)
