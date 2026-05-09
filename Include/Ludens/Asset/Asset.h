#pragma once

#include <Ludens/Asset/AssetDef.h>
#include <Ludens/DSA/String.h>
#include <Ludens/Header/Handle.h>

#include <cstddef>
#include <cstdint>

namespace LD {

struct AssetManagerObj;

/// @brief Get byte size of an asset type.
size_t get_asset_byte_size(AssetType type);

/// @brief Get static C string for asset type.
const char* get_asset_type_cstr(AssetType type);

/// @brief Get asset type for C string.
bool get_cstr_asset_type(const char* cstr, AssetType& outType);

/// @brief Base members of asset object implementation.
struct AssetObj
{
    AssetType type;
    AssetID id;
    AssetManagerObj* manager;
    bool isReserved;
};

/// @brief Asset raw handle, no ownership semantics.
struct Asset : Handle<struct AssetObj>
{
    Asset() = default;
    Asset(AssetObj* obj)
        : Handle<AssetObj>(obj) {}

    /// @brief Get asset type.
    AssetType get_type();

    /// @brief Get asset path.
    String get_path();

    /// @brief Get asset name.
    String get_name();

    /// @brief Get asset serial ID.
    AssetID get_id();
};

class Serializer;
class Deserializer;
class Diagnostics;
struct AssetLoadStatus;

/// @brief Write binary header for asset type.
/// @param serial Serializer used to write the header.
/// @param type Asset type information to serialize.
void asset_header_write(Serializer& serial, AssetType type);

/// @brief Attempts to read binary header from memory.
/// @param serial Deserializer used to read header.
/// @param outMajor Outputs the major engine version this asset is created with.
/// @param outMinor Outputs the minor engine version this asset is created with.
/// @param outPatch Outputs the patch engine version this asset is created with.
/// @param outType Outputs the asset type enum if recognized.
/// @return True if the header is recognized, and the serializer cursor sits right after the header.
bool asset_header_read(Deserializer& serial, uint16_t& outMajor, uint16_t& outMinor, uint16_t& outPatch, AssetType& outType);

/// @brief Attempts to read binary header from memory.
/// @return True if the asset type matches and the asset version matches engine version exactly.
bool asset_header_read(Deserializer& serial, AssetType expectedType, String& err);

} // namespace LD