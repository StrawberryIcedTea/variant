// Chams -- colored player model rendering via GeneratePrimitives hook

#include "chams.h"
#include "vmats.h"
#include "../../../sdk/entity.h"
#include "../../../core/interfaces.h"
#include "../../../core/variables.h"
#include "../../../core/convars.h"
#include "../../../utilities/memory.h"
#include "../../../utilities/debug.h"
#include <format>

static constexpr uint64_t k_kv3Unk0 = 0x469806E97412167CULL;
static constexpr uint64_t k_kv3Unk1 = 0xE73790B53EE6F2AFULL;

// s_pMat[type][xqz]: type 0=Flat 1=Glow, xqz 0=visible 1=through-wall
static CMaterial2* s_pMat[2][2] = {};
static void* s_pHolder[2][2] = {};

static CMaterial2* CreateMat(const char* szName, const char* szVmat, void*& outHolder)
{
    if (!I::fnLoadKV3 || !I::fnCreateMaterial || !I::pMaterialSystem)
        return nullptr;

    auto* buf = new uint8_t[0x100 + 0x200]();
    auto* kv3 = reinterpret_cast<CKeyValues3*>(buf + 0x100);

    KV3ID_t id{"generic", k_kv3Unk0, k_kv3Unk1};
    if (!I::fnLoadKV3(kv3, nullptr, szVmat, &id, nullptr))
    {
        C::Print(std::format("[chams] LoadKV3 failed: {}", szName));
        delete[] buf;
        return nullptr;
    }

    void* pHolder = nullptr;
    I::fnCreateMaterial(I::pMaterialSystem, reinterpret_cast<CMaterial2**>(&pHolder), szName, kv3, 0, 1);
    if (!pHolder)
    {
        C::Print(std::format("[chams] CreateMaterial failed: {}", szName));
        delete[] buf;
        return nullptr;
    }

    // CreateMaterial returns a holder/wrapper; actual CMaterial2 is at holder[0]
    CMaterial2* pMat = *reinterpret_cast<CMaterial2**>(pHolder);
    uintptr_t vtbl = pMat ? *reinterpret_cast<uintptr_t*>(pMat) : 0;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!vtbl || !VirtualQuery(reinterpret_cast<void*>(vtbl), &mbi, sizeof(mbi)) || mbi.Type != MEM_IMAGE)
    {
        C::Print(std::format("[chams] bad vtable: {}", szName));
        delete[] buf;
        return nullptr;
    }

    outHolder = pHolder;
    delete[] buf;
    return pMat;
}

bool Chams::Setup()
{
    s_pMat[0][0] = CreateMat("variant/chams_flat_vis.vmat", VMat::Flat, s_pHolder[0][0]);
    s_pMat[0][1] = CreateMat("variant/chams_flat_xqz.vmat", VMat::FlatXQZ, s_pHolder[0][1]);
    s_pMat[1][0] = CreateMat("variant/chams_glow_vis.vmat", VMat::Glow, s_pHolder[1][0]);
    s_pMat[1][1] = CreateMat("variant/chams_glow_xqz.vmat", VMat::GlowXQZ, s_pHolder[1][1]);
    return s_pMat[0][0] != nullptr;
}

static void TintRange(CMeshPrimitiveOutputBuffer* pBuf, int from, int to, Color_t col)
{
    for (int i = from; i < to; ++i)
        pBuf->m_out[i].colValue = col;
}

static void MaterialRange(CMeshPrimitiveOutputBuffer* pBuf, int from, int to, CMaterial2* pMat, Color_t col)
{
    for (int i = from; i < to; ++i)
    {
        pBuf->m_out[i].pMaterial = pMat;
        pBuf->m_out[i].colValue = col;
    }
}

static Color_t FloatToColor(const float* c)
{
    return {static_cast<uint8_t>(c[0] * 255.f), static_cast<uint8_t>(c[1] * 255.f), static_cast<uint8_t>(c[2] * 255.f),
            static_cast<uint8_t>(c[3] * 255.f)};
}

void Chams::OnGeneratePrimitives(GeneratePrimitivesFn oFn, void* pThis, CSceneAnimatableObject* pObject, void* pUnk,
                                 CMeshPrimitiveOutputBuffer* pBuf)
{
    static bool s_ready = Setup();

    if (!Vars.bChams || !IsValidPtr(pObject) || !IsValidPtr(pBuf))
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    auto* pES = EntitySystem::GetEntitySystem();
    if (!IsValidPtr(pES))
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    auto* pEnt = pES->Get<C_BaseEntity>(pObject->hOwner);
    if (!IsValidPtr(pEnt))
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    uint8_t team = pEnt->GetTeamNum();
    if (team != 2 && team != 3)
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    // Only cham player pawns — weapons and viewmodels share team 2/3 with their owner,
    // so we verify the entity handle matches a controller's m_hPlayerPawn.
    bool isPlayerPawn = false;
    for (int i = 1; i <= 64; ++i)
    {
        auto* pCtrl = pES->Get<CCSPlayerController>(i);
        if (pCtrl && pCtrl->GetPlayerPawnHandle().nIndex == pObject->hOwner.nIndex)
        {
            isPlayerPawn = true;
            break;
        }
    }
    if (!isPlayerPawn)
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    auto* pLocalPawn = EntitySystem::GetLocalPlayerPawn();
    bool isSelf = (pEnt == pLocalPawn);
    uint8_t localTeam = pLocalPawn ? pLocalPawn->GetTeamNum() : 0;
    bool isEnemy = (team != localTeam);

    if (isSelf)
    {
        if (!Vars.bChamsSelf)
        {
            oFn(pThis, pObject, pUnk, pBuf);
            return;
        }

        // Applying chams in first-person makes feet/legs invisible.
        // Suppress when alive unless cl_thirdperson is on.
        static CConVar* s_cvThirdPerson = nullptr;
        if (!s_cvThirdPerson)
            s_cvThirdPerson = Convar::Find("cl_thirdperson");
        bool thirdPerson = s_cvThirdPerson && s_cvThirdPerson->value.i32 != 0;

        auto* pLocalCtrl = EntitySystem::GetLocalController();
        if (!thirdPerson && pLocalCtrl && pLocalCtrl->IsPawnAlive())
        {
            oFn(pThis, pObject, pUnk, pBuf);
            return;
        }
    }
    else if (isEnemy && !Vars.bChamsEnemies)
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }
    else if (!isEnemy && !Vars.bChamsTeammates)
    {
        oFn(pThis, pObject, pUnk, pBuf);
        return;
    }

    auto pickColor = [&](const float* enemy, const float* team, const float* self) -> Color_t
    { return FloatToColor(isSelf ? self : (isEnemy ? enemy : team)); };

    Color_t col1 = pickColor(Vars.flChamsEnemyColor, Vars.flChamsTeamColor, Vars.flChamsSelfColor);
    Color_t col2 = pickColor(Vars.flChamsEnemyColor2, Vars.flChamsTeamColor2, Vars.flChamsSelfColor2);

    int t1 = Vars.nChamsMaterial < 2 ? Vars.nChamsMaterial : 0;

    auto applyPass = [&](int from, int to, int type, bool xqz, Color_t col)
    {
        CMaterial2* pMat = s_pMat[type][xqz ? 1 : 0];
        if (pMat)
            MaterialRange(pBuf, from, to, pMat, col);
        else
            TintRange(pBuf, from, to, col);
    };

    if (Vars.bChamsDouble)
    {
        int t2 = Vars.nChamsMaterial2 < 2 ? Vars.nChamsMaterial2 : 0;

        int b1 = pBuf->m_count;
        oFn(pThis, pObject, pUnk, pBuf);
        applyPass(b1, pBuf->m_count, t1, Vars.bChamsXQZ, col1);

        int b2 = pBuf->m_count;
        oFn(pThis, pObject, pUnk, pBuf);
        applyPass(b2, pBuf->m_count, t2, Vars.bChamsXQZ, col2);
    }
    else
    {
        int b = pBuf->m_count;
        oFn(pThis, pObject, pUnk, pBuf);
        applyPass(b, pBuf->m_count, t1, Vars.bChamsXQZ, col1);
    }
}
