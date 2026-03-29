#pragma once

// Cheat configuration variables — toggled by menu, read by features
struct Variables_t
{
    // Visuals
    bool bESP = false;

    // Misc
    enum BhopMode : int
    {
        BHOP_DISABLED = 0,
        BHOP_NORMAL,
        BHOP_SUBTICK
    };
    int nBhopMode = BHOP_DISABLED;

    enum AutoStrafeMode : int
    {
        STRAFE_DISABLED = 0,
        STRAFE_NORMAL,
        STRAFE_SUBTICK
    };
    int nAutoStrafeMode = STRAFE_DISABLED;

    bool bJumpBug = false;
    bool bEdgeBug = false;
};

inline Variables_t Vars;
