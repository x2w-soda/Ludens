#include <Ludens/Header/Hash.h>
#include <Ludens/Serial/Endianness.h>
#include <Ludens/Serial/Packet.h>
#include <Ludens/System/Memory.h>
#include <format>
#include <vector>

namespace LD {

// clang-format off
struct
{
    PacketFieldType type;
    const char* name;
    size_t byteSize;
} PacketFieldMeta[] = {
    {PACKET_FIELD_F32,  "F32",  4},
    {PACKET_FIELD_F64,  "F64",  8},
    {PACKET_FIELD_I8,   "I8",   1},
    {PACKET_FIELD_U8,   "U8",   1},
    {PACKET_FIELD_I16,  "I16",  2},
    {PACKET_FIELD_U16,  "U16",  2},
    {PACKET_FIELD_I32,  "I32",  4},
    {PACKET_FIELD_U32,  "U32",  4},
    {PACKET_FIELD_I64,  "I64",  8},
    {PACKET_FIELD_U64,  "U64",  8},
    {PACKET_FIELD_BOOL, "BOOL", 1},
};
// clang-format on

// will have to refactor if the field-type is not exactly two bytes.
static_assert(std::is_same_v<std::underlying_type_t<PacketFieldType>, uint16_t>);
static_assert(sizeof(PacketFieldMeta) / sizeof(*PacketFieldMeta) == PACKET_FIELD_TYPE_ENUM_COUNT);

static inline size_t get_packet_field_size(PacketFieldType type)
{
    return PacketFieldMeta[(int)type].byteSize;
}

static inline const char* get_packet_field_type_name(PacketFieldType type)
{
    return PacketFieldMeta[(int)type].name;
}

/// @brief Describes a single field in the packet.
struct PacketFieldEntry
{
    std::string name;
    size_t offset;
    Hash32 nameHash;
    PacketFieldType type;
};

/// @brief Packet schema implementation.
struct PacketSchemaObj
{
    size_t packetSize;
    std::string name;
    std::vector<PacketFieldEntry> fields;
};

void PacketSchema::destroy(PacketSchema schema)
{
    PacketSchemaObj* obj = schema.unwrap();

    heap_delete<PacketSchemaObj>(obj);
}

size_t PacketSchema::get_packet_size()
{
    return mObj->packetSize;
}

std::string PacketSchema::get_name()
{
    return mObj->name;
}

std::string PacketSchema::print()
{
    int fieldCount = (int)mObj->fields.size();

    std::string str = std::format("PacketSchema {} ({} bytes, {} fields)\n", mObj->name, mObj->packetSize, fieldCount);

    for (int i = 0; i < fieldCount; i++)
    {
        const PacketFieldEntry& field = mObj->fields[i];
        const char* typeName = get_packet_field_type_name(field.type);
        size_t size = get_packet_field_size(field.type);
        size_t offset = field.offset + 2; // PacketFieldType 2 bytes
        str += std::format("> Field {} {} {} ({}-{})\n", i, typeName, field.name, offset, offset + size - 1);
    }

    return str;
}

bool PacketSchema::validate(const void* packet, size_t packetSize)
{
    if (packetSize != mObj->packetSize)
        return false;

    size_t offset = 0;
    int fieldCount = (int)mObj->fields.size();
    for (int i = 0; i < fieldCount; i++)
    {
        const PacketFieldEntry& field = mObj->fields[i];
        uint16_t expectedType = (uint16_t)field.type;
        uint16_t actualType;

        le_bytes_to_u16((const byte*)packet + offset, actualType);

        if (actualType != expectedType)
            return false;
    }

    return true;
}

PacketSchemaBuilder& PacketSchemaBuilder::new_schema(const char* schemaName)
{
    if (mSchema)
        heap_delete<PacketSchemaObj>(mSchema);

    if (!schemaName)
        return *this;

    mSchema = heap_new<PacketSchemaObj>(MEMORY_USAGE_SCHEMA);
    mSchema->packetSize = 0;
    mSchema->name = std::string(schemaName);

    return *this;
}

PacketSchemaBuilder& PacketSchemaBuilder::add_field(PacketFieldType type, const char* fieldName)
{
    if (!mSchema || !fieldName)
        return *this;

    size_t offset = mSchema->packetSize;
    mSchema->fields.emplace_back(fieldName, offset, Hash32(fieldName), type);
    mSchema->packetSize += get_packet_field_size(type) + 2; // PacketFieldType 2 bytes

    return *this;
}

PacketSchema PacketSchemaBuilder::create()
{
    if (!mSchema)
        return {};

    PacketSchema schema(mSchema);
    mSchema = nullptr;

    return schema;
}

} // namespace LD