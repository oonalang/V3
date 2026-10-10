#include <android/log.h>  // stage tracing for the skin loader

#define SKINLOG(...) ((void)__android_log_print(ANDROID_LOG_INFO, "MWD-ASTRAL-SKIN", __VA_ARGS__))
#define SKINERR(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "MWD-ASTRAL-SKIN", __VA_ARGS__))
std::unordered_map<std::string, bool> sBool;
std::unordered_map<int, int> activeKillEffects;
std::unordered_map<int, int> activeBulletTrackEffects;
std::unordered_map<int, int> activeWeaponFireEffects;
std::unordered_map<int, int> activeWeaponBrocast;
std::vector<TargetChar> g_targetCharacters;
int g_selectedTargetCharIndex = 0;

std::vector<void *> itemInventoryInstance;
std::vector<void *> weaponExtraInstance;
std::vector<void *> weaponFireEffectInstance;
std::vector<void *> weaponConfInstance;
std::vector<void *> weaponAssetGroupInstance;
std::vector<void *> mythicArmorInstance;
std::vector<void *> mythicSightInstance;
std::vector<void *> killEffectItemInstance;
std::vector<void *> weaponSkinConfigInstance;
std::vector<void *> weaponCamosConfigInstance;
std::vector<void *> itemResourceConfigInstance;
std::vector<void *> CharacterModelConfigInstance;
std::vector<void *> RoleConfConfigInstance;
std::vector<void *> RoleSkinConfigInstance;
std::vector<void *> RolePackConfConfigInstance;
std::vector<void *> BRDeadboxSkinConfigInstance;
std::vector<void *> BRDropPlaneSkinConfigInstance;
std::vector<void *> vehicleSkinConfigInstance;
std::vector<void *> vehicleItemConfigInstance;
std::unordered_map<int, int> activeVehicleSkins;
std::unordered_map<int, int> activeVehicleSkinsById;
std::unordered_map<int, void*> activeVehicleSkinConfs;

std::vector<itemInfo> itemData;
std::vector<charInfo> charData;
std::vector<watcher> watch;
std::vector<deadbox> deadboxF;
std::vector<planeID> dropplane;
std::vector<snowboardInfo> snowboardData;

RoleSkinFields *roleskinFields;
CharacterModelFields *characterfields;
RolePackFields *packfields;
RoleConfFields *roleFields;
ItemResourceFields *itemFields;
WeaponConfFields *weaponconfFields;
WeaponCamosFields *weaponcamosFields;
WeaponSkinFields *weaponskinFields;
Item2InventoryFields *item2Fields;
WeaponAssetGroupFields *weaponAssetFields;
WeaponFireEffectFields *weaponfireFields;
WeaponConfExtraFields *weaponextraFields;
MythicArmorFields *mythicarmorFields;
MythicSightFields *mythicsightFields;
KillEffectItemFields *killeffectFields;
BRDeadboxSkinFields *deadboxFields;
BRDropPlaneSkinFields *dropplaneFields;

struct VehicleSkinConfFields {
    uint8_t ColorID;
    bool ExternalUnVisible;
    uint8_t GoGetPos;
    bool IsAutoDemolition;
    uint8_t LockedShowOrder;
    bool ShowInBag;
    bool ShowRare;
    uint8_t SkinMusicsID;
    uint8_t SkinStyle;
    char pad_19[3];
    uint CurrencyID;
    uint ID;
    int MusicsID;
    int NewVehicleResId;
    float RoomShowScale;
    int SellPrice;
    int SkinExtraMesh;
    int SkinParam;
    int SkinParam2;
    int SkinParam3;
    int UILowModel;
    int VehicleId;
};

VehicleSkinConfFields* vehicleSkinFields;

uintptr_t Item2InventoryAddress = 0x6971D34;
uintptr_t WeaponConfAddress = 0xAF7E7FC;
uintptr_t WeaponConfExtraAddress = 0xAF80080;
uintptr_t WeaponFireEffectAddress = 0xAF81990;
uintptr_t CharacterModelAddress = 0xB1A9738;
uintptr_t BRDeadboxSkinAddress = 0x6FF3160;
uintptr_t BRDropPlaneSkinAddress = 0xB1981D4;
uintptr_t WeaponAssetGroupAddress = 0xAF7A080;
uintptr_t MythicArmorConfigAddress = 0x5C90E84;
uintptr_t MythicSightConfigAddress = 0x90DDEEC;
uintptr_t KillEffectItemConfConfigAddress = 0x90C54CC;
uintptr_t WeaponSkinAddress = 0xAF8B4D4;
uintptr_t WeaponCamosAddress = 0xAF7E148;
uintptr_t ItemResourceAddress = 0x6985AB8;
uintptr_t RoleConfAddress = 0x90F20BC;
uintptr_t RoleSkinAddress = 0x90F95C8;
uintptr_t RolePackConfAddress = 0x90F784C;

uintptr_t WeaponConfName = 0xAF7FDC8;
uintptr_t GetDropPlaneName = 0xB198B08;
uintptr_t GetNameRoleSkin = 0x90F98F4;
uintptr_t GetRoleConfName = 0x90F4214;
uintptr_t GetDeadBoxIDAddress = 0x90F2CA0;
uintptr_t UnlockedCamo = 0xAF7E580;

uintptr_t VehicleSkinConfAddress = 0xAF74090;
uintptr_t VehicleItemConfAddress = 0xAF73A18;
uintptr_t VehicleSkinHelperChangeVehicleSkinIdAddress = 0x9B84878;
uintptr_t VehicleSkinHelperChangeVehicleSkinConfAddress = 0x9B849EC;
uintptr_t VehicleSkisSetupSkinAddress = 0x9A9F668;
uintptr_t VehicleSkisChangeSkinIdAddress = 0x9A9F7DC;
uintptr_t VehicleSkisChangeSkinConfAddress = 0x9A9FAE0;
uintptr_t VehicleSkisBuildVehicleModelAddress = 0x9AA24A0;
uintptr_t VehicleSkisLoadModelCompleteAddress = 0x9AA26EC;
uintptr_t VehicleSkisApplySkinAddress = 0x9AA2AFC;

typedef void (*Item2InventoryCtor)(void*);
typedef void (*WeaponConfExtraCtor)(void*);
typedef void (*WeaponFireEffectCtor)(void*);
typedef void (*WeaponConfCtor)(void*);
typedef void (*WeaponAssetGroupCtor)(void*);
typedef void (*MythicArmorCtor)(void*);
typedef void (*MythicSightCtor)(void*);
typedef void (*WeaponSkinCtor)(void*);
typedef void (*WeaponCamosCtor)(void*);
typedef void (*KillEffectItemCtor)(void*);
typedef void (*ItemResourceCtor)(void*);
typedef void (*CharacterModelCtor)(void*);
typedef void (*RoleConfCtor)(void*);
typedef void (*RoleSkinCtor)(void*);
typedef void (*RolePackConfCtor)(void*);
typedef void (*BRDeadboxSkinCtor)(void*);
typedef void (*BRDropPlaneSkinCtor)(void*);
typedef void (*VehicleSkinConfCtor)(void*);
typedef void (*VehicleItemConfCtor)(void*);

Item2InventoryCtor orig_Item2InventoryCtor = nullptr;
WeaponConfExtraCtor orig_WeaponConfExtraCtor = nullptr;
WeaponFireEffectCtor orig_WeaponFireEffectCtor = nullptr;
WeaponConfCtor orig_WeaponConfCtor = nullptr;
WeaponAssetGroupCtor orig_WeaponAssetGroupCtor = nullptr;
MythicArmorCtor orig_MythicArmorCtor = nullptr;
MythicSightCtor orig_MythicSightCtor = nullptr;
WeaponSkinCtor orig_WeaponSkinCtor = nullptr;
WeaponCamosCtor orig_WeaponCamosCtor = nullptr;
KillEffectItemCtor orig_KillEffectItemCtor = nullptr;
ItemResourceCtor orig_ItemResourceCtor = nullptr;
CharacterModelCtor orig_CharacterModelCtor = nullptr;
RoleConfCtor orig_RoleConfCtor = nullptr;
RoleSkinCtor orig_RoleSkinCtor = nullptr;
RolePackConfCtor orig_RolePackConfCtor = nullptr;
BRDeadboxSkinCtor orig_BRDeadboxSkinCtor = nullptr;
BRDropPlaneSkinCtor orig_BRDropPlaneSkinCtor = nullptr;
VehicleSkinConfCtor orig_VehicleSkinConfCtor = nullptr;
VehicleItemConfCtor orig_VehicleItemConfCtor = nullptr;

std::mutex g_dataMutex;

// Guards the published UI skin lists (charData / watch / deadboxF / dropplane /
// itemData / snowboardData / g_targetCharacters). The loader publishes into them
// from its own thread while the menu renders from them, so reads and writes have
// to be serialised: otherwise a reload can invalidate whatever the render thread
// is iterating (which showed up as the menu dying while browsing skins).
std::mutex g_skinUiMutex;

// Copies a game-side instance list while holding g_dataMutex, because the ctor
// hooks push into those vectors from the game thread.
inline std::vector<void*> SkinSnapshot(const std::vector<void*>& instances)
{
    std::lock_guard<std::mutex> lock(g_dataMutex);
    return instances;
}

void my_Item2InventoryCtor(void* instance) {
    orig_Item2InventoryCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        itemInventoryInstance.push_back(instance);
    }
}

void my_WeaponConfExtraCtor(void* instance) {
    orig_WeaponConfExtraCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponExtraInstance.push_back(instance);
    }
}

void my_WeaponFireEffectCtor(void* instance) {
    orig_WeaponFireEffectCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponFireEffectInstance.push_back(instance);
    }
}

void my_WeaponConfCtor(void* instance) {
    orig_WeaponConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponConfInstance.push_back(instance);
    }
}

void my_WeaponAssetGroupCtor(void* instance) {
    orig_WeaponAssetGroupCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponAssetGroupInstance.push_back(instance);
    }
}

void my_MythicArmorCtor(void* instance) {
    orig_MythicArmorCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        mythicArmorInstance.push_back(instance);
    }
}

void my_MythicSightCtor(void* instance) {
    orig_MythicSightCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        mythicSightInstance.push_back(instance);
    }
}

void my_WeaponSkinCtor(void* instance) {
    orig_WeaponSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponSkinConfigInstance.push_back(instance);
    }
}

void my_WeaponCamosCtor(void* instance) {
    orig_WeaponCamosCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponCamosConfigInstance.push_back(instance);
    }
}

void my_KillEffectItemCtor(void* instance) {
    orig_KillEffectItemCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        killEffectItemInstance.push_back(instance);
    }
}

void my_ItemResourceCtor(void* instance) {
    orig_ItemResourceCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        itemResourceConfigInstance.push_back(instance);
    }
}

void my_CharacterModelCtor(void* instance) {
    orig_CharacterModelCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        CharacterModelConfigInstance.push_back(instance);
    }
}

void my_RoleConfCtor(void* instance) {
    orig_RoleConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RoleConfConfigInstance.push_back(instance);
    }
}

void my_RoleSkinCtor(void* instance) {
    orig_RoleSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RoleSkinConfigInstance.push_back(instance);
    }
}

void my_RolePackConfCtor(void* instance) {
    orig_RolePackConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        RolePackConfConfigInstance.push_back(instance);
    }
}

void my_BRDeadboxSkinCtor(void* instance) {
    orig_BRDeadboxSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        BRDeadboxSkinConfigInstance.push_back(instance);
    }
}

void my_BRDropPlaneSkinCtor(void* instance) {
    orig_BRDropPlaneSkinCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        BRDropPlaneSkinConfigInstance.push_back(instance);
    }
}

void my_VehicleSkinConfCtor(void* instance) {
    orig_VehicleSkinConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        vehicleSkinConfigInstance.push_back(instance);
    }
}

void my_VehicleItemConfCtor(void* instance) {
    orig_VehicleItemConfCtor(instance);
    if (instance) {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        vehicleItemConfigInstance.push_back(instance);
    }
}

char searchQuery[256] = "";

bool loadskinhack = false;
bool loadCharacter = false;
bool isLoad = false;

std::string lastKnownName = "";
int emptyNameCount = 0;

std::unordered_map<std::string, int> nameCountMap;
std::unordered_map<std::string, int> nameCountChar;
std::unordered_map<std::string, bool> getplane;
std::unordered_map<std::string, bool> getguns;

uintptr_t location = 0;

uintptr_t getRealOffset(uintptr_t offset)
{
    while (location <= 0)
    {
        location = Tools::GetBaseAddress("libunity.so");
        if (location <= 0)
        {
            usleep(1000);
        }
    }
    return location + offset;
}

std::string GetNameString(uintptr_t off, void *getadd)
{
    auto getC = (String * (*)(void *))(getRealOffset(off));
    if (getadd && Tools::IsPtrValid((void *)getC))
    {
        auto getV = getC(getadd);
        if (getV && Tools::IsPtrValid(getV))
        {
            return getV->CString();
        }
    }
    return "";
}

int (*orig_GetCurrentWeaponKillEffect)(Weapon);
int _GetCurrentWeaponKillEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeKillEffects.find(currentID);
    if (it != activeKillEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentBulletTrackEffect)(Weapon);
int _GetCurrentBulletTrackEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeBulletTrackEffects.find(currentID);
    if (it != activeBulletTrackEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentWeaponFireEffect)(Weapon);
int _GetCurrentWeaponFireEffect(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeWeaponFireEffects.find(currentID);
    if (it != activeWeaponFireEffects.end()) {
        return it->second;
    }
    return 0;
}

int (*orig_GetCurrentWeaponBrocast)(Weapon);
int _GetCurrentWeaponBrocast(Weapon weapon) {
    Pawn* localPawn = GamePlay::get_LocalPawn();
    if (!localPawn) return 0;
    Weapon* currentWeapon = localPawn->get_CurrentWeapon();
    if (!currentWeapon) return 0;
    int currentID = currentWeapon->get_WeaponID();
    auto it = activeWeaponBrocast.find(currentID);
    if (it != activeWeaponBrocast.end()) {
        return it->second;
    }
    return 0;
}

#define ReadInt(base, offset) (*(int *)((uintptr_t)(base) + (offset)))
#define ReadBool(base, offset) (*(bool *)((uintptr_t)(base) + (offset)))
#define ReadFloat(base, offset) (*(float *)((uintptr_t)(base) + (offset)))
#define ReadByte(base, offset) (*(uint8_t *)((uintptr_t)(base) + (offset)))
#define READ_PTR(type, base, offset) (*(type **)((uintptr_t)(base) + (offset)))

inline std::string GetRarityPrefix(int colorID) {
    switch (colorID) {
        case 5: return "[M] ";
        case 4: return "[L] ";
        case 3: return "[E] ";
        default: return "[C] ";
    }
}

void LoadCharacterSkins() {

    if (loadCharacter)
        return;

    std::vector<void*> charModels;
    std::vector<void*> itemRes;
    std::vector<void*> roleConfs;
    std::vector<void*> roleSkins;
    std::vector<void*> rolePacks;
    std::vector<void*> deadboxSkins;

    {
        std::lock_guard<std::mutex> lock(g_dataMutex);

        charModels   = CharacterModelConfigInstance;
        itemRes      = itemResourceConfigInstance;
        roleConfs    = RoleConfConfigInstance;
        roleSkins    = RoleSkinConfigInstance;
        rolePacks    = RolePackConfConfigInstance;
        deadboxSkins = BRDeadboxSkinConfigInstance;
    }

    // Each category is loaded as soon as its own source exists: an empty side
    // table must not keep characters / watches / deadboxes hidden.
    static bool watchLoaded   = false;
    static bool deadboxLoaded = false;
    static bool charsLoaded   = false;

    if (watchLoaded && deadboxLoaded && charsLoaded)
        return;

    const bool canLoadWatch   = !roleSkins.empty();
    const bool canLoadDeadbox = !deadboxSkins.empty() && !roleConfs.empty();
    const bool canLoadChars   = !charModels.empty() && !itemRes.empty() && !roleConfs.empty() && !rolePacks.empty();

    if (!canLoadWatch && !canLoadDeadbox && !canLoadChars)
        return;

    if (!watchLoaded && canLoadWatch) {
        std::lock_guard<std::mutex> lock(g_skinUiMutex);
        watch.clear();
    }
    if (!deadboxLoaded && canLoadDeadbox) {
        std::lock_guard<std::mutex> lock(g_skinUiMutex);
        deadboxF.clear();
    }
    if (!charsLoaded && canLoadChars) {
        std::lock_guard<std::mutex> lock(g_skinUiMutex);
        charData.clear();
        g_targetCharacters.clear();
    }

    std::string lastKnownName = "";
    void* lastKnownLocId = nullptr;

    int lastKnownI = 0;
    int lastKnownJ = 0;
    int lastKnownH = 0;
    int lastKnownEntry = 0;
    int lastKnownGest = 0;
    int lastKnownHand = 0;
    int lastKnownKillS = 0;

    if (!watchLoaded && canLoadWatch) {

    for (auto &a : roleSkins) {

        if (!a || !Tools::IsPtrValid(a))
            continue;

        auto *fx = (RoleSkinFields *)((uintptr_t)a + 0x18);

        if (!Tools::IsPtrValid(fx))
            continue;

        if ((fx->FxAssetID_1P ^ 0) != 0) {

            std::string n = GetNameString(GetNameRoleSkin, (void*)a);

            if (!n.empty()) {

                watcher watchEntry{
                    GetRarityPrefix(fx->ColorID) + n,
                    fx->FxAssetID_1P
                };
                std::lock_guard<std::mutex> lock(g_skinUiMutex);
                watch.push_back(watchEntry);
            }
        }
    }

    watchLoaded = true;
    SKINLOG("skin loader: watch list = %d", (int)watch.size());

    }

    std::string _g = "";

    if (!deadboxLoaded && canLoadDeadbox) {

    for (auto &z : deadboxSkins) {

        if (!z || !Tools::IsPtrValid(z))
            continue;

        auto *y = (BRDeadboxSkinFields *)((uintptr_t)z + 0x18);

        if (!Tools::IsPtrValid(y))
            continue;

        bool __b = false;

        auto deadBoxIdFor = (int (*)(void *))(getRealOffset(GetDeadBoxIDAddress));

        if (!Tools::IsPtrValid((void*)deadBoxIdFor))
            continue;

        for (auto &q : roleConfs) {

            if (!q || !Tools::IsPtrValid(q))
                continue;

            std::string s = GetNameString(GetRoleConfName, q);

            std::string boxClueName = s + " Deadbox";

            if ((y->ID & 0xFFFFFFF) != 0) {

                int k = deadBoxIdFor(q);

                if (y->ID == k && !__b) {

                    auto dump = [&](const std::string &nm) {

                        deadbox boxEntry{
                            GetRarityPrefix(y->ColorID) + nm,
                            {
                                y->ColorID,
                                y->DeadBoxEffectAsset,
                                y->Flag,
                                y->FlagAsset,
                                y->ModelAsset3P,
                                y->ModelAssetUI
                            }
                        };
                        std::lock_guard<std::mutex> lock(g_skinUiMutex);
                        deadboxF.push_back(boxEntry);
                    };

                    if (y->ColorID == 5) {

                        _g = boxClueName;

                        dump(boxClueName);

                        __b = true;

                    } else {

                        dump(boxClueName);
                    }
                }

                if (__b)
                    break;

                if (k + 1 == y->ID) {

                    if (y->ColorID == 5 &&
                        y->FlagAsset != 0 &&
                        !_g.empty()) {

                        std::string v = _g + " (Variant)";

                        deadbox boxVariant{
                            GetRarityPrefix(y->ColorID) + v,
                            {
                                y->ColorID,
                                y->DeadBoxEffectAsset,
                                y->Flag,
                                y->FlagAsset,
                                y->ModelAsset3P,
                                y->ModelAssetUI
                            }
                        };
                        std::lock_guard<std::mutex> lock(g_skinUiMutex);
                        deadboxF.push_back(boxVariant);
                    }
                }
            }
        }
    }

    deadboxLoaded = true;
    SKINLOG("skin loader: deadbox list = %d", (int)deadboxF.size());

    }

    if (!charsLoaded && canLoadChars) {

    for (auto X0 : charModels) {

        if (!X0 || !Tools::IsPtrValid(X0))
            continue;

        auto *X1 = (CharacterModelFields *)((uintptr_t)X0 + 0x18);

        if (!Tools::IsPtrValid(X1))
            continue;

        auto A = X1->ItemID;

        auto B = X1->BRBagModel;
        auto C = X1->BRHeadModel;
        auto D = X1->BRLobby;
        auto E = X1->BRModel;
        auto F = X1->BindEffect1P;
        auto G = X1->ChangeClipEffect1P;
        auto H = X1->DefaultModelID;
        auto I = X1->Guarder1P;
        auto J = X1->Guarder3P;
        auto K = X1->GuarderBagModel;
        auto L = X1->GuarderHeadModel;
        auto M = X1->GuarderLobby;
        auto N = X1->Traitor1P;

        for (auto Y0 : itemRes) {

            if (!Y0 || !Tools::IsPtrValid(Y0))
                continue;

            auto *Y1 = (ItemResourceFields *)((uintptr_t)Y0 + 0x18);

            if (!Tools::IsPtrValid(Y1))
                continue;

            if (A != Y1->AvatarModelID)
                continue;

            auto P = Y1->FxAssetID;
            auto Q = Y1->InventoryModelID;
            auto R = Y1->ModelAssetIDRaw;

            auto S = Y1->UIMiniSpriteNameIndex;
            auto T = Y1->UISmallSpriteNameIndex;
            auto U = Y1->UISpriteNameIndex;
            auto V = Y1->UISquareSpriteNameIndex;

            auto W = Y1->ID;

            for (auto Z0 : roleConfs) {

                if (!Z0 || !Tools::IsPtrValid(Z0))
                    continue;

                auto *Z1 = (RoleConfFields *)((uintptr_t)Z0 + 0x18);

                if (!Tools::IsPtrValid(Z1))
                    continue;

                if (Z1->ID != W)
                    continue;

                auto a = Z1->roleLeftArmID;
                auto b = Z1->roleFinalSuitID;
                auto c = Z1->roleBasicHologramID;
                auto d = Z1->ColorID;
                auto e = Z1->ColorSubID;
                auto f = Z1->ShowRare;
                auto g = Z1->RoleLvGroupID;
                auto h = Z1->RolePackID;
                auto t = Z1->LOCID_Name;

                int i = 0;
                int entry = 0;
                int gest = 0;
                int hand = 0;
                int j = 0;
                int killS = 0;

                for (auto RP : rolePacks) {

                    if (!RP || !Tools::IsPtrValid(RP))
                        continue;

                    auto *rpF = (RolePackFields *)((uintptr_t)RP + 0x18);

                    if (!Tools::IsPtrValid(rpF))
                        continue;

                    if (rpF->RolePackID != h)
                        continue;

                    if (rpF->LoadingFrame &&
                        rpF->LobbySceneType == 0) {

                        i = rpF->RolePackID;
                        j = rpF->LoadingFrame;
                        entry = rpF->EntryAnimID;
                        gest = rpF->GestureId;
                        hand = rpF->HandEffectUI;
                        killS = rpF->KillStreakSkinID;
                    }
                }

                std::string n0 = GetNameString(GetRoleConfName, Z0);

                if (!n0.empty() && n0.length() > 0) {

                    lastKnownName = n0;
                    lastKnownLocId = t;

                    lastKnownI = i;
                    lastKnownJ = j;
                    lastKnownH = h;

                    lastKnownEntry = entry;
                    lastKnownGest = gest;
                    lastKnownHand = hand;
                    lastKnownKillS = killS;

                } else {

                    if (!lastKnownName.empty() &&
                        lastKnownLocId != nullptr) {

                        n0 = lastKnownName + " A+";

                        t = lastKnownLocId;

                        i = lastKnownI;
                        j = lastKnownJ;
                        h = lastKnownH;

                        entry = lastKnownEntry;
                        gest = lastKnownGest;
                        hand = lastKnownHand;
                        killS = lastKnownKillS;

                    } else {

                        continue;
                    }
                }

                std::string Zz;

                if (d == 5)
                    Zz = "[M] " + n0;
                else if (d == 4)
                    Zz = "[L] " + n0;
                else if (d == 3)
                    Zz = "[E] " + n0;
                else
                    Zz = "[C] " + n0;

                charInfo charEntry{

                    Zz,

                    {
                        B,
                        C,
                        D,
                        E,
                        F,
                        G,
                        H,
                        I,
                        J,
                        K,
                        L,
                        M
                    },

                    {
                        P,
                        Q,
                        R
                    },

                    {
                        S,
                        T,
                        U,
                        V
                    },

                    {
                        a,
                        b,
                        c,
                        d,
                        e,
                        f,
                        g,
                        h
                    },

                    {
                        t
                    },

                    {
                        i,
                        entry,
                        gest,
                        hand,
                        j,
                        killS
                    }
                };

                {
                    std::lock_guard<std::mutex> lock(g_skinUiMutex);
                    charData.push_back(charEntry);
                }

                if (!n0.empty()) {

                    TargetChar targetChar{

                        n0,
                        N,
                        X1->Traitor3P,
                        (int)W,
                        (int)Z1->ID,
                        h
                    };

                    std::lock_guard<std::mutex> lock(g_skinUiMutex);
                    g_targetCharacters.push_back(targetChar);
                }
            }

            break;
        }
    }

    charsLoaded = true;
    SKINLOG("skin loader: character list = %d", (int)charData.size());

    }

    if (!g_targetCharacters.empty()) {

        std::lock_guard<std::mutex> charIndexLock(g_skinUiMutex);

        bool foundCharly = false;

        for (size_t idx = 0; idx < g_targetCharacters.size(); ++idx) {

            if (g_targetCharacters[idx].name == "Charly") {

                g_selectedTargetCharIndex = (int)idx;

                foundCharly = true;

                break;
            }
        }

        if (!foundCharly) {

            for (size_t idx = 0; idx < g_targetCharacters.size(); ++idx) {

                std::string lowName = g_targetCharacters[idx].name;

                std::transform(
                    lowName.begin(),
                    lowName.end(),
                    lowName.begin(),
                    ::tolower
                );

                if (lowName.find("charly") != std::string::npos) {

                    g_selectedTargetCharIndex = (int)idx;

                    break;
                }
            }
        }
    }

    // Only stop re-scanning once every category produced data; partial results
    // are already published above.
    loadCharacter = watchLoaded && deadboxLoaded && charsLoaded;

    if (!g_targetCharacters.empty()) {

        std::lock_guard<std::mutex> charIndexLock(g_skinUiMutex);

        const int lastTargetIndex = (int)g_targetCharacters.size() - 1;
        if (g_selectedTargetCharIndex > lastTargetIndex) g_selectedTargetCharIndex = lastTargetIndex;
        if (g_selectedTargetCharIndex < 0) g_selectedTargetCharIndex = 0;
    }
}

void LoadWeaponSkins() {
    if (loadskinhack) return;

    std::vector<void*> weaponConfs, itemInvs, weaponAssets, weaponFires, weaponExtras, killEffects, mythicArmors, mythicSights, itemRes;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        weaponConfs = weaponConfInstance;
        itemInvs = itemInventoryInstance;
        weaponAssets = weaponAssetGroupInstance;
        weaponFires = weaponFireEffectInstance;
        weaponExtras = weaponExtraInstance;
        killEffects = killEffectItemInstance;
        mythicArmors = mythicArmorInstance;
        mythicSights = mythicSightInstance;
        itemRes = itemResourceConfigInstance;
    }

    // The weapon list only needs the weapon configs and their inventory entries;
    // the other tables are optional lookups, so an empty one must not hide the
    // whole weapon category.
    if (weaponConfs.empty() || itemInvs.empty()) {
        return;
    }

    std::unordered_map<int, void*> itemInvByItemID;
    std::unordered_map<int, void*> weaponAssetByID;
    std::unordered_map<int, void*> weaponFireByID;
    std::unordered_map<int, void*> weaponExtraByID;
    std::unordered_map<int, int>   mythicArmorBySecondTab;
    std::unordered_map<int, int>   mythicSightByWeapon;
    std::unordered_map<int, int>   killEffectByWeapon;
    std::unordered_map<int, void*> itemResByID;

    for (void* inv : itemInvs) {
        if (!inv || !Tools::IsPtrValid(inv)) continue;
        int itemID = *(int*)((uintptr_t)inv + 0x20);
        itemInvByItemID[itemID] = inv;
    }

    for (void* asset : weaponAssets) {
        if (!asset || !Tools::IsPtrValid(asset)) continue;
        int id = *(int*)((uintptr_t)asset + 0x44);
        weaponAssetByID[id] = asset;
    }

    for (void* fire : weaponFires) {
        if (!fire || !Tools::IsPtrValid(fire)) continue;
        int id = *(int*)((uintptr_t)fire + 0x80);
        weaponFireByID[id] = fire;
    }

    for (void* extra : weaponExtras) {
        if (!extra || !Tools::IsPtrValid(extra)) continue;
        weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x18);
        if (!Tools::IsPtrValid(weaponextraFields)) continue;
        weaponExtraByID[weaponextraFields->ID] = extra;
    }

    for (void* armor : mythicArmors) {
        if (!armor || !Tools::IsPtrValid(armor)) continue;
        mythicarmorFields = (MythicArmorFields*)((uintptr_t)armor + 0x14);
        if (!Tools::IsPtrValid(mythicarmorFields)) continue;
        if (mythicarmorFields->ThirdTab == 5) {
            mythicArmorBySecondTab[mythicarmorFields->SecondTab] = mythicarmorFields->AssetID;
        }
    }

    for (void* sight : mythicSights) {
        if (!sight || !Tools::IsPtrValid(sight)) continue;
        mythicsightFields = (MythicSightFields*)((uintptr_t)sight + 0x18);
        if (!Tools::IsPtrValid(mythicsightFields)) continue;
        auto* equipArray = *(Array<int>**)((uintptr_t)sight + 0x38);
        if (equipArray && Tools::IsPtrValid(equipArray) && equipArray->getLength() > 0) {
            int lastIndex = equipArray->getLength() - 1;
            int weaponID = equipArray->m_Items[lastIndex];
            int sightID = *(int*)((uintptr_t)sight + 0x14);
            mythicSightByWeapon[weaponID] = sightID;
        }
    }

    for (void* kill : killEffects) {
        if (!kill || !Tools::IsPtrValid(kill)) continue;
        killeffectFields = (KillEffectItemFields*)((uintptr_t)kill + 0x18);
        if (!Tools::IsPtrValid(killeffectFields)) continue;
        auto* equipArray = *(Array<int>**)((uintptr_t)kill + 0x90);
        if (equipArray && Tools::IsPtrValid(equipArray) && equipArray->getLength() > 0) {
            int lastIndex = equipArray->getLength() - 1;
            int weaponID = equipArray->m_Items[lastIndex];
            auto* realAssetIDs = *(Array<int>**)((uintptr_t)kill + 0x18);
            if (realAssetIDs && Tools::IsPtrValid(realAssetIDs) && realAssetIDs->getLength() > 0) {
                int lastAssetIndex = realAssetIDs->getLength() - 1;
                int lastAssetID = realAssetIDs->m_Items[lastAssetIndex];
                killEffectByWeapon[weaponID] = lastAssetID;
            }
        }
    }

    for (void* res : itemRes) {
        if (!res || !Tools::IsPtrValid(res)) continue;
        itemFields = (ItemResourceFields*)((uintptr_t)res + 0x18);
        if (!Tools::IsPtrValid(itemFields)) continue;
        itemResByID[itemFields->ID] = res;
    }

    for (void* conf : weaponConfs) {
        if (!conf || !Tools::IsPtrValid(conf)) continue;
        int baseID = *(int*)((uintptr_t)conf + 0x34);
        int confID = *(int*)((uintptr_t)conf + 0x40);

        auto itItem = itemInvByItemID.find(confID);
        if (itItem == itemInvByItemID.end()) continue;
        void* item = itItem->second;
        if (!item || !Tools::IsPtrValid(item)) continue;
        int itemIDbase = *(int*)((uintptr_t)item + 0x20);
        int itemBaseModified;
        int itemBase;
        if (baseID == itemIDbase) {
            itemBase = *(int*)((uintptr_t)item + 0x20);
            itemBaseModified = itemBase + 200;
        }
        if (confID == itemIDbase) {
            uint8_t confColorID = *(uint8_t*)((uintptr_t)conf + 0x22);
            int itemIDskin2 = *(int*)((uintptr_t)item + 0x24);
            int itemIDskin3 = *(int*)((uintptr_t)item + 0x28);

            std::string weaponName = GetNameString(WeaponConfName, conf);
            std::string displayName = GetRarityPrefix(confColorID) + weaponName;

            if (nameCountMap.find(displayName) != nameCountMap.end()) {
                nameCountMap[displayName]++;
                displayName += " +" + std::to_string(nameCountMap[displayName]);
            } else {
                nameCountMap[displayName] = 0;
            }

            if (!displayName.empty()) {
                int fireIds = 0, fireIds2 = 0, assetIds = 0;
                int originalFireID = 0;

                auto itAsset = weaponAssetByID.find(itemIDskin2);
                if (itAsset != weaponAssetByID.end()) {
                    void* asset = itAsset->second;
                    int fireEffectID = *(int*)((uintptr_t)asset + 0x40);
                    assetIds = itemIDskin2;

                    auto itFire = weaponFireByID.find(fireEffectID);
                    if (itFire != weaponFireByID.end()) {
                        void* fireConf = itFire->second;
                        int fireID = *(int*)((uintptr_t)fireConf + 0x80);
                        int assetIdBulletSmoke = *(int*)((uintptr_t)fireConf + 0x1C);

                        if (displayName.find("[M]") != std::string::npos) {
                            if (fireEffectID == fireID) {
                                if (assetIdBulletSmoke != 0) {
                                    fireIds = fireID;
                                    originalFireID = fireID;
                                } else {
                                    int nextFireID = fireID + 1;
                                    bool found = false;
                                    for (int i = 0; i < 10; i++) {
                                        auto itNext = weaponFireByID.find(nextFireID);
                                        if (itNext != weaponFireByID.end()) {
                                            void* nextFireConf = itNext->second;
                                            int nextAssetIdBulletSmoke = *(int*)((uintptr_t)nextFireConf + 0x1C);
                                            if (nextAssetIdBulletSmoke != 0) {
                                                fireIds = nextFireID;
                                                found = true;
                                                break;
                                            }
                                        }
                                        nextFireID++;
                                    }
                                    if (fireIds == 0) fireIds = fireID;
                                    fireIds2 = fireID;
                                }
                            }
                        }
                    }
                }

                int confBaseSkin = 0, confSkinID = 0, confBrocastID = 0, confColor = 0, confBluePrintID = 0;
                if (confID == itemIDbase) {
                    confBaseSkin = ReadInt(conf, 0x34);
                    confColor = ReadByte(conf, 0x22);
                    confSkinID = ReadInt(conf, 0x38);
                    confBrocastID = ReadInt(conf, 0x3C);
                    confBluePrintID = ReadByte(conf, 0x1C);
                }

                int mythicArmor = 0, deadReplay = 0, killEffect = 0, extraOrig = 0;

                auto itExtraBase = weaponExtraByID.find(baseID);
                if (itExtraBase != weaponExtraByID.end()) {
                    void* extra = itExtraBase->second;
                    weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x18);
                    if (Tools::IsPtrValid(weaponextraFields)) {
                        extraOrig = weaponextraFields->ID;
                    }
                }
                auto itExtraConf = weaponExtraByID.find(confID);
                if (itExtraConf != weaponExtraByID.end()) {
                    void* extra = itExtraConf->second;
                    weaponextraFields = (WeaponConfExtraFields*)((uintptr_t)extra + 0x18);
                    if (Tools::IsPtrValid(weaponextraFields)) {
                        deadReplay = weaponextraFields->DefaultDeadReplayEffectId;
                        killEffect = weaponextraFields->DefaultKillEffectId;
                    }
                }

                auto itMythicArmor = mythicArmorBySecondTab.find(itemIDskin3);
                if (itMythicArmor != mythicArmorBySecondTab.end()) {
                    if (displayName.find("[M]") != std::string::npos)
                        mythicArmor = itMythicArmor->second;
                }

                int sightMythic = 0;
                auto itMythicSight = mythicSightByWeapon.find(itemIDskin3);
                if (itMythicSight != mythicSightByWeapon.end()) {
                    if (displayName.find("[M]") != std::string::npos)
                        sightMythic = itMythicSight->second;
                }

                int killEffectFromItem = 0;
                auto itKillEffect = killEffectByWeapon.find(itemIDskin3);
                if (itKillEffect != killEffectByWeapon.end())
                    killEffectFromItem = itKillEffect->second;
                else {
                    itKillEffect = killEffectByWeapon.find(confID);
                    if (itKillEffect != killEffectByWeapon.end())
                        killEffectFromItem = itKillEffect->second;
                    else {
                        itKillEffect = killEffectByWeapon.find(baseID);
                        if (itKillEffect != killEffectByWeapon.end())
                            killEffectFromItem = itKillEffect->second;
                        else {
                            itKillEffect = killEffectByWeapon.find(itemBase);
                            if (itKillEffect != killEffectByWeapon.end())
                                killEffectFromItem = itKillEffect->second;
                        }
                    }
                }
                if (killEffectFromItem != 0)
                    killEffect = killEffectFromItem;

                uint16_t spr1 = 0;
                uint16_t spr2 = 0;
                uint16_t spr3 = 0;
                uint16_t spr4 = 0;

                int xItem1 = 0;
                int xItem2 = 0;
                int xItem3 = 0;

                auto itItemRes = itemResByID.find(confID);
                if (itItemRes != itemResByID.end()) {
                    void* itemResource = itItemRes->second;
                    itemFields = (ItemResourceFields*)((uintptr_t)itemResource + 0x18);
                    if (Tools::IsPtrValid(itemFields)) {
                        xItem1 = itemFields->FxAssetID;
                        xItem2 = itemFields->InventoryModelID;
                        xItem3 = itemFields->ModelAssetIDRaw;
                        spr1 = itemFields->UISmallSpriteNameIndex;
                        spr2 = itemFields->UIMiniSpriteNameIndex;
                        spr3 = itemFields->UISpriteNameIndex;
                        spr4 = itemFields->UISquareSpriteNameIndex;
                    }
                }

                itemInfo weaponEntry{displayName,
                    {itemBase, itemIDskin2, itemIDskin3, itemBaseModified},
                    {confBaseSkin, confColor, confID, confBrocastID, confBluePrintID},
                    {extraOrig, mythicArmor, sightMythic, deadReplay, killEffect},
                    {assetIds, fireIds, fireIds2},
                    {xItem1, xItem2, xItem3},
                    {spr1, spr2, spr3, spr4}};
                {
                    std::lock_guard<std::mutex> lock(g_skinUiMutex);
                    itemData.push_back(weaponEntry);
                }
            }
        }
    }

    loadskinhack = true;
}

void LoadPlaneSkins() {
    std::vector<void*> dropPlaneSkins;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        dropPlaneSkins = BRDropPlaneSkinConfigInstance;
    }

    if (dropPlaneSkins.empty()) return;

    for (void* plane : dropPlaneSkins) {
        if (!plane || !Tools::IsPtrValid(plane)) continue;
        dropplaneFields = (BRDropPlaneSkinFields*)((uintptr_t)plane + 0x18);
        if (!Tools::IsPtrValid(dropplaneFields)) continue;
        std::string planeName = GetNameString(GetDropPlaneName, plane);
        if (dropplaneFields->ModelAsset1P != 0 && !getplane[planeName]) {
            getplane[planeName] = true;
            std::string prefix = GetRarityPrefix(dropplaneFields->ColorID);
            planeID planeEntry{prefix + planeName, {dropplaneFields->ColorID, dropplaneFields->ModelAsset1P, dropplaneFields->ModelAsset3P, dropplaneFields->ModelAssetCutScene, dropplaneFields->ModelAssetUI, dropplaneFields->Priority}};
            std::lock_guard<std::mutex> lock(g_skinUiMutex);
            dropplane.push_back(planeEntry);
            if (getplane.size() == dropPlaneSkins.size())
                break;
        }
    }
}

// ── VehicleSkis / VehicleSkinHelper forward declarations ─────────────────────

inline bool SkinPtr(void* ptr) {
    return ptr != nullptr && Tools::IsPtrValid(ptr);
}

inline int GetIl2CppArrayLen(void* arr) {
    if (!SkinPtr(arr)) return 0;
    const int len = *(int*)((uintptr_t)arr + 0x18);
    return len >= 0 && len < 512 ? len : 0;
}

inline void* VehicleSkinParams(void* skinConf) {
    if (!SkinPtr(skinConf)) return nullptr;
    return *(void**)((uintptr_t)skinConf + 0xC8);
}

inline void* FindVehicleSkinConfigById(int skinId) {
    if (skinId <= 0) return nullptr;
    auto exact = activeVehicleSkinConfs.find(skinId);
    if (exact != activeVehicleSkinConfs.end() && SkinPtr(exact->second))
        return exact->second;
    for (void* skin : SkinSnapshot(vehicleSkinConfigInstance)) {
        if (!SkinPtr(skin)) continue;
        auto* fields = (VehicleSkinConfFields*)((uintptr_t)skin + 0x18);
        if (SkinPtr(fields) && (int)fields->ID == skinId)
            return skin;
    }
    return nullptr;
}

inline int ActiveVehicleSkinIdForType(int vehicleType) {
    auto it = activeVehicleSkins.find(vehicleType);
    return it != activeVehicleSkins.end() ? it->second : 0;
}

inline int ActiveVehicleSkinIdForConf(void* skinConf) {
    if (!SkinPtr(skinConf)) return 0;
    auto* fields = (VehicleSkinConfFields*)((uintptr_t)skinConf + 0x18);
    if (!SkinPtr(fields)) return 0;
    auto byId = activeVehicleSkinsById.find((int)fields->VehicleId);
    if (byId != activeVehicleSkinsById.end()) return byId->second;
    return ActiveVehicleSkinIdForType((int)fields->VehicleId);
}

// Function pointers resolved at runtime
inline void (*VehicleSkinHelper_ChangeVehicleSkinId)(void* vehicleObj, uint vehicleSkinId) = nullptr;
inline void (*VehicleSkinHelper_ChangeVehicleSkinConf)(void* vehicleObj, void* vehConf) = nullptr;
inline void (*VehicleSkis_ApplySkin)(void* instance) = nullptr;

// Hook orig pointers
inline void (*orig_VehicleSkis_SetupSkin)(void* instance, uint skinID, bool findItem) = nullptr;
inline void (*orig_VehicleSkis_ChangeSkinId)(void* instance, uint skinID) = nullptr;
inline void (*orig_VehicleSkis_ChangeSkinConf)(void* instance, void* skinConf) = nullptr;
inline void (*orig_VehicleSkis_BuildVehicleModel)(void* instance) = nullptr;
inline void (*orig_VehicleSkis_LoadModelComplete)(void* instance, int assetID, void* go) = nullptr;

inline int ActiveSkisSkinId() {
    const int selected = ActiveVehicleSkinIdForType(31707110);
    return selected > 0 ? selected : 0;
}

inline void ForceSkisSkinFields(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    auto* fields = SkinPtr(conf) ? (VehicleSkinConfFields*)((uintptr_t)conf + 0x18) : nullptr;
    const int mesh = SkinPtr(fields) ? fields->NewVehicleResId : 0;
    *(uint*)((uintptr_t)instance + 0x88) = (uint)selected;
    if (mesh > 0) {
        *(int*)((uintptr_t)instance + 0x20) = mesh;
        *(int*)((uintptr_t)instance + 0xA8) = mesh;
    }
}

inline void ApplySkisSkinConf(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0 || orig_VehicleSkis_ChangeSkinConf == nullptr) return;
    void* conf = FindVehicleSkinConfigById(selected);
    if (SkinPtr(conf)) orig_VehicleSkis_ChangeSkinConf(instance, conf);
}

inline void ApplySkisSkinToObject(void* targetObj, int selected) {
    if (!SkinPtr(targetObj) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    if (!SkinPtr(conf)) return;
    if (VehicleSkinHelper_ChangeVehicleSkinId != nullptr)
        VehicleSkinHelper_ChangeVehicleSkinId(targetObj, (uint)selected);
    if (VehicleSkinHelper_ChangeVehicleSkinConf != nullptr)
        VehicleSkinHelper_ChangeVehicleSkinConf(targetObj, conf);
}

inline void ApplySkisMaterials(void* instance, int selected) {
    if (!SkinPtr(instance) || selected <= 0) return;
    void* conf = FindVehicleSkinConfigById(selected);
    void* mats = VehicleSkinParams(conf);
    if (!SkinPtr(mats) || GetIl2CppArrayLen(mats) <= 0 || VehicleSkis_ApplySkin == nullptr) return;
    *(void**)((uintptr_t)instance + 0x90) = mats;
    ForceSkisSkinFields(instance, selected);
    VehicleSkis_ApplySkin(instance);
}

inline void hook_VehicleSkis_SetupSkin(void* instance, uint skinID, bool findItem) {
    uint nextSkin = skinID;
    const int selected = ActiveSkisSkinId();
    if (selected > 0) nextSkin = (uint)selected;
    ForceSkisSkinFields(instance, (int)nextSkin);
    if (orig_VehicleSkis_SetupSkin != nullptr)
        orig_VehicleSkis_SetupSkin(instance, nextSkin, findItem);
    ForceSkisSkinFields(instance, (int)nextSkin);
    ApplySkisSkinConf(instance, (int)nextSkin);
}

inline void hook_VehicleSkis_ChangeSkinId(void* instance, uint skinID) {
    uint nextSkin = skinID;
    const int selected = ActiveSkisSkinId();
    if (selected > 0) nextSkin = (uint)selected;
    ForceSkisSkinFields(instance, (int)nextSkin);
    if (orig_VehicleSkis_ChangeSkinId != nullptr)
        orig_VehicleSkis_ChangeSkinId(instance, nextSkin);
    ForceSkisSkinFields(instance, (int)nextSkin);
    ApplySkisSkinConf(instance, (int)nextSkin);
}

inline void hook_VehicleSkis_ChangeSkinConf(void* instance, void* skinConf) {
    void* nextSkin = skinConf;
    const int selectedId = ActiveSkisSkinId() > 0 ? ActiveSkisSkinId() : ActiveVehicleSkinIdForConf(skinConf);
    if (selectedId > 0) {
        void* selected = FindVehicleSkinConfigById(selectedId);
        if (SkinPtr(selected)) nextSkin = selected;
    }
    ForceSkisSkinFields(instance, selectedId);
    if (orig_VehicleSkis_ChangeSkinConf != nullptr)
        orig_VehicleSkis_ChangeSkinConf(instance, nextSkin);
    ForceSkisSkinFields(instance, selectedId);
}

inline void hook_VehicleSkis_BuildVehicleModel(void* instance) {
    const int selected = ActiveSkisSkinId();
    ForceSkisSkinFields(instance, selected);
    if (orig_VehicleSkis_BuildVehicleModel != nullptr)
        orig_VehicleSkis_BuildVehicleModel(instance);
    ForceSkisSkinFields(instance, selected);
    ApplySkisSkinConf(instance, selected);
}

inline void hook_VehicleSkis_LoadModelComplete(void* instance, int assetID, void* go) {
    const int selected = ActiveSkisSkinId();
    ForceSkisSkinFields(instance, selected);
    if (orig_VehicleSkis_LoadModelComplete != nullptr)
        orig_VehicleSkis_LoadModelComplete(instance, assetID, go);
    ForceSkisSkinFields(instance, selected);
    ApplySkisSkinToObject(go, selected);
    ApplySkisSkinConf(instance, selected);
    ApplySkisMaterials(instance, selected);
}

// ─────────────────────────────────────────────────────────────────────────────

void LoadSnowboardSkins() {
    std::vector<void*> skinConfs;
    {
        std::lock_guard<std::mutex> lock(g_dataMutex);
        skinConfs = vehicleSkinConfigInstance;
    }
    if (skinConfs.empty()) return;

    static std::unordered_map<int, bool> loadedIds;

    for (void* skin : skinConfs) {
        if (!skin || !Tools::IsPtrValid(skin)) continue;
        auto* fields = (VehicleSkinConfFields*)((uintptr_t)skin + 0x18);
        if (!Tools::IsPtrValid(fields)) continue;
        if (fields->VehicleId != 31707110) continue;
        int skinId = (int)fields->ID;
        if (skinId <= 0 || loadedIds[skinId]) continue;
        loadedIds[skinId] = true;
        std::string prefix = GetRarityPrefix((int)fields->ColorID);
        std::string name = prefix + "Snowboard Skin " + std::to_string(skinId);
        {
            std::lock_guard<std::mutex> lock(g_skinUiMutex);
            snowboardData.push_back({name, skinId});
        }
    }
}

// ───────── crash diagnostics ─────────
// The skin loader runs on its own thread a few seconds after startup. When it
// dies there is no stack trace pointing at the step, so every stage logs a line
// before running: `adb logcat -s MWD-ASTRAL` (or AIDE's logcat) then shows the
// last stage reached before the crash.

// DobbyHook() on an address that does not match the current game build jumps the
// game straight into our hook, which takes the whole process down. Probe first
// and skip (with a log) when the target offset is not mapped yet.
template <typename HookFn, typename OrigFn>
inline bool Skins_HookAt(uintptr_t offset, HookFn replace, OrigFn *backup)
{
    uintptr_t addr = getRealOffset(offset);
    if (!Tools::IsPtrValid((void *)addr))
    {
        SKINERR("hook skipped, bad address: 0x%lx", (unsigned long)offset);
        return false;
    }
    DobbyHook((void *)addr, (void *)replace, (void **)backup);
    return true;
}

// ───────── retryable ctor-hook table ─────────
// Every skin ctor hook is kept in a table so one that could not be installed on
// the first attempt (libunity still mapping, or an address that has not been
// probed yet) is retried by the loader loop instead of being lost for the whole
// session. Losing ItemInventory / WeaponConf is exactly the state that shows up
// as "No weapon skins loaded" in the Skins tab.
struct SkinHookEntry {
    uintptr_t offset;
    void     *replace;
    void    **backup;
    bool      installed;
};

inline SkinHookEntry g_skinHookEntries[] = {
    { Item2InventoryAddress,           (void *) my_Item2InventoryCtor,    (void **) &orig_Item2InventoryCtor,    false },
    { WeaponConfExtraAddress,          (void *) my_WeaponConfExtraCtor,   (void **) &orig_WeaponConfExtraCtor,   false },
    { WeaponFireEffectAddress,         (void *) my_WeaponFireEffectCtor,  (void **) &orig_WeaponFireEffectCtor,  false },
    { WeaponConfAddress,               (void *) my_WeaponConfCtor,        (void **) &orig_WeaponConfCtor,        false },
    { WeaponAssetGroupAddress,         (void *) my_WeaponAssetGroupCtor,  (void **) &orig_WeaponAssetGroupCtor,  false },
    { MythicArmorConfigAddress,        (void *) my_MythicArmorCtor,       (void **) &orig_MythicArmorCtor,       false },
    { MythicSightConfigAddress,        (void *) my_MythicSightCtor,       (void **) &orig_MythicSightCtor,       false },
    { WeaponSkinAddress,               (void *) my_WeaponSkinCtor,        (void **) &orig_WeaponSkinCtor,        false },
    { WeaponCamosAddress,              (void *) my_WeaponCamosCtor,       (void **) &orig_WeaponCamosCtor,       false },
    { KillEffectItemConfConfigAddress, (void *) my_KillEffectItemCtor,    (void **) &orig_KillEffectItemCtor,    false },
    { ItemResourceAddress,             (void *) my_ItemResourceCtor,      (void **) &orig_ItemResourceCtor,      false },
    { CharacterModelAddress,           (void *) my_CharacterModelCtor,    (void **) &orig_CharacterModelCtor,    false },
    { RoleConfAddress,                 (void *) my_RoleConfCtor,          (void **) &orig_RoleConfCtor,          false },
    { RoleSkinAddress,                 (void *) my_RoleSkinCtor,          (void **) &orig_RoleSkinCtor,          false },
    { RolePackConfAddress,             (void *) my_RolePackConfCtor,      (void **) &orig_RolePackConfCtor,      false },
    { BRDeadboxSkinAddress,            (void *) my_BRDeadboxSkinCtor,     (void **) &orig_BRDeadboxSkinCtor,     false },
    { BRDropPlaneSkinAddress,          (void *) my_BRDropPlaneSkinCtor,   (void **) &orig_BRDropPlaneSkinCtor,   false },
    { VehicleSkinConfAddress,          (void *) my_VehicleSkinConfCtor,   (void **) &orig_VehicleSkinConfCtor,   false },
    { VehicleItemConfAddress,          (void *) my_VehicleItemConfCtor,   (void **) &orig_VehicleItemConfCtor,   false },
};

// Idempotent: installs every entry that is not hooked yet and whose target is
// currently mapped. Returns how many hooks were installed by this call.
inline int Skins_InstallCtorHooks()
{
    int installedNow = 0;
    const int n = (int) (sizeof(g_skinHookEntries) / sizeof(g_skinHookEntries[0]));
    for (int i = 0; i < n; ++i)
    {
        SkinHookEntry &e = g_skinHookEntries[i];
        if (e.installed)
            continue;
        uintptr_t addr = getRealOffset(e.offset);
        if (!Tools::IsPtrValid((void *) addr))
        {
            SKINERR("hook deferred, address not mapped yet: 0x%lx", (unsigned long) e.offset);
            continue;
        }
        DobbyHook((void *) addr, e.replace, e.backup);
        e.installed = true;
        ++installedNow;
    }
    return installedNow;
}

void Skins_Thread()
{
    SKINLOG("skins thread: waiting for libunity");
    while (!m_unity)
    {
        sleep(1);
    }

    SKINLOG("skins thread: ctor hooks installed (%d)", Skins_InstallCtorHooks());
    // Skin data is collected on its own thread so a missing helper library can
    // never stall it. The lists start filling as soon as the game creates the
    // config objects, which is what the Skins tab renders.
    std::thread([] {
        // Small head start so the game finishes its early config pass first.
        std::this_thread::sleep_for(std::chrono::seconds(5));
        SKINLOG("skin loader: starting first pass");
        // Log every stage of the first pass only: if the loader thread dies, the
        // last "loader:" line in logcat names the step that killed it.
        bool firstPass = true;
        while (true)
        {
            // Re-attempt any ctor hook that was skipped on the first try.
            Skins_InstallCtorHooks();
            if (firstPass) SKINLOG("loader: characters");
            LoadCharacterSkins();
            if (firstPass) SKINLOG("loader: weapons");
            LoadWeaponSkins();
            if (firstPass) SKINLOG("loader: planes");
            LoadPlaneSkins();
            if (firstPass) SKINLOG("loader: snowboards");
            LoadSnowboardSkins();
            if (firstPass) SKINLOG("loader: first pass complete");
            firstPass = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }).detach();

    // The feature hooks below need the game's helper library: wait for it, but
    // do not block forever if it never shows up.
    for (int i = 0; i < 30 && !Tools::GetBaseAddress("libRoosterNN.so"); ++i)
    {
        sleep(1);
    }

    sleep(5);
    
    Tools::Hook((void*)(m_unity + 0xA61AC70), (void*)_GetCurrentWeaponKillEffect, (void**)&orig_GetCurrentWeaponKillEffect);
    Tools::Hook((void*)(m_unity + 0xA61AAA8), (void*)_GetCurrentWeaponFireEffect, (void**)&orig_GetCurrentWeaponFireEffect);
    Tools::Hook((void*)(m_unity + 0xA61AB8C), (void*)_GetCurrentBulletTrackEffect, (void**)&orig_GetCurrentBulletTrackEffect);
    Tools::Hook((void*)(m_unity + 0xA61ADFC), (void*)_GetCurrentWeaponBrocast, (void**)&orig_GetCurrentWeaponBrocast);

    // VehicleSkis hooks
    auto hookVehicleRva = [&](uintptr_t rva, void* hook, void** orig) {
        uintptr_t addr = KittyMemory::getAbsoluteAddress("libunity.so", rva);
        if (addr != 0 && Tools::IsPtrValid((void*)addr))
            Tools::Hook((void*)addr, hook, orig);
    };
    hookVehicleRva(VehicleSkisSetupSkinAddress,       (void*)hook_VehicleSkis_SetupSkin,       (void**)&orig_VehicleSkis_SetupSkin);
    hookVehicleRva(VehicleSkisChangeSkinIdAddress,    (void*)hook_VehicleSkis_ChangeSkinId,    (void**)&orig_VehicleSkis_ChangeSkinId);
    hookVehicleRva(VehicleSkisChangeSkinConfAddress,  (void*)hook_VehicleSkis_ChangeSkinConf,  (void**)&orig_VehicleSkis_ChangeSkinConf);
    hookVehicleRva(VehicleSkisBuildVehicleModelAddress,(void*)hook_VehicleSkis_BuildVehicleModel,(void**)&orig_VehicleSkis_BuildVehicleModel);
    hookVehicleRva(VehicleSkisLoadModelCompleteAddress,(void*)hook_VehicleSkis_LoadModelComplete,(void**)&orig_VehicleSkis_LoadModelComplete);

    uintptr_t vehicleSkinHelperIdAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkinHelperChangeVehicleSkinIdAddress);
    if (vehicleSkinHelperIdAddr != 0 && Tools::IsPtrValid((void*)vehicleSkinHelperIdAddr))
        VehicleSkinHelper_ChangeVehicleSkinId = reinterpret_cast<void (*)(void*, uint)>(vehicleSkinHelperIdAddr);

    uintptr_t vehicleSkinHelperConfAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkinHelperChangeVehicleSkinConfAddress);
    if (vehicleSkinHelperConfAddr != 0 && Tools::IsPtrValid((void*)vehicleSkinHelperConfAddr))
        VehicleSkinHelper_ChangeVehicleSkinConf = reinterpret_cast<void (*)(void*, void*)>(vehicleSkinHelperConfAddr);

    uintptr_t skisApplySkinAddr = KittyMemory::getAbsoluteAddress("libunity.so", VehicleSkisApplySkinAddress);
    if (skisApplySkinAddr != 0 && Tools::IsPtrValid((void*)skisApplySkinAddr))
        VehicleSkis_ApplySkin = reinterpret_cast<void (*)(void*)>(skisApplySkinAddr);
    
}
