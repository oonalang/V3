#pragma once

#include <cstdint>
#include <string>
#include <dlfcn.h>

extern bool SnowB;
extern float SnowBsize;
extern bool isSpeedHackEnabled;
extern float speedHackMultiplier;
extern bool isJumpAdjustmentEnabled;
extern float jumpHeightMultiplier;
extern float SlideRange;
extern bool UnlimitedAmmo;
extern std::chrono::steady_clock::time_point lastAmmoRefillTime;

//-- Smart Reload
namespace SmartReloadCfg
{
    constexpr uintptr_t CarriedAmmoCountRva = 0x50EC6E8;
    constexpr uintptr_t PawnTryChangeClipRva = 0x5262594;
}

inline bool g_SmartReloadPending = false;

inline int SmartReloadCarriedAmmo(void *weapon)
{
    using GetAmmoFn = int (*)(void *);
    static GetAmmoFn getCarriedAmmo = reinterpret_cast<GetAmmoFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::CarriedAmmoCountRva));
    return getCarriedAmmo != nullptr ? getCarriedAmmo(weapon) : 0;
}

inline void SmartReloadRequestChangeClip(Pawn *local)
{
    using TryChangeClipFn = void (*)(Pawn *, bool);
    static TryChangeClipFn tryChangeClip = reinterpret_cast<TryChangeClipFn>(
        getAbsoluteAddress("libunity.so", SmartReloadCfg::PawnTryChangeClipRva));

    if (tryChangeClip != nullptr)
        tryChangeClip(local, true);
}

inline void SmartReloadMarkShotFired()
{
    if (!Config.ExtraMenu.SmartReload)
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive())
        return;

    g_SmartReloadPending = true;
}

inline bool SmartReloadCanRequestForPawn(Pawn *local)
{
    if (!Config.ExtraMenu.SmartReload || !g_SmartReloadPending)
        return false;

    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_SmartReloadPending = false;
        return false;
    }

    Pawn *currentLocal = GamePlay::get_LocalPawn();
    if (currentLocal != local) {
        g_SmartReloadPending = false;
        return false;
    }

    Weapon *weapon = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(weapon)) {
        g_SmartReloadPending = false;
        return false;
    }

    if (SmartReloadCarriedAmmo(weapon) <= 0) {
        g_SmartReloadPending = false;
        return false;
    }

    return true;
}

inline void SmartReloadRequestAfterStopFire(Pawn *local)
{
    if (!SmartReloadCanRequestForPawn(local))
        return;

    g_SmartReloadPending = false;
    SmartReloadRequestChangeClip(local);
}

inline void (*orig_Pawn_StopFire)(Pawn *instance, bool isImmidiately) = nullptr;
inline void hook_Pawn_StopFire(Pawn *instance, bool isImmidiately)
{
    orig_Pawn_StopFire(instance, isImmidiately);
    SmartReloadRequestAfterStopFire(instance);
}


//-- Clear Terrain
inline void (*orig_SetGrassShowState)(bool showGrass) = nullptr;
inline void hook_SetGrassShowState(bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain)
        showGrass = false;
    return orig_SetGrassShowState(showGrass);
}

inline void (*orig_SetGrassLODBias)(float lodBias, float midLodBias, bool showGrass) = nullptr;
inline void hook_SetGrassLODBias(float lodBias, float midLodBias, bool showGrass)
{
    if (Config.ExtraMenu.ClearTerrain) {
        lodBias = 0.0f;
        midLodBias = 0.0f;
        showGrass = false;
    }
    return orig_SetGrassLODBias(lodBias, midLodBias, showGrass);
}

inline void ApplyWorldVisualsRuntime()
{
    static bool lastClearTerrain = false;
    if (Config.ExtraMenu.ClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(false);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(0.0f, 0.0f, false);
    } else if (lastClearTerrain) {
        if (orig_SetGrassShowState)
            orig_SetGrassShowState(true);
        if (orig_SetGrassLODBias)
            orig_SetGrassLODBias(1.0f, 1.0f, true);
    }
    lastClearTerrain = Config.ExtraMenu.ClearTerrain;
}

//-- Aim Assist
inline float (*orig_GetAssitAimSpeed)(void *, Vector3, float, float, float, bool, bool) = nullptr;
inline float GetAssitAimSpeed(void * instance, Vector3 assistCentorPos, float assistDis, float dis, float angle, bool isPVE, bool gamepadInput) {
    if (instance != NULL) {
        if (Config.Aim.AimAssistSize > 0.0f) {
            return (float)Config.Aim.AimAssistSize;
        }
    }
    return orig_GetAssitAimSpeed(instance, assistCentorPos, assistDis, dis, angle, isPVE, gamepadInput);
}

//-- Anti Flashbang
inline void (*orig_OnFlashBangExplode)(void *, int, float, float, float) = nullptr;
inline void hook_OnFlashBangExplode(void *instance, int weaponItemID, float whiteTime, float whiteAlphaTime, float initIntensity) {
    if (instance != nullptr && Config.ExtraMenu.Flash) {
        whiteTime = 0.1f;
        whiteAlphaTime = 0.1f;
        initIntensity = 0.1f;
    }
    orig_OnFlashBangExplode(instance, weaponItemID, whiteTime, whiteAlphaTime, initIntensity);
}

//-- Firerate Speed
inline float (*orig_get_FireBoltTime)(void *) = nullptr;
inline float get_FireBoltTime(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_FireBoltTime(instance);
}

inline float (*orig_get_FireInterval)(void *) = nullptr;
inline float get_FireInterval(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_FireInterval(instance);
}

inline float (*orig_get_DelaySprintFire)(void *) = nullptr;
inline float get_DelaySprintFire(void * instance) {
    if (instance != NULL) {
        // No Sprint-Fire Delay is its own toggle so it can be used without the
        // firerate change; turning Firerate on still zeroes it as before.
        if (Config.ExtraMenu.NoSprintFireDelay || Config.ExtraMenu.Fire) {
            return 0.00001f;
        }
    }
    return orig_get_DelaySprintFire(instance);
}

typedef void (*SetUltraFrameRateDeviceInfo_t)(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate);
SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo;

void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 144;
        ultraFrameRateBR = 144;
        ultraFrameRateQualityLimit = 144;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
}

//-- High Jump
inline float (*orig_GetMaxJumpHeight)(void*) = nullptr;
inline float hook_GetMaxJumpHeight(void* instance) {
    float orig_ = orig_GetMaxJumpHeight(instance);
    return (jumpHeightMultiplier > 1.0f) ? orig_ * jumpHeightMultiplier : orig_;
}

//-- Increase Damage
inline bool (*orig_SingleLineCheckPhysics)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;

// Aim-aware hitbox state: the physics hook remembers which enemy (and its head
// height / head position) accepted the expanded shot, so the damage hook can
// decide Head vs Body from where the shot actually landed.
inline void*  g_hitboxAcceptedPawn   = nullptr;   // enemy Pawn accepted by the physics check
inline float  g_hitboxHeadHeightY    = 0.0f;      // that pawn's head world Y
inline Vector3 g_hitboxHeadPos       = Vector3::zero();
inline uintptr_t g_hitboxDamageInfo  = 0;

// A-Fire (triggerbot / auto-fire). Weapon::Tick runs every frame on the game
// thread for the weapon the local player is holding, so that is where the
// trigger is pressed and released: while a target sits under the crosshair the
// weapon is told to start firing, and told to stop once the target is gone.
inline bool     g_afireHoldingFire   = false;
inline void    *g_afireFiringOn      = nullptr;
inline float    g_afireLastPressTime = -1000.0f;

inline void (*orig_Weapon_Tick)(void *instance, float deltaTime) = nullptr;
inline void (*orig_Weapon_StartFire)(void *instance) = nullptr;
inline void (*orig_Weapon_StopFire)(void *instance, bool isImmidiately) = nullptr;

inline uintptr_t A_FireFindTarget() {
    if (Camera::get_main() == nullptr)
        return 0;
    return (Config.Aim.By == EAim::Distance) ? GetClosestTarget() : GetInsideFOVTarget();
}

// Release the trigger we pressed (used when the toggle is switched back off).
inline void A_FireReleaseTrigger(void *instance) {
    if (!g_afireHoldingFire)
        return;
    if (instance == nullptr || instance == g_afireFiringOn) {
        if (orig_Weapon_StopFire != nullptr && g_afireFiringOn != nullptr)
            orig_Weapon_StopFire(g_afireFiringOn, false);
        g_afireHoldingFire = false;
        g_afireFiringOn = nullptr;
    }
}

inline void hook_Weapon_Tick(void *instance, float deltaTime) {
    if (orig_Weapon_Tick != nullptr)
        orig_Weapon_Tick(instance, deltaTime);

    if (instance == nullptr)
        return;

    if (!Config.ExtraMenu.A_Fire) {
        A_FireReleaseTrigger(instance);
        return;
    }

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        A_FireReleaseTrigger(instance);
        return;
    }

    // Only drive the weapon the local player is currently holding.
    Weapon *held = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(held) || (void *) held != instance) {
        A_FireReleaseTrigger(instance);
        return;
    }

    bool ready = A_FireFindTarget() != 0;

    // Criteria: 0 = whenever a target is under the crosshair,
    //           1 = only while aiming / scoped,
    //           2 = only while the fire button is already held.
    if (ready && Config.ExtraMenu.A_FireTrigger == 1)
        ready = Class_Pawn_IsAiming != 0 && ((bool (*)(uintptr_t)) Class_Pawn_IsAiming)((uintptr_t) local);
    else if (ready && Config.ExtraMenu.A_FireTrigger == 2)
        ready = Class_Pawn_get_IsFiring != 0 && ((bool (*)(uintptr_t)) Class_Pawn_get_IsFiring)((uintptr_t) local);

    // Optional delay between trigger presses.
    if (ready && Config.ExtraMenu.A_FireDelay > 0.0f) {
        const float now = ImGui::GetTime();
        if (now - g_afireLastPressTime < Config.ExtraMenu.A_FireDelay)
            ready = false;
    }

    if (ready) {
        if (!g_afireHoldingFire) {
            if (orig_Weapon_StartFire != nullptr)
                orig_Weapon_StartFire(instance);
            g_afireHoldingFire = true;
            g_afireFiringOn = instance;
        }
        g_afireLastPressTime = ImGui::GetTime();
    } else if (g_afireHoldingFire && instance == g_afireFiringOn) {
        if (orig_Weapon_StopFire != nullptr)
            orig_Weapon_StopFire(instance, false);
        g_afireHoldingFire = false;
        g_afireFiringOn = nullptr;
    }
}

         // DamageInfo produced right after the accepted shot

struct HitboxAcceptResult {
    bool     hit;
   Pawn*    pawn;
    float    headY;
    Vector3  headPos;
};

static HitboxAcceptResult HitboxRayTest(const Vector3& startPos, const Vector3& dir) {
    HitboxAcceptResult result{false, nullptr, 0.0f, Vector3::zero()};
    MatchGame* matchGame = GamePlay::get_MatchGame();
    if (!Tools::IsPtrValid(matchGame)) return result;
    List<Pawn*>* enemyPawns = matchGame->EnemyPawns();
    if (!Tools::IsPtrValid(enemyPawns)) return result;
    Pawn** enemyItems = enemyPawns->getItems();
    const int enemyCount = enemyPawns->getSize();
    if (!Tools::IsPtrValid(enemyItems) || enemyCount <= 0) return result;

    // Slider breaks the fixed size: scale grows the capsule in every direction.
    // scale 1 (old default) -> radius 0.875m, top 1.75m, bottom 0.10m
    const float s = ImClamp(Config.ExtraMenu.HitboxScale, 0.5f, 25.0f);
    const float radius      = 0.50f + 0.375f * s;         // grows with the slider
    const float radiusSq    = radius * radius;
    const float topY        = 0.95f + 0.80f * s;         // capsule top (head zone)
    const float bottomY     = 0.10f - 0.02f * s;         // capsule bottom (feet/ground)
    const float topBonusY   = 0.30f + 0.10f * s;         // zone above the head that still counts
    const Vector3 rayAxis = Vector3::Normalized(dir);

    float bestAlong = FLT_MAX;
    for (int i = 0; i < enemyCount; i++) {
        Pawn* enemy = enemyItems[i];
        if (!Tools::IsPtrValid(enemy) || !enemy->m_IsAlive()) continue;

        const Vector3 root   = enemy->get_LastPawnPos();     // pelvis/root of the model
        const Vector3 head   = enemy->get_HeadPosition();
        const float   headY  = head.y;

        // Vertical capsule around the enemy model: base at bottomY above the
        // root position, up to topY. Center at each Y level.
        const float baseY = root.y + bottomY;   // capsule bottom in world space

        // Closest point on the ray to the capsule axis (vertical at root.x/root.z)
        const Vector3 toEnemy = root - startPos;
        const float alongRay = Vector3::Dot(toEnemy, rayAxis);
        if (alongRay <= 0.0f) continue;                      // behind us
        const Vector3 closest = startPos + rayAxis * alongRay;

        // Horizontal distance from the ray (at its closest approach) to the axis
        const float dx = closest.x - root.x;
        const float dz = closest.z - root.z;
        const float horizDistSq = dx * dx + dz * dz;

        // The ray height while passing the model decides where it "lands".
        // Applying the same Y to both sides keeps the range checks scale-free.
        const float closestWorldY = closest.y;
        const float rel = closestWorldY - baseY;             // capsule-relative height
        const float segTop = (root.y + topY + topBonusY) - baseY;

        if (horizDistSq <= radiusSq && rel >= 0.0f && rel <= segTop) {
            if (alongRay < bestAlong) {
                bestAlong = alongRay;
                result.hit = true;
                result.pawn = enemy;
                result.headY = headY;
                result.headPos = head;
            }
        }
    }
    return result;
}

inline bool SingleLineCheckPhysics(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) {
    if (instance != NULL && Config.ExtraMenu.Hit) {
        const HitboxAcceptResult acc = HitboxRayTest(startPos, dir);
        if (acc.hit) {
            g_hitboxAcceptedPawn = (void*)acc.pawn;
            g_hitboxHeadHeightY = acc.headY;
            g_hitboxHeadPos = acc.headPos;
            return true;
        }
    }
    return orig_SingleLineCheckPhysics(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

inline void* (*orig_CalcDamageInfoInstantHit)(void* instance, void** inImpactInfo, unsigned char inFireMode, void* sourcePos, int clientTime, int ammoCount, float punchX, float punchY, float spreadX, float spreadY, float fightOffSpeed, float fightOffUp) = nullptr;
inline void* CalcDamageInfoInstantHit(void* instance, void** inImpactInfo, unsigned char inFireMode, void* sourcePos, int clientTime, int ammoCount, float punchX, float punchY, float spreadX, float spreadY, float fightOffSpeed, float fightOffUp) {
    void* damageInfo = orig_CalcDamageInfoInstantHit(instance, inImpactInfo, inFireMode, sourcePos, clientTime, ammoCount, punchX, punchY, spreadX, spreadY, fightOffSpeed, fightOffUp);
    g_hitboxDamageInfo = (uintptr_t)damageInfo;
    if (Config.ExtraMenu.Hit && damageInfo != NULL) {
        // dump.cs DamageInfo: HitPos 0x30 (Vector3), HitNormal 0x3C, SourcePos 0x48,
        // HitGroup 0x54 (offset already used by the project, confirms the block).
        const uintptr_t di = (uintptr_t)damageInfo;
        const Vector3 hitPos = *(const Vector3*)(di + 0x30);

        // Selected hit group (COMBAT > Hit Group). Auto decides from where the shot
        // landed; anything else is written through verbatim.
        int hitGroup = EHitGroup_Body;

        switch (Config.Aim.HitGroup) {
            case HitGroupHead:      hitGroup = EHitGroup_Head;      break;
            case HitGroupHand:      hitGroup = EHitGroup_Hand;      break;
            case HitGroupBody:      hitGroup = EHitGroup_Body;      break;
            case HitGroupFoot:      hitGroup = EHitGroup_Foot;      break;
            case HitGroupWeakPoint: hitGroup = EHitGroup_WeakPoint; break;
            case HitGroupNeck:      hitGroup = EHitGroup_Neck;      break;
            default: {
                // Head band: how far below the head a hit still counts as a headshot.
                const float headBand = ImClamp(Config.ExtraMenu.TuneHitboxHeadBand, 0.0f, 1.5f);

                // Preferred: the enemy pawn whose expanded hitbox accepted this shot.
                Pawn* accPawn = (Pawn*)g_hitboxAcceptedPawn;
                if (Tools::IsPtrValid(accPawn)) {
                    const Vector3 head = accPawn->get_HeadPosition();
                    if (hitPos.y >= head.y - headBand) {
                        // At head level or above (also "next to / above the head") -> headshot.
                        hitGroup = EHitGroup_Head;
                    }
                } else if (g_hitboxHeadHeightY > 0.0f) {
                    // Fallback: remembered head height from the physics check.
                    if (hitPos.y >= g_hitboxHeadHeightY - headBand) {
                        hitGroup = EHitGroup_Head;
                    }
                } else {
                    // Last resort: shooter-relative estimate (head is ~1.5m above the eye line
                    // source in CODM's model space; anything clearly upper counts as head).
                    const Vector3 src = *(const Vector3*)(di + 0x48);
                    if (hitPos.y - src.y >= 0.65f) {
                        hitGroup = EHitGroup_Head;
                    }
                }
                break;
            }
        }

        *(int*)(di + Class_DamageInfo_m_HitGroup) = hitGroup;

        // One shot -> one decision: clear so a normal miss/bullet does not inherit it.
        g_hitboxAcceptedPawn = nullptr;
        g_hitboxHeadHeightY = 0.0f;
    }
    return damageInfo;
}

// Creates a managed string through the il2cpp runtime. The runtime export is
// resolved once; if it cannot be found the rename simply stays inert instead of
// calling a null pointer.
inline String *CreateManagedString(const char *text) {
    typedef String *(*StringNewFn)(const char *);
    static StringNewFn stringNew = nullptr;
    static bool resolved = false;
    if (!resolved) {
        resolved = true;
        const char *candidates[] = {"libil2cpp.so", "libunity.so"};
        for (const char *lib : candidates) {
            void *handle = dlopen(lib, 4);
            if (handle == nullptr)
                continue;
            stringNew = reinterpret_cast<StringNewFn>(dlsym(handle, "il2cpp_string_new"));
            if (stringNew != nullptr)
                break;
        }
    }
    return (stringNew != nullptr) ? stringNew(text) : nullptr;
}

//-- Report spoof (incoming)
// A report is built by the client that files it, from the synced profile of the
// player being reported. So while this is on the local profile advertises the
// decoy identity (name + game player id) and a report filed against you is
// attributed to the decoy instead of your account.
inline void ApplyReportSpoofIdentity() {
    if (!Config.ExtraMenu.ReportSpoof)
        return;
    if (Config.ExtraMenu.ReportSpoofTargetId == 0 && Config.ExtraMenu.ReportSpoofName[0] == '\0')
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local))
        return;

    PlayerInfo *info = *(PlayerInfo **) ((uintptr_t) local + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid(info))
        return;

    const uintptr_t base = (uintptr_t) info;

    if (Config.ExtraMenu.ReportSpoofTargetId != 0) {
        // Keep both the public GamePlayerID property and the protected game player
        // id on the decoy so every reader (report packet, scoreboard) agrees.
        *(unsigned long long *) (base + Class_PlayerInfo_m_GamePlayerIDBacking) = Config.ExtraMenu.ReportSpoofTargetId;
        *(unsigned long long *) (base + Class_PlayerInfo_m_GamePlayerId) = Config.ExtraMenu.ReportSpoofTargetId;
    }

    if (Config.ExtraMenu.ReportSpoofName[0] != '\0') {
        String **nameSlot = (String **) (base + Class_PlayerInfo_m_NickName);
        String *current = *nameSlot;
        const char *currentText = (current != nullptr) ? current->CString() : nullptr;
        if (currentText == nullptr || std::string(currentText) != std::string(Config.ExtraMenu.ReportSpoofName)) {
            String *replacement = CreateManagedString(Config.ExtraMenu.ReportSpoofName);
            if (replacement != nullptr)
                *nameSlot = replacement;
        }
    }
}

//-- Report spoof (outgoing)
// An outgoing player report is serialized by CSAccountReportUserReq.Write(writer).
// With the toggle on, the reported account id is replaced just before the packet
// is written, so a report this client files is filed against the decoy instead.
inline void (*orig_CSAccountReportUserReq_Write)(void *instance, void *writer) = nullptr;
inline void hook_CSAccountReportUserReq_Write(void *instance, void *writer) {
    if (Config.ExtraMenu.ReportSpoof && instance != nullptr && Config.ExtraMenu.ReportSpoofTargetId != 0) {
        // CSAccountReportUserReq.reported_player_id (dump.cs, offset 0x60)
        *(unsigned long long *) ((uintptr_t) instance + 0x60) = Config.ExtraMenu.ReportSpoofTargetId;
    }
    if (orig_CSAccountReportUserReq_Write != nullptr)
        orig_CSAccountReportUserReq_Write(instance, writer);
}

//-- Rename card
// Rewrites your own PlayerInfo nickname so the new name is what you (and the
// clients that read it) see. Only runs when a name card is selected.
inline void ApplyRenameCard() {
    if (!Config.ExtraMenu.RenameCard || Config.ExtraMenu.RenameCardGid == 0)
        return;
    if (Config.ExtraMenu.RenameCardName[0] == '\0')
        return;

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local))
        return;

    PlayerInfo *info = *(PlayerInfo **) ((uintptr_t) local + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid(info))
        return;

    String **nameSlot = (String **) ((uintptr_t) info + Class_PlayerInfo_m_NickName);
    String *current = *nameSlot;
    const char *currentText = (current != nullptr) ? current->CString() : nullptr;
    if (currentText != nullptr && std::string(currentText) == std::string(Config.ExtraMenu.RenameCardName))
        return;

    String *replacement = CreateManagedString(Config.ExtraMenu.RenameCardName);
    if (replacement != nullptr)
        *nameSlot = replacement;
}

//-- Long Slide

//-- Forbid kick-off (multi-device / "account logged in on another device")
// The game ships its own Network.ForbidKickOffHandler with static fields
//   CurrentStat (bool), UntilTime (int)
// and a static method ForbidKickOff(bool forbid). When CurrentStat is true the
// client suppresses the kick-off popup/kick for the current session. We activate
// that built-in path on login when ForbidKickOff is enabled, so the cheat client
// is not kicked when another device logs into the same account.
//
// IMPORTANT: the SDK-generator helpers (Il2CppGetClassType, Il2CppGetStaticFieldOffset,
// Il2CppIsAssembliesLoaded, ...) all route through Il2CppAttach(), which is never
// called anywhere in this build -- their function pointers are null. Calling them
// crashed the render thread on login success whenever this feature was enabled.
// Instead we resolve the public il2cpp C API directly through dlsym, the same way
// CreateManagedString resolves il2cpp_string_new, and if any export is missing we
// stay inert instead of dereferencing null.
//
// Image name is "Assembly-CSharp.dll" (the game's managed assembly in this build).
// Namespace is "Network", class is "ForbidKickOffHandler".
namespace ForbidKickOffCfg
{
    constexpr int32_t UntilTime_Force = 2147483647; // INT32_MAX: forbid stays active until explicitly cleared
    constexpr int32_t UntilTime_Disabled = 0;
}

typedef void *(*Il2CppDomainGetFn)();
typedef void *(*Il2CppDomainAssemblyOpenFn)(void *, const char *);
typedef void *(*Il2CppAssemblyGetImageFn)(void *);
typedef void *(*Il2CppClassFromNameFn)(const void *, const char *, const char *);
typedef void *(*Il2CppClassGetFieldFn)(void *, const char *);
typedef void (*Il2CppFieldStaticSetFn)(void *, void *);

struct Il2CppForbidApi
{
    Il2CppDomainGetFn domainGet;
    Il2CppDomainAssemblyOpenFn domainAssemblyOpen;
    Il2CppAssemblyGetImageFn assemblyGetImage;
    Il2CppClassFromNameFn classFromName;
    Il2CppClassGetFieldFn classGetFieldFromName;
    Il2CppFieldStaticSetFn fieldStaticSetValue;
    bool ok;
};

inline Il2CppForbidApi ResolveIl2CppForbidApi()
{
    Il2CppForbidApi api = {};
    const char *libs[] = {"libil2cpp.so", "libunity.so"};
    for (const char *lib : libs)
    {
        // RTLD_NOLOAD (4): only succeed if the library is already loaded,
        // mirroring CreateManagedString's resolution pattern.
        void *handle = dlopen(lib, 4);
        if (handle == nullptr)
            continue;

        Il2CppForbidApi candidate = {};
        candidate.domainGet = reinterpret_cast<Il2CppDomainGetFn>(dlsym(handle, "il2cpp_domain_get"));
        candidate.domainAssemblyOpen = reinterpret_cast<Il2CppDomainAssemblyOpenFn>(dlsym(handle, "il2cpp_domain_assembly_open"));
        candidate.assemblyGetImage = reinterpret_cast<Il2CppAssemblyGetImageFn>(dlsym(handle, "il2cpp_assembly_get_image"));
        candidate.classFromName = reinterpret_cast<Il2CppClassFromNameFn>(dlsym(handle, "il2cpp_class_from_name"));
        candidate.classGetFieldFromName = reinterpret_cast<Il2CppClassGetFieldFn>(dlsym(handle, "il2cpp_class_get_field_from_name"));
        candidate.fieldStaticSetValue = reinterpret_cast<Il2CppFieldStaticSetFn>(dlsym(handle, "il2cpp_field_static_set_value"));

        if (candidate.domainGet && candidate.domainAssemblyOpen && candidate.assemblyGetImage &&
            candidate.classFromName && candidate.classGetFieldFromName && candidate.fieldStaticSetValue)
        {
            candidate.ok = true;
            return candidate;
        }
    }
    return api; // all-null, ok=false: feature stays inert, never crashes
}

inline void ApplyForbidKickOff(bool enable)
{
    if (!Config.ExtraMenu.ForbidKickOff)
        return;

    static const Il2CppForbidApi api = ResolveIl2CppForbidApi();
    if (!api.ok)
        return;

    void *domain = api.domainGet();
    if (!domain)
        return;

    void *assembly = api.domainAssemblyOpen(domain, "Assembly-CSharp.dll");
    if (!assembly)
        assembly = api.domainAssemblyOpen(domain, "Assembly-CSharp");
    if (!assembly)
        return;

    void *image = api.assemblyGetImage(assembly);
    if (!image)
        return;

    void *klass = api.classFromName(image, "Network", "ForbidKickOffHandler");
    if (!klass)
        return;

    // Authoritative state the game's kick handler reads.
    void *currentStatField = api.classGetFieldFromName(klass, "CurrentStat");
    void *untilTimeField = api.classGetFieldFromName(klass, "UntilTime");

    bool statValue = enable;
    int32_t untilValue = enable ? ForbidKickOffCfg::UntilTime_Force
                                : ForbidKickOffCfg::UntilTime_Disabled;
    if (currentStatField)
        api.fieldStaticSetValue(currentStatField, &statValue);
    if (untilTimeField)
        api.fieldStaticSetValue(untilTimeField, &untilValue);
    // Note: we deliberately do NOT call the managed ForbidKickOff(bool) method
    // pointer directly -- without il2cpp_runtime_invoke its native ABI includes a
    // hidden trailing MethodInfo* parameter we cannot supply safely. The field
    // writes above are what the game's kick handler actually reads.
}

// Re-activate the forbid after a successful login so the client is not kicked
// when another device is still sharing the account. Called on the login-success
// paths in Login() and ConsumePendingLoginResult() and in the in-window success
// handler.
inline void ApplyForbidKickOffOnLogin()
{
    if (!Config.ExtraMenu.ForbidKickOff)
        return;
    // One-shot: only do this when the user asked for it on this login attempt.
    if (Config.ExtraMenu.ForbidKickOffOnLogin)
    {
        ApplyForbidKickOff(true);
        Config.ExtraMenu.ForbidKickOffOnLogin = false;
    }
}

//-- Long Slide
inline float (*o_get_SlideTackleAcclerationSpeed)(void*) = nullptr;
inline float h_get_SlideTackleAcclerationSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 1;
    }
    return o_get_SlideTackleAcclerationSpeed(ins);
}

inline float (*o_PawnGetMaxSpeed)(void*) = nullptr;
inline float h_PawnGetMaxSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_PawnGetMaxSpeed(ins);
}

inline float (*o_get_SlideTackleSpeed)(void*) = nullptr;
inline float h_get_SlideTackleSpeed(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange;
    }
    return o_get_SlideTackleSpeed(ins);
}

inline float (*o_GetSuperSlideRate)(void*) = nullptr;
inline float h_GetSuperSlideRate(void* ins) {
    if (SlideRange > 0.0f) {
        return SlideRange + 256;
    }
    return o_GetSuperSlideRate(ins);
}

inline void (*o_TickLocalPlayer)(void*, float) = nullptr;
inline void h_TickLocalPlayer(void* ins, float deltaTime) {
    if (SlideRange <= 0.0f) {
        o_TickLocalPlayer(ins, deltaTime);
    }
}

//-- No Overheat
inline float (*orig_get_AddHotTime)(void* instance) = nullptr;
inline float get_AddHotTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Rpd) {
            return 0.00001f;
        }
    }
    return orig_get_AddHotTime(instance);
}

//-- No Parachute
inline void (*orig_OpenParachute)(void* instance, bool isAuto) = nullptr;
inline void OpenParachute(void* instance, bool isAuto) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Parachute) {
            return;
        }
    }
    return orig_OpenParachute(instance, isAuto);
}

//-- Quick Reload
inline float (*orig_get_ChangeClipTime)(void* instance) = nullptr;
inline float get_ChangeClipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipTime(instance);
}

inline float (*orig_get_ChangeClipLoopTime)(void* instance) = nullptr;
inline float get_ChangeClipLoopTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Reload) {
            return 0.0001f;
        }
    }
    return orig_get_ChangeClipLoopTime(instance);
}

//-- Quick Scope
inline float (*orig_get_AimingTime)(void* instance) = nullptr;
inline float get_AimingTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Scope) {
            return 0.0001f;
        }
    }
    return orig_get_AimingTime(instance);
}

//-- Quick Switch
inline float (*orig_get_EquipTime)(void* instance) = nullptr;
inline float get_EquipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_EquipTime(instance);
}

inline float (*orig_get_UnequipTime)(void* instance) = nullptr;
inline float get_UnequipTime(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Switch) {
            return 0.0001f;
        }
    }
    return orig_get_UnequipTime(instance);
}

//-- Red Wallhack
bool (*orig_IsInEM3Eye)(void *instance);
bool get_IsInEM3Eye(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return true;
        }
    }
    return orig_IsInEM3Eye(instance);
}

float (*orig_GetAccDistance)(void *instance);
float GetAccDistance(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.RedWallhack) {
            return 50.0f;
        }
    }
    return orig_GetAccDistance(instance);
}

//-- Skip Tutorial
inline bool IsTutorialEnabled() {
    return false;
}

//-- Sky Diving Speed
inline float (*orig_get_AccelerationForwardSpeedUp)(void* instance) = nullptr;
inline float get_AccelerationForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_AccelerationForwardSpeedUp(instance);
}

inline float (*orig_get_MaxVelocityForwardSpeedUp)(void* instance) = nullptr;
inline float get_MaxVelocityForwardSpeedUp(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Diving) {
            return 200.0f;
        }
    }
    return orig_get_MaxVelocityForwardSpeedUp(instance);
}

//-- Snowboard Boost
inline float (*get_m_PhysSkisMaxSpeed)(void*) = nullptr;
inline float hooked_get_m_PhysSkisMaxSpeed(void* instance) {
    if (SnowBsize > 0.0f) {
        return SnowBsize;
    }
    return get_m_PhysSkisMaxSpeed(instance);
}

//-- Speed Hack
inline float (*original_CalcFinalMoveScale)(void*) = nullptr;
inline float hooked_CalcFinalMoveScale(void* instance) {
    if (instance == nullptr) {
        return original_CalcFinalMoveScale(instance);
    }
    if (speedHackMultiplier > 1.0f && speedHackMultiplier <= 100.0f) {
        return speedHackMultiplier;
    }
    return original_CalcFinalMoveScale(instance);
}

//-- Weapon Kinetic
inline bool (*orig_get_IsKineticArmor)(void* instance) = nullptr;
inline bool get_IsKineticArmor(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Kinetic) {
            return true;
        }
    }
    return orig_get_IsKineticArmor(instance);
}

//-- Zero Recoil
inline float (*orig_GetScaleRecoil)(void* instance) = nullptr;
inline float GetScaleRecoil(void* instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Recoil) {
            return 0.00001f;
        }
    }
    return orig_GetScaleRecoil(instance);
}

//-- Zero Spread
inline float (*orig_MinInaccuracy)(void *) = nullptr;
inline float MinInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MinInaccuracy(instance);
}

inline float (*orig_MaxInaccuracy)(void *) = nullptr;
inline float MaxInaccuracy(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_MaxInaccuracy(instance);
}

inline float (*orig_DisperseBase)(void *) = nullptr;
inline float DisperseBase(void * instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Spread) {
            return 0.00001f;
        }
    }
    return orig_DisperseBase(instance);
} 

// Unlock Blueprints
inline bool (*orig_IsUnlocked)(void *instance);
inline bool IsUnlocked(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Blueprints) {
            return true;
        }
    }
    return orig_IsUnlocked(instance);
}

inline bool (*orig_IsWeaponAttachmentUnlock)(void *instance);
inline bool IsWeaponAttachmentUnlock(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Attachment) {
            return true;
        }
    }
    return orig_IsWeaponAttachmentUnlock(instance);
}
/*
//-- Unlimited Bullets
inline bool (*orig_IsInfiniteBullet)(void* instance) = nullptr;
inline bool IsInfiniteBullet(void* instance) {
    if (Config.ExtraMenu.UnliAmmo) {
        return true;
    }
    if (instance != NULL) {
        return orig_IsInfiniteBullet(instance);
    }
    return false;
}

inline bool (*orig_IsNoCostAmmo)(void* instance) = nullptr;
inline bool IsNoCostAmmo(void* instance) {
    if (Config.ExtraMenu.UnliAmmo) {
        return true;
    }
    if (instance != NULL) {
        return orig_IsNoCostAmmo(instance);
    }
    return false;
}

//-- Unlimited Ammo (Functions)
inline bool (*orig_AmmoCanFire)(uintptr_t thiz) = nullptr;
inline bool hk_AmmoCanFire(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_AmmoCanFire(thiz);
}

inline bool (*orig_HasAmmo)(uintptr_t thiz) = nullptr;
inline bool hk_HasAmmo(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_HasAmmo(thiz);
}

inline bool (*orig_HasAmmo_IgnoreInfinite)(uintptr_t thiz) = nullptr;
inline bool hk_HasAmmo_IgnoreInfinite(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_HasAmmo_IgnoreInfinite(thiz);
}

inline void (*orig_ServerStopFire)(uintptr_t thiz, int costAmmo) = nullptr;
inline void hk_ServerStopFire(uintptr_t thiz, int costAmmo) {
    if (UnlimitedAmmo) {
        return orig_ServerStopFire(thiz, 0);
    }
    return orig_ServerStopFire(thiz, costAmmo);
}

inline int (*orig_get_ShotCost)(uintptr_t thiz) = nullptr;
inline int hk_get_ShotCost(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return 0;
    }
    return orig_get_ShotCost(thiz);
}

inline bool (*orig_IsAmmoFree)(uintptr_t thiz) = nullptr;
inline bool hk_IsAmmoFree(uintptr_t thiz) {
    if (UnlimitedAmmo) {
        return true;
    }
    return orig_IsAmmoFree(thiz);
}

float (*orig_get_SmokeEffectScale)(void *instance);
float get_SmokeEffectScale(void *instance) {
    if (instance != nullptr && Config.ExtraMenu.NoSmoke) {
        return 0.0f;
    }
    return orig_get_SmokeEffectScale(instance);
}
*/
// No Spectate Deelay
inline bool (*o_NeedDelayProcess)(void *instance, int a, int b);
inline bool h_NeedDelayProcess(void *instance, int a, int b) {
    bool orig_val = o_NeedDelayProcess(instance, a, b);

    if (Config.ExtraMenu.Spectatex) {
        return 0; // Return 0 instead of false for int return type
    }

    return orig_val;
}

inline bool (*o_ProcessForDelay)(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex);
inline bool h_ProcessForDelay(void *instance, byte* packData, int PackDataCount, int Sequence, float timeQueued, int aiIndex) {
    bool orig_val = o_ProcessForDelay(instance, packData, PackDataCount, Sequence, timeQueued, aiIndex);

    if (Config.ExtraMenu.Spectatex) {
        return false;
    }

    return orig_val;
}

inline bool (*o_get_IsObserver)(void *);
inline bool h_get_IsObserver(void *instance) {
    bool orig_val = o_get_IsObserver(instance);

    if (Config.ExtraMenu.Spectatex) {
        return true;
    }

    return orig_val;
}

inline float (*o_GetDelayCountDown)(void *);
inline float h_GetDelayCountDown(void *instance) {
    float orig_val = o_GetDelayCountDown(instance);

    if (Config.ExtraMenu.Spectatex) {
        return 0;
    }

    return orig_val;
}

int (*orig_get_DeviceCapacityLevel)(void *instance);
int get_DeviceCapacityLevel(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.Grap) {
            return 6;
        }
    }
    return orig_get_DeviceCapacityLevel(instance);
}

// Hook for CheckTargetIsValid_DyingInAzurGameMode
inline bool (*orig_CheckTargetIsValid_DyingInAzurGameMode)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_DyingInAzurGameMode_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_DyingInAzurGameMode(instance, targetPawn);
}

// Hook for CheckTargetIsValid_PhysState
inline bool (*orig_CheckTargetIsValid_PhysState)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_PhysState_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_PhysState(instance, targetPawn);
}

// Hook for CheckTargetIsValid_ManualParameter
inline bool (*orig_CheckTargetIsValid_ManualParameter)(void* instance, void* tempPawn, float* distance);
inline bool CheckTargetIsValid_ManualParameter_Hook(void* instance, void* tempPawn, float* distance) {
    if (instance != nullptr && tempPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_ManualParameter(instance, tempPawn, distance);
}

// Hook for CheckTargetIsValid_Ult
inline bool (*orig_CheckTargetIsValid_Ult)(void* instance, void* targetPawn);
inline bool CheckTargetIsValid_Ult_Hook(void* instance, void* targetPawn) {
    if (instance != nullptr && targetPawn != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return orig_CheckTargetIsValid_Ult(instance, targetPawn);
}



inline bool (*orig_CalculateObstacleSide)(void* instance, float* backDis, float* beforeDis);
inline bool CalculateObstacleSide_Hook(void* instance, float* backDis, float* beforeDis) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleSide(instance, backDis, beforeDis);
}

inline bool (*orig_CalculateObstacleTop)(void* instance, Vector3 pawnPos);
inline bool CalculateObstacleTop_Hook(void* instance, Vector3 pawnPos) {
    if (instance != nullptr) {
        if (isExecute) {
            return false;
        }
    }
    return orig_CalculateObstacleTop(instance, pawnPos);
}

int (*oCheckTargetIsValid)(void *ins, void* tempPawn, float distance);
int CheckTargetIsValid(void *ins, void* tempPawn, float distance) {
    if ( ins != nullptr ) {
        if (isExecute) {
            return (int)true;
        }
    }
    return oCheckTargetIsValid(ins, tempPawn, distance);
}

inline bool (*oCheckCanExcution_Extra)(void *ins);
inline bool CheckCanExcution_Extra(void *ins) {
    if ( ins != nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return oCheckCanExcution_Extra(ins);
}


inline bool (*o_CheckExecution_ObstacleAround)(void *ins);
inline bool CheckExecution_ObstacleAround(void *ins) {
    if (ins!=nullptr) {
        if (isExecute) {
            return true;
        }
    }
    return o_CheckExecution_ObstacleAround(ins);
}
/*
//ADJUSTABLE HOOK FOR FRAMES
 int (*original_Fpslevel)(void*) = nullptr;
 int hooked_Fpslevel(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? 7 : original_Fpslevel(ins);
}

 int (*original_Fpslevel1)(void*) = nullptr;
 int hooked_Fpslevel1(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? 7 : original_Fpslevel1(ins);
}

 int (*orig_GetFrameRateValue)(void*, int, int) = nullptr;
 int GetFrameRateValue(void* ins, int f, int m) {
    if (ins) {
        return 360;
    }
    return orig_GetFrameRateValue(ins, f, m);
}

 int (*orig_GetUltraFrameRateFinalFPS)(void*) = nullptr;
 int GetUltraFrameRateFinalFPS(void* g) {
    return (g && Config.ExtraMenu.EnhanceFPS) ? 360 : orig_GetUltraFrameRateFinalFPS(g);
}

inline bool (*original_Extremequality)(void*) = nullptr;
inline bool hooked_Extremequality(void* ins) {
    return (ins && Config.ExtraMenu.EnhanceFPS) ? true : original_Extremequality(ins);
}
*/
/// Walk Under Water
inline bool (*oPawn_GetCurrentDistToWaterSurface)(void *instance);
inline bool Pawn_GetCurrentDistToWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPawn_GetCurrentDistToWaterSurface(instance);
}

inline bool (*oPlayerPawn_IsUnderWaterSurface)(void *instance);
inline bool PlayerPawn_IsUnderWaterSurface(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return false;
        }
    }
    return oPlayerPawn_IsUnderWaterSurface(instance);
}

inline bool (*oCheckInWaterComponent_get_CurrentWaterSurfaceHeight)(void *instance);
inline bool CheckInWaterComponent_get_CurrentWaterSurfaceHeight(void *instance) {
    if (instance != NULL) {
        if (Config.ExtraMenu.WalkUnderWater) {
        	return true;
        }
    }
    return oCheckInWaterComponent_get_CurrentWaterSurfaceHeight(instance);
}

///Camera Pov
float (*oInputSettingConfig_Instant_GetMainCameraFov_3p)(void *instance);
float InputSettingConfig_Instant_GetMainCameraFov_3p(void *instance) {
    if (instance && Config.ExtraMenu.CameraPov) {
        return (float)Config.ExtraMenu.CameraPovSize;
    }
    return oInputSettingConfig_Instant_GetMainCameraFov_3p(instance);
}

//-- No Smoke
inline bool NameHasSmokeToken(const char *name)
{
    if (!name)
        return false;

    std::string s(name);
    for (char &c : s) {
        if (c >= 'A' && c <= 'Z')
            c = static_cast<char>(c - 'A' + 'a');
    }

    return s.find("smoke") != std::string::npos ||
           s.find("smog") != std::string::npos ||
           s.find("smk") != std::string::npos;
}

inline String *(*UnityObject_get_name)(void *instance) = nullptr;
inline void (*orig_GameObjectSetActive)(void *instance, bool value) = nullptr;
inline void hook_GameObjectSetActive(void *instance, bool value)
{
    if (Config.ExtraMenu.NoSmoke && value && instance && UnityObject_get_name) {
        String *name = UnityObject_get_name(instance);
        if (name && NameHasSmokeToken(name->CString()))
            value = false;
    }
    return orig_GameObjectSetActive(instance, value);
}

/*
typedef void (*SetUltraFrameRateDeviceInfo_t)(void*, bool, int, int, int, bool);
inline SetUltraFrameRateDeviceInfo_t orig_SetUltraFrameRateDeviceInfo = nullptr;

 void hooked_SetUltraFrameRateDeviceInfo(void* thiz, bool enableUltraFrameRate, int ultraFrameRate, int ultraFrameRateBR, int ultraFrameRateQualityLimit, bool customizedFrameRate) {
    if (thiz != NULL) {
        enableUltraFrameRate = true;
        ultraFrameRate = 240;
        ultraFrameRateBR = 240;
        ultraFrameRateQualityLimit = 240;
        customizedFrameRate = true;
    }
    orig_SetUltraFrameRateDeviceInfo(thiz, enableUltraFrameRate, ultraFrameRate, ultraFrameRateBR, ultraFrameRateQualityLimit, customizedFrameRate);
} 
*/
inline void InitializeAllHooks() {

    //-- Smart Reload
    HOOK_LIB("libunity.so", "0x5260B78", hook_Pawn_StopFire, orig_Pawn_StopFire);

    //-- Aim Assist
    HOOK_LIB("libunity.so", "0x666FD90", GetAssitAimSpeed, orig_GetAssitAimSpeed);

    //-- Anti Flashbang
    HOOK_LIB("libunity.so", "0x51E977C", hook_OnFlashBangExplode, orig_OnFlashBangExplode);
    
    //-- Firerate Speed
    HOOK_LIB("libunity.so", "0x51237F4", get_FireBoltTime, orig_get_FireBoltTime);
    HOOK_LIB("libunity.so", "0x5105C74", get_FireInterval, orig_get_FireInterval);
    HOOK_LIB("libunity.so", "0x513CBC4", get_DelaySprintFire, orig_get_DelaySprintFire);

    //-- High Jump
    HOOK_LIB("libunity.so", "0x5221D00", hook_GetMaxJumpHeight, orig_GetMaxJumpHeight);

    //-- Increase Damage
    HOOK_LIB("libunity.so", "0xC1514C0", SingleLineCheckPhysics, orig_SingleLineCheckPhysics);

    HOOK_LIB("libunity.so", "0x5109DE4", CalcDamageInfoInstantHit, orig_CalcDamageInfoInstantHit);

    //-- A-Fire (triggerbot / auto-fire)
    // Weapon::Tick / StartFire / StopFire (RVAs from dump.cs).
    orig_Weapon_StartFire = reinterpret_cast<void (*)(void *)>(getAbsoluteAddress("libunity.so", string2Offset("0x5123C84")));
    orig_Weapon_StopFire = reinterpret_cast<void (*)(void *, bool)>(getAbsoluteAddress("libunity.so", string2Offset("0x5124EAC")));
    HOOK_LIB("libunity.so", "0x5114C34", hook_Weapon_Tick, orig_Weapon_Tick);

    //-- Report spoof (CSAccountReportUserReq.Write)
    HOOK_LIB("libunity.so", "0x4298DDC", hook_CSAccountReportUserReq_Write, orig_CSAccountReportUserReq_Write);

/*
    //-- Long Slide
    HOOK_LIB("libunity.so", "0x5167AA4", h_get_SlideTackleAcclerationSpeed, o_get_SlideTackleAcclerationSpeed);
    HOOK_LIB("libunity.so", "0xC2E3374", h_PawnGetMaxSpeed, o_PawnGetMaxSpeed);
    HOOK_LIB("libunity.so", "0x8BF71F0", h_get_SlideTackleSpeed, o_get_SlideTackleSpeed);
    HOOK_LIB("libunity.so", "0x8BF62D4", h_GetSuperSlideRate, o_GetSuperSlideRate);
    HOOK_LIB("libunity.so", "0x8F141B8", h_TickLocalPlayer, o_TickLocalPlayer);
*/
    //-- No Overheat
    // HOOK_LIB("libunity.so", "0xC14B314", get_AddHotTime, orig_get_AddHotTime);

    //-- No Parachute
    HOOK_LIB("libunity.so", "0x68823FC", OpenParachute, orig_OpenParachute);

    //-- Quick Reload
    HOOK_LIB("libunity.so", "0x50ECADC", get_ChangeClipTime, orig_get_ChangeClipTime);

    //-- Quick Scope
    HOOK_LIB("libunity.so", "0x50EB944", get_AimingTime, orig_get_AimingTime);

    //-- Quick Switch
    HOOK_LIB("libunity.so", "0x50ED8D4", get_EquipTime, orig_get_EquipTime);

    //-- Red Wallhack
    HOOK_LIB("libunity.so", "0x9677554", get_IsInEM3Eye, orig_IsInEM3Eye); 
    HOOK_LIB("libunity.so", "0xAD1DD78", GetAccDistance, orig_GetAccDistance);

    //-- Skip Tutorial
    HOOK_LIB_NO_ORIG("libunity.so", "0x9DE0E58", IsTutorialEnabled);
    
    //-- Sky Diving Speed
    HOOK_LIB("libunity.so", "0x5DE981C", get_AccelerationForwardSpeedUp, orig_get_AccelerationForwardSpeedUp);
    HOOK_LIB("libunity.so", "0x5DE9880", get_MaxVelocityForwardSpeedUp, orig_get_MaxVelocityForwardSpeedUp);

    //-- Snowboard Boost
    HOOK_LIB("libunity.so", "0x522860C", hooked_get_m_PhysSkisMaxSpeed, get_m_PhysSkisMaxSpeed);

    //-- Speed Hack
    HOOK_LIB("libunity.so", "0x51D2EB8", hooked_CalcFinalMoveScale, original_CalcFinalMoveScale);

    //-- Weapon Kinetic
    HOOK_LIB("libunity.so", "0x51B3D64", get_IsKineticArmor, orig_get_IsKineticArmor);

    //-- Zero Recoil
    HOOK_LIB("libunity.so", "0xC9BAFF8", GetScaleRecoil, orig_GetScaleRecoil);

    //-- Zero Spread
    HOOK_LIB("libunity.so", "0xC9B8F78", MinInaccuracy, orig_MinInaccuracy);
    HOOK_LIB("libunity.so", "0xC159288", MaxInaccuracy, orig_MaxInaccuracy);
    HOOK_LIB("libunity.so", "0xC9C76C4", DisperseBase, orig_DisperseBase);
    
    //-- Spectate
    HOOK_LIB("libunity.so", "0x519676C", h_ProcessForDelay, o_ProcessForDelay);
    HOOK_LIB("libunity.so", "0x51966D8", h_NeedDelayProcess, o_NeedDelayProcess);
    HOOK_LIB("libunity.so", "0x519676C", h_ProcessForDelay, o_ProcessForDelay);  

/*
    //-- Unlimited Bullets
    HOOK_LIB("libunity.so", "0x5984C18", IsInfiniteBullet, orig_IsInfiniteBullet);
    HOOK_LIB("libunity.so", "0xBF93BA4", IsNoCostAmmo, orig_IsNoCostAmmo);

	//-- Unli Ammo (Functions)
	HOOK_LIB("libunity.so", "0x50EB73C", hk_AmmoCanFire, orig_AmmoCanFire);          // AmmoCanFire - RVA: 0x50EB73C
    HOOK_LIB("libunity.so", "0x5107904", hk_HasAmmo, orig_HasAmmo);                  // HasAmmo - RVA: 0x5107904
    HOOK_LIB("libunity.so", "0xC14E570", hk_HasAmmo_IgnoreInfinite, orig_HasAmmo_IgnoreInfinite); // HasAmmo_IgnoreInfinite - RVA: 0xC14E570
    HOOK_LIB("libunity.so", "0xC14D614", hk_ServerStopFire, orig_ServerStopFire);    // ServerStopFire - RVA: 0xC14D614
    HOOK_LIB("libunity.so", "0x50EC794", hk_get_ShotCost, orig_get_ShotCost);        // get_ShotCost - RVA: 0x50EC794
    HOOK_LIB("libunity.so", "0x510CE2C", hk_IsAmmoFree, orig_IsAmmoFree);            // IsAmmoFree - RVA: 0x510CE2C
 
     //-- Long Execute
    HOOK_LIB("libunity.so", "0x5947970", CheckTargetIsValid_DyingInAzurGameMode_Hook, orig_CheckTargetIsValid_DyingInAzurGameMode);
    HOOK_LIB("libunity.so", "0x5947A24", CheckTargetIsValid_PhysState_Hook, orig_CheckTargetIsValid_PhysState);
    HOOK_LIB("libunity.so", "0x5947C18", CheckTargetIsValid_ManualParameter_Hook, orig_CheckTargetIsValid_ManualParameter);


    //-- Optimize Fps Lock
    HOOK_LIB("libunity.so", "0x9FE01CC", GetFrameRateValue, orig_GetFrameRateValue);
    HOOK_LIB("libunity.so", "0x9FDFBBC", GetUltraFrameRateFinalFPS, orig_GetUltraFrameRateFinalFPS);
    HOOK_LIB("libunity.so", "0x9FDFC58", hooked_Fpslevel, original_Fpslevel);
    HOOK_LIB("libunity.so", "0x9FDFC58", hooked_Fpslevel1, original_Fpslevel1);
    HOOK_LIB("libunity.so", "0x9FE3FB4", hooked_Extremequality, original_Extremequality);
    HOOK_LIB("libunity.so", "0x9FDEE28", hooked_SetUltraFrameRateDeviceInfo, orig_SetUltraFrameRateDeviceInfo);
*/    
    //-- No smoke
    UnityObject_get_name = reinterpret_cast<String *(*)(void *)>(
          getAbsoluteAddress("libunity.so", 0x5052F10));

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x5036EB8),
          (void *)&hook_GameObjectSetActive,
          (void **)&orig_GameObjectSetActive);
          
    //-- Throwable Alert      
    DobbyHook((void *)getAbsoluteAddress("libunity.so", ThrowAlertCfg::TickRva),
          (void *)&hook_WeaponProjectileTick,
          (void **)&orig_WeaponProjectileTick);
          
    //-- Clear Terrain
    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0xAE2E1B0),
          (void *)&hook_SetGrassShowState,
          (void **)&orig_SetGrassShowState);

    DobbyHook((void *)getAbsoluteAddress("libunity.so", 0x7B4E290),
          (void *)&hook_SetGrassLODBias,
          (void **)&orig_SetGrassLODBias);         
          
    DobbyHook((void*)getAbsoluteAddress("libunity.so", 0x9FDEE28), (void*)hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo);

    ///Walk Under Water
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x51D31BC), (void *)  &Pawn_GetCurrentDistToWaterSurface, (void **) &oPawn_GetCurrentDistToWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x54BC450), (void *)  &PlayerPawn_IsUnderWaterSurface, (void **) &oPlayerPawn_IsUnderWaterSurface); 
    DobbyHook((void *) getAbsoluteAddress("libunity.so", 0x757162C), (void *)  &CheckInWaterComponent_get_CurrentWaterSurfaceHeight, (void **) &oCheckInWaterComponent_get_CurrentWaterSurfaceHeight); 

  ///Camera Pov
  DobbyHook((void *) getAbsoluteAddress("libunity.so", 0xC347950), (void *)  &InputSettingConfig_Instant_GetMainCameraFov_3p, (void **) &oInputSettingConfig_Instant_GetMainCameraFov_3p); 

    //-- Unlock Blueprints
    HOOK_LIB("libunity.so", "0x901F988", IsUnlocked, orig_IsUnlocked);
    
    HOOK_LIB("libunity.so", "0xA61AA00", IsWeaponAttachmentUnlock, orig_IsWeaponAttachmentUnlock);
}