/*
 * MySimpleSIP — port de lib/CSVFile.h (MicroSIP 3.22.16): leitura/gravação de CSV.
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>. GPL-2.0+.
 */
#pragma once
#include "afx.h"
#include "StdioFileEx.h" // MySimpleSIP: CStdioFileEx de compat/

class CCSVFile : public CStdioFileEx
{
public:
  CCSVFile();
  bool ReadData(CStringArray &arr);
  void WriteData(CStringArray &arr);
};
