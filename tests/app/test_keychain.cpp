// Teste do KeychainPasswordStore contra o chaveiro real (Secret Service).
// Opcional: só roda com MSIP_TEST_KEYRING=1, para não mexer no chaveiro em todo ctest.

#include "KeychainPasswordStore.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <QCoreApplication>

#include <cstdlib>

TEST(KeychainPasswordStore, GravaLeApaga)
{
    if (!std::getenv("MSIP_TEST_KEYRING"))
        GTEST_SKIP() << "defina MSIP_TEST_KEYRING=1 para testar com o chaveiro real";
    int argc = 1;
    char name[] = "test_app";
    char* argv[] = {name, nullptr};
    QCoreApplication app(argc, argv);

    KeychainPasswordStore store(_T("teste"));
    ASSERT_TRUE(store.Save(_T("Account99"), _T("s3nh@ çã 😀")));
    CString out;
    ASSERT_TRUE(store.Load(_T("Account99"), out));
    EXPECT_EQ(out, _T("s3nh@ çã 😀"));
    store.Remove(_T("Account99"));
    CString gone;
    EXPECT_FALSE(store.Load(_T("Account99"), gone));
}
