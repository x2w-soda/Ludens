#pragma once

#include "RBackend.h"
#include <initializer_list>
#include <optional>
#include <unordered_map>

namespace LD {

/// @brief creates render passes on behalf of a render device
class RPassFactory
{
public:
    RPassFactory() = delete;
    RPassFactory(RDevice device);

    RPassFactory& add_color_attachment(const RPassColorAttachment& attachment);
    RPassFactory& add_depth_stencil_attachment(const RPassDepthStencilAttachment& attachment);
    RPassFactory& add_src_pass_dependency(const RPassDependency& dep);
    RPassFactory& add_dst_pass_dependency(const RPassDependency& dep);

    RPass build();

    /// @brief find a previously created render pass by its hash
    /// @return the corresponding render pass if present, or a null handle if not found
    static RPass find_by_hash(uint32_t hash);

    static void destroy_all(RDevice device);

private:
    RDevice mDevice;
    std::vector<RPassColorAttachment> mColorAttachments;
    std::optional<RPassDepthStencilAttachment> mDepthStencilAttachment;
    std::optional<RPassDependency> mSrcPassDependency;
    std::optional<RPassDependency> mDstPassDependency;

    static std::unordered_map<uint32_t, RPass> sPasses;
};

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