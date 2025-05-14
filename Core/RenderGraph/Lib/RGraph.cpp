#include "RGraphObj.h"
#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/RenderBackend/RUtil.h>
#include <Ludens/RenderGraph/RGraph.h>
#include <Ludens/System/Memory.h>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stack>
#include <utility>

namespace LD {

struct ImageState
{
    RImageLayout lastLayout;
    RImageUsageFlags usage;
    RImage handle;
    uint32_t width;
    uint32_t height;
    uint32_t hash;
};

/// @brief physical resource storage
struct RStorage
{
    std::unordered_map<uint32_t, ImageState> images;
};

/// @brief While the render graph is an immediate mode API describing virtual resources,
///        the actual resources should not be recreated every frame. Currently each
///        Component has its own storage.
static std::unordered_map<uint32_t, RStorage> sStorages;

static std::stack<std::pair<void*, RGraph::OnReleaseCallback>> sReleaseCallbacks;
static std::stack<std::pair<void*, RGraph::OnReleaseCallback>> sDestroyCallbacks;

/// @brief returns true if srcUsage and dstUsage might cause pipeline hazards, and needs a happens-before access separation.
static bool has_image_dependency(RGraphImageUsage srcUsage, RGraphImageUsage dstUsage)
{
    return (srcUsage == RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT && dstUsage == RGRAPH_IMAGE_USAGE_SAMPLED) ||        // RAW
           (srcUsage == RGRAPH_IMAGE_USAGE_SAMPLED && dstUsage == RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT) ||        // WAR
           (srcUsage == RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT && dstUsage == RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT); // WAW
}

static bool has_pass_dependency(RGraphicsPass src, RGraphicsPass dst, RPassDependency& dep)
{
    // TODO:

    const RGraphicsPassObj* srcObj = src;
    const RGraphicsPassObj* dstObj = dst;
    dep.srcAccessMask = srcObj->accessFlags;
    dep.dstAccessMask = dstObj->accessFlags;
    dep.srcStageMask = srcObj->stageFlags;
    dep.dstStageMask = dstObj->stageFlags;

    return true;
}

/// @brief returns false upon invalid input
static inline bool check_graphics_pass_image(RGraphicsPassObj* passObj, Name name)
{
    RComponentObj* compObj = passObj->component;

    if (!compObj->images.contains(name))
    {
        printf("image not found in component\n");
        return false;
    }

    if (passObj->imageUsages.contains(name))
    {
        printf("image already used in grahpics pass\n");
        return false;
    }

    return true;
}

/// @brief returns false upon invalid input
static inline bool check_loadop_clear_value(RAttachmentLoadOp loadOp, const void* clear)
{
    if (loadOp == RATTACHMENT_LOAD_OP_CLEAR && clear == nullptr)
    {
        printf("forgot to supply clear value\n");
        return false;
    }

    if (loadOp != RATTACHMENT_LOAD_OP_CLEAR && clear != nullptr)
    {
        printf("redundant clear value\n");
        return false;
    }

    return true;
}

static GraphImage& dereference_image(RComponentObj** compObj, Name* name)
{
    LD_ASSERT(compObj && name);

    if ((*compObj)->imageRefs.contains(*name))
    {
        RComponentObj* srcCompObj = (*compObj)->imageRefs[*name].srcComponent;
        Name srcOutName = (*compObj)->imageRefs[*name].srcOutputName;

        *compObj = srcCompObj;
        *name = srcOutName;

        return srcCompObj->images[srcOutName];
    }

    LD_ASSERT((*compObj)->images.contains(*name));
    return (*compObj)->images[*name];
}

/// @brief map render graph image usage to render backend bit flags
static RImageUsageFlags get_native_image_usage(RGraphImageUsage renderGraphUsage)
{
    switch (renderGraphUsage)
    {
    case RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT:
        return RIMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    case RGRAPH_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT:
        return RIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    case RGRAPH_IMAGE_USAGE_SAMPLED:
        return RIMAGE_USAGE_SAMPLED_BIT;
    }

    LD_UNREACHABLE;
}

/// @brief hash of an image based on physical dimensions and declared name
static uint32_t get_image_hash(RImageUsageFlags usage, RFormat format, Name name)
{
    std::size_t hash = (std::size_t)usage;

    hash_combine(hash, (uint32_t)format);
    hash_combine(hash, (uint32_t)name);

    return (uint32_t)hash;
}

/// @brief associates user declared name with actual image resource
static RImage get_or_create_image(RGraphObj* graphObj, RComponentObj* compObj, Name name)
{
    LD_PROFILE_SCOPE;

    RDevice device = graphObj->info.device;
    GraphImage& graphImage = dereference_image(&compObj, &name);

    RImageInfo imageI{};
    imageI.type = RIMAGE_TYPE_2D;
    imageI.samples = RSAMPLE_COUNT_1_BIT;
    imageI.usage = graphImage.usage;
    imageI.format = graphImage.format;
    imageI.layers = 1;
    imageI.width = graphImage.width;
    imageI.height = graphImage.height;
    imageI.sampler = graphImage.sampler;
    imageI.depth = 1;

    RStorage& storage = sStorages[compObj->name];
    LD_ASSERT(storage.images.contains(name));
    ImageState& state = storage.images[name];

    // usage generalization: dont invalidate image when usage narrows
    imageI.usage |= state.usage;

    // size generalization: dont invalidate image when size shrinks
    imageI.width = std::max(state.width, imageI.width);
    imageI.height = std::max(state.height, imageI.height);

    uint32_t imageHash = get_image_hash(imageI.usage, imageI.format, name);

    // create or invalidate image
    if (!storage.images[name].handle || storage.images[name].hash != imageHash)
    {
        LD_PROFILE_SCOPE_NAME("get_or_create_image invalidate");

        if (!state.handle)
            state.usage = imageI.usage;

        if (state.handle && imageHash != state.hash)
        {
            // NOTE: invalidation is slow path, we must wait until GPU finishes work
            //       from frames in flight before destroying images.
            device.wait_idle();
            device.destroy_image(state.handle);
        }

        state.lastLayout = RIMAGE_LAYOUT_UNDEFINED;
        state.usage = imageI.usage;
        state.width = imageI.width;
        state.height = imageI.height;
        state.handle = device.create_image(imageI);
        state.hash = imageHash;
    }

    LD_ASSERT(state.handle);
    return state.handle;
}

static bool is_physical_image(const GraphImage& image)
{
    return image.type == NODE_TYPE_OUTPUT;
}

static void topological_visit(std::unordered_set<uint32_t>& visited, std::vector<RGraphicsPass>& order, RGraphicsPass pass)
{
    const RGraphicsPassObj* passObj = pass;

    if (visited.contains(passObj->name))
        return;

    visited.insert(passObj->name);

    for (RGraphicsPass other : passObj->edges)
    {
        if (visited.contains(other.name()))
            continue;

        topological_visit(visited, order, other);
    }

    order.push_back(pass);
}

/// @brief sort all graphics passes in dependency order
static void topological_sort(const std::unordered_map<Name, RComponent, NameHash>& components, std::vector<RGraphicsPass>& order)
{
    LD_PROFILE_SCOPE;

    std::unordered_set<uint32_t> visited;

    for (auto& compIte : components)
    {
        const RComponentObj* comp = compIte.second;

        for (auto& passIte : comp->graphicsPasses)
        {
            if (visited.contains(passIte.first))
                continue;

            topological_visit(visited, order, passIte.second);
        }
    }

    std::reverse(order.begin(), order.end());
}

static void save_graph_to_dot(RGraphObj* graphObj, const char* path)
{
    std::ostringstream os;
    std::filesystem::path fsPath(path);
    std::ofstream outFile(fsPath);

    os << "digraph RenderGraph {\n";
    os << "bgcolor = \"#181818\"\n";
    os << "node [shape = plain, fontcolor = \"#e6e6e6\", color = \"#e6e6e6\"];\n";

    for (const RGraphicsPassObj* passObj : graphObj->passOrder)
    {
        os << '"' << passObj->debugName << '"' << "[label = <<table><tr><td>" << passObj->debugName << "</td></tr></table>>]\n";
    }

    for (const RGraphicsPassObj* srcPassObj : graphObj->passOrder)
    {
        for (const RGraphicsPassObj* dstPassObj : srcPassObj->edges)
        {
            os << '"' << srcPassObj->debugName << '"' << " -> " << '"' << dstPassObj->debugName << '"' << "[color = \"#e6e6e6\"]" << '\n';
        }
    }

    os << "}";

    if (!outFile)
    {
        std::cerr << "save_graph_to_dot: failed to open file for writing: " << path << std::endl;
        return;
    }

    outFile << os.str();
    outFile.close();

    std::cout << "save_graph_to_dot: written to " << path << std::endl;
}

Name RGraphicsPass::name() const
{
    return mObj->name;
}

void RGraphicsPass::use_image_sampled(Name name)
{
    LD_PROFILE_SCOPE;

    if (!check_graphics_pass_image(mObj, name))
        return;

    RComponentObj* compObj = mObj->component;
    GraphImage& graphImage = compObj->images[name];

    mObj->sampledImages.insert(name);

    // how the pass uses the image
    mObj->imageUsages[name] = RGRAPH_IMAGE_USAGE_SAMPLED;

    // how the component uses the image
    compObj->images[name].usage |= get_native_image_usage(RGRAPH_IMAGE_USAGE_SAMPLED);

    // if existing passes in the component also use this image, check for dependencies
    for (RGraphicsPassObj* srcPassObj : compObj->graphicsPassOrder)
    {
        Name srcPassName = srcPassObj->name;

        if (mObj->name == srcPassName)
            break;

        if (srcPassObj->imageUsages.contains(name) && has_image_dependency(srcPassObj->imageUsages[name], mObj->imageUsages[name]))
            srcPassObj->edges.insert(RGraphicsPass{mObj});
    }
}

void RGraphicsPass::use_color_attachment(Name name, RAttachmentLoadOp loadOp, const RClearColorValue* clear)
{
    LD_PROFILE_SCOPE;

    if (!check_graphics_pass_image(mObj, name))
        return;

    if (!check_loadop_clear_value(loadOp, clear))
        return;

    RComponentObj* compObj = mObj->component;

    const GraphImage& image = compObj->images[name];

    // how the pass uses the image
    mObj->imageUsages[name] = RGRAPH_IMAGE_USAGE_COLOR_ATTACHMENT;

    // how the component uses the image
    compObj->images[name].usage |= RIMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    RGraphicsPassColorAttachment attachment{};
    attachment.name = name;
    if (clear)
        attachment.clearValue = *clear;

    RPassColorAttachment attachmentInfo{};
    attachmentInfo.colorFormat = image.format;
    attachmentInfo.colorLoadOp = loadOp;
    attachmentInfo.colorStoreOp = RATTACHMENT_STORE_OP_STORE;   // resolved by render graph
    attachmentInfo.initialLayout = RIMAGE_LAYOUT_UNDEFINED;     // resolved by render graph
    attachmentInfo.passLayout = RIMAGE_LAYOUT_COLOR_ATTACHMENT; // use_color_attachment

    mObj->colorAttachments.push_back(std::move(attachment));
    mObj->colorAttachmentInfos.push_back(std::move(attachmentInfo));

    mObj->accessFlags |= RACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    mObj->stageFlags |= RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // if existing passes in the component also use this image, check for dependencies
    for (RGraphicsPassObj* srcPassObj : compObj->graphicsPassOrder)
    {
        Name srcPassName = srcPassObj->name;

        if (mObj->name == srcPassName)
            break;

        if (srcPassObj->imageUsages.contains(name) && has_image_dependency(srcPassObj->imageUsages[name], mObj->imageUsages[name]))
            srcPassObj->edges.insert(RGraphicsPass{mObj});
    }
}

void RGraphicsPass::use_depth_stencil_attachment(Name name, RAttachmentLoadOp loadOp, const RClearDepthStencilValue* clear)
{
    LD_PROFILE_SCOPE;

    if (!check_graphics_pass_image(mObj, name))
        return;

    if (!check_loadop_clear_value(loadOp, clear))
        return;

    RComponentObj* compObj = mObj->component;
    const GraphImage& image = compObj->images[name];

    if (mObj->hasDepthStencil)
    {
        printf("already using a depth stencil attachment\n");
        return;
    }

    mObj->hasDepthStencil = true;

    // how the pass uses the image
    mObj->imageUsages[name] = RGRAPH_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT;

    // how the component uses the image
    compObj->images[name].usage |= RIMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    mObj->depthStencilAttachment.name = name;
    if (clear)
        mObj->depthStencilAttachment.clearValue = *clear;

    mObj->depthStencilAttachmentInfo.depthStencilFormat = image.format;
    mObj->depthStencilAttachmentInfo.depthLoadOp = loadOp;
    mObj->depthStencilAttachmentInfo.depthStoreOp = RATTACHMENT_STORE_OP_STORE;           // resolved by render graph
    mObj->depthStencilAttachmentInfo.initialLayout = RIMAGE_LAYOUT_UNDEFINED;             // resolved by render graph
    mObj->depthStencilAttachmentInfo.stencilLoadOp = RATTACHMENT_LOAD_OP_DONT_CARE;       // TODO:
    mObj->depthStencilAttachmentInfo.stencilStoreOp = RATTACHMENT_STORE_OP_DONT_CARE;     // TODO:
    mObj->depthStencilAttachmentInfo.passLayout = RIMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT; // use_depth_stencil_attachment

    mObj->accessFlags |= RACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    mObj->stageFlags |= RPIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | RPIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
}

RImage RGraphicsPass::get_image(Name name)
{
    if (!mObj->isCallbackScope)
    {
        printf("get_image can only be called during callback");
        return {};
    }

    RComponentObj* compObj = mObj->component;
    dereference_image(&compObj, &name);
    RStorage& storage = sStorages[compObj->name];

    RImage imageHandle = storage.images[name].handle;
    LD_ASSERT(imageHandle);
    return imageHandle;
}

Name RComponent::name() const
{
    return mObj->name;
}

void RComponent::add_private_image(const char* nameStr, RFormat format, uint32_t width, uint32_t height, RSamplerInfo* sampler)
{
    Name name(nameStr);

    if (mObj->images.contains(name))
    {
        printf("image already declared in component\n");
        return;
    }

    mObj->images[name] = {
        .type = NODE_TYPE_PRIVATE,
        .name = name,
        .debugName = nameStr,
        .format = format,
        .width = width,
        .height = height,
    };

    if (sampler)
        mObj->images[name].sampler = *sampler;

    // first time this private image is declared in this component
    if (!sStorages[mObj->name].images.contains(name))
    {
        ImageState& state = sStorages[mObj->name].images[name];
        state.lastLayout = RIMAGE_LAYOUT_UNDEFINED;
        state.width = width;
        state.height = height;
    }
}

void RComponent::add_output_image(const char* nameStr, RFormat format, uint32_t width, uint32_t height, RSamplerInfo* sampler)
{
    Name name(nameStr);

    if (mObj->images.contains(name))
    {
        printf("image already declared in component\n");
        return;
    }

    mObj->images[name] = {
        .type = NODE_TYPE_OUTPUT,
        .name = name,
        .debugName = nameStr,
        .format = format,
        .width = width,
        .height = height,
    };

    if (sampler)
        mObj->images[name].sampler = *sampler;

    // first time this output image is declared in this component
    if (!sStorages[mObj->name].images.contains(name))
    {
        ImageState& state = sStorages[mObj->name].images[name];
        state.lastLayout = RIMAGE_LAYOUT_UNDEFINED;
        state.width = width;
        state.height = height;
    }
}

void RComponent::add_input_image(const char* nameStr, RFormat format, uint32_t width, uint32_t height)
{
    Name name(nameStr);

    if (mObj->images.contains(name))
    {
        printf("image already declared in component\n");
        return;
    }

    mObj->images[name] = {
        .type = NODE_TYPE_INPUT,
        .name = name,
        .debugName = nameStr,
        .sampler = {},
        .format = format,
        .width = width,
        .height = height,
    };
}

void RComponent::add_io_image(const char* nameStr, RFormat format, uint32_t width, uint32_t height)
{
    Name name(nameStr);

    if (mObj->images.contains(name))
    {
        printf("image already declared in component\n");
        return;
    }

    mObj->images[name] = {
        .type = NODE_TYPE_IO,
        .name = name,
        .debugName = nameStr,
        .sampler = {},
        .format = format,
        .width = width,
        .height = height,
    };
}

RGraphicsPass RComponent::add_graphics_pass(const RGraphicsPassInfo& gpI, void* userData, RGraphicsPassCallback callback)
{
    LD_PROFILE_SCOPE;

    // TODO: linear allocator and placement new?
    RGraphicsPassObj* obj = heap_new<RGraphicsPassObj>(MEMORY_USAGE_RENDER);
    obj->name = Name(gpI.name);
    obj->debugName = gpI.name;
    obj->width = gpI.width;
    obj->height = gpI.height;
    obj->userData = userData;
    obj->callback = callback;
    obj->component = *this;
    obj->isCallbackScope = false;
    obj->hasDepthStencil = false;

    mObj->graphicsPasses[gpI.name] = {obj};
    mObj->graphicsPassOrder.push_back({obj});

    return {obj};
}

RGraph RGraph::create(const RGraphInfo& graphI)
{
    LD_PROFILE_SCOPE;

    RGraphObj* obj = heap_new<RGraphObj>(MEMORY_USAGE_RENDER);
    obj->info = graphI;
    obj->blitCompObj = nullptr;

    return {obj};
}

void RGraph::destroy(RGraph graph)
{
    LD_PROFILE_SCOPE;

    while (!sDestroyCallbacks.empty())
    {
        LD_PROFILE_SCOPE_NAME("destroy callbacks");

        auto& pair = sDestroyCallbacks.top();
        pair.second(pair.first);
        sDestroyCallbacks.pop();
    }

    RGraphObj* graphObj = (RGraphObj*)graph;

    // TODO: linear allocator + placement free instead of
    //       individual heap_create/heap_delete.

    for (auto& compIte : graphObj->components)
    {
        LD_PROFILE_SCOPE_NAME("delete component");
        RComponentObj* compObj = compIte.second;
        for (auto& passIte : compObj->graphicsPasses)
        {
            LD_PROFILE_SCOPE_NAME("delete grahics pass");
            RGraphicsPassObj* passObj = passIte.second;
            heap_delete(passObj);
        }
        heap_delete(compObj);
    }
    heap_delete(graphObj);
}

void RGraph::release(RDevice device)
{
    device.wait_idle();

    while (!sReleaseCallbacks.empty())
    {
        auto& pair = sReleaseCallbacks.top();
        pair.second(pair.first);
        sReleaseCallbacks.pop();
    }

    for (auto& ite : sStorages)
    {
        RStorage& storage = ite.second;

        for (auto& imageIte : storage.images)
        {
            if (imageIte.second.handle)
                device.destroy_image(imageIte.second.handle);
        }
    }
}

RDevice RGraph::get_device()
{
    return mObj->info.device;
}

RImage RGraph::get_swapchain_image()
{
    return mObj->info.swapchainImage;
}

RComponent RGraph::add_component(const char* nameStr)
{
    LD_PROFILE_SCOPE;

    // TODO: linear allocator + placement new?
    RComponentObj* comp = heap_new<RComponentObj>(MEMORY_USAGE_RENDER);
    comp->name = Name(nameStr);
    comp->debugName = nameStr;

    mObj->components[comp->name] = {comp};

    return {comp};
}

void RGraph::connect_image(const char* srcCompStr, const char* srcOutImageStr, const char* dstCompStr, const char* dstInImageStr)
{
    LD_PROFILE_SCOPE;

    Name srcComp(srcCompStr);
    Name dstComp(dstCompStr);
    Name srcOutImage(srcOutImageStr);
    Name dstInImage(dstInImageStr);

    if (!mObj->components.contains(srcComp))
    {
        printf("source component does no exist");
        return;
    }

    if (!mObj->components.contains(dstComp))
    {
        printf("destination component does no exist");
        return;
    }

    // Alias for an output image from one component to be the input image of another component.
    // Let set A be the set containing all GraphicsPass in srcComp that accesses srcOutImage.
    // Let set B be the set containing all GraphicsPass in dstComp that accesses dstInImage.
    // Add dependency edge for each tuple in the cartesian product A x B that has a dependency.
    RComponentObj* srcCompObj = mObj->components[srcComp];
    RComponentObj* dstCompObj = mObj->components[dstComp];
    RImageUsageFlags dstUsages = 0;
    for (auto& srcPassIte : srcCompObj->graphicsPasses)
    {
        RGraphicsPassObj* srcPassObj = srcPassIte.second;

        for (auto& dstPassIte : dstCompObj->graphicsPasses)
        {
            RGraphicsPassObj* dstPassObj = dstPassIte.second;

            if (has_image_dependency(srcPassObj->imageUsages[srcOutImage], dstPassObj->imageUsages[dstInImage]))
                srcPassObj->edges.insert(RGraphicsPass{dstPassObj});

            dstUsages |= get_native_image_usage(dstPassObj->imageUsages[dstInImage]);
        }
    }

    // image usage inheritance:
    //   since the dstInImage is a reference to the srcOutImage,
    //   srcComp image usage inherits all dstComp image usages
    GraphImage& srcGraphImage = dereference_image(&srcCompObj, &srcOutImage);
    GraphImage& dstGraphImage = dstCompObj->images[dstInImage];

    srcGraphImage.usage |= dstUsages;

    dstGraphImage.format = srcGraphImage.format;
    dstGraphImage.sampler = srcGraphImage.sampler;
    dstGraphImage.width = srcGraphImage.width;
    dstGraphImage.height = srcGraphImage.height;

    // establish reference link, find the component-name pair of the physical resource
    GraphImageRef& dstGraphImageRef = dstCompObj->imageRefs[dstInImage];
    dstGraphImageRef.srcComponent = srcCompObj;
    dstGraphImageRef.srcOutputName = srcOutImage;
}

void RGraph::connect_swapchain_image(const char* srcCompStr, const char* srcOutImageStr)
{
    Name srcComp(srcCompStr);
    Name srcOutImage(srcOutImageStr);

    RComponentObj* srcCompObj = mObj->components[srcComp];

    GraphImage& srcGraphImage = dereference_image(&srcCompObj, &srcOutImage);
    srcGraphImage.usage |= RIMAGE_USAGE_TRANSFER_SRC_BIT;

    mObj->blitCompObj = srcCompObj;
    mObj->blitOutputName = srcOutImage;
}

void RGraph::submit(bool save)
{
    LD_PROFILE_SCOPE;

    // building and validation
    // topological sort of all graphics passes, linearize passes
    topological_sort(mObj->components, mObj->passOrder);

    if (save)
    {
        save_graph_to_dot(mObj, "saved.dot");
    }

    // recording
    RCommandList list = mObj->info.list;
    list.begin();
    for (uint32_t passIdx = 0; passIdx < (uint32_t)mObj->passOrder.size(); passIdx++)
    {
        LD_PROFILE_SCOPE_NAME("record render pass");

        RGraphicsPassObj* passObj = mObj->passOrder[passIdx];
        RComponentObj* compObj = passObj->component;

        uint32_t colorAttachmentCount = (uint32_t)passObj->colorAttachments.size();
        std::vector<RImage> colorHandles(colorAttachmentCount);
        RImage depthStencilHandle = {};

        // build render pass info
        RPassInfo passI = {};
        passI.samples = RSAMPLE_COUNT_1_BIT;
        passI.colorAttachmentCount = colorAttachmentCount;
        passI.colorAttachments = passObj->colorAttachmentInfos.data();
        passI.depthStencilAttachment = nullptr;

        // retrieve color attachment handles
        for (uint32_t colorIdx = 0; colorIdx < colorAttachmentCount; colorIdx++)
        {
            LD_PROFILE_SCOPE_NAME("render pass color attachments");

            RGraphicsPassColorAttachment* attachment = passObj->colorAttachments.data() + colorIdx;
            RPassColorAttachment* attachmentInfo = passObj->colorAttachmentInfos.data() + colorIdx;

            Name srcOutputName = attachment->name;
            RComponentObj* srcCompObj = compObj;
            dereference_image(&srcCompObj, &srcOutputName);
            RImage imageHandle = get_or_create_image(mObj, srcCompObj, srcOutputName);

            RStorage& compStorage = sStorages[srcCompObj->name];
            LD_ASSERT(compStorage.images.contains(srcOutputName));

            ImageState& imageState = compStorage.images[srcOutputName];
            attachmentInfo->initialLayout = imageState.lastLayout;

            // pass layout should already be decided upon declaration
            LD_ASSERT(attachmentInfo->passLayout != RIMAGE_LAYOUT_UNDEFINED);

            imageState.lastLayout = attachmentInfo->passLayout;
            imageState.handle = imageHandle;

            colorHandles[colorIdx] = imageHandle;
        }

        // clear colors
        std::vector<RClearColorValue> clearColors(passObj->colorAttachments.size());
        for (size_t i = 0; i < clearColors.size(); i++)
            clearColors[i] = passObj->colorAttachments[i].clearValue.value_or(RClearColorValue{});

        // retrieve depth stencil attachment handle
        if (passObj->hasDepthStencil)
        {
            Name srcOutputName = passObj->depthStencilAttachment.name;
            RComponentObj* srcCompObj = compObj;
            dereference_image(&srcCompObj, &srcOutputName);
            RImage imageHandle = get_or_create_image(mObj, srcCompObj, srcOutputName);

            RStorage& compStorage = sStorages[srcCompObj->name];
            LD_ASSERT(compStorage.images.contains(srcOutputName));

            ImageState& imageState = compStorage.images[srcOutputName];
            passObj->depthStencilAttachmentInfo.initialLayout = imageState.lastLayout;

            // pass layout should already be decided upon declaration
            LD_ASSERT(passObj->depthStencilAttachmentInfo.passLayout != RIMAGE_LAYOUT_UNDEFINED);

            imageState.lastLayout = passObj->depthStencilAttachmentInfo.passLayout;
            imageState.handle = imageHandle;

            passI.depthStencilAttachment = &passObj->depthStencilAttachmentInfo;
            depthStencilHandle = imageHandle;
        }

        // clear depth stencil
        RClearDepthStencilValue clearDepthStencil{};
        if (passObj->hasDepthStencil && passObj->depthStencilAttachment.clearValue.has_value())
            clearDepthStencil = passObj->depthStencilAttachment.clearValue.value();

        // dependency on previous pass
        if (passIdx > 0 && has_pass_dependency(mObj->passOrder[passIdx - 1], mObj->passOrder[passIdx], passObj->passDep))
            passI.dependency = &passObj->passDep;

        // perform image layout transitions for sampled images, right before render pass
        for (Name imageName : passObj->sampledImages)
        {
            LD_PROFILE_SCOPE_NAME("render pass sampled images");

            RGraphImageUsage passUsage = passObj->imageUsages[imageName];
            LD_ASSERT(passUsage == RGRAPH_IMAGE_USAGE_SAMPLED);

            RComponentObj* srcCompObj = nullptr;
            dereference_image(&compObj, &imageName);
            ImageState& state = sStorages[compObj->name].images[imageName];

            RImage image = state.handle;
            RImageMemoryBarrier barrier = RUtil::make_image_memory_barrier(image, state.lastLayout, RIMAGE_LAYOUT_SHADER_READ_ONLY, RACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);
            list.cmd_image_memory_barrier(RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RPIPELINE_STAGE_TOP_OF_PIPE_BIT, barrier);
            state.lastLayout = RIMAGE_LAYOUT_SHADER_READ_ONLY;
        }

        RPassBeginInfo passBI{
            .width = passObj->width,
            .height = passObj->height,
            .depthStencilAttachment = depthStencilHandle,
            .colorAttachmentCount = (uint32_t)colorHandles.size(),
            .colorAttachments = colorHandles.data(),
            .colorResolveAttachments = nullptr,
            .clearColors = clearColors.data(),
            .clearDepthStencil = clearDepthStencil,
            .pass = passI,
        };

        list.cmd_begin_pass(passBI);
        passObj->isCallbackScope = true;
        passObj->callback({passObj}, list, passObj->userData);
        passObj->isCallbackScope = false;
        list.cmd_end_pass();
    }

    if (mObj->blitCompObj)
    {
        LD_PROFILE_SCOPE_NAME("record swapchain blit");

        ImageState& srcBlitState = sStorages[mObj->blitCompObj->name].images[mObj->blitOutputName];
        RImage srcBlit = srcBlitState.handle;
        RImage dstBlit = mObj->info.swapchainImage;

        // transition src image from final layout to transfer src
        RImageMemoryBarrier barrier = RUtil::make_image_memory_barrier(srcBlit, srcBlitState.lastLayout, RIMAGE_LAYOUT_TRANSFER_SRC, RACCESS_COLOR_ATTACHMENT_WRITE_BIT, RACCESS_TRANSFER_READ_BIT);
        list.cmd_image_memory_barrier(RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);
        srcBlitState.lastLayout = RIMAGE_LAYOUT_TRANSFER_SRC;

        // transition swapchain image to transfer dst
        barrier = RUtil::make_image_memory_barrier(dstBlit, RIMAGE_LAYOUT_UNDEFINED, RIMAGE_LAYOUT_TRANSFER_DST, 0, RACCESS_TRANSFER_WRITE_BIT);
        list.cmd_image_memory_barrier(RPIPELINE_STAGE_TOP_OF_PIPE_BIT, RPIPELINE_STAGE_TRANSFER_BIT, barrier);

        // insert blit command
        RImageBlit region{};
        region.srcMaxOffset.x = srcBlit.width();
        region.srcMaxOffset.y = srcBlit.height();
        region.srcMaxOffset.z = 1;
        region.dstMaxOffset.x = dstBlit.width();
        region.dstMaxOffset.y = dstBlit.height();
        region.dstMaxOffset.z = 1;
        list.cmd_blit_image(srcBlit, RIMAGE_LAYOUT_TRANSFER_SRC, dstBlit, RIMAGE_LAYOUT_TRANSFER_DST, 1, &region, RFILTER_NEAREST);

        // transition swapchain image to present src optimal
        barrier = RUtil::make_image_memory_barrier(dstBlit, RIMAGE_LAYOUT_TRANSFER_DST, RIMAGE_LAYOUT_PRESENT_SRC, RACCESS_TRANSFER_WRITE_BIT, 0);
        list.cmd_image_memory_barrier(RPIPELINE_STAGE_TRANSFER_BIT, RPIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, barrier);
    }

    list.end();

    // submission
    // TODO: multi queue submission to saturate GPU, need to expand RenderBackend API first
    RQueue queue = mObj->info.device.get_graphics_queue();
    RPipelineStageFlags waitStages = RPIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    RSubmitInfo submitI{
        .waitCount = 1,
        .waitStages = &waitStages,
        .waits = &mObj->info.imageAcquired,
        .listCount = 1,
        .lists = &list,
        .signalCount = 1,
        .signals = &mObj->info.presentReady,
    };
    queue.submit(submitI, mObj->info.frameComplete);
}

void RGraph::add_release_callback(void* user, OnReleaseCallback onRelease)
{
    sReleaseCallbacks.push({user, onRelease});
}

void RGraph::add_destroy_callback(void* user, OnDestroyCallback onDestroy)
{
    sDestroyCallbacks.push({user, onDestroy});
}

} // namespace LD
