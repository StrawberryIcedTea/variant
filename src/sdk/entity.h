#pragma once
#include <cstdint>
#include <cstddef>
#include <type_traits>
#include "const.h"
#include "entity_handle.h"
#include "../core/schema.h"
#include "../utilities/memory.h"

// Forward declarations
struct CPlayer_WeaponServices;
struct CCSWeaponBaseVData;

// -----------------------------------------------------------------------
// CGameSceneNode -- position and transform data
// Accessed via C_BaseEntity::GetGameSceneNode()
// -----------------------------------------------------------------------
struct CGameSceneNode
{
    SCHEMA_ADD_PFIELD(float, GetAbsOrigin, "CGameSceneNode->m_vecAbsOrigin")
    SCHEMA_ADD_PFIELD(float, GetAbsRotation, "CGameSceneNode->m_angAbsRotation")
    SCHEMA_ADD_FIELD(bool, IsDormant, "CGameSceneNode->m_bDormant")
};

// -----------------------------------------------------------------------
// CCollisionProperty -- player collision bounds
// -----------------------------------------------------------------------
struct CCollisionProperty
{
    SCHEMA_ADD_PFIELD(float, GetMins, "CCollisionProperty->m_vecMins")
    SCHEMA_ADD_PFIELD(float, GetMaxs, "CCollisionProperty->m_vecMaxs")
};

// -----------------------------------------------------------------------
// C_BaseEntity -- schema-based entity access
// Fields are resolved at runtime from Source 2 SchemaSystem
// -----------------------------------------------------------------------
struct C_BaseEntity
{
    // --- Schema fields (update-proof) ---
    SCHEMA_ADD_FIELD(CGameSceneNode*, GetGameSceneNode, "C_BaseEntity->m_pGameSceneNode")
    SCHEMA_ADD_FIELD(CCollisionProperty*, GetCollision, "C_BaseEntity->m_pCollision")
    SCHEMA_ADD_FIELD(int32_t, GetHealth, "C_BaseEntity->m_iHealth")
    SCHEMA_ADD_FIELD(uint8_t, GetLifeState, "C_BaseEntity->m_lifeState")
    SCHEMA_ADD_FIELD(uint32_t, GetFlags, "C_BaseEntity->m_fFlags")
    SCHEMA_ADD_PFIELD(float, GetAbsVelocity, "C_BaseEntity->m_vecAbsVelocity")
    SCHEMA_ADD_FIELD(MoveType_t, GetMoveType, "C_BaseEntity->m_MoveType")
    SCHEMA_ADD_FIELD(uint8_t, GetTeamNum, "C_BaseEntity->m_iTeamNum")

    // --- Convenience helpers ---

    // World position (goes through scene node)
    float* GetAbsOrigin()
    {
        auto* pNode = GetGameSceneNode();
        if (!IsValidPtr(pNode))
            return nullptr;
        return pNode->GetAbsOrigin();
    }

    bool IsAlive() { return GetHealth() > 0 && GetLifeState() == 0; }

    bool IsOnGround() { return (GetFlags() & FL_ONGROUND) != 0; }

    bool IsMovetypeBlocked()
    {
        auto mt = GetMoveType();
        return mt == MOVETYPE_LADDER || mt == MOVETYPE_NOCLIP || mt == MOVETYPE_OBSERVER;
    }
};

// -----------------------------------------------------------------------
// C_BasePlayerPawn -- extends C_BaseEntity with movement services
// -----------------------------------------------------------------------
struct C_BasePlayerPawn : C_BaseEntity
{
    SCHEMA_ADD_FIELD(void*, GetMovementServices, "C_BasePlayerPawn->m_pMovementServices")
    SCHEMA_ADD_FIELD(CBaseHandle, GetControllerHandle, "C_BasePlayerPawn->m_hController")
    SCHEMA_ADD_FIELD(CPlayer_WeaponServices*, GetWeaponServices, "C_BasePlayerPawn->m_pWeaponServices")
};

// -----------------------------------------------------------------------
// C_CSPlayerPawn -- CS2 player pawn with combat fields
// -----------------------------------------------------------------------
struct C_CSPlayerPawn : C_BasePlayerPawn
{
    SCHEMA_ADD_FIELD(bool, IsScoped, "C_CSPlayerPawn->m_bIsScoped")
    SCHEMA_ADD_FIELD(bool, IsDefusing, "C_CSPlayerPawn->m_bIsDefusing")
    SCHEMA_ADD_FIELD(int32_t, GetShotsFired, "C_CSPlayerPawn->m_iShotsFired")
    SCHEMA_ADD_FIELD(int32_t, GetArmorValue, "C_CSPlayerPawn->m_ArmorValue")
    SCHEMA_ADD_PFIELD(float, GetAimPunchAngle, "C_CSPlayerPawn->m_aimPunchAngle")
    SCHEMA_ADD_PFIELD(float, GetEyeAngles, "C_CSPlayerPawn->m_angEyeAngles")
    SCHEMA_ADD_FIELD(bool, IsWalking, "C_CSPlayerPawn->m_bIsWalking")
};

// -----------------------------------------------------------------------
// CBasePlayerController -- base player controller
// -----------------------------------------------------------------------
struct CBasePlayerController : C_BaseEntity
{
    SCHEMA_ADD_FIELD(uint64_t, GetSteamId, "CBasePlayerController->m_steamID")
    SCHEMA_ADD_FIELD(CBaseHandle, GetPawnHandle, "CBasePlayerController->m_hPawn")
    SCHEMA_ADD_FIELD(bool, IsLocalPlayerController, "CBasePlayerController->m_bIsLocalPlayerController")
    SCHEMA_ADD_PFIELD(char, GetPlayerName, "CBasePlayerController->m_iszPlayerName")
};

// -----------------------------------------------------------------------
// CCSPlayerController -- CS2-specific player controller
// -----------------------------------------------------------------------
struct CCSPlayerController : CBasePlayerController
{
    SCHEMA_ADD_FIELD(int32_t, GetPing, "CCSPlayerController->m_iPing")
    SCHEMA_ADD_FIELD(CBaseHandle, GetPlayerPawnHandle, "CCSPlayerController->m_hPlayerPawn")
    SCHEMA_ADD_FIELD(bool, IsPawnAlive, "CCSPlayerController->m_bPawnIsAlive")
    SCHEMA_ADD_FIELD(int32_t, GetPawnHealth, "CCSPlayerController->m_iPawnHealth")
    SCHEMA_ADD_FIELD(int32_t, GetPawnArmor, "CCSPlayerController->m_iPawnArmor")
    SCHEMA_ADD_FIELD(bool, PawnHasDefuser, "CCSPlayerController->m_bPawnHasDefuser")
    SCHEMA_ADD_FIELD(bool, PawnHasHelmet, "CCSPlayerController->m_bPawnHasHelmet")
};

// -----------------------------------------------------------------------
// Weapon services -- active weapon handle
// -----------------------------------------------------------------------
struct CPlayer_WeaponServices
{
    SCHEMA_ADD_FIELD(CBaseHandle, GetActiveWeaponHandle, "CPlayer_WeaponServices->m_hActiveWeapon")
};

// -----------------------------------------------------------------------
// Weapon types
// -----------------------------------------------------------------------
enum CSWeaponType : uint32_t
{
    WEAPONTYPE_KNIFE = 0,
    WEAPONTYPE_PISTOL = 1,
    WEAPONTYPE_SUBMACHINEGUN = 2,
    WEAPONTYPE_RIFLE = 3,
    WEAPONTYPE_SHOTGUN = 4,
    WEAPONTYPE_SNIPER_RIFLE = 5,
    WEAPONTYPE_MACHINEGUN = 6,
    WEAPONTYPE_C4 = 7,
    WEAPONTYPE_TASER = 8,
    WEAPONTYPE_GRENADE = 9,
    WEAPONTYPE_EQUIPMENT = 10,
    WEAPONTYPE_FISTS = 12,
    WEAPONTYPE_MELEE = 16,
    WEAPONTYPE_UNKNOWN = 19,
};

// -----------------------------------------------------------------------
// CCSWeaponBaseVData -- static weapon data (damage, range, spread, etc.)
// Accessed via weapon entity -> m_nSubclassID + 0x8 -> VData pointer
// -----------------------------------------------------------------------
struct CCSWeaponBaseVData
{
    SCHEMA_ADD_FIELD(uint32_t, GetWeaponType, "CCSWeaponBaseVData->m_WeaponType")
    SCHEMA_ADD_FIELD(int32_t, GetDamage, "CCSWeaponBaseVData->m_nDamage")
    SCHEMA_ADD_FIELD(float, GetHeadshotMultiplier, "CCSWeaponBaseVData->m_flHeadshotMultiplier")
    SCHEMA_ADD_FIELD(float, GetArmorRatio, "CCSWeaponBaseVData->m_flArmorRatio")
    SCHEMA_ADD_FIELD(float, GetPenetration, "CCSWeaponBaseVData->m_flPenetration")
    SCHEMA_ADD_FIELD(float, GetRange, "CCSWeaponBaseVData->m_flRange")
    SCHEMA_ADD_FIELD(float, GetRangeModifier, "CCSWeaponBaseVData->m_flRangeModifier")
    SCHEMA_ADD_FIELD(bool, IsFullAuto, "CCSWeaponBaseVData->m_bIsFullAuto")
};

// -----------------------------------------------------------------------
// C_BasePlayerWeapon -- base weapon entity
// -----------------------------------------------------------------------
struct C_BasePlayerWeapon : C_BaseEntity
{
    SCHEMA_ADD_FIELD(int32_t, GetClip1, "C_BasePlayerWeapon->m_iClip1")
};

// -----------------------------------------------------------------------
// C_CSWeaponBase -- CS2 weapon entity with VData access
// -----------------------------------------------------------------------
struct C_CSWeaponBase : C_BasePlayerWeapon
{
    SCHEMA_ADD_FIELD(float, GetAccuracyPenalty, "C_CSWeaponBase->m_fAccuracyPenalty")

    // VData -- static weapon data (damage, range, etc.)
    // Accessed via m_nSubclassID + 0x8 (pointer to VData object)
    CCSWeaponBaseVData* GetWeaponVData()
    {
        // m_nSubclassID is on C_BaseEntity; the VData pointer is 8 bytes after it
        static const uint32_t uOffset = Schema::GetOffset(FNV1A::HashConst("C_BaseEntity->m_nSubclassID"));
        auto* pVData = *reinterpret_cast<CCSWeaponBaseVData**>(reinterpret_cast<uint8_t*>(this) + uOffset + 0x8);
        return IsValidPtr(pVData) ? pVData : nullptr;
    }
};

// -----------------------------------------------------------------------
// Movement services -- friction and stamina
// -----------------------------------------------------------------------
struct CPlayer_MovementServices_Humanoid
{
    SCHEMA_ADD_FIELD(float, GetSurfaceFriction, "CPlayer_MovementServices_Humanoid->m_flSurfaceFriction")
};

struct CCSPlayer_MovementServices : CPlayer_MovementServices_Humanoid
{
    SCHEMA_ADD_FIELD(float, GetStamina, "CCSPlayer_MovementServices->m_flStamina")
};

// -----------------------------------------------------------------------
// CGameEntitySystem -- entity list access
// Inline chunk-walk matching engine disassembly (no pattern scan needed):
//   sar eax, 9 ; cmp eax, 0x3F ; cdqe
//   mov rcx, [rbp+rax*8+0x10]   ; chunk = this->chunks[chunkIdx]
//   and eax, 0x1FF ; imul rax, rax, 0x70
//   add rax, rcx                 ; entry = chunk + slot * 0x70
// -----------------------------------------------------------------------
constexpr int MAX_ENTITIES_IN_LIST = 512;
constexpr int MAX_ENTITY_LISTS = 64;
constexpr int MAX_TOTAL_ENTITIES = MAX_ENTITIES_IN_LIST * MAX_ENTITY_LISTS;

struct CGameEntitySystem
{
    template <typename T = C_BaseEntity> T* Get(int nIndex) { return reinterpret_cast<T*>(GetEntityByIndex(nIndex)); }

    template <typename T = C_BaseEntity> T* Get(CBaseHandle hHandle)
    {
        if (!hHandle.IsValid())
            return nullptr;
        return reinterpret_cast<T*>(GetEntityByIndex(hHandle.GetEntryIndex()));
    }

    int GetHighestEntityIndex()
    {
        auto* pThis = reinterpret_cast<uint8_t*>(this);
        if (!IsValidPtr(pThis))
            return 64;
        int highest = *reinterpret_cast<int*>(pThis + 0x1510);
        return (highest > 0 && highest <= 32768) ? highest : 64;
    }

  private:
    void* GetEntityByIndex(int nIndex)
    {
        if (nIndex < 0 || nIndex >= MAX_TOTAL_ENTITIES)
            return nullptr;

        int nChunk = nIndex / MAX_ENTITIES_IN_LIST;
        int nSlot = nIndex % MAX_ENTITIES_IN_LIST;

        // Chunks stored inline at this+0x10
        auto pChunk = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uintptr_t>(this) + 0x10 +
                                                    static_cast<uintptr_t>(nChunk) * 8);
        if (pChunk == 0)
            return nullptr;

        // Each identity entry is 0x70 bytes, entity ptr at offset 0x0
        return *reinterpret_cast<void**>(pChunk + static_cast<uintptr_t>(nSlot) * 0x70);
    }
};

// -----------------------------------------------------------------------
// Entity system helpers
// -----------------------------------------------------------------------
namespace EntitySystem
{
    // Get local player via IEngineClient::GetLocalPlayer() + entity system
    // No hardcoded offsets — survives game updates
    C_BaseEntity* GetLocalPlayerPawn();
    CCSPlayerController* GetLocalController();

    // Get the CGameEntitySystem singleton (cached from I::pEntitySystem)
    CGameEntitySystem* GetEntitySystem();
}
