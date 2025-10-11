#include "Extra/doctest/doctest.h"
#include <Ludens/Serial/Packet.h>
#include <Ludens/System/Memory.h>
#include <cassert>
#include <iostream>

using namespace LD;

static PacketSchema create_schema()
{
    PacketSchemaBuilder builder;
    PacketSchema schema = builder.new_schema("UpdateEntityPosition")
                              .add_field(PACKET_FIELD_U32, "EntityID")
                              .add_field(PACKET_FIELD_U16, "DeltaX")
                              .add_field(PACKET_FIELD_U16, "DeltaY")
                              .add_field(PACKET_FIELD_U16, "DeltaZ")
                              .add_field(PACKET_FIELD_BOOL, "OnGround")
                              .create();

    assert(schema);
    return schema;
}

TEST_CASE("PacketSchema basic")
{
    PacketSchema schema = create_schema();
    CHECK(schema);
    CHECK(schema.get_name() == "UpdateEntityPosition");
    CHECK(schema.get_packet_size() == 21);

    std::cout << schema.print() << std::endl;

    PacketSchema::destroy(schema);
    int leaks = get_memory_leaks(nullptr);
    CHECK(leaks == 0);
}