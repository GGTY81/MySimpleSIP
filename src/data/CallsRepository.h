/*
 * MySimpleSIP — port de Data/CallsRepository.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include "ICallsRepository.h"
#include <SQLiteCpp/SQLiteCpp.h>

class CallsRepository : public ICallsRepository
{
public:
    explicit CallsRepository(SQLite::Database& db);
    
    void BeginTransaction();
    void Commit();
    void Rollback();    

    int Add(const Call& call) override;
    void Update(const Call& call) override;
    void Delete(int id) override;
    void DeleteByIds(std::vector<int> ids);
    void DeleteAll() override;

    std::unique_ptr<Call> GetById(int id) override;
    std::unique_ptr<Call> GetByCallId(const CString& callId);
    std::vector<std::unique_ptr<Call>> GetAll(int limit = 0) override;

private:
    SQLite::Database& m_db;
    std::unique_ptr<Call> MapQueryToCall(SQLite::Statement& query);
    void BindCallFields(SQLite::Statement& query, const Call& call, bool includeId);
};
