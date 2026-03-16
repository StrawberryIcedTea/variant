#pragma once
#include <cstdint>

// Movement features (bunny hop, etc.)
namespace Movement
{
    // Called from CreateMove — queries EntitySystem for player state
    void BunnyHop();
}
