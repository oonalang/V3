#pragma once

#include <iomanip>
#include <sstream>
#include <ctime>

#include "../foxcheats/include/json.hpp"
#include "../../Hacks/Feature/Init_Hacks.h"

// Externals defined in Main.cpp
extern float menu[4];
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;

std::string androidFilesDir = "/storage/emulated/0/Android/data/com.garena.game.codm/files/";

void SaveConfiguration(const std::string& filename) {
    nlohmann::json config;

    config["ESPMenu"]["Box"] = Config.ESPMenu.Box;
    config["ESPMenu"]["Name"] = Config.ESPMenu.Name;
    config["ESPMenu"]["Health"] = Config.ESPMenu.Health;
    config["ESPMenu"]["Distance"] = Config.ESPMenu.Distance;
    config["ESPMenu"]["Target"] = static_cast<int>(Config.ESPMenu.Target);
    config["ESPMenu"]["BoxType"] = static_cast<int>(Config.ESPMenu.BoxType);
    config["ESPMenu"]["HealthPosition"] = static_cast<int>(Config.ESPMenu.HealthPosition);
    config["ESPMenu"]["CrosshairType"] = static_cast<int>(Config.ESPMenu.CrosshairType);
    config["ESPMenu"]["EspStyle"] = static_cast<int>(Config.ESPMenu.EspStyle);
    
    config["Aim"]["Aimbot360"] = Config.Aim.Aimbot360;
    config["Aim"]["AimSilent"] = Config.Aim.AimSilent;
    config["Aim"]["AimAssistSize"] = Config.Aim.AimAssistSize;
    config["Aim"]["Target"] = static_cast<int>(Config.Aim.Target);
    config["Aim"]["Trigger"] = static_cast<int>(Config.Aim.Trigger);
    config["Aim"]["By"] = static_cast<int>(Config.Aim.By);
    config["Aim"]["size"] = Config.Aim.size;

    config["ExtraMenu"]["Flash"] = Config.ExtraMenu.Flash;
    config["ExtraMenu"]["Diving"] = Config.ExtraMenu.Diving;
    config["ExtraMenu"]["Fire"] = Config.ExtraMenu.Fire;
    config["ExtraMenu"]["Hit"] = Config.ExtraMenu.Hit;
    config["ExtraMenu"]["HitboxScale"] = Config.ExtraMenu.HitboxScale;
    config["ExtraMenu"]["NoCrouch"] = Config.ExtraMenu.NoCrouch;
    config["ExtraMenu"]["Rpd"] = Config.ExtraMenu.Rpd;
    config["ExtraMenu"]["Parachute"] = Config.ExtraMenu.Parachute;
    config["ExtraMenu"]["Recoil"] = Config.ExtraMenu.Recoil;
    config["ExtraMenu"]["Shake"] = Config.ExtraMenu.Shake;
    config["ExtraMenu"]["Spread"] = Config.ExtraMenu.Spread;
    config["ExtraMenu"]["Reload"] = Config.ExtraMenu.Reload;
    config["ExtraMenu"]["Scope"] = Config.ExtraMenu.Scope;
    config["ExtraMenu"]["Switch"] = Config.ExtraMenu.Switch;
    config["ExtraMenu"]["KineticArmor"] = Config.ExtraMenu.Kinetic;

    config["isJumpAdjustmentEnabled"] = isJumpAdjustmentEnabled;
    config["jumpHeightMultiplier"] = jumpHeightMultiplier;
    config["SnowB"] = SnowB;
    config["SnowBsize"] = SnowBsize;
    config["isSpeedHackEnabled"] = isSpeedHackEnabled;
    config["speedHackMultiplier"] = speedHackMultiplier;

    config["menu"][0] = menu[0];
    config["menu"][1] = menu[1];
    config["menu"][2] = menu[2];

    std::string configDir = androidFilesDir + "configs";
    mkdir(configDir.c_str(), 0777);

    std::string filePath = configDir + "/" + filename + ".json";
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << std::setw(4) << config;
        file.close();
    }
}

bool LoadConfiguration(const std::string& filename) {
    try {
        std::string filePath = androidFilesDir + "configs/" + filename + ".json";
        std::ifstream file(filePath);
        if (!file.is_open()) {
            return false;
        }

        nlohmann::json config;
        file >> config;

        if (config.contains("ESPMenu")) {
            Config.ESPMenu.Box = config["ESPMenu"]["Box"].get<bool>();
            Config.ESPMenu.Name = config["ESPMenu"]["Name"].get<bool>();
            Config.ESPMenu.Health = config["ESPMenu"]["Health"].get<bool>();
            Config.ESPMenu.Distance = config["ESPMenu"]["Distance"].get<bool>();
            if (config["ESPMenu"].contains("Target")) Config.ESPMenu.Target = static_cast<LineTarget>(config["ESPMenu"]["Target"].get<int>());
            if (config["ESPMenu"].contains("BoxType")) Config.ESPMenu.BoxType = static_cast<EspBoxType>(config["ESPMenu"]["BoxType"].get<int>());
            if (config["ESPMenu"].contains("HealthPosition")) Config.ESPMenu.HealthPosition = static_cast<EspHealthPosition>(config["ESPMenu"]["HealthPosition"].get<int>());
            if (config["ESPMenu"].contains("CrosshairType")) Config.ESPMenu.CrosshairType = static_cast<CrosshairTarget>(config["ESPMenu"]["CrosshairType"].get<int>());
            if (config["ESPMenu"].contains("EspStyle")) Config.ESPMenu.EspStyle = static_cast<EspStyleTarget>(config["ESPMenu"]["EspStyle"].get<int>());
        }

        if (config.contains("Aim")) {
            Config.Aim.Aimbot360 = config["Aim"]["Aimbot360"].get<bool>();
            Config.Aim.AimSilent = config["Aim"]["AimSilent"].get<bool>();
            Config.Aim.AimAssistSize = config["Aim"]["AimAssistSize"].get<float>();
            Config.Aim.Target = static_cast<EAimTarget>(config["Aim"]["Target"].get<int>());
            Config.Aim.Trigger = static_cast<EAimTrigger>(config["Aim"]["Trigger"].get<int>());
            Config.Aim.By = static_cast<EAim>(config["Aim"]["By"].get<int>());
            Config.Aim.size = config["Aim"]["size"].get<float>();
        }

        if (config.contains("ExtraMenu")) {
            Config.ExtraMenu.Flash = config["ExtraMenu"]["Flash"].get<bool>();
            Config.ExtraMenu.Diving = config["ExtraMenu"]["Diving"].get<bool>();
            Config.ExtraMenu.Fire = config["ExtraMenu"]["Fire"].get<bool>();
            Config.ExtraMenu.Hit = config["ExtraMenu"]["Hit"].get<bool>();
            if (config["ExtraMenu"].contains("HitboxScale")) Config.ExtraMenu.HitboxScale = config["ExtraMenu"]["HitboxScale"].get<float>();
            if (config["ExtraMenu"].contains("NoCrouch")) Config.ExtraMenu.NoCrouch = config["ExtraMenu"]["NoCrouch"].get<bool>();
            Config.ExtraMenu.Rpd = config["ExtraMenu"]["Rpd"].get<bool>();
            Config.ExtraMenu.Parachute = config["ExtraMenu"]["Parachute"].get<bool>();
            Config.ExtraMenu.Recoil = config["ExtraMenu"]["Recoil"].get<bool>();
            Config.ExtraMenu.Shake = config["ExtraMenu"]["Shake"].get<bool>();
            Config.ExtraMenu.Spread = config["ExtraMenu"]["Spread"].get<bool>();
            Config.ExtraMenu.Reload = config["ExtraMenu"]["Reload"].get<bool>();
            Config.ExtraMenu.Scope = config["ExtraMenu"]["Scope"].get<bool>();
            Config.ExtraMenu.Switch = config["ExtraMenu"]["Switch"].get<bool>();
            Config.ExtraMenu.Kinetic = config["ExtraMenu"]["Kinetic"].get<bool>();
        }

        if (config.contains("isJumpAdjustmentEnabled"))
            isJumpAdjustmentEnabled = config["isJumpAdjustmentEnabled"].get<bool>();
        if (config.contains("jumpHeightMultiplier"))
            jumpHeightMultiplier = config["jumpHeightMultiplier"].get<float>();
        if (config.contains("SnowB"))
            SnowB = config["SnowB"].get<bool>();
        if (config.contains("SnowBsize"))
            SnowBsize = config["SnowBsize"].get<float>();
        if (config.contains("isSpeedHackEnabled"))
            isSpeedHackEnabled = config["isSpeedHackEnabled"].get<bool>();
        if (config.contains("speedHackMultiplier"))
            speedHackMultiplier = config["speedHackMultiplier"].get<float>();

        if (config.contains("menu")) {
            menu[0] = config["menu"][0].get<float>();
            menu[1] = config["menu"][1].get<float>();
            menu[2] = config["menu"][2].get<float>();
        }

        return true;
    }
    catch (const std::exception& e) {
        return false;
    }
}
