/*
 * MySimpleSIP — port de Data/IContactsRepository.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include <vector>
#include <memory>
#include "../domain/Contact.h"

class IContactsRepository
{
public:
    virtual ~IContactsRepository() = default;

    virtual void BeginTransaction() = 0;
    virtual void Commit() = 0;
    virtual void Rollback() = 0;

    virtual int Add(const Contact& log) = 0;
    virtual void Update(const Contact& log) = 0;
    virtual void Delete(int id) = 0;
    virtual void DeleteAll() = 0;

    virtual std::unique_ptr<Contact> GetById(int id) = 0;
    virtual std::unique_ptr<Contact> GetByContactId(const std::string& contactId) = 0;
    virtual std::vector<std::unique_ptr<Contact>> GetAll() = 0;
};
