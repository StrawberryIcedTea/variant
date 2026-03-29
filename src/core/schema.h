#pragma once
#include <cstdint>

// -----------------------------------------------------------------------
// FNV-1a 64-bit hash — compile-time + runtime
// Used to hash schema field names ("ClassName->FieldName")
// -----------------------------------------------------------------------
namespace FNV1A
{
    using Hash_t = uint64_t;

    constexpr Hash_t kBasis = 0xCBF29CE484222325ULL;
    constexpr Hash_t kPrime = 0x100000001B3ULL;

    // Compile-time hash (consteval)
    consteval Hash_t HashConst(const char* str, Hash_t key = kBasis) noexcept
    {
        return (str[0] == '\0') ? key : HashConst(&str[1], (key ^ static_cast<Hash_t>(str[0])) * kPrime);
    }

    // Runtime hash
    inline Hash_t Hash(const char* str, Hash_t key = kBasis) noexcept
    {
        while (*str)
        {
            key = (key ^ static_cast<Hash_t>(*str)) * kPrime;
            ++str;
        }
        return key;
    }
}

// -----------------------------------------------------------------------
// Schema structs — Source 2 schema system types (stable across builds)
// -----------------------------------------------------------------------

// Single field in a schema class
struct SchemaClassFieldData_t
{
    const char* szName;                // 0x00
    void* pSchemaType;                 // 0x08
    uint32_t nSingleInheritanceOffset; // 0x10
    int32_t nMetadataSize;             // 0x14
    void* pMetaData;                   // 0x18
};

// Class info returned by FindDeclaredClass
struct SchemaClassInfoData_t
{
    void* pVtable;                   // 0x00
    const char* szName;              // 0x08
    char* szDescription;             // 0x10
    int32_t m_nSize;                 // 0x18
    int16_t nFieldSize;              // 0x1C
    int16_t nStaticSize;             // 0x1E
    int16_t nMetadataSize;           // 0x20
    uint8_t nAlignOf;                // 0x22
    uint8_t nBaseClassesCount;       // 0x23
    char pad[0x4];                   // 0x24
    SchemaClassFieldData_t* pFields; // 0x28
};

// -----------------------------------------------------------------------
// Schema namespace — runtime offset resolution via Source 2 SchemaSystem
// -----------------------------------------------------------------------
namespace Schema
{
    // Initialize: walk schema for module, cache all field hashes + offsets
    bool Setup(const char* szModuleName = "client.dll");

    // Look up a cached field offset by its FNV1A hash ("ClassName->FieldName")
    uint32_t GetOffset(FNV1A::Hash_t uHashedFieldName);
}

// -----------------------------------------------------------------------
// Schema field accessor macros
// -----------------------------------------------------------------------

// SCHEMA_ADD_FIELD(type, GetterName, "ClassName->m_fieldName")
// Generates a method that resolves the offset once at first call
#define SCHEMA_ADD_FIELD(TYPE, NAME, FIELD)                                                                            \
    [[nodiscard]] inline TYPE& NAME()                                                                                  \
    {                                                                                                                  \
        static const uint32_t uOffset = Schema::GetOffset(FNV1A::HashConst(FIELD));                                    \
        return *reinterpret_cast<TYPE*>(reinterpret_cast<uint8_t*>(this) + uOffset);                                   \
    }

// SCHEMA_ADD_PFIELD(type, GetterName, "ClassName->m_fieldName")
// Same but returns a pointer instead of a reference
#define SCHEMA_ADD_PFIELD(TYPE, NAME, FIELD)                                                                           \
    [[nodiscard]] inline TYPE* NAME()                                                                                  \
    {                                                                                                                  \
        static const uint32_t uOffset = Schema::GetOffset(FNV1A::HashConst(FIELD));                                    \
        return reinterpret_cast<TYPE*>(reinterpret_cast<uint8_t*>(this) + uOffset);                                    \
    }

// SCHEMA_ADD_FIELD_OFFSET(type, GetterName, "ClassName->m_fieldName", extra_offset)
// Schema-resolved offset + additional static offset (e.g. VData at m_nSubclassID + 0x8)
#define SCHEMA_ADD_FIELD_OFFSET(TYPE, NAME, FIELD, EXTRA)                                                              \
    [[nodiscard]] inline TYPE& NAME()                                                                                  \
    {                                                                                                                  \
        static const uint32_t uOffset = Schema::GetOffset(FNV1A::HashConst(FIELD));                                    \
        return *reinterpret_cast<TYPE*>(reinterpret_cast<uint8_t*>(this) + uOffset + (EXTRA));                         \
    }

// SCHEMA_ADD_OFFSET(type, GetterName, hardcoded_offset)
// For fields not in the schema (e.g. engine internals)
#define SCHEMA_ADD_OFFSET(TYPE, NAME, OFFSET)                                                                          \
    [[nodiscard]] inline TYPE& NAME()                                                                                  \
    {                                                                                                                  \
        return *reinterpret_cast<TYPE*>(reinterpret_cast<uint8_t*>(this) + (OFFSET));                                  \
    }
