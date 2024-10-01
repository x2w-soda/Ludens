#include "Core/DSA/Include/Array.h"
#include "Core/RenderFX/Include/Passes/ColorPass.h"

namespace LD
{

void ColorPass::Startup(const ColorPassInfo& info)
{
    mDevice = info.Device;
    mColorFormat = info.ColorFormat;

    Array<RPassAttachment, 1> colorAttachment;
    colorAttachment[0].Format = info.ColorFormat;
    colorAttachment[0].InitialState = info.InitialState;
    colorAttachment[0].FinalState = info.FinalState;
    colorAttachment[0].LoadOp = RLoadOp::Discard;
    colorAttachment[0].StoreOp = RStoreOp::Store;

    RPassInfo passI;
    passI.Name = "ColorPass";
    passI.Attachments = colorAttachment.GetView();
    mDevice.CreateRenderPass(mHandle, passI);
}

void ColorPass::Cleanup()
{
    mDevice.DeleteRenderPass(mHandle);
    mDevice.ResetHandle();
}

} // namespace LD