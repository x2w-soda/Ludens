#pragma once

#include <string>
#include "Core/Header/Include/Types.h"
#include "Core/RenderBase/Include/RTypes.h"
#include "Core/RenderBase/Include/RDevice.h"
#include "Core/DSA/Include/View.h"
#include "Core/DSA/Include/Vector.h"

namespace LD
{

struct RBindingInfo;
struct RBindingGroupInfo;
struct RBindingGroupLayoutInfo;
struct RBindingGroupBase;
struct RBindingGroupLayoutBase;
class RBindingGroupLayout;
class RBindingGroup;
class RTexture;

enum class RBindingType : u8
{
    Texture = 0,
    UniformBuffer,
};

/// Binding group handle and interface.
/// Describes a group of bindings that will be made visible to the shader together.
class RBindingGroup : public RHandle<RBindingGroupBase>
{
    friend struct RBindingGroupGL;
    friend struct RBindingGroupVK;

public:
    RResult BindTexture(u32 binding, RTexture& textureH, int arrayIndex = 0);
    RResult BindUniformBuffer(u32 binding, RBuffer& bufferH);

private:
    RBackend mBackend;
};

/// Binding group layout handle and interface.
/// Used during binding group creation and pipeline creation.
class RBindingGroupLayout : public RHandle<RBindingGroupLayoutBase>
{
};

struct RBindingInfo
{
    RBindingType Type;
    int Count = 1;
};

// Info to create a binding group.
struct RBindingGroupInfo
{
    RBindingGroupLayout Layout;
};

// Info to create a binding group layout
struct RBindingGroupLayoutInfo
{
    View<RBindingInfo> Bindings;
};

/// plain old data describing the layout of a binding group,
/// without creating actual resources such as RBindingGroupLayout
using RBindingGroupLayoutData = SmallVector<RBindingInfo, 8>;

} // namespace LD
