/*
 * MySimpleSIP — port de Data/ICallsRepository.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include <vector>
#include <memory>
#include "../domain/Call.h"

class ICallsRepository
{
public:
    virtual ~ICallsRepository() = default;

    virtual void BeginTransaction() = 0;
    virtual void Commit() = 0;
    virtual void Rollback() = 0;

    virtual int Add(const Call& log) = 0;
    virtual void Update(const Call& log) = 0;
    virtual void Delete(int id) = 0;
    virtual void DeleteByIds(std::vector<int> ids) = 0;
    virtual void DeleteAll() = 0;

    virtual std::unique_ptr<Call> GetById(int id) = 0;
    virtual std::unique_ptr<Call> GetByCallId(const CString& callId) = 0;
    virtual std::vector<std::unique_ptr<Call>> GetAll(int limit = 0) = 0;
};
