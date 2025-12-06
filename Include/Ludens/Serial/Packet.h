#pragma once

#include <Ludens/Header/Handle.h>
#include <Ludens/Serial/Value.h>
#include <cstdint>
#include <string>

namespace LD {

/// @brief A packet schema describes how a binary packet
///        can be safely serialized.
struct PacketSchema : Handle<struct PacketSchemaObj>
{
    /// @brief Free the packet schema.
    static void destroy(PacketSchema schema);

    /// @brief Get packet byte size.
    size_t get_packet_size();

    /// @brief Get name of packet schema.
    std::string get_name();

    /// @brief Print packet schema into human readable string.
    std::string print();

    /// @brief Check if a packet follows this schema by comparing
    ///        its size and individual field types.
    bool validate(const void* packet, size_t packetSize);
};

/// @brief Builder API to create a packet schema.
class PacketSchemaBuilder
{
public:
    /// @brief Begin building a new schema.
    PacketSchemaBuilder& new_schema(const char* schemaName);

    /// @brief Append a field to the current schema being defined.
    PacketSchemaBuilder& add_field(ValueType type, const char* fieldName);

    /// @brief Finalize and create the schema.
    /// @return The handle to the new schema on success.
    /// @warn User is expected to call PacketSchema::destroy on the returned value later.
    PacketSchema create();

private:
    struct PacketSchemaObj* mSchema = nullptr;
};

} // namespace LD