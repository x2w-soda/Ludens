#pragma once

#include <Ludens/Header/Math/Rect.h>
#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct Sprite2DComponent;

enum Sprite2DProp
{
    SPRITE_2D_PROP_TRANSFORM = COMPONENT_PROP_TRANSFORM,
    SPRITE_2D_PROP_TEXTURE_2D_ASSET,
    SPRITE_2D_PROP_Z_DEPTH,
    SPRITE_2D_PROP_PIVOT,
    SPRITE_2D_PROP_REGION,
    SPRITE_2D_PROP_SCREEN_LAYER,
};

/// @brief Public interface for Sprite2D components.
class Sprite2DView : public ComponentView
{
public:
    Sprite2DView() = delete;
    Sprite2DView(ComponentView comp);
    Sprite2DView(Sprite2DComponent* comp);

    bool load(SUID screenLayerSUID, AssetID textureID);

    void set_texture_2d_asset(AssetID textureID);
    AssetID get_texture_2d_asset();
    uint32_t get_z_depth();
    void set_z_depth(uint32_t zDepth);
    Vec2 get_pivot();
    void set_pivot(Vec2 pivot);
    Rect get_region();
    void set_region(Rect region);
    RUID get_screen_layer_ruid();
    SUID get_screen_layer_suid();

    /// @brief Rect(-pivot.x, -pivot.y, region.w, region.h)
    Rect local_rect();

private:
    Sprite2DComponent* mSprite = nullptr;
};

} // namespace LD