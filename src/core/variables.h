#pragma once

// Cheat configuration variables — toggled by menu, read by features
struct Variables_t
{
    // Visuals
    bool bESP = false;
    bool  bChams          = false;
    bool  bChamsXQZ       = false;
    bool  bChamsEnemies   = true;
    bool  bChamsTeammates = false;
    bool  bChamsSelf      = false;
    int   nChamsMaterial  = 0;      // 0=Flat, 1=Glow
    bool  bChamsDouble    = false;
    int   nChamsMaterial2 = 0;      // second pass material

    // Pass 1 colors
    float flChamsEnemyColor[4]  = { 1.f,  0.f,  0.f, 1.f };
    float flChamsTeamColor[4]   = { 0.1f, 0.5f, 1.f, 1.f };
    float flChamsSelfColor[4]   = { 0.f,  1.f,  0.f, 1.f };

    // Pass 2 colors
    float flChamsEnemyColor2[4] = { 1.f,  0.f,  0.f, 1.f };
    float flChamsTeamColor2[4]  = { 0.1f, 0.5f, 1.f, 1.f };
    float flChamsSelfColor2[4]  = { 0.f,  1.f,  0.f, 1.f };

    // Misc
    enum BhopMode : int { BHOP_DISABLED = 0, BHOP_NORMAL, BHOP_SUBTICK };
    int nBhopMode = BHOP_DISABLED;

    enum AutoStrafeMode : int { STRAFE_DISABLED = 0, STRAFE_NORMAL, STRAFE_SUBTICK };
    int nAutoStrafeMode = STRAFE_DISABLED;

    bool bJumpBug = false;
    bool bEdgeBug = false;
};

inline Variables_t Vars;
