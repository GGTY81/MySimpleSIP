/*
 * MySimpleSIP — Get/WritePrivateProfileString* do Win32 sobre IniFile.
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Mesma assinatura e mesmos valores de retorno do Win32, para portar o settings.cpp
// do MicroSIP sem reescrevê-lo. Os arquivos ficam em cache (recarregados se mudarem
// no disco) e cada gravação é salva imediatamente, como no Windows.
//
// GetPrivateProfileString:
//   key != NULL  → valor (ou default, sem espaços finais); retorna nº de caracteres
//                  copiados; se não couber, copia size-1 e retorna size-1.
//   key == NULL  → nomes das chaves da seção; section == NULL → nomes das seções;
//                  cada nome terminado em NUL e um NUL extra no fim; se não couber,
//                  trunca e retorna size-2.
// WritePrivateProfileString: value NULL apaga a chave; key NULL apaga a seção.
// WritePrivateProfileSection: data NULL apaga a seção; senão substitui o conteúdo
//   pela lista "chave=valor\0...\0\0".

#pragma once

#include "WinTypes.h"

DWORD GetPrivateProfileString(LPCTSTR section, LPCTSTR key, LPCTSTR defaultValue,
                              LPTSTR buffer, DWORD size, LPCTSTR fileName);
UINT GetPrivateProfileInt(LPCTSTR section, LPCTSTR key, int defaultValue, LPCTSTR fileName);
BOOL WritePrivateProfileString(LPCTSTR section, LPCTSTR key, LPCTSTR value, LPCTSTR fileName);
BOOL WritePrivateProfileSection(LPCTSTR section, LPCTSTR data, LPCTSTR fileName);

// Descarta o cache (testes, ou após alterar o arquivo por fora).
void PrivateProfileFlushCache();
