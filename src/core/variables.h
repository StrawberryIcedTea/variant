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
};

inline Variables_t Vars;
