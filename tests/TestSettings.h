// Reinicia o accountSettings global para um teste. AccountSettings tem construtor vazio
// (como no MicroSIP): no app o objeto global começa zerado por ser estático, mas uma
// cópia de AccountSettings() temporário leva lixo nos bool/int (UBSan: "load of value N,
// which is not a valid value for type 'bool'"). Aqui a cópia vem de um estático zerado.

#pragma once

#include "settings.h"

inline void ResetAccountSettings()
{
    static const AccountSettings fresh;
    accountSettings = fresh;
}
