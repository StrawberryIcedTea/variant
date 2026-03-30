// Entity system access -- local player via IEngineClient + entity system

#include "entity.h"
#include "../core/interfaces.h"

// -----------------------------------------------------------------------
// Local player accessors -- via IEngineClient::GetLocalPlayer() + entity system
// -----------------------------------------------------------------------
CGameEntitySystem* EntitySystem::GetEntitySystem()
{
    if (!I::ppEntitySystem)
        return nullptr;
    return static_cast<CGameEntitySystem*>(*I::ppEntitySystem);
}

CCSPlayerController* EntitySystem::GetLocalController()
{
    auto* pES = GetEntitySystem();
    if (!pES || !I::pEngine)
        return nullptr;

    int nIndex = I::pEngine->GetLocalPlayer();
    if (nIndex <= 0)
        return nullptr;

    return pES->Get<CCSPlayerController>(nIndex);
}

C_BaseEntity* EntitySystem::GetLocalPlayerPawn()
{
    auto* pController = GetLocalController();
    if (!pController)
        return nullptr;

    CBaseHandle hPawn = pController->GetPawnHandle();
    if (!hPawn.IsValid())
        return nullptr;

    auto* pES = GetEntitySystem();
    if (!pES)
        return nullptr;

    return pES->Get<C_BaseEntity>(hPawn);
}
