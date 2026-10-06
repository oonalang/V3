#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <string>
#include <fstream>
#include <sstream>
#include <jni.h>
#include "FileUtils.h"
#include "Hacks/Hacks.h"

extern JavaVM* VM;

void LoadConfig() {
    std::string textfile = GetFilesDirPath(VM) + "/codmconfig.ini";
    std::ifstream file(textfile.c_str());

    if (!file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        size_t lastSpace = line.find_last_of(' ');
        if (lastSpace == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, lastSpace);
        std::string valueStr = line.substr(lastSpace + 1);

        if (key == "ESPMenu.isPlayerLine") Config.ESPMenu.isPlayerLine = (valueStr == "1");
        else if (key == "ESPMenu.Box") Config.ESPMenu.Box = (valueStr == "1");
        else if (key == "ESPMenu.Health") Config.ESPMenu.Health = (valueStr == "1");
        else if (key == "ESPMenu.Name") Config.ESPMenu.Name = (valueStr == "1");
        else if (key == "ESPMenu.Distance") Config.ESPMenu.Distance = (valueStr == "1");
        else if (key == "ESPMenu.Count") Config.ESPMenu.Count = (valueStr == "1");
        else if (key == "ESPMenu.Crosshair") Config.ESPMenu.Crosshair = (valueStr == "1");
        else if (key == "ESPMenu.Aimline") Config.ESPMenu.Aimline = (valueStr == "1");
        else if (key == "ESPMenu.Target") Config.ESPMenu.Target = static_cast<LineTarget>(std::stoi(valueStr));
        else if (key == "ESPMenu.BoxType") Config.ESPMenu.BoxType = static_cast<EspBoxType>(std::stoi(valueStr));
        else if (key == "ESPMenu.HealthPosition") Config.ESPMenu.HealthPosition = static_cast<EspHealthPosition>(std::stoi(valueStr));
        else if (key == "ESPMenu.CrosshairType") Config.ESPMenu.CrosshairType = static_cast<CrosshairTarget>(std::stoi(valueStr));
        else if (key == "ESPMenu.EspStyle") Config.ESPMenu.EspStyle = static_cast<EspStyleTarget>(std::stoi(valueStr));
        else if (key == "Aim.Size") Config.Aim.size = std::stof(valueStr);
        else if (key == "Aim.AimAssistSize") Config.Aim.AimAssistSize = std::stof(valueStr);
        else if (key == "Aim.Aimbot360") Config.Aim.Aimbot360 = (valueStr == "1");
        else if (key == "Aim.AimSilent") Config.Aim.AimSilent = (valueStr == "1");
        else if (key == "Aim.Target") Config.Aim.Target = static_cast<EAimTarget>(std::stoi(valueStr));
        else if (key == "Aim.Trigger") Config.Aim.Trigger = static_cast<EAimTrigger>(std::stoi(valueStr));
        else if (key == "Aim.By") Config.Aim.By = static_cast<EAim>(std::stoi(valueStr));
        else if (key == "ExtraMenu.Kinetic") Config.ExtraMenu.Kinetic = (valueStr == "1");
        else if (key == "ExtraMenu.Recoil") Config.ExtraMenu.Recoil = (valueStr == "1");
        else if (key == "ExtraMenu.Spread") Config.ExtraMenu.Spread = (valueStr == "1");
        else if (key == "ExtraMenu.Reload") Config.ExtraMenu.Reload = (valueStr == "1");
        else if (key == "ExtraMenu.Scope") Config.ExtraMenu.Scope = (valueStr == "1");
        else if (key == "ExtraMenu.Switch") Config.ExtraMenu.Switch = (valueStr == "1");
        else if (key == "ExtraMenu.Shake") Config.ExtraMenu.Shake = (valueStr == "1");
        else if (key == "ExtraMenu.Flash") Config.ExtraMenu.Flash = (valueStr == "1");
        else if (key == "ExtraMenu.Rpd") Config.ExtraMenu.Rpd = (valueStr == "1");
        else if (key == "ExtraMenu.Hit") Config.ExtraMenu.Hit = (valueStr == "1");
        else if (key == "ExtraMenu.HitboxScale") Config.ExtraMenu.HitboxScale = std::stof(valueStr);
        else if (key == "ExtraMenu.NoCrouch") Config.ExtraMenu.NoCrouch = (valueStr == "1");
        else if (key == "ExtraMenu.Fire") Config.ExtraMenu.Fire = (valueStr == "1");
        else if (key == "ExtraMenu.Parachute") Config.ExtraMenu.Parachute = (valueStr == "1");
        else if (key == "ExtraMenu.Diving") Config.ExtraMenu.Diving = (valueStr == "1");
        else if (key == "ExtraMenu.WallHack") Config.ExtraMenu.WallHack = (valueStr == "1");
    }
    file.close();
}

void SaveConfig() {
    std::string textfile = GetFilesDirPath(VM) + "/codmconfig.ini";
    std::ofstream file(textfile.c_str());

    if (!file.is_open()) {
        return;
    }

    file << "ESPMenu.isPlayerLine " << Config.ESPMenu.isPlayerLine << "\n";
    file << "ESPMenu.Box " << Config.ESPMenu.Box << "\n";
    file << "ESPMenu.Health " << Config.ESPMenu.Health << "\n";
    file << "ESPMenu.Name " << Config.ESPMenu.Name << "\n";
    file << "ESPMenu.Distance " << Config.ESPMenu.Distance << "\n";
    file << "ESPMenu.Count " << Config.ESPMenu.Count << "\n";
    file << "ESPMenu.Crosshair " << Config.ESPMenu.Crosshair << "\n";
    file << "ESPMenu.Aimline " << Config.ESPMenu.Aimline << "\n";
    file << "ESPMenu.Target " << Config.ESPMenu.Target << "\n";
    file << "ESPMenu.BoxType " << Config.ESPMenu.BoxType << "\n";
    file << "ESPMenu.HealthPosition " << Config.ESPMenu.HealthPosition << "\n";
    file << "ESPMenu.CrosshairType " << Config.ESPMenu.CrosshairType << "\n";
    file << "ESPMenu.EspStyle " << Config.ESPMenu.EspStyle << "\n";
    file << "Aim.size " << Config.Aim.size << "\n";
    file << "Aim.AimAssistSize " << Config.Aim.AimAssistSize << "\n";
    file << "Aim.Aimbot360 " << Config.Aim.Aimbot360 << "\n";
    file << "Aim.AimSilent " << Config.Aim.AimSilent << "\n";
    file << "Aim.Target " << Config.Aim.Target << "\n";
    file << "Aim.Trigger " << Config.Aim.Trigger << "\n";
    file << "Aim.By " << Config.Aim.By << "\n";
    file << "ExtraMenu.Kinetic " << Config.ExtraMenu.Kinetic << "\n";
    file << "ExtraMenu.Recoil " << Config.ExtraMenu.Recoil << "\n";
    file << "ExtraMenu.Spread " << Config.ExtraMenu.Spread << "\n";
    file << "ExtraMenu.Reload " << Config.ExtraMenu.Reload << "\n";
    file << "ExtraMenu.Scope " << Config.ExtraMenu.Scope << "\n";
    file << "ExtraMenu.Switch " << Config.ExtraMenu.Switch << "\n";
    file << "ExtraMenu.Shake " << Config.ExtraMenu.Shake << "\n";
    file << "ExtraMenu.Flash " << Config.ExtraMenu.Flash << "\n";
    file << "ExtraMenu.Rpd " << Config.ExtraMenu.Rpd << "\n";
    file << "ExtraMenu.Hit " << Config.ExtraMenu.Hit << "\n";
    file << "ExtraMenu.HitboxScale " << Config.ExtraMenu.HitboxScale << "\n";
    file << "ExtraMenu.NoCrouch " << Config.ExtraMenu.NoCrouch << "\n";
    file << "ExtraMenu.Fire " << Config.ExtraMenu.Fire << "\n";
    file << "ExtraMenu.Parachute " << Config.ExtraMenu.Parachute << "\n";
    file << "ExtraMenu.Diving " << Config.ExtraMenu.Diving << "\n";
    file << "ExtraMenu.WallHack " << Config.ExtraMenu.WallHack << "\n";
    
    file.close();
}

#endif
