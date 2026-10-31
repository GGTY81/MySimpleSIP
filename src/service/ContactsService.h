/*
 * MySimpleSIP — port de Service/ContactsService.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once

#include <memory>
#include <vector>
#include "afx.h" // MySimpleSIP: CString/CArray (no original, via stdafx.h)
#include "../domain/Contact.h"
#include "../data/IContactsRepository.h"

class ContactsService
{
public:
    explicit ContactsService(std::unique_ptr<IContactsRepository> repository);

    bool IsContactFiletered(const Contact& contact, CString search);
    bool ContactPrepare(Contact& contact);

    // MySimpleSIP: CListCtrl* (itens na ordem da lista) → vetor de Contact* na mesma ordem
    void ExportContactsCSV(CString filename, CString search, const std::vector<Contact*>& list);
    void ExportContactsXML(CString filename, CString search, const std::vector<Contact*>& list);

    CString ImportContactsCSV(CString filename, CArray<ContactWithFields*>& contactsWithFields, bool directory = false);

private:
    std::unique_ptr<IContactsRepository> m_repository;

    bool Validate(const Contact& contact, CString* error = nullptr);
    
};
