#include <Ludens/Scene/Component/Camera2DView.h>
#include <Ludens/Serial/Property.h>

#include "Camera2DComponent.h"

#define DEFAULT_VIEWPORT Rect(0.0f, 0.0f, 1.0f, 1.0f)

namespace LD {

// clang-format off
static PropertyMeta sCamera2DPropMeta[] = {
    {"transform",  nullptr, VALUE_TYPE_TRANSFORM_2D, Value64(Transform2D::identity())},
    {"viewport",   nullptr, VALUE_TYPE_RECT, Value64(DEFAULT_VIEWPORT) },
    {"extent",     nullptr, VALUE_TYPE_VEC2, Value64(Vec2(500.0f)) },
    {"zoom",       nullptr, VALUE_TYPE_F32,  Value64(1.0f) },
    {"constraint", nullptr, VALUE_TYPE_BOOL, Value64(true) },
};
// clang-format on

static bool camera_2d_prop_getter(void* data, uint32_t propIndex, uint32_t arrayIndex, Value64& val)
{
    Camera2DView view((Camera2DComponent*)data);
    Transform2D transform2D;

    switch (propIndex)
    {
    case CAMERA_2D_PROP_TRANSFORM:
        (void)view.get_transform_2d(transform2D);
        val.set_transform_2d(transform2D);
        break;
    case CAMERA_2D_PROP_VIEWPORT:
        val.set_rect(view.get_viewport());
        break;
    case CAMERA_2D_PROP_EXTENT:
        val.set_vec2(view.get_extent());
        break;
    case CAMERA_2D_PROP_ZOOM:
        val.set_f32(view.get_zoom());
        break;
    case CAMERA_2D_PROP_CONSTRAINT:
        val.set_bool((bool)view.get_constraint());
        break;
    default:
        return false;
    }

    return true;
}

static bool camera_2d_prop_setter(void* data, uint32_t propIndex, uint32_t arrayIndex, const Value64& val)
{
    Camera2DView view((Camera2DComponent*)data);

    switch (propIndex)
    {
    case CAMERA_2D_PROP_TRANSFORM:
        view.set_transform_2d(val.get_transform_2d());
        break;
    case CAMERA_2D_PROP_VIEWPORT:
        view.set_viewport(val.get_rect());
        break;
    case CAMERA_2D_PROP_EXTENT:
        view.set_extent(val.get_vec2());
        break;
    case CAMERA_2D_PROP_ZOOM:
        view.set_zoom(val.get_f32());
        break;
    case CAMERA_2D_PROP_CONSTRAINT:
        view.set_constraint((Camera2DConstraint)val.get_bool());
        break;
    default:
        return false;
    }

    return true;
}

TypeMeta Camera2DMeta::sTypeMeta = {
    .name = "Camera2DComponent",
    .props = sCamera2DPropMeta,
    .propCount = sizeof(sCamera2DPropMeta) / sizeof(*sCamera2DPropMeta),
    .getLocal = &camera_2d_prop_getter,
    .setLocal = &camera_2d_prop_setter,
};

static Rect clamp_rect(Rect rect)
{
    return Rect(std::clamp(rect.x, 0.0f, 1.0f), std::clamp(rect.y, 0.0f, 1.0f), std::clamp(rect.w, 0.0f, 1.0f), std::clamp(rect.h, 0.0f, 1.0f));
}

void Camera2DMeta::init(ComponentBase** dstData)
{
    Vec2 extent = sScene->tick.extent;

    ComponentBase* base = *dstData;
    Camera2DComponent* dstCamera2D = (Camera2DComponent*)dstData;
    dstCamera2D->transform = base->transform2D;
    dstCamera2D->camera = Camera2D::create(extent);
    dstCamera2D->viewport = Rect(0.0f, 0.0f, 1.0f, 1.0f);
    LD_ASSERT(dstCamera2D->transform);
}

bool Camera2DMeta::clone(SceneObj* scene, ComponentBase** dstData, ComponentBase** srcData, String& err)
{
    Camera2DView srcCamera(srcData);
    LD_ASSERT(srcCamera);

    Rect viewport = srcCamera.get_viewport();
    Camera2DInfo info = srcCamera.get_info();
    Camera2DComponent* dstCamera2D = (Camera2DComponent*)dstData;
    if (!load(scene, dstCamera2D, info, viewport, err))
        return false;

    dstCamera2D->constraint = srcCamera.get_constraint();
    return true;
}

bool Camera2DMeta::load(SceneObj* scene, Camera2DComponent* camera, const Camera2DInfo& info, const Rect& viewport, String& err)
{
    if (camera->camera)
        Camera2D::destroy(camera->camera);

    camera->camera = Camera2D::create(info);
    camera->viewport = viewport;

    return (bool)camera->camera;
}

bool Camera2DMeta::load_from_props(SceneObj* scene, ComponentBase** data, const Vector<PropertyValue>& props, String& err)
{
    Transform2D transform = Transform2D::identity();
    Camera2DInfo info{};
    Rect viewport = DEFAULT_VIEWPORT;
    bool constraint = false;

    for (const PropertyValue& prop : props)
    {
        switch (prop.propIndex)
        {
        case CAMERA_2D_PROP_TRANSFORM:
            transform = prop.value.get_transform_2d();
            break;
        case CAMERA_2D_PROP_VIEWPORT:
            viewport = prop.value.get_rect();
            break;
        case CAMERA_2D_PROP_EXTENT:
            info.extent = prop.value.get_vec2();
            break;
        case CAMERA_2D_PROP_ZOOM:
            info.zoom = prop.value.get_f32();
            break;
        case CAMERA_2D_PROP_CONSTRAINT:
            constraint = prop.value.get_bool();
            break;
        default:
            break;
        }
    }

    auto* cameraC = (Camera2DComponent*)data;
    if (!load(sScene, cameraC, info, viewport, err))
        return false;

    Camera2DView cameraV(cameraC);
    cameraV.set_constraint((Camera2DConstraint)constraint);

    return true;
}

bool Camera2DMeta::unload(SceneObj* scene, ComponentBase** cameraData, String& err)
{
    Camera2DComponent* camera = (Camera2DComponent*)cameraData;

    if (camera->camera)
    {
        Camera2D::destroy(camera->camera);
        camera->camera = {};
    }

    return true;
}

bool Camera2DMeta::startup(SceneObj* scene, ComponentBase** data, String& err)
{
    Camera2DComponent* camera = (Camera2DComponent*)data;

    if (!camera->camera)
    {
        camera->camera = Camera2D::create(scene->tick.extent);
    }

    return (bool)camera->camera;
}

bool Camera2DMeta::cleanup(SceneObj* scene, ComponentBase** data, String& err)
{
    // Camera2D not destroyed until unload.
    return true;
}

Camera2DView::Camera2DView(ComponentView comp)
{
    if (comp && comp.type() == COMPONENT_TYPE_CAMERA_2D)
    {
        mData = comp.data();
        mCamera = (Camera2DComponent*)mData;
    }
}

Camera2DView::Camera2DView(Camera2DComponent* comp)
{
    if (comp && comp->base && comp->base->cuid)
    {
        mData = (ComponentBase**)comp;
        mCamera = comp;
    }
}

bool Camera2DView::load(const Camera2DInfo& info, Rect viewport, String& err)
{
    return Camera2DMeta::load(sScene, mCamera, info, viewport, err);
}

Camera2DInfo Camera2DView::get_info()
{
    Camera2D cam = mCamera->camera;

    Camera2DInfo info{};
    info.position = cam.get_position();
    info.extent = cam.get_extent();
    info.zoom = cam.get_zoom();
    info.rotation = cam.get_rotation();

    return info;
}

Rect Camera2DView::get_viewport()
{
    return mCamera->viewport;
}

void Camera2DView::set_viewport(Rect viewport)
{
    mCamera->viewport = clamp_rect(viewport);
}

Vec2 Camera2DView::get_extent()
{
    return mCamera->camera.get_extent();
}

void Camera2DView::set_extent(Vec2 extent)
{
    mCamera->camera.set_extent(extent);
}

float Camera2DView::get_zoom()
{
    return mCamera->camera.get_zoom();
}

void Camera2DView::set_zoom(float zoom)
{
    if (zoom != 0.0f)
        mCamera->camera.set_zoom(zoom);
}

Camera2DConstraint Camera2DView::get_constraint()
{
    return mCamera->constraint;
}

void Camera2DView::set_constraint(Camera2DConstraint constraint)
{
    mCamera->constraint = constraint;
}

} // namespace LD