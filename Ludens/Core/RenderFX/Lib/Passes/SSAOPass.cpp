#include "Core/RenderFX/Include/Passes/SSAOPass.h"

namespace LD
{

void SSAOPass::Startup(RDevice device)
{
    mDevice = device;

    RPassAttachment color;
    color.Format = RTextureFormat::R8;
    color.InitialState = RState::Undefined;
    color.FinalState = RState::ShaderResource;
    color.LoadOp = RLoadOp::Discard;
    color.StoreOp = RStoreOp::Store;

    RPassInfo passI;
    passI.Attachments = { 1, &color };
    mDevice.CreateRenderPass(mHandle, passI);
}

void SSAOPass::Cleanup()
{
    mDevice.DeleteRenderPass(mHandle);
    mDevice.ResetHandle();
}

} // namespace LD