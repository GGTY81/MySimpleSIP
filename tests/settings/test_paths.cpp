// Testes de ResolveAppPaths (XDG e modo portátil).

#include "Paths.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace {

class PathsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        char tmpl[] = "/tmp/msip-paths-XXXXXX";
        root = mkdtemp(tmpl);
        fs::create_directories(root / "bin");
        exe = root / "bin" / "mysimplesip";
        std::ofstream(exe) << "";
        setenv("XDG_CONFIG_HOME", (root / "config").c_str(), 1);
        setenv("XDG_DATA_HOME", (root / "data").c_str(), 1);
        setenv("XDG_STATE_HOME", (root / "state").c_str(), 1);
    }
    void TearDown() override
    {
        unsetenv("XDG_CONFIG_HOME");
        unsetenv("XDG_DATA_HOME");
        unsetenv("XDG_STATE_HOME");
        fs::remove_all(root);
    }
    CString s(const fs::path& p) { return CString(p.string().c_str()); }

    fs::path root;
    fs::path exe;
};

} // namespace

TEST_F(PathsTest, InstaladoUsaXdg)
{
    AppPaths p = ResolveAppPaths(_T("MySimpleSIP"), s(exe));
    EXPECT_FALSE(p.isPortable);
    EXPECT_TRUE(p.firstRun);
    EXPECT_EQ(p.exeFile, s(exe));
    EXPECT_EQ(p.pathExe, s(root / "bin"));
    EXPECT_EQ(p.iniFile, s(root / "config/MySimpleSIP/MySimpleSIP.ini"));
    EXPECT_EQ(p.pathRoaming, s(root / "data/MySimpleSIP/"));
    EXPECT_EQ(p.pathLocal, s(root / "state/MySimpleSIP/"));
    EXPECT_EQ(p.logFile, s(root / "state/MySimpleSIP/MySimpleSIP_log.txt"));
    EXPECT_TRUE(fs::is_directory(root / "data/MySimpleSIP"));

    std::ifstream in(root / "config/MySimpleSIP/MySimpleSIP.ini");
    std::string first;
    std::getline(in, first);
    EXPECT_EQ(first, "[Global]");

    // segunda execução: INI já existe e não está vazio
    AppPaths p2 = ResolveAppPaths(_T("MySimpleSIP"), s(exe));
    EXPECT_FALSE(p2.firstRun);
}

TEST_F(PathsTest, PortatilQuandoIniAoLadoDoExecutavel)
{
    std::ofstream(root / "bin" / "MySimpleSIP.ini") << "[Global]\nversion=0.0.9\n";
    AppPaths p = ResolveAppPaths(_T("MySimpleSIP"), s(exe));
    EXPECT_TRUE(p.isPortable);
    EXPECT_FALSE(p.firstRun);
    EXPECT_EQ(p.iniFile, s(root / "bin/MySimpleSIP.ini"));
    EXPECT_EQ(p.pathRoaming, s(root / "bin/"));
    EXPECT_EQ(p.pathLocal, s(root / "bin/"));
    EXPECT_FALSE(fs::exists(root / "config"));
}

TEST_F(PathsTest, IniVazioContaComoPrimeiraExecucao)
{
    fs::create_directories(root / "config/MySimpleSIP");
    std::ofstream(root / "config/MySimpleSIP/MySimpleSIP.ini") << "";
    AppPaths p = ResolveAppPaths(_T("MySimpleSIP"), s(exe));
    EXPECT_TRUE(p.firstRun);
    EXPECT_GT(fs::file_size(root / "config/MySimpleSIP/MySimpleSIP.ini"), 0u);
}
