// Schema system — runtime offset resolution via Source 2 SchemaSystem
// Walks ISchemaSystem -> CSchemaSystemTypeScope -> class bindings -> fields
// Caches FNV1A("ClassName->FieldName") -> offset for O(n) lookup

#include "schema.h"
#include "../utilities/debug.h"
#include "../utilities/memory.h"
#include <vector>
#include <format>
#include <algorithm>

// -----------------------------------------------------------------------
// Cached schema data — populated during Setup(), read during GetOffset()
// -----------------------------------------------------------------------
struct SchemaData_t
{
    FNV1A::Hash_t uHash = 0;
    uint32_t uOffset = 0;
};

static std::vector<SchemaData_t> s_schemaData;

// ISchemaSystem pointer — captured during Setup
static void* s_pSchemaSystem = nullptr;

// -----------------------------------------------------------------------
// ISchemaSystem vtable calls
// -----------------------------------------------------------------------

// vtable[13]: FindTypeScopeForModule(const char* moduleName) -> CSchemaSystemTypeScope*
static void* FindTypeScopeForModule(void* pSchemaSystem, const char* moduleName)
{
    return M::CallVFunc<void*>(pSchemaSystem, 13, moduleName, nullptr);
}

// -----------------------------------------------------------------------
// CSchemaSystemTypeScope vtable calls
// -----------------------------------------------------------------------

// vtable[2]: FindDeclaredClass(SchemaClassInfoData_t** ppOut, const char* className)
static void FindDeclaredClass(void* pTypeScope, SchemaClassInfoData_t** ppOut, const char* className)
{
    M::CallVFunc<void>(pTypeScope, 2, ppOut, className);
}

// -----------------------------------------------------------------------
// Schema::Setup — walk a module's schema and cache all field offsets
// -----------------------------------------------------------------------
bool Schema::Setup(const char* szModuleName)
{
    // Get ISchemaSystem from schemasystem.dll
    using CreateInterfaceFn = void* (*)(const char*, int*);
    auto pCreateInterface = reinterpret_cast<CreateInterfaceFn>(M::GetExport("schemasystem.dll", "CreateInterface"));
    if (!pCreateInterface)
    {
        C::Print("[schema] CreateInterface not found in schemasystem.dll");
        return false;
    }

    s_pSchemaSystem = pCreateInterface("SchemaSystem_001", nullptr);
    if (!s_pSchemaSystem)
    {
        C::Print("[schema] SchemaSystem_001 interface not found");
        return false;
    }

    // Get type scope for the target module
    void* pTypeScope = FindTypeScopeForModule(s_pSchemaSystem, szModuleName);
    if (!pTypeScope)
    {
        C::Print(std::format("[schema] type scope not found for {}", szModuleName));
        return false;
    }

    // Walk known classes we care about — use FindDeclaredClass per class
    // This avoids needing CUtlTSHash internal layout (which varies between builds)
    static const char* classNames[] = {
        // Entity hierarchy
        "C_BaseEntity",
        "C_BaseModelEntity",
        "C_BasePlayerPawn",
        "C_CSPlayerPawn",
        "CBasePlayerController",
        "CCSPlayerController",

        // Components
        "CGameSceneNode",
        "CSkeletonInstance",
        "CCollisionProperty",
        "CBodyComponentPoint",

        // Movement
        "CPlayer_MovementServices",
        "CPlayer_MovementServices_Humanoid",
        "CCSPlayer_MovementServices",

        // Weapons
        "C_BasePlayerWeapon",
        "C_CSWeaponBase",
        "CCSWeaponBaseVData",
        "C_WeaponCSBase",

        // Items/services
        "CCSPlayer_ItemServices",
        "CPlayer_WeaponServices",
        "CPlayer_CameraServices",

        // Global state
        "C_CSGameRules",
    };

    int totalFields = 0;
    char fieldBuf[512];

    for (const char* className : classNames)
    {
        SchemaClassInfoData_t* pClassInfo = nullptr;
        FindDeclaredClass(pTypeScope, &pClassInfo, className);

        if (!pClassInfo || !pClassInfo->pFields || pClassInfo->nFieldSize <= 0)
            continue;

        for (int i = 0; i < pClassInfo->nFieldSize; ++i)
        {
            auto& field = pClassInfo->pFields[i];
            if (!field.szName)
                continue;

            // Build "ClassName->FieldName" string and hash it
            snprintf(fieldBuf, sizeof(fieldBuf), "%s->%s", className, field.szName);

            s_schemaData.push_back({FNV1A::Hash(fieldBuf), field.nSingleInheritanceOffset});
            ++totalFields;
        }
    }

    C::Print(std::format("[schema] cached {} fields from {} classes in {}", totalFields, std::size(classNames),
                         szModuleName));

    return totalFields > 0;
}

// -----------------------------------------------------------------------
// Schema::GetOffset — look up cached offset by hash
// -----------------------------------------------------------------------
uint32_t Schema::GetOffset(FNV1A::Hash_t uHashedFieldName)
{
    for (const auto& entry : s_schemaData)
    {
        if (entry.uHash == uHashedFieldName)
            return entry.uOffset;
    }

    // Field not found — this is a bug (wrong field name or class not in list)
    C::Print(std::format("[schema] WARNING: offset not found for hash {:#x}", uHashedFieldName));
    return 0;
}
