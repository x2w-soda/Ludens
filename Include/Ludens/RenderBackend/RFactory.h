#pragma once

#include "RBackend.h"
#include <initializer_list>
#include <unordered_map>

namespace LD {

/// @brief creates resource set layouts on behalf of a render device
class RSetLayoutFactory
{
public:
    RSetLayoutFactory() = delete;
    RSetLayoutFactory(RDevice device);

    RSetLayoutFactory& add_binding(const RSetBindingInfo& bindingI);

    RSetLayout build();

    /// @brief find a previously created resource set layout by its hash
    /// @return the corresponding layout if present, or a null handle if not found
    static RSetLayout find_by_hash(uint32_t hash);

    static void destroy_all(RDevice device);

private:
    RDevice mDevice;
    std::vector<RSetBindingInfo> mBindings;

    static std::unordered_map<uint32_t, RSetLayout> sSetLayouts;
};

/// @brief creates pipeline layouts on behalf of a render device.
class RPipelineLayoutFactory
{
public:
    RPipelineLayoutFactory() = delete;
    RPipelineLayoutFactory(RDevice device);

    RPipelineLayoutFactory& add_set_layout(const std::initializer_list<RSetBindingInfo>& list);

    RPipelineLayout build();

    /// @brief find a previously created pipeline layout by its hash
    /// @return the corresponding layout if present, or a null handle if not found
    static RPipelineLayout find_by_hash(uint32_t hash);

    static void destroy_all(RDevice device);

private:
    RDevice mDevice;
    std::vector<RSetLayout> mSetLayouts;

    static std::unordered_map<uint32_t, RPipelineLayout> sPipelineLayouts;
};

} // namespace LD