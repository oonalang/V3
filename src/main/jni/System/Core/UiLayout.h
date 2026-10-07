#ifndef UILAYOUT_H
#define UILAYOUT_H
#pragma once

// Persists the free-floating overlay positions (radial category wheel and the
// menu container) so the layout the user drags into place survives a restart.
// Stored next to codmconfig.ini as a tiny "key x y" ini file. Nothing here
// touches game state.

#include <fstream>
#include <sstream>
#include <string>

#include "FileUtils.h"

extern JavaVM* VM;

namespace ui_layout
{
    struct State
    {
        bool  hasWheel = false;
        float wheelX   = 0.0f;
        float wheelY   = 0.0f;
        bool  hasMenu  = false;
        float menuX    = 0.0f;
        float menuY    = 0.0f;
    };

    inline State &Get()
    {
        static State state;
        return state;
    }

    inline std::string LayoutFilePath()
    {
        if (VM == nullptr)
        {
            return std::string();
        }

        const std::string filesDir = GetFilesDirPath(VM);
        if (filesDir.empty())
        {
            return std::string();
        }

        return filesDir + "/ui_layout.ini";
    }

    inline void LoadLayout()
    {
        const std::string path = LayoutFilePath();
        if (path.empty())
        {
            return;
        }

        std::ifstream file(path.c_str());
        if (!file.is_open())
        {
            return;
        }

        State &state = Get();
        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream stream(line);
            std::string key;
            float x = 0.0f;
            float y = 0.0f;
            if (!(stream >> key >> x >> y))
            {
                continue;
            }

            if (key == "wheel")
            {
                state.wheelX   = x;
                state.wheelY   = y;
                state.hasWheel = true;
            }
            else if (key == "menu")
            {
                state.menuX   = x;
                state.menuY   = y;
                state.hasMenu = true;
            }
        }

        file.close();
    }

    inline void SaveLayout()
    {
        const std::string path = LayoutFilePath();
        if (path.empty())
        {
            return;
        }

        const State &state = Get();
        std::ofstream file(path.c_str(), std::ios::out | std::ios::trunc);
        if (!file.is_open())
        {
            return;
        }

        if (state.hasWheel)
        {
            file << "wheel " << state.wheelX << ' ' << state.wheelY << '\n';
        }
        if (state.hasMenu)
        {
            file << "menu " << state.menuX << ' ' << state.menuY << '\n';
        }

        file.close();
    }

    inline void RememberWheel(float x, float y)
    {
        State &state    = Get();
        state.hasWheel  = true;
        state.wheelX    = x;
        state.wheelY    = y;
        SaveLayout();
    }

    inline void RememberMenu(float x, float y)
    {
        State &state  = Get();
        state.hasMenu = true;
        state.menuX   = x;
        state.menuY   = y;
        SaveLayout();
    }
}

#endif
