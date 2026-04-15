#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Assert.h>
#include <Ludens/UI/UIFont.h>

namespace LD {

struct UIFontObj
{
    FontAtlas atlas = {};
    RImage image = {};
};

FontAtlas UIFont::font_atlas()
{
    return mObj->atlas;
}

RImage UIFont::image()
{
    return mObj->image;
}

struct UIFontRegistryObj
{
    Vector<UIFontObj*> fonts;
};

UIFontRegistry UIFontRegistry::create()
{
    auto* obj = heap_new<UIFontRegistryObj>(MEMORY_USAGE_UI);

    return UIFontRegistry(obj);
}

void UIFontRegistry::destroy(UIFontRegistry registry)
{
    auto* obj = registry.unwrap();

    for (UIFontObj* fontObj : obj->fonts)
        heap_delete<UIFontObj>(fontObj);

    heap_delete<UIFontRegistryObj>(obj);
}

UIFont UIFontRegistry::add_font(FontAtlas atlas, RImage image)
{
    LD_ASSERT(atlas);

    auto* fontObj = heap_new<UIFontObj>(MEMORY_USAGE_UI);
    fontObj->atlas = atlas;
    fontObj->image = image;
    mObj->fonts.push_back(fontObj);

    return UIFont(fontObj);
}

} // namespace LD