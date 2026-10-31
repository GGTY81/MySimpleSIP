/*
 * MySimpleSIP — port de Domain/Contact.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 Giovanni Grimaldi Torelly <giovannigrimaldi@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include "afx.h" // MySimpleSIP: CString (no original, via stdafx.h)
#include "define.h"

class Contact
{
public:
    CString name;
    CString number;
    CString firstname;
    CString lastname;
    CString phone;
    CString mobile;
    CString email;
    CString address;
    CString city;
    CString state;
    CString zip;
    CString comment;
    CString id;
    bool presence = false;
    bool starred = false;
    bool directory = false;
    CString info;
    bool ringing = false;
    int image = 0;
    bool candidate = false;
};

struct ContactWithFields {
    Contact contact;
    CStringList fields;
    bool processed = false;
};
