#pragma once

#include "afx.h" // MySimpleSIP

class CMask
{
public:
	BOOL WildMatch(CString sWild, CString sString, CString sLimitChar);
};
