#include <Ludens/RenderBackend/RFactory.h>

namespace LD {

std::unordered_map<uint32_t, RSetLayout> RSetLayoutFactory::sSetLayouts;
std::unordered_map<uint32_t, RPipelineLayout> RPipelineLayoutFactory::sPipelineLayouts;

RSetLayoutFactory::RSetLayoutFactory(RDevice device)
    : mDevice(device)
{
}

RSetLayoutFactory& RSetLayoutFactory::add_binding(const RSetBindingInfo& bindingI)
{
    mBindings.push_back(bindingI);

    return *this;
}

RSetLayout RSetLayoutFactory::build()
{
    RSetLayoutInfo layoutI{
        .bindingCount = (uint32_t)mBindings.size(),
        .bindings = mBindings.data(),
    };

    RSetLayout layout = find_by_hash(hash32_set_layout_info(layoutI));
    if (layout)
        return layout;

    layout = mDevice.create_set_layout(layoutI);
    sSetLayouts[layout.hash()] = layout;

    return layout;
}

RSetLayout RSetLayoutFactory::find_by_hash(uint32_t hash)
{
    if (sSetLayouts.find(hash) != sSetLayouts.end())
        return sSetLayouts[hash];

    return {};
}

void RSetLayoutFactory::destroy_all(RDevice device)
{
    for (auto& ite : sSetLayouts)
        device.destroy_set_layout(ite.second);

    printf("RSetLayoutFactory destroyed %d unique set layouts\n", (int)sSetLayouts.size());

    sSetLayouts.clear();
}

RPipelineLayoutFactory::RPipelineLayoutFactory(RDevice device)
    : mDevice(device)
{
}

RPipelineLayoutFactory& RPipelineLayoutFactory::add_set_layout(const std::initializer_list<RSetBindingInfo>& list)
{
    // create the required set layout if the set layout factory doesnt have one yet.
    RSetLayoutFactory setLF(mDevice);

    for (const RSetBindingInfo& bindingI : list)
        setLF.add_binding(bindingI);

    mSetLayouts.push_back(setLF.build());

    return *this;
}

RPipelineLayout RPipelineLayoutFactory::build()
{
    RPipelineLayoutInfo layoutI{
        .setLayoutCount = (uint32_t)mSetLayouts.size(),
        .setLayouts = mSetLayouts.data(),
    };

    RPipelineLayout layout = find_by_hash(hash32_pipeline_layout_info(layoutI));
    if (layout)
        return layout;

    layout = mDevice.create_pipeline_layout(layoutI);
    sPipelineLayouts[layout.hash()] = layout;

    return layout;
}

RPipelineLayout RPipelineLayoutFactory::find_by_hash(uint32_t hash)
{
    if (sPipelineLayouts.find(hash) != sPipelineLayouts.end())
        return sPipelineLayouts[hash];

    return {};
}

void RPipelineLayoutFactory::destroy_all(RDevice device)
{
    for (auto& ite : sPipelineLayouts)
        device.destroy_pipeline_layout(ite.second);

    printf("RPipelineLayoutFactory destroyed %d unique pipeline layouts\n", (int)sPipelineLayouts.size());

    sPipelineLayouts.clear();
}

} // namespace LD