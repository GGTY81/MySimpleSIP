#include <gtest/gtest.h>

#include <clocale>
#include <cstdlib>

int main(int argc, char** argv)
{
    // Mesma condição do aplicativo: LC_CTYPE UTF-8 (o Qt aplica o locale do sistema)
    std::setlocale(LC_ALL, "C.UTF-8");
    // Testes de CTime em fuso fixo
    setenv("TZ", "America/Sao_Paulo", 1);
    tzset();
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
