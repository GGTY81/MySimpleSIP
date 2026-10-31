/*
 * MySimpleSIP — port de Domain/Call.h (MicroSIP 3.22.16).
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#pragma once
#include "afx.h" // MySimpleSIP: CString (no original, via stdafx.h)
#include "define.h"

enum { MSIP_CALL_TYPE_OUTGOING, MSIP_CALL_TYPE_INCOMING, MSIP_CALL_TYPE_MISSED, MSIP_CALL_TYPE_AE};
#define MSIP_CALL_TYPE_MIN MSIP_CALL_TYPE_OUTGOING
#define MSIP_CALL_TYPE_MAX MSIP_CALL_TYPE_AE

class Call
{
public:
    int id = 0;
    CString callId;
    CString number;
    CString name;
    int type = 0;
    int64_t time = 0;
    int duration = 0;
    CString info;
};
