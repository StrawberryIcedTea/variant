// Entity system access -- local player via IEngineClient + entity system

#include "entity.h"
#include "../core/interfaces.h"

// -----------------------------------------------------------------------
// Local player accessors -- via IEngineClient::GetLocalPlayer() + entity system
// -----------------------------------------------------------------------
CCSPlayerController* EntitySystem::GetLocalController()
{
    if (!I::pEngine || !I::pEntitySystem)
        return nullptr;

    auto* pEntitySystem = static_cast<CGameEntitySystem*>(I::pEntitySystem);
    int nIndex = I::pEngine->GetLocalPlayer();
    if (nIndex <= 0)
        return nullptr;

    return pEntitySystem->Get<CCSPlayerController>(nIndex);
}

C_BaseEntity* EntitySystem::GetLocalPlayerPawn()
{
    auto* pController = GetLocalController();
    if (!pController)
        return nullptr;

    CBaseHandle hPawn = pController->GetPawnHandle();
    if (!hPawn.IsValid())
        return nullptr;

    auto* pEntitySystem = static_cast<CGameEntitySystem*>(I::pEntitySystem);
    return pEntitySystem->Get<C_BaseEntity>(hPawn);
}

CGameEntitySystem* EntitySystem::GetEntitySystem()
{
    return static_cast<CGameEntitySystem*>(I::pEntitySystem);
}
