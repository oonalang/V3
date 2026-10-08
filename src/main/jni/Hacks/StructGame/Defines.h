#pragma once

#define m_UNITYLIB "libunity.so"
#define m_ANOGSLIB "libanogs.so"

#define SLEEP_TIME 1000LL / 60LL
uintptr_t m_unity;

//-- Hit groups written into DamageInfo::m_HitGroup.
// Values come from the game's own EHitGroup enum (dump.cs, TypeDefIndex 31063).
typedef int EHitGroup;
#define EHitGroup_Default (0)
#define EHitGroup_Head (1)
#define EHitGroup_Hand (2)
#define EHitGroup_Body (3)
#define EHitGroup_Foot (4)
#define EHitGroup_WeakPoint (5)
#define EHitGroup_Shield (6)
#define EHitGroup_Left_Hand (7)
#define EHitGroup_Right_Hand (8)
#define EHitGroup_Left_Foot (9)
#define EHitGroup_Right_Foot (10)
#define EHitGroup_Neck (11)
#define EHitGroup_UpBody (12)
#define EHitGroup_UpArm (13)
// Compatibility aliases: older builds referred to the generic/body region as "Field".
#define EHitGroup_Field EHitGroup_Body

uintptr_t libs;
uintptr_t Class_Pawn_get_AimRotation;
uintptr_t Class_Component_get_gameObject;
uintptr_t Class_Transform_get_position;
uintptr_t Class_Transform_set_position;
uintptr_t Class_Transform_set_localPosition;
uintptr_t Class_Transform_get_eulerAngles;
uintptr_t Class_Gameplay_get_MatchGame;
uintptr_t Class_Gameplay_get_LocalPawn;
uintptr_t Class_Pawn_get_IsFiring;
uintptr_t Class_Pawn_IsAiming;
uintptr_t Class_Pawn_get_TriggerQuickKnife;
uintptr_t Class_Pawn_set_AimRotation;
uintptr_t Class_Pawn_get_MapShowRotation;
uintptr_t Class_MatchGame_get_DroppedPickUpMgr;

uintptr_t Class_PlayerInfo_m_VestValue;
uintptr_t Class_PlayerInfo_m_VestMaxValue;
uintptr_t Class_Pawn_get_LastPawnPos;
uintptr_t Class_Pawn_get_HeadPosition;
uintptr_t Class_Pawn_get_PlayerName;
uintptr_t Class_Pawn_get_TeamSeatId;
uintptr_t PlayerVisibleC;

//=========={ ImGui Rqd }============
uintptr_t Class_Input_GetTouch;
uintptr_t Class_Input_get_touchSupported;
uintptr_t Class_Input_get_touchCount;
uintptr_t Class_Input_get_mousePosition;
uintptr_t Screen_get_width;
uintptr_t Screen_get_height;
uintptr_t Screen_get_dpi;

//=========={ FIELDS }================
uintptr_t Class_Pawn_m_PlayerInfo;
uintptr_t Class_PlayerInfo_m_IsPlayerDead;
uintptr_t Class_PlayerInfo_m_NickName;
uintptr_t Class_AttackableTarget_m_AttackableInfo;
uintptr_t Class_AttackableTarget_m_Health;
uintptr_t Class_AttackableTarget_m_MaxHealth;
//uintptr_t Class_Pawn_CheckVisible;
uintptr_t Class_AttackableTarget_m_ExtraHealth;
uintptr_t Class_AttackableTarget_m_MaxExtraHealth;
uintptr_t Class_Pawn_m_IsBot;
uintptr_t Class_Pawn_m_IsAlive;
uintptr_t Class_Pawn_IsFadeOut;
uintptr_t Class_Pawn_m_Mesh;
uintptr_t Class_Pawn_m_HeadBone;
uintptr_t Class_BaseGame_EnemyPawns;
uintptr_t Class_Pawn_bDiedRespawn;
uintptr_t Class_Pawn_m_LastFireWeaponName;
uintptr_t Class_PhysicsVehicle_vehicleType;
uintptr_t Class_PhysicsVehicle_get_Health;
uintptr_t Class_BaseGame_m_AllVehicles;
uintptr_t Class_Transform_set_localScale;

uintptr_t Class_PlayerInfo_m_ProfessionChipID;

uintptr_t Class_Physics_Raycast;

//-- Esp Items
// uintptr_t Class_MatchGame_get_DroppedPickUpMgr;
uintptr_t Class_LethalAndTacticalConfConfig_MarkOfName;
uintptr_t Class_BRDroppedPickUpNormal_mThrowConf;
uintptr_t Class_BRDroppedPickUpMgr_mNormalItemEntityDic;
uintptr_t Class_BRDroppedPickUpNormal_ItemConfigType;
uintptr_t Class_BRDroppedPickUpNormal_mWeaponConf;
uintptr_t Class_WeaponConfConfig_LOCID_Name;
uintptr_t Class_LethalAndTacticalConfConfig_LOCID_Name;

uintptr_t api1 = 0x5057E08;
uintptr_t api2 = 0x5057E18;
uintptr_t api3 = 0x50577C0;
uintptr_t api4 = 0x50504BC;
uintptr_t api5 = 0x5050520;
uintptr_t api6 = 0x5087930;
uintptr_t api7 = 0x5024010;
uintptr_t api8 = 0x5023AB8; 
uintptr_t api9 = 0x50291B0;
uintptr_t api10 = 0x507C19C;
uintptr_t api11 = 0x8569B10;
uintptr_t api12 = 0x856AA08;
uintptr_t api13 = 0x51A8A38;
uintptr_t api14 = 0x51B42E4;
uintptr_t api15 = 0x519BB10;
uintptr_t api16 = 0x4BF6E00;
uintptr_t api17 = 0x51B8D30;
uintptr_t api18 = 0x51B86EC;
uintptr_t api19 = 0x51B8BDC;
uintptr_t api20 = 0x51B89F4;
uintptr_t api21 = 0x5057F00;
uintptr_t api22 = 0x518BEA8;
uintptr_t api23 = 0x51199AC;

//-- Room Info
uintptr_t api24 = 0x5F1;
uintptr_t api25 = 0x530;
uintptr_t api26 = 0x19C;
uintptr_t api27 = 0x198;
uintptr_t api28 = 0x50C;
uintptr_t api29 = 0x2D8;
uintptr_t api30 = 0x2F0;
uintptr_t api31 = 0x300;

void UpdateAllOffset(){
#define Class_Input_get_touchCount (m_unity + api1)
#define Class_Input_GetTouch (m_unity + api2)
#define Class_Input_get_mousePosition (m_unity + api3)
#define Class_Screen_get_width (m_unity + api4)
#define Class_Screen_get_height (m_unity + api5)
#define Class_Screen_get_dpi (m_unity + api6)
#define Class_Camera_get_main (m_unity + api7)
#define Class_Camera_WorldToScreenPoint (m_unity + api8)
#define Class_Component_get_transform (m_unity + api9)
#define Class_Transform_get_position (m_unity + api10)
#define Class_Gameplay_get_MatchGame (m_unity + api11)
#define Class_Gameplay_get_LocalPawn (m_unity + api12)
#define Class_Pawn_get_IsFiring (m_unity + api13)
#define Class_Pawn_set_AimRotation (m_unity + api14)
#define Class_Pawn_IsAiming (m_unity + api15)
#define Class_Pawn_get_LocalOrViewTargetPlayerInfo (m_unity + api16)
#define Class_Pawn_get_LastPawnPos (m_unity + api17)
#define Class_Pawn_get_HeadPosition (m_unity + api18)
#define Class_Pawn_get_PlayerName (m_unity + api19)
#define Class_Pawn_get_TeamSeatId (m_unity + api20)
#define Class_Input_get_touchSupported (m_unity + api21)

#define Class_Pawn_m_PlayerInfo (0x5C0)
#define Class_Pawn_m_IsBot (0x5B9)
#define Class_Pawn_m_IsAlive (0x548)
#define Class_Pawn_IsFadeOut (0x5B8)
#define Class_Pawn_m_Mesh (0x628)
#define Class_Pawn_m_HeadBone (0x308)
#define Class_PlayerInfo_m_NickName (0x160)
#define Class_AttackableTarget_m_AttackableInfo (0x78)
#define Class_AttackableTarget_m_Health (0x34)
#define Class_AttackableTarget_m_MaxHealth (0x38)

#define Class_DamageInfo_m_Damage (0x14)
#define Class_DamageInfo_m_HitGroup (0x54)


#define Class_Pawn_m_SpineBone (0x1D68)
#define Class_Pawn_m_NeckBone (0x1D70)
#define Class_Pawn_m_HipsBone (0x1D78)
#define Class_Pawn_m_LeftAnkleBone (0x1D80)
#define Class_Pawn_m_RightAnkleBone (0x1D88)

#define Class_MatchGame_get_DroppedPickUpMgr (m_unity + 0x5914C74)
#define Pawn_get_CurrentWeapon (m_unity + api22)
#define Weapon_GetWeaponId (m_unity + api23)

#define Class_Transform_set_localScale (m_unity + 0x509C8F4)
#define Class_PhysicsVehicle__vehicleType (0x164)
#define Class_BaseGame__m_AllVehicles (0x1A0)
#define Class_PhysicsVehicle_get_Health (m_unity + 0xA1AA7B8)

#define Class_Transform_set_position (m_unity + 0x50206AC)
#define Class_Transform_set_localPosition (m_unity + 0x507AD40)
#define Class_Time_set_timeScale (m_unity + 0x509B320)
#define Class_Time_get_timeScale (m_unity + 0x5020738)

#define Class_Weapon_m_FireComponents (0x00)
#define Class_FireComponentArray_m_CurrentComponent (0x00)
#define Class_WeaponFireComponent_m_ClipAmmoCount (0xB8)
#define Class_WeaponFireComponent_m_CarriedAmmoCount (0xAC)
#define Class_WeaponFireComponent_m_BaseClipAmmoCount (0x00)

#define Class_BaseGame_EnemyPawns (0x178)

#define Class_BaseGame_m_World (0x150)
#define Class_BaseWorld_m_ActorList (0x40)

#define Class_PlayerInfo_m_ProfessionChipID 0x1E8
#define Class_PlayerInfo_m_PreProfessionChipID 0xC4
#define Class_PlayerInfo_ChipSkinID 0x3F0
#define Class_BRProfessionChipConfig_ItemID 0x24

#define Class_PlayerInfo_m_VestValue (0x5F8)
#define Class_PlayerInfo_m_VestMaxValue (0x628)

#define Class_Physics_Raycast (m_unity + 0x506BEA8)
//-- ESP Items
// #define Class_MatchGame_get_DroppedPickUpMgr (m_unity + 0x59B4C80)
// #define Weapon_GetWeaponId (m_unity + 0x51199AC)
#define Class_BRDroppedPickUpNormal_ItemConfigType (0xB0)
#define Class_BRDroppedPickUpNormal_mThrowConf (0xE8)
#define Class_BRDroppedPickUpNormal_mWeaponConf (0xE0)
#define Class_BRDroppedPickUpNormal_mDroppedPickUpConfig (0xD8)
#define Class_DroppedPickUpConfig_configID (0x68)
#define Class_BRDroppedPickUpMgr_mNormalItemEntityDic (0x118)
#define Class_WeaponConfConfig_LOCID_Name (0x68)
#define Class_LethalAndTacticalConfConfig_LOCID_Name (0x68)
#define Class_LethalAndTacticalConfConfig_MarkOfName (0x50)

}

bool get_touchSupported(void *instance) {
    return reinterpret_cast<bool(__fastcall *)(void *)>(Class_Input_get_touchSupported)(instance);
}

int get_touchCount() {
    return reinterpret_cast<int(__fastcall *)()>(Class_Input_get_touchCount)();
}

Vector3 get_mousePosition(void *instance) {
    return reinterpret_cast<Vector3(__fastcall *)(void *)>(Class_Input_get_mousePosition)(instance);
}

int get_width() {
    return reinterpret_cast<int(__fastcall *)()>(Class_Screen_get_width)();
}

int get_height() {
    return reinterpret_cast<int(__fastcall *)()>(Class_Screen_get_height)();
}

int get_dpi() {
    return reinterpret_cast<int(__fastcall *)()>(Class_Screen_get_dpi)();
}