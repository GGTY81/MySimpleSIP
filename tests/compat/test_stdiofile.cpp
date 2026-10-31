// CStdioFileEx (compat): texto UTF-8 com BOM, quebras de linha e leitura linha a linha.

#include "afx.h"

#include <gtest/gtest.h>

#include "TestPrint.h"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

TEST(StdioFileEx, GravaELeUtf8ComBom)
{
    const fs::path file = fs::temp_directory_path() / "msip-stdiofile-test.csv";
    {
        CStdioFileEx f;
        f.SetCodePage(CP_UTF8);
        ASSERT_TRUE(f.Open(CString(file.c_str()), CFile::modeCreate | CFile::modeWrite | CFile::typeText));
        f.WriteString(_T("Nome,Número\n"));
        f.WriteString(_T("José,1001\n"));
        f.Close();
    }
    std::ifstream in(file, std::ios::binary);
    std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    EXPECT_EQ(bytes, "\xEF\xBB\xBFNome,N\xC3\xBAmero\r\nJos\xC3\xA9,1001\r\n");

    CStdioFileEx f;
    f.SetCodePage(CP_UTF8);
    ASSERT_TRUE(f.Open(CString(file.c_str()), CFile::modeRead | CFile::typeText));
    CString line;
    ASSERT_TRUE(f.ReadString(line));
    EXPECT_EQ(line, _T("Nome,Número")); // BOM e \r\n removidos
    ASSERT_TRUE(f.ReadString(line));
    EXPECT_EQ(line, _T("José,1001"));
    EXPECT_FALSE(f.ReadString(line));
    f.Close();
    fs::remove(file);

    CStdioFileEx missing;
    EXPECT_FALSE(missing.Open(_T("/nonexistent/dir/file.csv"), CFile::modeRead));
}
