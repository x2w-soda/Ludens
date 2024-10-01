#pragma once

#include "Core/RenderBase/Include/RDevice.h"
#include "Core/RenderBase/Include/RTexture.h"
#include "Core/DSA/Include/View.h"

namespace LD
{

enum class RLoadOp
{
    Discard = 0,
    Load,
    Clear
};

enum class RStoreOp
{
    Discard = 0,
    Store,
};

enum class RState
{
    Undefined = 0,
    ColorAttachment,
    Present,
    DepthStencilRead,
    DepthStencilWrite,
    ShaderResource,
};

struct RPassAttachment
{
    RTextureFormat Format;
    RState InitialState;
    RState FinalState;
    RLoadOp LoadOp;
    RStoreOp StoreOp;
};

struct RPassInfo
{
    const char* Name = nullptr;
    View<RPassAttachment> Attachments;
};

struct RPassBase;
struct RPassGL;
struct RPassVK;

// render pass handle and interface
class RPass : public RHandle<RPassBase>
{
public:

    bool HasDepthStencilAttachment();
};

} // namespace LD
