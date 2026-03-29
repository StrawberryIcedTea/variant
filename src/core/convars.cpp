// ConVar cache — resolves frequently-used convar pointers at startup

#include "convars.h"
#include "../utilities/debug.h"
#include <format>

bool Convar::Setup()
{
    if (!pCvar)
        return false;

    sv_airaccelerate = Find("sv_airaccelerate");
    sv_air_max_wishspeed = Find("sv_air_max_wishspeed");
    sv_gravity = Find("sv_gravity");
    sv_autobunnyhopping = Find("sv_autobunnyhopping");
    sv_maxspeed = Find("sv_maxspeed");
    sv_standable_normal = Find("sv_standable_normal");

    C::Print(std::format("[convar] resolved: airaccel={} airwish={} gravity={} maxspeed={} autobhop={}",
                         sv_airaccelerate != nullptr, sv_air_max_wishspeed != nullptr, sv_gravity != nullptr,
                         sv_maxspeed != nullptr, sv_autobunnyhopping != nullptr));

    return true;
}
