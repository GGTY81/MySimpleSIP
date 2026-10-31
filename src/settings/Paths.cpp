/*
 * MySimpleSIP — caminhos de configuração e dados.
 * Copyright (C) 2011-2026 MicroSIP (http://www.microsip.org)
 * Copyright (C) 2026 MySimpleSIP
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include "Paths.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <system_error>

namespace fs = std::filesystem;

namespace {

fs::path toPath(const CString& s)
{
    return fs::path(CStringA(s).GetString());
}

CString fromPath(const fs::path& p)
{
    return CString(p.string().c_str());
}

CString withSlash(const fs::path& p)
{
    CString s = fromPath(p);
    if (s.IsEmpty() || s[s.GetLength() - 1] != L'/')
        s.AppendChar(L'/');
    return s;
}

// Diretório-base XDG: variável de ambiente absoluta ou o padrão relativo ao HOME.
fs::path xdgDir(const char* var, const char* fallback)
{
    const char* v = std::getenv(var);
    if (v && *v == '/')
        return fs::path(v);
    const char* home = std::getenv("HOME");
    return fs::path(home && *home ? home : "/tmp") / fallback;
}

} // namespace

CString XdgUserDir(const CString& name)
{
    const char* homeEnv = std::getenv("HOME");
    const std::string home = homeEnv && *homeEnv ? homeEnv : "/tmp";
    const std::string key = "XDG_" + std::string(CStringA(name).GetString()) + "_DIR=";
    std::ifstream in(xdgDir("XDG_CONFIG_HOME", ".config") / "user-dirs.dirs");
    std::string line;
    while (std::getline(in, line)) {
        if (line.rfind(key, 0) != 0)
            continue;
        std::string value = line.substr(key.size());
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"')
            value = value.substr(1, value.size() - 2);
        if (value.rfind("$HOME", 0) == 0)
            value = home + value.substr(5);
        if (!value.empty() && value.back() == '/')
            value.pop_back();
        return CString(value.c_str());
    }
    std::error_code ec;
    if (name == _T("DESKTOP") && fs::is_directory(fs::path(home) / "Desktop", ec))
        return CString((home + "/Desktop").c_str());
    return CString(home.c_str());
}

AppPaths ResolveAppPaths(const CString& appName, const CString& exePath)
{
    AppPaths p;
    std::error_code ec;

    fs::path exe = exePath.IsEmpty() ? fs::read_symlink("/proc/self/exe", ec) : toPath(exePath);
    p.exeFile = fromPath(exe);
    p.pathExe = fromPath(exe.parent_path());

    const CString iniName = appName + _T(".ini");
    const CString logName = appName + _T("_log.txt");

    const fs::path portableIni = exe.parent_path() / toPath(iniName);
    if (!exe.empty() && fs::exists(portableIni, ec)) {
        p.isPortable = true;
        p.pathRoaming = withSlash(exe.parent_path());
        p.pathLocal = p.pathRoaming;
        p.iniFile = fromPath(portableIni);
    } else {
        const fs::path name = toPath(appName);
        const fs::path config = xdgDir("XDG_CONFIG_HOME", ".config") / name;
        const fs::path data = xdgDir("XDG_DATA_HOME", ".local/share") / name;
        const fs::path state = xdgDir("XDG_STATE_HOME", ".local/state") / name;
        fs::create_directories(config, ec);
        fs::create_directories(data, ec);
        fs::create_directories(state, ec);
        p.pathRoaming = withSlash(data);
        p.pathLocal = withSlash(state);
        p.iniFile = fromPath(config / toPath(iniName));
    }
    p.logFile = p.pathLocal + logName;

    // Como no MicroSIP: INI ausente ou vazio → primeira execução, cria com "[Global]"
    const fs::path ini = toPath(p.iniFile);
    if (!fs::exists(ini, ec) || fs::file_size(ini, ec) == 0) {
        p.firstRun = true;
        std::ofstream out(ini, std::ios::binary | std::ios::trunc);
        out << "[Global]\n";
        if (!out) {
            CStringA msg;
            msg.Format("Failed to open file for writing %s", CStringA(p.iniFile).GetString());
            throw std::runtime_error(msg.GetString());
        }
    }
    return p;
}
