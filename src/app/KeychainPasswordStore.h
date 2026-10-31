/*
 * MySimpleSIP — senhas das contas no chaveiro do sistema (Secret Service via QtKeychain).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// Implementa IPasswordStore (settings/PasswordStore.h). Cada senha fica no chaveiro sob
// o serviço "MySimpleSIP" e a chave "<seção>@<arquivo INI>" (ex.: "Account1@/home/u/
// .config/MySimpleSIP/MySimpleSIP.ini"), para perfis diferentes não se misturarem.
// As operações são síncronas (esperam o job do QtKeychain num QEventLoop local); se o
// chaveiro não estiver disponível, retornam false e o AccountSettings usa o fallback
// ofuscado no INI.

#pragma once

#include "PasswordStore.h"

class KeychainPasswordStore : public IPasswordStore {
public:
    // iniFile vazio = usa accountSettings.iniFile no momento de cada operação (o store é
    // instalado antes de accountSettings.Init(), que já lê as senhas)
    explicit KeychainPasswordStore(const CString& iniFile = CString());

    bool Load(const CString& account, CString& password) override;
    bool Save(const CString& account, const CString& password) override;
    void Remove(const CString& account) override;

private:
    CString Key(const CString& account) const;
    CString m_iniFile;
};
