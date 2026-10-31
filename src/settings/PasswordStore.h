/*
 * MySimpleSIP — armazenamento das senhas das contas SIP.
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Substitui IniEncrypt/IniDecrypt do MicroSIP (CryptoAPI com chave fixa, não
// disponível no Linux). Regras do port, aplicadas em AccountLoad/AccountSave:
//  - com um IPasswordStore instalado (chaveiro do sistema), a senha vai para ele
//    sob o nome da seção ("Account1"...) e o INI recebe password= vazio;
//  - sem armazenamento disponível, o INI recebe "msip1:" + Base64 (apenas ofuscação,
//    equivalente em segurança à cifra de chave fixa do original);
//  - um valor de password= em outro formato é tratado como texto puro e migrado no
//    ato, como o MicroSIP faz quando IniDecrypt falha. Senhas cifradas de um
//    MicroSIP.ini do Windows não podem ser decifradas e precisarão ser redigitadas.

#pragma once

#include "afx.h"

class IPasswordStore {
public:
    virtual ~IPasswordStore() = default;
    virtual bool Load(const CString& account, CString& password) = 0;
    virtual bool Save(const CString& account, const CString& password) = 0;
    virtual void Remove(const CString& account) = 0;
};

// Armazenamento usado por AccountSettings (nullptr = nenhum; não assume a posse).
void SetPasswordStore(IPasswordStore* store);
IPasswordStore* GetPasswordStore();

// Ofuscação de fallback "msip1:<base64 de UTF-8>".
CString PasswordObfuscate(const CString& password);
bool PasswordDeobfuscate(const CString& value, CString& password);
