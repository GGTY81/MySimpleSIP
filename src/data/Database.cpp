/*
 * MySimpleSIP — port de Data/Database.cpp (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "afx.h" // MySimpleSIP: stdafx.h
#include "Database.h"

// MySimpleSIP: #pragma comment(lib, ...) → target_link_libraries (src/data/CMakeLists.txt)

Database::Database(const std::string& filename)
{
    m_db = std::make_unique<SQLite::Database>(
        filename,
        SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE
    );
    Get().exec("PRAGMA busy_timeout=5000;");
}

SQLite::Database& Database::Get()
{
    return *m_db;
}

void Database::InitCalls()
{
    Get().exec(
        "CREATE TABLE IF NOT EXISTS call_log ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT"
        ",callId TEXT NOT NULL"
        ",number TEXT NOT NULL"
        ",name TEXT NOT NULL"
        ",type INTEGER"
        ",time INTEGER"
        ",duration INTEGER"
        ",info TEXT NOT NULL"
        ");"
    );
    Get().exec("CREATE UNIQUE INDEX IF NOT EXISTS idx_call_log_callId_unique ON call_log(callId);");

    Get().exec("DELETE FROM call_log\
        WHERE(SELECT COUNT(*) FROM call_log) > 1000\
        AND id <= (\
            SELECT id FROM call_log\
            ORDER BY id DESC\
            LIMIT 1 OFFSET 1000\
            );"
    );

}

void Database::InitContacts()
{
}
