/*
 * MySimpleSIP — senhas das contas no chaveiro do sistema (Secret Service via QtKeychain).
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "KeychainPasswordStore.h"

#include "settings.h"

#include <QEventLoop>
#include <qt6keychain/keychain.h>

namespace {

const QString kService = QStringLiteral("MySimpleSIP");


// Executa o job e espera terminar; true se não houve erro.
bool run(QKeychain::Job& job)
{
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.setAutoDelete(false);
    job.start();
    loop.exec();
    return job.error() == QKeychain::NoError;
}

} // namespace

KeychainPasswordStore::KeychainPasswordStore(const CString& iniFile) : m_iniFile(iniFile)
{
}

CString KeychainPasswordStore::Key(const CString& account) const
{
    return account + _T("@") + (m_iniFile.IsEmpty() ? accountSettings.iniFile : m_iniFile);
}

bool KeychainPasswordStore::Load(const CString& account, CString& password)
{
    QKeychain::ReadPasswordJob job(kService);
    job.setKey(QString::fromStdWString(Key(account).str()));
    if (!run(job))
        return false;
    password = CString(job.textData().toStdWString());
    return true;
}

bool KeychainPasswordStore::Save(const CString& account, const CString& password)
{
    QKeychain::WritePasswordJob job(kService);
    job.setKey(QString::fromStdWString(Key(account).str()));
    job.setTextData(QString::fromStdWString(password.str()));
    return run(job);
}

void KeychainPasswordStore::Remove(const CString& account)
{
    QKeychain::DeletePasswordJob job(kService);
    job.setKey(QString::fromStdWString(Key(account).str()));
    run(job);
}
