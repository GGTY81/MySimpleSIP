// Identidade própria: ícone do aplicativo e os da bandeja vêm de resources/brand/ (e não
// do logotipo do MicroSIP), em todos os tamanhos do hicolor.

#include "UiCommon.h"
#include "resource.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QImage>

TEST(Brand, IconesProprios)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    int argc = 1;
    char name[] = "test_ui";
    char* argv[] = {name, nullptr};
    QApplication app(argc, argv);
    Q_INIT_RESOURCE(resources);

    for (int id : {IDI_MAINFRAME, IDI_INACTIVE, IDI_MISSED}) {
        const QIcon icon = LoadImageIcon(id);
        ASSERT_FALSE(icon.isNull()) << id;
        EXPECT_EQ(icon.availableSizes().size(), 7) << id;
        EXPECT_EQ(icon.pixmap(256).width(), 256);
    }
    // aplicativo azul, bandeja sem registro cinza
    const QImage app256 = LoadImageIcon(IDI_MAINFRAME).pixmap(256).toImage();
    const QColor blue = app256.pixelColor(30, 128);
    EXPECT_GT(blue.blue(), blue.red() + 60);
    const QColor grey = LoadImageIcon(IDI_INACTIVE).pixmap(256).toImage().pixelColor(30, 128);
    EXPECT_LT(qAbs(grey.blue() - grey.red()), 10);
    // chamada perdida: ponto vermelho no canto
    const QColor red = LoadImageIcon(IDI_MISSED).pixmap(256).toImage().pixelColor(200, 200);
    EXPECT_GT(red.red(), red.blue() + 100);
}
