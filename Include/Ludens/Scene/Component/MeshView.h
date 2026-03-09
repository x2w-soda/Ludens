#pragma once

#include <Ludens/Scene/ComponentView.h>

namespace LD {

struct MeshComponent;

/// @brief Public interface for mesh components.
class MeshView : public ComponentView
{
public:
    MeshView() = delete;
    MeshView(ComponentView comp);
    MeshView(MeshComponent* comp);

    bool load();

    bool set_mesh_asset(AssetID meshID);
    AssetID get_mesh_asset();

private:
    MeshComponent* mMesh = nullptr;
};

} // namespace LD