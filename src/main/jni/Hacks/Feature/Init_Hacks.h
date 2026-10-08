#pragma once

float SlideRange;
bool isExecute;

enum TouchPhase {
    Began = 0,
    Moved = 1,
    Stationary = 2,
    Ended = 3,
    Canceled = 4,
};

enum TouchType {
    Direct = 0,
    Indirect = 1,
    Stylus = 2,
};

struct Touch {
    int m_FingerId;
    Vector2 m_Position;
    Vector2 m_RawPosition;
    Vector2 m_PositionDelta;
    float m_TimeDelta;
    int m_TapCount;
    TouchPhase m_Phase;
    TouchType m_Type;
    float m_Pressure;
    float m_maximumPossiblePressure;
    float m_Radius;
    float m_RadiusVariance;
    float m_AltitudeAngle;
    float m_AzimuthAngle;
};

enum LineTarget {
    Top = 0,
    Center = 1,
    Bottom = 2
};

enum EspBoxType {
    Fill = 0,
    Outline = 1,
    Corner = 2,
    ThreeD = 3
};

enum EspHealthPosition {
    HealthTop = 0,
    HealthSide = 1
};

enum CrosshairTarget {
    Normal = 0,
    Circle = 1,
    Cross = 2
};

enum EspStyleTarget {
    EspStyleNone = 0,
    EspStyle3DSphere = 1,
    EspStylePlayerSignal = 2
};

enum EAim {
  Distance = 0,
  Crosshair = 1
};

enum EAimTarget {
    Heads = 0,
    Chests = 1,
    Body = 2
};

enum EAimTrigger {
    None = 0,
    Shooting = 1,
    Scoping = 2
};

// Hit group forced into the damage info. Auto lets the hitbox code decide from
// where the shot landed; every other entry forces that group (values map to
// the game's EHitGroup enum in the damage hook).
enum EHitGroupSelect {
    HitGroupAuto = 0,
    HitGroupHead = 1,
    HitGroupHand = 2,
    HitGroupBody = 3,
    HitGroupFoot = 4,
    HitGroupWeakPoint = 5,
    HitGroupNeck = 6
};

struct sConfig {
        float Pline;
		float Bline;
		float PskelLine;
		float BskelLine;
        bool TeleportEnemy = false;
        bool UnlimitedAmmo;
    struct sInitImGui {
        uintptr_t thiz;
    };
    sInitImGui ImGuiMenu{0};
    
struct sWeaponAim {
        bool Aimbot360;
        float AimAssistSize;
        bool AimSilent;
        EAimTarget Target;
        EAimTrigger Trigger;
        EAim By;
        float size;
        float Cross;
        int HitGroup = HitGroupAuto;     // EHitGroupSelect: which hit group shots are reported as
        
         // Frame Rate Controls
        bool FpsLevel;
        float FpsLevel1;
        bool showFPSLevelSlider;

        bool FpsLevelUltra;              // New: toggle for Ultra FPS
        float FpsLevelUltra1 = 240.0f;   // Default Ultra FPS value
        bool showFPSLevelUltraSlider;    // New: show Ultra FPS slider
    };
    sWeaponAim Aim{0};
    
    struct sESPMenuLineScale {
        float lineSize;
    };
    sESPMenuLineScale sESPMenuLineScale{0};
    
    struct sESPMenu {
        bool Signal;
        bool Alert;
        bool Count;
        bool Name;
        bool isPlayerLine;
        LineTarget Target;
        EspBoxType BoxType;
        EspHealthPosition HealthPosition;
        CrosshairTarget CrosshairType;
        EspStyleTarget EspStyle;
        bool Armor;
        bool GrenadeWarn;
        bool BRClass;
        bool Vehicle;
        bool VehicleHealth;
        bool Box;
        bool Health;
        bool Distance;
        bool Skeleton;
        bool ShowFov;
        bool Crosshair;
        bool Aimline;
    };
    sESPMenu ESPMenu{0};
    
    struct sColorsESPPLAYER {
    float *LinePLAYER;
    float *BoxPLAYER;
    float *NamePLAYER;
    float *HealthPLAYER;
    float *DistancePLAYER;
    float *SkeletonPLAYER;
};
sColorsESPPLAYER sColorsESPPLAYER{0};

struct sColorsESPBOT {
    float *LineBOT;
    float *BoxBOT;
    float *NameBOT;
    float *HealthBOT;
    float *DistanceBOT;
    float *SkeletonBOT;
};
sColorsESPBOT sColorsESPBOT{0};
    
    struct sColorsESPOTHERS {
        float *PovOTHERS;
    };
    sColorsESPOTHERS sColorsESPOTHERS{0};
    
    struct sExtraMenu {
        bool SmartReload;
        bool NoGravity;
        float NoGravityScale = 1.0f;
        bool ClearTerrain;
        bool BlackSky;
        bool Execute;
        bool NoSmoke;
        bool UnliAmmo;
        float VehicleScale = 1.0f;
        bool SpeedhackX;
        bool CameraPov;
        float CameraPovSize;
        bool LongSlide;
        bool UnliSlide;
        bool NoWingsuit;
        bool WalkUnderWater;
        bool NoCrouch;
        bool Frame;
        bool HighFps;
        bool Fps;
        bool Grap;
        bool Grapss;
        bool EnhanceFPS;
        bool Spectatex;
        bool Attachment;
		bool Blueprints;
		bool ClearDisplay;
		bool ResetGuest;
		bool RedWallhack;
    	bool Spread;
    	bool Fire;
    	bool Diving;
        bool Recoil;
        bool Reload;
        bool Shake;
        bool Scope;
        bool Switch;
        bool Flash;
        bool Hit;
        float HitboxScale = 3.0f;
        bool Rpd;
        bool Parachute;
		bool WallHack;
		bool Kinetic;
        bool CamoTest;
        int CamoTestMode;

        // ---- NEW FEATURES ----
        bool ReportSpoof;          // true = redirect any incoming report away from you (toward ReportSpoofTargetId)
        unsigned long long ReportSpoofTargetId = 0;   // account id the outgoing report is redirected to
        bool RenameCard;           // true = you can rename yourself from the menu using a name card
        int  RenameCardGid = 0;    // name card gid to use (0 = none equipped)
        char RenameCardName[32] = {};                 // name written into your own PlayerInfo while RenameCard is on
        float TuneHitboxHeadBand = 0.35f;             // how far below the head still counts as a headshot (auto hit group)
        bool A_Fire;               // triggerbot / auto-fire: shoots the moment the aim / fire criteria is met
        int  A_FireTrigger = 0;    // when A_Fire fires: 0 = always, 1 = only if enemy flanks / is in front, 2 = only if no teammates near crosshair
        float A_FireDelay = 0.0f;  // min seconds between A_Fire shots; 0 = as fast as the game allows
        // End of ExtraMenu feature toggles
    };
    sExtraMenu ExtraMenu{0};

	struct sColorsESP {
	
        float *Vehicle;
    };
    sColorsESP ColorsESP{0};
};

sConfig Config{0};
