/*
 * MySimpleSIP — port de Data/ContactsRepository.cpp (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "afx.h" // MySimpleSIP: stdafx.h
#include "ContactsRepository.h"
#include "../utils/StringConverter.h" // MySimpleSIP: caminho em minúsculas

ContactsRepository::ContactsRepository(SQLite::Database& db)
    : m_db(db)
{
}

void ContactsRepository::BeginTransaction()
{
    m_db.exec("BEGIN TRANSACTION;");
}

void ContactsRepository::Commit()
{
    m_db.exec("COMMIT;");
}

void ContactsRepository::Rollback()
{
    m_db.exec("ROLLBACK;");
}

int ContactsRepository::Add(const Contact& contact)
{
    SQLite::Statement query(m_db,
        "INSERT INTO contacts "
        "(number, name, info"
        ") VALUES (?, ?, ?"
        ");");

    BindContactFields(query, contact, false);
    query.exec();

    return static_cast<int>(m_db.getLastInsertRowid());
}

void ContactsRepository::Update(const Contact& contact)
{
    SQLite::Statement query(m_db,
        "UPDATE contacts SET "
        "number=?, name=?, info=? "
        "WHERE id=?;");

    BindContactFields(query, contact, true);
    query.exec();
}

void ContactsRepository::Delete(int id)
{
    SQLite::Statement query(m_db,
        "DELETE FROM contacts WHERE id=?;");
    query.bind(1, id);
    query.exec();
}

void ContactsRepository::DeleteAll()
{
    m_db.exec("DELETE FROM contacts;");
}

std::unique_ptr<Contact> ContactsRepository::GetById(int id)
{
    SQLite::Statement query(m_db,
        "SELECT * FROM contacts WHERE id=?;");
    query.bind(1, id);

    if (query.executeStep())
        return MapQueryToContact(query);

    return nullptr;
}

std::unique_ptr<Contact> ContactsRepository::GetByContactId(const std::string& contactId)
{
    SQLite::Statement query(m_db,
        "SELECT * FROM contacts WHERE contactId=?;");
    query.bind(1, contactId);

    if (query.executeStep())
        return MapQueryToContact(query);

    return nullptr;
}

std::vector<std::unique_ptr<Contact>> ContactsRepository::GetAll()
{
    std::vector<std::unique_ptr<Contact>> result;
    SQLite::Statement query(m_db, "SELECT * FROM contacts;");

    while (query.executeStep())
    {
        result.push_back(MapQueryToContact(query));
    }

    return result;
}

std::unique_ptr<Contact> ContactsRepository::MapQueryToContact(SQLite::Statement& query)
{
    auto contact = std::make_unique<Contact>();
    contact->number = Utf8ToCString(query.getColumn("number").getString());
    contact->name = Utf8ToCString(query.getColumn("name").getString());
    contact->info = Utf8ToCString(query.getColumn("info").getString());
    return contact;
}

void ContactsRepository::BindContactFields(SQLite::Statement& query, const Contact& contact, bool includeId)
{
    //int index = 1;
    //query.bind(index++, CStringToUtf8(contact.number));
    //query.bind(index++, CStringToUtf8(contact.name));
    //query.bind(index++, CStringToUtf8(contact.info));
    //if (includeId) {
    //    query.bind(index++, contact.id);
    //}
}
