#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Passes/GBufferPass.h"

namespace LD
{

void GBufferPass::Startup(const GBufferPassInfo& info)
{
    LD_DEBUG_ASSERT(info.Device);
    mDevice = info.Device;
    mPositionFormat = info.PositionFormat;
    mNormalFormat = info.NormalFormat;
    mAlbedoFormat = info.AlbedoFormat;
    mDepthStencilFormat = info.DepthStencilFormat;

    Array<RPassAttachment, 4> attachments;
    attachments[0].InitialState = RState::Undefined;
    attachments[0].FinalState = RState::ShaderResource;
    attachments[0].Format = mPositionFormat;
    attachments[0].LoadOp = RLoadOp::Clear;
    attachments[0].StoreOp = RStoreOp::Store;

    attachments[1].InitialState = RState::Undefined;
    attachments[1].FinalState = RState::ShaderResource;
    attachments[1].Format = mNormalFormat;
    attachments[1].LoadOp = RLoadOp::Clear;
    attachments[1].StoreOp = RStoreOp::Store;

    attachments[2].InitialState = RState::Undefined;
    attachments[2].FinalState = RState::ShaderResource;
    attachments[2].Format = mAlbedoFormat;
    attachments[2].LoadOp = RLoadOp::Clear;
    attachments[2].StoreOp = RStoreOp::Store;

    attachments[3].InitialState = RState::Undefined;
    attachments[3].FinalState = RState::DepthStencilWrite;
    attachments[3].Format = mDepthStencilFormat;
    attachments[3].LoadOp = RLoadOp::Clear;
    attachments[3].StoreOp = RStoreOp::Discard;

    RPassInfo passI{};
    passI.Name = "GBufferPass";
    passI.Attachments = attachments.GetView();
    mDevice.CreateRenderPass(mHandle, passI);
}

void GBufferPass::Cleanup()
{
    mDevice.DeleteRenderPass(mHandle);
    mDevice.ResetHandle();
}

} // namespace LD