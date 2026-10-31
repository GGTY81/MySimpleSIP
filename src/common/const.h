/*
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

// MySimpleSIP: equivalente ao const.h do MicroSIP 3.22.16, com a identidade do
// MySimpleSIP. A versão vem do arquivo VERSION (via version.h gerado pelo CMake).

#pragma once

#include "version.h"

#define _GLOBAL_VERSION MYSIMPLESIP_VERSION
#define _GLOBAL_VIDEO
#define _GLOBAL_NAME "MySimpleSIP"
#define _GLOBAL_COMPANY ""
// _GLOBAL_KEY (chave da cifra de senhas do MicroSIP) não existe no port: as senhas
// ficam no chaveiro do sistema (ver settings/PasswordStore.h).
