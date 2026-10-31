/*
 * MySimpleSIP — port de Service/CallsService.h (MicroSIP 3.22.16).
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
#include "../domain/Call.h"
#include "../data/ICallsRepository.h"

class CallsService
{
public:
    explicit CallsService(std::unique_ptr<ICallsRepository> repository);

    int AddCall(Call& log);
    void UpdateCall(const Call& log);
    void DeleteCall(int id);
    void DeleteCalls(std::vector<int> ids);
    void DeleteAllCalls();

    std::unique_ptr<Call> GetCall(int id);
    std::unique_ptr<Call> GetCallByCallId(const CString& callId);
    std::vector<std::unique_ptr<Call>> GetAllCalls();

    bool IsCallFiletered(const Call& call, CString search);

    void ExportCallsCSV(CString filename, CString search);

    std::string ImportCallsCSV(CString filename);

private:
    std::unique_ptr<ICallsRepository> m_repository;

    bool Validate(const Call& call, std::string* error = nullptr);
    
    void IniCallDecode(CString str, Call& call);
    std::vector<std::unique_ptr<Call>> IniCopyCalls();

};
