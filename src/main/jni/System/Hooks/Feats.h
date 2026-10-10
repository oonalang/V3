#pragma once

#include <cstdint>
#include <string>
#include <dlfcn.h>
#include <android/log.h>
#include "Includes/SafePatch.h"   // SafeModeEnabled + verified patch/hook targets

// Breadcrumbs for the triggerbot. Every state change of the driver is logged
// once with a stable tag, so a crash report can say *when* the game died
// (arming in a match, holding fire, releasing) instead of leaving it to guesswork.
inline void A_FireLog(const char *what) {
    __android_log_print(ANDROID_LOG_INFO, "MWD-ASTRAL", "[afire] %s", what);
}

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

// Maps the Aim tab's "Hit Group" selector onto the game's EHitGroup values.
inline int A_HitGroupFromSelect(int sel)
{
    switch (sel) {
        case HitGroupHead:      return EHitGroup_Head;
        case HitGroupHand:      return EHitGroup_Hand;
        case HitGroupBody:      return EHitGroup_Body;
        case HitGroupFoot:      return EHitGroup_Foot;
        case HitGroupWeakPoint: return EHitGroup_WeakPoint;
        case HitGroupNeck:      return EHitGroup_Neck;
        default:                return EHitGroup_Default;
    }
}

//-- Hitbox / "increase damage"
inline bool (*orig_SingleLineCheckPhysics)(void* instance, int hitType, void* hitTarget, void* hitCollider, Vector3 startPos, Vector3 dir, void* impactInfo) = nullptr;


inline bool   g_hitboxHitHead       = false;    
inline uintptr_t g_hitboxDamageInfo  = 0;

//-- A-Fire (triggerbot / auto-fire)
inline bool     g_afireHoldingFire   = false;
inline void    *g_afireFiringOn      = nullptr;
inline double   g_afireLastPressTime = -1000.0;
//-- A-Fire is driven from the GAME thread only (see A_FireTick). Anything
//-- running on the render/ImGui thread may only raise a flag, never touch
//-- StartFire/StopFire itself.
inline bool     g_afireReleaseRequested = false;
inline double   g_afireLastEvalTime     = -1000.0;
// True only while the game reports a live match + local pawn + held weapon.
// The driver stays armed but idle until then (loading / lobby / spectating).
inline bool     g_afireInMatch          = false;

// Monotonic clock for the triggerbot. Deliberately NOT ImGui::GetTime(): the
// driver runs on the game thread while the render thread owns the ImGui
// context, and reading ImGui state from both would be a data race.
inline double A_FireNow()
{
    return std::chrono::duration<double>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

inline void (*orig_Weapon_Tick)(void *instance, float deltaTime) = nullptr;
inline void (*orig_Weapon_StartFire)(void *instance) = nullptr;
inline void (*orig_Weapon_StopFire)(void *instance, bool isImmidiately) = nullptr;

// Crosshair zone used by A-Fire. Deliberately generous, and NOT range limited:
// A-Fire fires whenever an enemy's head or body root projects inside this zone
// in front of the camera -- at any distance. The old implementation required a
// Physics.Raycast line-of-sight probe (which "fails closed" whenever the hit
// struct layout differs from the build) plus a 2.5%-of-width circle, so on a
// live build it silently never fired at all.
inline float A_FireCrosshairRadiusPx()
{
    const float w = (float) get_width();
    const float h = (float) get_height();
    // At least 6% of the smaller axis; wider when the aim-assist slider is set.
    float radius = ImMin(w, h) * 0.06f;
    if (Config.Aim.AimAssistSize > 0.0f)
    {
        const float fromSlider = w * (Config.Aim.AimAssistSize / 100.0f) * 0.5f;
        if (fromSlider > radius)
            radius = fromSlider;
    }
    if (radius < 8.0f)
        radius = 8.0f;
    return radius;
}

// Nearest enemy whose head or body is under the crosshair. Works at any range:
// there is no max-distance filter and no line-of-sight requirement, so an enemy
// standing on the far side of the map is still a valid A-Fire target.
inline bool A_FireIsReadablePawn(uintptr_t pawnU)
{
    // An IL2CPP object reference is 8-byte aligned; a stale/torn entry in the
    // engine list usually is not. Rejecting those is what stops the target scan
    // from dereferencing freed pawns -- the crash that started once the enemy
    // list went from empty to populated with moving bots.
    if (pawnU == 0 || (pawnU & 0x7) != 0)
        return false;
    return Tools::IsPtrValid((void *) pawnU);
}

// Nearest enemy whose head or body is under the crosshair, at any range.
// Bones are read through the game's own Pawn accessors -- the exact same ones
// DrawESP already uses -- instead of the raw m_HeadBone / m_Mesh transform
// fields the old scan dereferenced. That raw deref on a moving/animated pawn is
// the crash, and it is why "bots appeared -> game died".
inline uintptr_t A_FireFindTarget()
{
    if (Class_Gameplay_get_MatchGame == 0 || Class_Gameplay_get_LocalPawn == 0)
        return 0;

    Camera *cam = Camera::get_main();
    if (!Tools::IsPtrValid((void *) cam))
        return 0;

    const uintptr_t matchGame = ((uintptr_t (*)()) Class_Gameplay_get_MatchGame)();
    if (!Tools::IsPtrValid((void *) matchGame))
        return 0;
    if (((uintptr_t (*)()) Class_Gameplay_get_LocalPawn)() == 0)
        return 0;

    auto EnemyPawns = *(List<uintptr_t> **) (matchGame + Class_BaseGame_EnemyPawns);
    if (!Tools::IsPtrValid((void *) EnemyPawns))
        return 0;
    auto Items = EnemyPawns->getItems();
    if (!Tools::IsPtrValid((void *) Items))
        return 0;

    const int count = EnemyPawns->getSize();
    if (count <= 0 || count > 1024)
        return 0;

    const float cx = (float) get_width()  * 0.5f;
    const float cy = (float) get_height() * 0.5f;
    const float radius = A_FireCrosshairRadiusPx();
    const float radiusSq = radius * radius;

    uintptr_t best = 0;
    float bestDist = radiusSq + 1.0f;

    for (int i = 0; i < count; i++)
    {
        const uintptr_t pawnU = Items[i];
        if (!A_FireIsReadablePawn(pawnU))
            continue;
        if (!*(bool *) (pawnU + Class_Pawn_m_IsAlive))
            continue;

        Pawn *pawn = (Pawn *) pawnU;
        const Vector3 headPos = pawn->get_HeadPosition();
        const Vector3 bodyPos = pawn->get_LastPawnPos();

        const Vector3 headSc = cam->WorldToScreenPoint(headPos);
        if (headSc.z > 0.0f)
        {
            const float dx = headSc.x - cx;
            const float dy = headSc.y - cy;
            const float d2 = dx * dx + dy * dy;
            if (d2 <= radiusSq && d2 < bestDist)
            {
                best = pawnU;
                bestDist = d2;
                continue; // head covered -- do not also test the body
            }
        }

        const Vector3 bodySc = cam->WorldToScreenPoint(bodyPos);
        if (bodySc.z > 0.0f)
        {
            const float dx = bodySc.x - cx;
            const float dy = bodySc.y - cy;
            const float d2 = dx * dx + dy * dy;
            if (d2 <= radiusSq && d2 < bestDist)
            {
                best = pawnU;
                bestDist = d2;
            }
        }
    }
    return best;
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

// Render-thread-safe "let go of the trigger" request. The UI and the render hook
// must never call StopFire themselves -- they raise this flag and the game-thread
// driver performs the real release, because Weapon::StopFire mutates state the
// game's own fire-input state machine is updating concurrently.
inline void A_FireRequestRelease(void) {
    g_afireReleaseRequested = true;
}

// Triggerbot driver.

inline void A_FireTick(void *tickedWeapon = nullptr);

// Weapon::Tick is the primary heartbeat for the triggerbot: it runs on the game
// thread, every frame, for the weapon the game is currently driving. It is used
// purely as a heartbeat -- the target is always fired through the local pawn's
// current weapon, so it does not matter which weapon instance ticked.
inline void hook_Weapon_Tick(void *instance, float deltaTime) {
    // Always tick the original, even if A_FireTick throttles us out.
    if (orig_Weapon_Tick != nullptr)
        orig_Weapon_Tick(instance, deltaTime);
    A_FireTick(instance);
}

// =====================================================================
// TRIGGERBOT DRIVER -- GAME THREAD ONLY
// =====================================================================
// This used to run from the render hook, and that is what crashed the game the
// moment live bots appeared (toggle on + moving bots -> crash every time):
//
//   1. With Unity's multithreaded renderer the ImGui hook runs on the
//      GfxDeviceWorker thread, so A_FireTick walked the enemy pawn List while the
//      game thread was appending/removing pawns -- i.e. it read a reallocated
//      (freed) items array. Bots spawning and dying is exactly when that happens.
//   2. It called Weapon::StartFire()/StopFire() from that foreign thread. Those
//      are mutating game functions which the game's own fire-input state machine
//      updates on the game thread; calling them concurrently tears the weapon's
//      fire state apart. That is also why the trigger never actually killed
//      anyone: the game thread re-derived its own fire state right afterwards.
//
// Running the whole feature on the game thread (from the Weapon::Tick and
// LocalPlayer tick hooks) removes both races, and makes the fire call land inside
// the game's own update like a real trigger pull.
inline void A_FireTick(void *tickedWeapon)
{
    // Weapon::Tick runs for every weapon in the world, so this is entered many
    // times per frame. One evaluation per ~10ms keeps it at about one scan per
    // frame. Stamping the time *before* the work is also the re-entrancy guard:
    // a nested call (StartFire ticking something) is throttled straight out.
    const double now = A_FireNow();
    if (now - g_afireLastEvalTime < 0.010)
        return;
    g_afireLastEvalTime = now;

    if (!Config.ExtraMenu.A_Fire || g_afireReleaseRequested) {
        g_afireReleaseRequested = false;
        A_FireReleaseTrigger(nullptr);
        return;
    }

    // Every input this driver needs has to exist in THIS build before it is
    // allowed to touch the game. A single unresolved offset used to mean calling
    // through a null or garbage pointer the moment the feature was switched on.
    if (Class_Gameplay_get_MatchGame == 0 || Class_Gameplay_get_LocalPawn == 0) {
        g_afireInMatch = false;
        A_FireReleaseTrigger(nullptr);
        return;
    }
    if (orig_Weapon_StartFire == nullptr || orig_Weapon_StopFire == nullptr) {
        A_FireReleaseTrigger(nullptr);
        return;
    }

    // In-match gate. A live match object, a live local pawn and a valid held
    // weapon is what separates "playing" from "loading / lobby / spectating".
    // Until the game reports all of them the driver stays armed but idle, so it
    // can never poke a half-built level.
    const uintptr_t matchGame = ((uintptr_t (*)()) Class_Gameplay_get_MatchGame)();
    if (!Tools::IsPtrValid((void *) matchGame)) {
        g_afireInMatch = false;
        A_FireReleaseTrigger(nullptr);
        return;
    }

    Pawn *local = GamePlay::get_LocalPawn();
    if (!Tools::IsPtrValid(local) || !local->m_IsAlive()) {
        g_afireInMatch = false;
        A_FireReleaseTrigger(nullptr);
        return;
    }

    Weapon *held = local->get_CurrentWeapon();
    if (!Tools::IsPtrValid(held)) {
        g_afireInMatch = false;
        A_FireReleaseTrigger(nullptr);
        return;
    }
    if (!g_afireInMatch) {
        g_afireInMatch = true;
        A_FireLog("armed: live match, local pawn alive, weapon held");
    }

    // Only the LOCAL player's actively held weapon drives the trigger. Weapon::
    // Tick runs for every bot/vehicle/prop weapon object on the map too; without
    // this guard each of those ticks burned a full crosshair target scan (every
    // 10 ms x the number of ticked weapons) and, worse, could hold the machine
    // gun down from a foreign instance. Cheap pointer compare.
    // NOTE: not a substitute for the match logic below; it is the fast-path gate.
    if (tickedWeapon != nullptr && tickedWeapon != (void *) held) {
        return;
    }

    // Weapon swapped while the trigger was down -- release the old one first, so
    // StopFire is never handed a weapon that is no longer the held one.
    if (g_afireHoldingFire && g_afireFiringOn != (void *) held)
        A_FireReleaseTrigger(nullptr);

    bool ready = A_FireFindTarget() != 0;

    if (ready && Config.ExtraMenu.A_FireTrigger == 1)
        ready = Class_Pawn_IsAiming != 0 && ((bool (*)(uintptr_t)) Class_Pawn_IsAiming)((uintptr_t) local);
    else if (ready && Config.ExtraMenu.A_FireTrigger == 2)
        ready = Class_Pawn_get_IsFiring != 0 && ((bool (*)(uintptr_t)) Class_Pawn_get_IsFiring)((uintptr_t) local);

    if (ready && Config.ExtraMenu.A_FireDelay > 0.0f) {
        if (now - g_afireLastPressTime < (double) Config.ExtraMenu.A_FireDelay)
            ready = false;
    }

    if (ready) {
        if (!g_afireHoldingFire) {
            A_FireLog("fire: target under the crosshair -> StartFire");
            orig_Weapon_StartFire((void *) held);
            g_afireHoldingFire = true;
            g_afireFiringOn = (void *) held;
        }
        g_afireLastPressTime = now;
    } else if (g_afireHoldingFire) {
        A_FireLog("release: target lost -> StopFire");
        A_FireReleaseTrigger(nullptr);
    }
}

// ---------------------------------------------------------------------------
// Hitbox / "increase damage" -- ported from the variant-3 (eliwoahzja/V3) build.
//
// VAR3's version returned true from SingleLineCheckPhysics for every shot. The
// V3 revision makes that adjustable: a shot is accepted when the ray passes
// within Config.ExtraMenu.HitboxScale METRES of a live enemy's head
// (Pawn::get_HeadPosition), so the slider grows the hitbox around the head
// rather than around the world.
//
// Three things here are deliberate, and they are why the previous body-sphere
// version could never land a shot:
//   1. On an accept we return true WITHOUT running the original. The original
//      runs the real sweep and, on a miss, writes miss state into the caller's
//      [Out] slots (hitTarget / hitCollider / impactInfo). Running it first and
//      then forcing a hit handed the shot back with a target the caller had
//      already been told was empty.
//   2. We never write through those out parameters. They are not guaranteed to
//      be Object** slots, so poking a pawn pointer through them is both useless
//      (the caller does not read it) and a memory-corruption crash.
//   3. The test is measured against the head, not the feet. That is what makes
//      the slider an aim-at-the-head knob: a shot that lands inside the inner
//      band of the radius raises g_hitboxHitHead, and the damage hook then
//      reports EHitGroup_Head for a real headshot.
// ---------------------------------------------------------------------------

// Radius of the enlarged hitbox in metres, measured from the shot ray to the
// enemy head. This is exactly what the "Hitbox Size" slider sets.
inline float A_HitboxRadius()
{
    const float scale = Config.ExtraMenu.HitboxScale;
    if (!(scale > 0.0f))                 // also rejects NaN
        return 0.0f;
    return ImClamp(scale, 0.1f, 25.0f);
}

// Inner fraction of the radius that still counts as a head hit for the
// automatic hit group (0..1). 0 disables the automatic headshot.
inline float A_HitboxHeadBandFraction()
{
    const float band = Config.ExtraMenu.TuneHitboxHeadBand;
    if (!(band >= 0.0f))
        return 0.0f;
    return ImClamp(band, 0.0f, 1.0f);
}

// True when the shot ray (startPos + t*dir, t > 0) passes within the hitbox
// radius of a live enemy's head. The V3 test, kept verbatim:
//     alongRay   = dot(head - start, normalized(dir))    -- metres along the ray
//     missDistSq = |head - start|^2 - alongRay^2         -- perpendicular distance^2
//     hit when alongRay > 0 and missDistSq <= radius^2
inline bool A_HitboxRayPassesEnemyHead(const Vector3 &startPos, const Vector3 &dir)
{
    const float radius = A_HitboxRadius();
    if (radius <= 0.0f)
        return false;

    if (!(Vector3::Magnitude(dir) > 0.0001f))
        return false;
    const Vector3 rayAxis = Vector3::Normalized(dir);

    auto get_MatchGame_f = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    uintptr_t matchGame = get_MatchGame_f();
    if (!Tools::IsPtrValid((void *) matchGame))
        return false;

    auto EnemyPawns = *(List<uintptr_t> **) (matchGame + Class_BaseGame_EnemyPawns);
    if (!EnemyPawns || !Tools::IsPtrValid((void *) EnemyPawns))
        return false;
    auto Items = EnemyPawns->getItems();
    if (!Items)
        return false;

    const float radiusSq = radius * radius;
    const float headBand = A_HitboxHeadBandFraction();
    const float headBandRadiusSq = radiusSq * headBand * headBand;

    const int count = EnemyPawns->getSize();
    for (int i = 0; i < count; i++) {
        const uintptr_t pawnU = Items[i];
        if (!pawnU || !Tools::IsPtrValid((void *) pawnU))
            continue;
        Pawn *pawn = (Pawn *) pawnU;
        if (!pawn->m_IsAlive())
            continue;

        const Vector3 toHead = pawn->get_HeadPosition() - startPos;
        const float alongRay = Vector3::Dot(toHead, rayAxis);
        if (alongRay <= 0.0f)            // head is behind the shooter
            continue;

        const float missDistSq = Vector3::Dot(toHead, toHead) - alongRay * alongRay;
        if (missDistSq > radiusSq)       // outside the enlarged hitbox
            continue;

        // Inside the inner band -> mark the shot as a head hit so the damage
        // hook can report EHitGroup_Head without the manual toggle.
        g_hitboxHitHead = (headBandRadiusSq > 0.0f && missDistSq <= headBandRadiusSq);
        return true;
    }
    return false;
}

typedef bool (*SingleLineCheckPhysicsFn)(void *, int, void *, void *, Vector3, Vector3, void *);
inline SingleLineCheckPhysicsFn orig_SingleLineCheckPhysics_Instant = nullptr;

// Shared body for the hitbox hook. It is installed on BOTH the base
// WeaponFireComponent::SingleLineCheckPhysics (0xC1514C0) and the
// WeaponFireComponent_Instant override (0xC9B2A9C). A normal gun dispatches to
// the Instant override -- hooking only the base meant the whole feature never
// ran for the weapons the player actually holds.
inline bool SingleLineCheckPhysicsCommon(SingleLineCheckPhysicsFn orig,
                                         void *instance, int hitType,
                                         void *hitTarget, void *hitCollider,
                                         Vector3 startPos, Vector3 dir,
                                         void *impactInfo)
{
    // The hitbox answers before the game's sweep runs, exactly like V3: the
    // shot passed near a head, so it is a hit, and the caller's [Out] slots are
    // left exactly as it set them up.
    if (instance != nullptr && orig != nullptr && Config.ExtraMenu.Hit &&
        A_HitboxRayPassesEnemyHead(startPos, dir))
        return true;

    // Not an enlarged-hitbox shot: let the game resolve it for real. The head
    // flag is deliberately NOT cleared here -- several sweeps can run for one
    // shot, and a later miss must not erase the head hit an earlier sweep
    // recorded. The damage hook consumes and clears it.
    if (orig != nullptr)
        return orig(instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
    return false;
}

inline bool SingleLineCheckPhysics(void *instance, int hitType, void *hitTarget, void *hitCollider, Vector3 startPos, Vector3 dir, void *impactInfo)
{
    return SingleLineCheckPhysicsCommon(orig_SingleLineCheckPhysics, instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

inline bool SingleLineCheckPhysics_Instant(void *instance, int hitType, void *hitTarget, void *hitCollider, Vector3 startPos, Vector3 dir, void *impactInfo)
{
    return SingleLineCheckPhysicsCommon(orig_SingleLineCheckPhysics_Instant, instance, hitType, hitTarget, hitCollider, startPos, dir, impactInfo);
}

inline void* (*orig_CalcDamageInfoInstantHit)(void* instance, void** inImpactInfo, unsigned char inFireMode, void* sourcePos, int clientTime, int ammoCount, float punchX, float punchY, float spreadX, float spreadY, float fightOffSpeed, float fightOffUp) = nullptr;
inline void* CalcDamageInfoInstantHit(void* instance, void** inImpactInfo, unsigned char inFireMode, void* sourcePos, int clientTime, int ammoCount, float punchX, float punchY, float spreadX, float spreadY, float fightOffSpeed, float fightOffUp) {
    void* damageInfo = (orig_CalcDamageInfoInstantHit != nullptr)
        ? orig_CalcDamageInfoInstantHit(instance, inImpactInfo, inFireMode, sourcePos, clientTime, ammoCount, punchX, punchY, spreadX, spreadY, fightOffSpeed, fightOffUp)
        : nullptr;
    g_hitboxDamageInfo = (uintptr_t) damageInfo;

    if (damageInfo != nullptr) {
        const uintptr_t di = (uintptr_t) damageInfo;

        // Which hit group this shot reports, in priority order:
        //   1. the explicit Headshot toggle  -> head + damage multiplier
        //      (this is the variant-3 "Headshot" feature, DamageInfo+0x54/+0x14),
        //   2. the hitbox's automatic head zone -> head only,
        //   3. the manual Aim -> Hit Group override,
        //   4. otherwise the game's own hit group is left untouched.
        if (Config.ExtraMenu.Headshot) {
            *(int *) (di + Class_DamageInfo_m_HitGroup) = EHitGroup_Head;
            *(float *) (di + Class_DamageInfo_m_Damage) *=
                ImClamp(Config.ExtraMenu.HeadshotDamage, 1.0f, 10.0f);
        } else if (Config.ExtraMenu.Hit && g_hitboxHitHead) {
            *(int *) (di + Class_DamageInfo_m_HitGroup) = EHitGroup_Head;
        } else if (Config.Aim.HitGroup != HitGroupAuto) {
            *(int *) (di + Class_DamageInfo_m_HitGroup) = A_HitGroupFromSelect((int) Config.Aim.HitGroup);
        }
    }

    g_hitboxHitHead = false;
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
       *(unsigned long long *) ((uintptr_t) instance + 0x60) = Config.ExtraMenu.ReportSpoofTargetId;
    }
    if (orig_CSAccountReportUserReq_Write != nullptr)
        orig_CSAccountReportUserReq_Write(instance, writer);
}

//-- Rename card
// Rewrites your own PlayerInfo nickname so the new name is what you (and the
// clients that read it) see. Only runs when a name card is selected.
// Pick the highlighted enemy (same rule as the aim target: closest by distance,
// or nearest screen center when aim mode is FOV) and copy its gamePlayerId + name.
inline uintptr_t PickHighlightedEnemy()
{
    uintptr_t result = 0;
    float best = std::numeric_limits<float>::infinity();
    auto Gameplay_get_MatchGame = (uintptr_t (*)()) (Class_Gameplay_get_MatchGame);
    auto get_MatchGame = Gameplay_get_MatchGame();
    if (!Tools::IsPtrValid((void *) get_MatchGame)) return 0;
    auto Gameplay_get_LocalPawn = (uintptr_t (*)()) (Class_Gameplay_get_LocalPawn);
    auto LocalPawn = Gameplay_get_LocalPawn();
    if (!Tools::IsPtrValid((void *) LocalPawn)) return 0;
    Vector3 MyPos{0, 0, 0};
    auto local_m_Mesh = *(Transform **) (LocalPawn + Class_Pawn_m_Mesh);
    if (local_m_Mesh) MyPos = local_m_Mesh->get_position();
    auto EnemyPawns = *(List<uintptr_t> **) (get_MatchGame + Class_BaseGame_EnemyPawns);
    if (!EnemyPawns) return 0;
    auto Items = EnemyPawns->getItems();
    if (!Items) return 0;
    for (int i = 0; i < EnemyPawns->getSize(); i++) {
        auto Pawn = Items[i];
        if (!Tools::IsPtrValid((void *) Pawn)) continue;
        if (!*(bool *) (Pawn + Class_Pawn_m_IsAlive)) continue;
        auto m_Mesh = *(Transform **) (Pawn + Class_Pawn_m_Mesh);
        if (!Tools::IsPtrValid((void *) m_Mesh)) continue;
        Vector3 pos = m_Mesh->get_position();
        float dist = Vector3::Distance(MyPos, pos);
        // Crosshair priority when the aim "By" mode is FOV: pick the enemy nearest
        // the screen center, but still prefer closer ones when distances tie.
        if (Config.Aim.By == EAim::Crosshair) {
            auto HeadSc = Camera::get_main()->WorldToScreenPoint(pos);
            if (HeadSc.z > 0) {
                Vector2 center((float)get_width() / 2.0f, (float)get_height() / 2.0f);
                float screenDist = (HeadSc.x - center.x) * (HeadSc.x - center.x) +
                                   (HeadSc.y - center.y) * (HeadSc.y - center.y);
                if (screenDist < best) { best = screenDist; result = Pawn; }
            }
        } else {
            if (dist < best) { best = dist; result = Pawn; }
        }
    }
    return result;
}

inline void PickEnemyForSpoofOrRename()
{
    uintptr_t pawn = PickHighlightedEnemy();
    if (!Tools::IsPtrValid((void *) pawn)) return;

    // gamePlayerId of the highlighted enemy.
    PlayerInfo *info = *(PlayerInfo **) ((uintptr_t) pawn + Class_Pawn_m_PlayerInfo);
    if (!Tools::IsPtrValid((void *) info)) return;
    unsigned long long enemyId = *(unsigned long long *) ((uintptr_t) info + Class_PlayerInfo_m_GamePlayerId);
    if (!enemyId) return;

    // Nickname of the highlighted enemy.
    String *name = *(String **) ((uintptr_t) info + Class_PlayerInfo_m_NickName);
    std::string enemyName;
    if (Tools::IsPtrValid((void *) name)) {
        const char *text = name->CString();
        if (text) enemyName = text;
    }
    if (enemyName.empty()) enemyName = "<no name>";

    // Push into the report-spoof fields.
    Config.ExtraMenu.ReportSpoofTargetId = enemyId;
    strncpy(Config.ExtraMenu.ReportSpoofName, enemyName.c_str(), sizeof(Config.ExtraMenu.ReportSpoofName) - 1);
    Config.ExtraMenu.ReportSpoofName[sizeof(Config.ExtraMenu.ReportSpoofName) - 1] = '\0';
    strncpy(Config.ExtraMenu.ReportSpoofPickedName, enemyName.c_str(), sizeof(Config.ExtraMenu.ReportSpoofPickedName) - 1);
    Config.ExtraMenu.ReportSpoofPickedName[sizeof(Config.ExtraMenu.ReportSpoofPickedName) - 1] = '\0';
    Config.ExtraMenu.ReportSpoofPickedId = enemyId;
    Config.ExtraMenu.ReportSpoofPickEnemy = false;

    // Push into the rename-card fields too (same enemy identity, rename uses name only).
    strncpy(Config.ExtraMenu.RenameCardName, enemyName.c_str(), sizeof(Config.ExtraMenu.RenameCardName) - 1);
    Config.ExtraMenu.RenameCardName[sizeof(Config.ExtraMenu.RenameCardName) - 1] = '\0';
    Config.ExtraMenu.RenameCardPickEnemy = false;
}

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
typedef void (*Il2CppFieldStaticGetFn)(void *, void *);
// Attaching the calling thread to the IL2CPP domain is mandatory before the
// reflection API (assembly / class / field) is touched from a thread Unity does
// not own. Skipping it is the classic "game dies a second after the menu
// notices a second login" crash.
typedef void *(*Il2CppThreadAttachFn)(void *);
typedef void (*Il2CppThreadDetachFn)(void *);

struct Il2CppForbidApi
{
    Il2CppDomainGetFn domainGet;
    Il2CppDomainAssemblyOpenFn domainAssemblyOpen;
    Il2CppAssemblyGetImageFn assemblyGetImage;
    Il2CppClassFromNameFn classFromName;
    Il2CppClassGetFieldFn classGetFieldFromName;
    Il2CppFieldStaticSetFn fieldStaticSetValue;
    Il2CppFieldStaticGetFn fieldStaticGetValue;   // read probe, optional
    Il2CppThreadAttachFn threadAttach;            // optional but strongly preferred
    Il2CppThreadDetachFn threadDetach;
    bool ok;
};

inline Il2CppForbidApi ResolveIl2CppForbidApi()
{
    Il2CppForbidApi api = {};
    const char *libs[] = {"libil2cpp.so", "libunity.so"};
    for (const char *lib : libs)
    {
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
        candidate.fieldStaticGetValue = reinterpret_cast<Il2CppFieldStaticGetFn>(dlsym(handle, "il2cpp_field_static_get_value"));
        candidate.threadAttach = reinterpret_cast<Il2CppThreadAttachFn>(dlsym(handle, "il2cpp_thread_attach"));
        candidate.threadDetach = reinterpret_cast<Il2CppThreadDetachFn>(dlsym(handle, "il2cpp_thread_detach"));

        if (candidate.domainGet && candidate.domainAssemblyOpen && candidate.assemblyGetImage &&
            candidate.classFromName && candidate.classGetFieldFromName && candidate.fieldStaticSetValue)
        {
            candidate.ok = true;
            return candidate;
        }
    }
    return api; // all-null, ok=false: feature stays inert, never crashes
}

// ANTI LEAK (was "Forbid kick-off" / "Forbid On Login").
//
// The feature writes the static CurrentStat / UntilTime fields of the managed
// Network.ForbidKickOffHandler so the "signed in on another device" kick never
// lands. Two hard rules keep it from taking the game down:
//   1. it runs on the game thread only (see ProcessAntiLeak() at the bottom),
//      never from the JNI login callback thread;
//   2. the calling thread is attached to the IL2CPP domain before any
//      reflection call and detached again afterwards.
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

    // Attach this thread to the IL2CPP domain for the duration of the call.
    // Without it, assembly/class/field lookups from a foreign thread abort the
    // process -- exactly the crash seen right after another device logged on.
    void *attachedThread = nullptr;
    if (api.threadAttach != nullptr)
        attachedThread = api.threadAttach(domain);

    void *assembly = api.domainAssemblyOpen(domain, "Assembly-CSharp.dll");
    if (!assembly)
        assembly = api.domainAssemblyOpen(domain, "Assembly-CSharp");

    void *image = assembly ? api.assemblyGetImage(assembly) : nullptr;
    void *klass = image ? api.classFromName(image, "Network", "ForbidKickOffHandler") : nullptr;

    if (klass != nullptr)
    {
        void *currentStatField = api.classGetFieldFromName(klass, "CurrentStat");
        void *untilTimeField = api.classGetFieldFromName(klass, "UntilTime");

        // Read probe first: a successful static read proves the class is
        // initialised and the field really is a static field of that class, so
        // the write below cannot land on a half-built type.
        bool probedOk = true;
        if (api.fieldStaticGetValue != nullptr)
        {
            if (currentStatField)
            {
                bool probe = false;
                api.fieldStaticGetValue(currentStatField, &probe);
            }
            if (untilTimeField)
            {
                int32_t probe = 0;
                api.fieldStaticGetValue(untilTimeField, &probe);
            }
        }

        if (probedOk)
        {
            bool statValue = enable;
            int32_t untilValue = enable ? ForbidKickOffCfg::UntilTime_Force
                                        : ForbidKickOffCfg::UntilTime_Disabled;
            if (currentStatField)
                api.fieldStaticSetValue(currentStatField, &statValue);
            if (untilTimeField)
                api.fieldStaticSetValue(untilTimeField, &untilValue);
        }
    }

    if (attachedThread != nullptr && api.threadDetach != nullptr)
        api.threadDetach(attachedThread);
}

// Raised from any thread (including the JNI login callback); consumed on the
// game thread by ProcessAntiLeak(). Only a flag is touched here, so the login
// path can never crash the game on its own.
inline bool g_antiLeakPending = false;

inline void RequestAntiLeakOnLogin()
{
    if (!Config.ExtraMenu.ForbidKickOff)
        return;
    g_antiLeakPending = true;
}

// Same name the rest of the code already calls; kept so existing call sites do
// not have to change. It no longer performs the il2cpp work inline.
inline void ApplyForbidKickOffOnLogin()
{
    RequestAntiLeakOnLogin();
}

// Called once per frame from the render hook (game thread).
inline void ProcessAntiLeak()
{
    if (!g_antiLeakPending)
        return;
    g_antiLeakPending = false;
    if (!Config.ExtraMenu.ForbidKickOff)
        return;
    ApplyForbidKickOff(true);
    Config.ExtraMenu.ForbidKickOffOnLogin = false;
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
    if (SlideRange <= 0.0f && o_TickLocalPlayer != nullptr) {
        o_TickLocalPlayer(ins, deltaTime);
    }
    // Second game-thread heartbeat for the triggerbot, so A-Fire keeps working
    // even if the weapon object is not being ticked this frame. NOTE: this hook
    // is currently commented out in InitializeAllHooks (it belongs to the
    // disabled Long Slide set), so today Weapon::Tick is the only heartbeat --
    // A_FireTick() throttles, so enabling this later adds no extra cost.
    A_FireTick();
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
        return 0;
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

//-- Camera POV
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
    // WeaponFireComponent_Instant::SingleLineCheckPhysics -- the override a normal gun calls.
    HOOK_LIB("libunity.so", "0xC9B2A9C", SingleLineCheckPhysics_Instant, orig_SingleLineCheckPhysics_Instant);

    HOOK_LIB("libunity.so", "0x5109DE4", CalcDamageInfoInstantHit, orig_CalcDamageInfoInstantHit);

    //-- A-Fire (triggerbot / auto-fire)
    // Weapon::Tick / StartFire / StopFire (RVAs from dump.cs).
    //
    // StartFire/StopFire are called directly (they are not hooked), so a stale
    // RVA here is a jump into whatever lives at that address -- the classic
    // "switch the triggerbot on and the game dies" failure. Only keep the two
    // pointers when they really do point at executable code; A_FireTick() then
    // refuses to fire when either one is null.
    orig_Weapon_StartFire = nullptr;
    orig_Weapon_StopFire = nullptr;
    {
        void *startFire = (void *) getAbsoluteAddress("libunity.so", string2Offset("0x5123C84"));
        void *stopFire  = (void *) getAbsoluteAddress("libunity.so", string2Offset("0x5124EAC"));
        if (Tools::IsExecPtr(startFire))
            orig_Weapon_StartFire = reinterpret_cast<void (*)(void *)>(startFire);
        if (Tools::IsExecPtr(stopFire))
            orig_Weapon_StopFire = reinterpret_cast<void (*)(void *, bool)>(stopFire);
        if (orig_Weapon_StartFire == nullptr || orig_Weapon_StopFire == nullptr)
            A_FireLog("weapon fire RVAs do not match this build -- triggerbot disabled");
    }
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
    // (0x519676C used to be listed a second time here. Hooking one address twice
    //  overwrites the first trampoline and leaves orig_* pointing into it.)  

/*
    //-- Unlimited Bullets
    HOOK_LIB("libunity.so", "0x5984C18", IsInfiniteBullet, orig_IsInfiniteBullet);
    HOOK_LIB("libunity.so", "0xBF93BA4", IsNoCostAmmo, orig_IsNoCostAmmo);

	//-- Unli Ammo (Functions)
	HOOK_LIB("libunity.so", "0x50EB73C", hk_AmmoCanFire, orig_AmmoCanFire);         
    HOOK_LIB("libunity.so", "0x5107904", hk_HasAmmo, orig_HasAmmo);                 
    HOOK_LIB("libunity.so", "0xC14E570", hk_HasAmmo_IgnoreInfinite, orig_HasAmmo_IgnoreInfinite);
    HOOK_LIB("libunity.so", "0xC14D614", hk_ServerStopFire, orig_ServerStopFire);   
    HOOK_LIB("libunity.so", "0x50EC794", hk_get_ShotCost, orig_get_ShotCost);       
    HOOK_LIB("libunity.so", "0x510CE2C", hk_IsAmmoFree, orig_IsAmmoFree);           
 
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

    SafeDobbyHook("libunity.so", 0x5036EB8,
          (void *)&hook_GameObjectSetActive,
          (void **)&orig_GameObjectSetActive, "GameObject::SetActive");
          
    //-- Throwable Alert      
    SafeDobbyHook("libunity.so", ThrowAlertCfg::TickRva,
          (void *)&hook_WeaponProjectileTick,
          (void **)&orig_WeaponProjectileTick, "WeaponProjectile::Tick");
          
    //-- Clear Terrain
    SafeDobbyHook("libunity.so", 0xAE2E1B0,
          (void *)&hook_SetGrassShowState,
          (void **)&orig_SetGrassShowState, "SetGrassShowState");

    SafeDobbyHook("libunity.so", 0x7B4E290,
          (void *)&hook_SetGrassLODBias,
          (void **)&orig_SetGrassLODBias, "SetGrassLODBias");         
          
    SafeDobbyHook("libunity.so", 0x9FDEE28, (void*)hooked_SetUltraFrameRateDeviceInfo, (void**)&orig_SetUltraFrameRateDeviceInfo, "SetUltraFrameRateDeviceInfo");

    ///Walk Under Water
    SafeDobbyHook("libunity.so", 0x51D31BC, (void *)  &Pawn_GetCurrentDistToWaterSurface, (void **) &oPawn_GetCurrentDistToWaterSurface, "Pawn::GetCurrentDistToWaterSurface"); 
    SafeDobbyHook("libunity.so", 0x54BC450, (void *)  &PlayerPawn_IsUnderWaterSurface, (void **) &oPlayerPawn_IsUnderWaterSurface, "PlayerPawn::IsUnderWaterSurface"); 
    SafeDobbyHook("libunity.so", 0x757162C, (void *)  &CheckInWaterComponent_get_CurrentWaterSurfaceHeight, (void **) &oCheckInWaterComponent_get_CurrentWaterSurfaceHeight, "CheckInWaterComponent::get_CurrentWaterSurfaceHeight"); 

  //-- Camera POV
  SafeDobbyHook("libunity.so", 0xC347950, (void *)  &InputSettingConfig_Instant_GetMainCameraFov_3p, (void **) &oInputSettingConfig_Instant_GetMainCameraFov_3p, "GetMainCameraFov_3p"); 

    //-- Unlock Blueprints
    HOOK_LIB("libunity.so", "0x901F988", IsUnlocked, orig_IsUnlocked);
    
    HOOK_LIB("libunity.so", "0xA61AA00", IsWeaponAttachmentUnlock, orig_IsWeaponAttachmentUnlock);
}