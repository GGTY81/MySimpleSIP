/*
 * MySimpleSIP — port de Data/Database.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <string>

class Database
{
public:
    explicit Database(const std::string& filename);
    SQLite::Database& Get();
    void InitCalls();
    void InitContacts();
private:
    std::unique_ptr<SQLite::Database> m_db;
};
