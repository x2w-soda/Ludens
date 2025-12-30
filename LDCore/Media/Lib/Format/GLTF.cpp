#include <Ludens/DSA/View.h>
#include <Ludens/Media/Format/GLTF.h>
#include <Ludens/Media/Format/JSON.h>

#include <format>

namespace LD {

/// @brief GLTF event parsing implementation. Should be externally synchronized,
///        but for convenience each 'GLTFEventParser::parse' stack frame will allocate
///        its own object, so the event parsing API should pretty much be thread safe.
class GLTFEventParserObj
{
public:
    GLTFEventParserObj(const GLTFEventCallback& callbacks, void* user)
        : mCallbacks(callbacks), mUser(user), mState(STATE_ZERO), mStringSlot(nullptr)
    {
    }

    bool parse(const void* fileData, size_t fileSize, std::string& error);
    static bool on_json_enter_object(void*);
    static bool on_json_leave_object(size_t memberCount, void*);
    static bool on_json_enter_array(void*);
    static bool on_json_leave_array(size_t elementCount, void*);
    static bool on_json_key(const View& key, void*);
    static bool on_json_string(const View& string, void*);
    static bool on_json_null(void* user);
    static bool on_json_bool(bool b, void* user);
    static bool on_json_i64(int64_t i64, void* user);
    static bool on_json_u64(uint64_t u64, void* user);
    static bool on_json_f64(double f64, void* user);

private:
    bool escape_json_value();
    bool on_json_f64_value(double f64);
    bool on_json_u64_value(uint64_t u64);
    bool on_json_u32_value(uint32_t u32);
    bool on_json_root_key(const View& key);
    bool on_json_asset_key(const View& key);
    bool on_json_scene_key(const View& key);
    bool on_json_node_key(const View& key);
    bool on_json_mesh_key(const View& key);
    bool on_json_mesh_primitive_key(const View& key);
    bool on_json_mesh_primitive_attributes_key(const View& key);
    bool on_json_material_key(const View& key);
    bool on_json_material_emissive_texture_key(const View& key);
    bool on_json_material_normal_texture_key(const View& key);
    bool on_json_material_occlusion_texture_key(const View& key);
    bool on_json_material_pbr_key(const View& key);
    bool on_json_material_pbr_base_color_texture_key(const View& key);
    bool on_json_material_pbr_metallic_roughness_texture_key(const View& key);
    bool on_json_texture_key(const View& key);
    bool on_json_sampler_key(const View& key);
    bool on_json_image_key(const View& key);
    bool on_json_buffer_key(const View& key);
    bool on_json_buffer_view_key(const View& key);
    bool on_json_accessor_key(const View& key);

private:
    enum State
    {
        STATE_ZERO = 0,
        STATE_ROOT, // top level GLTF object
        STATE_ROOT_ASSET_KEY,
        STATE_ROOT_SCENE_KEY,
        STATE_ROOT_SCENES_KEY,
        STATE_ROOT_NODES_KEY,
        STATE_ROOT_MESHES_KEY,
        STATE_ROOT_MATERIALS_KEY,
        STATE_ROOT_TEXTURES_KEY,
        STATE_ROOT_SAMPLERS_KEY,
        STATE_ROOT_IMAGES_KEY,
        STATE_ROOT_BUFFERS_KEY,
        STATE_ROOT_BUFFER_VIEWS_KEY,
        STATE_ROOT_ACCESSORS_KEY,
        STATE_ASSET,
        STATE_SCENES_ARRAY,
        STATE_SCENE,
        STATE_SCENE_NODES,
        STATE_NODES_ARRAY,
        STATE_NODE,
        STATE_NODE_CHILDREN,
        STATE_NODE_MESH,
        STATE_NODE_MATRIX,
        STATE_NODE_TRANSLATION,
        STATE_NODE_ROTATION,
        STATE_NODE_SCALE,
        STATE_MESHES_ARRAY,
        STATE_MESH,
        STATE_MESH_PRIMITIVES_KEY,
        STATE_MESH_PRIMITIVES_ARRAY,
        STATE_MESH_PRIMITIVE,
        STATE_MESH_PRIMITIVE_INDICES,
        STATE_MESH_PRIMITIVE_MATERIAL,
        STATE_MESH_PRIMITIVE_MODE,
        STATE_MESH_PRIMITIVE_ATTRIBUTES_KEY,
        STATE_MESH_PRIMITIVE_ATTRIBUTES,
        STATE_MESH_PRIMITIVE_ATTRIBUTES_INDEX,
        STATE_MATERIALS_ARRAY,
        STATE_MATERIAL,
        STATE_MATERIAL_DOUBLE_SIDED,
        STATE_MATERIAL_ALPHA_CUTOFF,
        STATE_MATERIAL_EMISSIVE_FACTOR,
        STATE_MATERIAL_EMISSIVE_TEXTURE,
        STATE_MATERIAL_EMISSIVE_TEXTURE_INDEX,
        STATE_MATERIAL_EMISSIVE_TEXTURE_TEXCOORD,
        STATE_MATERIAL_NORMAL_TEXTURE,
        STATE_MATERIAL_NORMAL_TEXTURE_INDEX,
        STATE_MATERIAL_NORMAL_TEXTURE_TEXCOORD,
        STATE_MATERIAL_NORMAL_TEXTURE_SCALE,
        STATE_MATERIAL_OCCLUSION_TEXTURE,
        STATE_MATERIAL_OCCLUSION_TEXTURE_INDEX,
        STATE_MATERIAL_OCCLUSION_TEXTURE_TEXCOORD,
        STATE_MATERIAL_OCCLUSION_TEXTURE_STRENGTH,
        STATE_MATERIAL_PBR,
        STATE_MATERIAL_PBR_BASE_COLOR_FACTOR,
        STATE_MATERIAL_PBR_METALLIC_FACTOR,
        STATE_MATERIAL_PBR_ROUGHNESS_FACTOR,
        STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE,
        STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_INDEX,
        STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_TEXCOORD,
        STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE,
        STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_INDEX,
        STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_TEXCOORD,
        STATE_TEXTURES_ARRAY,
        STATE_TEXTURE,
        STATE_TEXTURE_SAMPLER,
        STATE_TEXTURE_SOURCE,
        STATE_SAMPLERS_ARRAY,
        STATE_SAMPLER,
        STATE_SAMPLER_MIN_FILTER,
        STATE_SAMPLER_MAG_FILTER,
        STATE_SAMPLER_WRAP_S,
        STATE_SAMPLER_WRAP_T,
        STATE_IMAGES_ARRAY,
        STATE_IMAGE,
        STATE_IMAGE_BUFFER_VIEW,
        STATE_BUFFERS_ARRAY,
        STATE_BUFFER,
        STATE_BUFFER_BYTE_LENGTH,
        STATE_BUFFER_VIEWS_ARRAY,
        STATE_BUFFER_VIEW,
        STATE_BUFFER_VIEW_BUFFER,
        STATE_BUFFER_VIEW_BYTE_LENGTH,
        STATE_BUFFER_VIEW_BYTE_OFFSET,
        STATE_BUFFER_VIEW_BYTE_STRIDE,
        STATE_BUFFER_VIEW_TARGET,
        STATE_ACCESSORS_ARRAY,
        STATE_ACCESSOR,
        STATE_ACCESSOR_BUFFER_VIEW,
        STATE_ACCESSOR_BYTE_OFFSET,
        STATE_ACCESSOR_COMPONENT_TYPE,
        STATE_ACCESSOR_COUNT,
        STATE_ACCESSOR_NORMALIZED,
        STATE_ACCESSOR_MIN,
        STATE_ACCESSOR_MAX,
    };

    State mState;
    void* mUser;
    uint32_t mEscapeDepth = 0;
    Buffer mPrimitiveAttributeKey;
    Buffer* mStringSlot;
    GLTFEventCallback mCallbacks;
    GLTFAssetProp mAssetProp{};
    GLTFSceneProp mSceneProp{};
    GLTFNodeProp mNodeProp{};
    GLTFMeshProp mMeshProp{};
    GLTFMeshPrimitiveProp mMeshPrimitiveProp{};
    GLTFMaterialProp mMaterialProp{};
    GLTFTextureProp mTextureProp{};
    GLTFSamplerProp mSamplerProp{};
    GLTFImageProp mImageProp{};
    GLTFBufferProp mBufferProp{};
    GLTFBufferViewProp mBufferViewProp{};
    GLTFAccessorProp mAccessorProp{};
    uint32_t mSceneIndexProp = 0;
    uint32_t mArrayCtr = 0;
};

bool GLTFEventParserObj::on_json_enter_object(void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
    {
        self.mEscapeDepth++;
        return true; // skip object
    }

    switch (self.mState)
    {
    case STATE_ZERO:
        self.mState = STATE_ROOT;
        return true;
    case STATE_ROOT_ASSET_KEY:
        self.mState = STATE_ASSET;
        self.mAssetProp = {};
        return true;
    case STATE_SCENES_ARRAY:
        self.mState = STATE_SCENE;
        self.mSceneProp = {};
        return true;
    case STATE_NODES_ARRAY:
        self.mState = STATE_NODE;
        self.mNodeProp = {};
        return true;
    case STATE_MESHES_ARRAY:
        self.mState = STATE_MESH;
        self.mMeshProp = {};
        return true;
    case STATE_MESH_PRIMITIVES_ARRAY:
        self.mState = STATE_MESH_PRIMITIVE;
        self.mMeshPrimitiveProp = {};
        return true;
    case STATE_MESH_PRIMITIVE_ATTRIBUTES_KEY:
        self.mState = STATE_MESH_PRIMITIVE_ATTRIBUTES;
        self.mMeshPrimitiveProp.attributes.clear();
        return true;
    case STATE_MATERIALS_ARRAY:
        self.mState = STATE_MATERIAL;
        self.mMaterialProp = {};
        return true;
    case STATE_TEXTURES_ARRAY:
        self.mState = STATE_TEXTURE;
        self.mTextureProp = {};
        return true;
    case STATE_SAMPLERS_ARRAY:
        self.mState = STATE_SAMPLER;
        self.mSamplerProp = {};
        return true;
    case STATE_IMAGES_ARRAY:
        self.mState = STATE_IMAGE;
        self.mImageProp = {};
        return true;
    case STATE_BUFFERS_ARRAY:
        self.mState = STATE_BUFFER;
        self.mBufferProp = {};
        return true;
    case STATE_BUFFER_VIEWS_ARRAY:
        self.mState = STATE_BUFFER_VIEW;
        self.mBufferViewProp = {};
        return true;
    case STATE_ACCESSORS_ARRAY:
        self.mState = STATE_ACCESSOR;
        self.mAccessorProp = {};
        return true;
    case STATE_MATERIAL_EMISSIVE_TEXTURE:
        self.mMaterialProp.emissiveTexture = GLTFTextureInfo();
        return true;
    case STATE_MATERIAL_NORMAL_TEXTURE:
        self.mMaterialProp.normalTexture = GLTFNormalTextureInfo();
        return true;
    case STATE_MATERIAL_OCCLUSION_TEXTURE:
        self.mMaterialProp.occlusionTexture = GLTFOcclusionTextureInfo();
        return true;
    case STATE_MATERIAL_PBR:
        self.mMaterialProp.pbr = GLTFPbrMetallicRoughness();
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE:
        LD_ASSERT(self.mMaterialProp.pbr.has_value());
        self.mMaterialProp.pbr->baseColorTexture = GLTFTextureInfo();
        return true;
    case STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE:
        LD_ASSERT(self.mMaterialProp.pbr.has_value());
        self.mMaterialProp.pbr->metallicRoughnessTexture = GLTFTextureInfo();
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_leave_object(size_t memberCount, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
    {
        if (--self.mEscapeDepth == 1)
            self.mEscapeDepth = 0;
        return true;
    }

    switch (self.mState)
    {
    case STATE_ASSET:
        self.mState = STATE_ROOT;
        if (self.mCallbacks.onAsset)
            self.mCallbacks.onAsset(self.mAssetProp, self.mUser);
        return true;
    case STATE_SCENE:
        self.mState = STATE_SCENES_ARRAY;
        if (self.mCallbacks.onScene)
            self.mCallbacks.onScene(self.mSceneProp, self.mUser);
        return true;
    case STATE_NODE:
        self.mState = STATE_NODES_ARRAY;
        if (self.mCallbacks.onNode)
            self.mCallbacks.onNode(self.mNodeProp, self.mUser);
        return true;
    case STATE_MESH:
        self.mState = STATE_MESHES_ARRAY;
        if (self.mCallbacks.onMesh)
            self.mCallbacks.onMesh(self.mMeshProp, self.mUser);
        return true;
    case STATE_MESH_PRIMITIVE:
        self.mState = STATE_MESH_PRIMITIVES_ARRAY;
        if (self.mCallbacks.onMeshPrimitive)
            self.mCallbacks.onMeshPrimitive(self.mMeshPrimitiveProp, self.mUser);
        return true;
    case STATE_MESH_PRIMITIVE_ATTRIBUTES:
        self.mState = STATE_MESH_PRIMITIVE;
        return true;
    case STATE_MATERIAL:
        self.mState = STATE_MATERIALS_ARRAY;
        if (self.mCallbacks.onMaterial)
            self.mCallbacks.onMaterial(self.mMaterialProp, self.mUser);
        return true;
    case STATE_TEXTURE:
        self.mState = STATE_TEXTURES_ARRAY;
        if (self.mCallbacks.onTexture)
            self.mCallbacks.onTexture(self.mTextureProp, self.mUser);
        return true;
    case STATE_SAMPLER:
        self.mState = STATE_SAMPLERS_ARRAY;
        if (self.mCallbacks.onSampler)
            self.mCallbacks.onSampler(self.mSamplerProp, self.mUser);
        return true;
    case STATE_IMAGE:
        self.mState = STATE_IMAGES_ARRAY;
        if (self.mCallbacks.onImage)
            self.mCallbacks.onImage(self.mImageProp, self.mUser);
        return true;
    case STATE_BUFFER:
        self.mState = STATE_BUFFERS_ARRAY;
        if (self.mCallbacks.onBuffer)
            self.mCallbacks.onBuffer(self.mBufferProp, self.mUser);
        return true;
    case STATE_BUFFER_VIEW:
        self.mState = STATE_BUFFER_VIEWS_ARRAY;
        if (self.mCallbacks.onBufferView)
            self.mCallbacks.onBufferView(self.mBufferViewProp, self.mUser);
        return true;
    case STATE_ACCESSOR:
        self.mState = STATE_ACCESSORS_ARRAY;
        if (self.mCallbacks.onAccessor)
            self.mCallbacks.onAccessor(self.mAccessorProp, self.mUser);
        return true;
    case STATE_MATERIAL_EMISSIVE_TEXTURE:
    case STATE_MATERIAL_NORMAL_TEXTURE:
    case STATE_MATERIAL_OCCLUSION_TEXTURE:
    case STATE_MATERIAL_PBR:
        self.mState = STATE_MATERIAL;
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE:
    case STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE:
        self.mState = STATE_MATERIAL_PBR;
        return true;
    case STATE_ROOT: // parsing complete
        self.mState = STATE_ZERO;
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_enter_array(void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
    {
        self.mEscapeDepth++;
        return true; // skip array
    }

    switch (self.mState)
    {
    case STATE_ROOT_SCENES_KEY:
        self.mState = STATE_SCENES_ARRAY;
        return true;
    case STATE_ROOT_NODES_KEY:
        self.mState = STATE_NODES_ARRAY;
        return true;
    case STATE_ROOT_MESHES_KEY:
        self.mState = STATE_MESHES_ARRAY;
        return true;
    case STATE_ROOT_MATERIALS_KEY:
        self.mState = STATE_MATERIALS_ARRAY;
        return true;
    case STATE_ROOT_TEXTURES_KEY:
        self.mState = STATE_TEXTURES_ARRAY;
        return true;
    case STATE_ROOT_SAMPLERS_KEY:
        self.mState = STATE_SAMPLERS_ARRAY;
        return true;
    case STATE_ROOT_IMAGES_KEY:
        self.mState = STATE_IMAGES_ARRAY;
        return true;
    case STATE_ROOT_BUFFERS_KEY:
        self.mState = STATE_BUFFERS_ARRAY;
        return true;
    case STATE_ROOT_BUFFER_VIEWS_KEY:
        self.mState = STATE_BUFFER_VIEWS_ARRAY;
        return true;
    case STATE_ROOT_ACCESSORS_KEY:
        self.mState = STATE_ACCESSORS_ARRAY;
        return true;
    case STATE_MESH_PRIMITIVES_KEY:
        self.mState = STATE_MESH_PRIMITIVES_ARRAY;
        return true;
    case STATE_SCENE_NODES:
    case STATE_NODE_CHILDREN:
    case STATE_NODE_MATRIX:
    case STATE_NODE_TRANSLATION:
    case STATE_NODE_ROTATION:
    case STATE_NODE_SCALE:
    case STATE_MATERIAL_EMISSIVE_FACTOR:
    case STATE_MATERIAL_PBR_BASE_COLOR_FACTOR:
    case STATE_ACCESSOR_MIN:
    case STATE_ACCESSOR_MAX:
        self.mArrayCtr = 0;
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_leave_array(size_t elementCount, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
    {
        if (--self.mEscapeDepth == 1)
            self.mEscapeDepth = 0;
        return true;
    }

    switch (self.mState)
    {
    case STATE_SCENES_ARRAY:
    case STATE_NODES_ARRAY:
    case STATE_MESHES_ARRAY:
    case STATE_MATERIALS_ARRAY:
    case STATE_IMAGES_ARRAY:
    case STATE_TEXTURES_ARRAY:
    case STATE_SAMPLERS_ARRAY:
    case STATE_BUFFERS_ARRAY:
    case STATE_BUFFER_VIEWS_ARRAY:
    case STATE_ACCESSORS_ARRAY:
        self.mState = STATE_ROOT;
        return true;
    case STATE_MESH_PRIMITIVES_ARRAY:
        self.mState = STATE_MESH;
        return true;
    case STATE_ACCESSOR_MAX:
    case STATE_ACCESSOR_MIN:
        self.mState = STATE_ACCESSOR;
        return true;
    case STATE_SCENE_NODES:
        self.mState = STATE_SCENE;
        return true;
    case STATE_NODE_CHILDREN:
    case STATE_NODE_MESH:
    case STATE_NODE_MATRIX:
    case STATE_NODE_TRANSLATION:
    case STATE_NODE_ROTATION:
    case STATE_NODE_SCALE:
        self.mState = STATE_NODE;
        return true;
    case STATE_MATERIAL_EMISSIVE_FACTOR:
        self.mState = STATE_MATERIAL;
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_FACTOR:
        self.mState = STATE_MATERIAL_PBR;
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_key(const View& key, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
    {
        if (self.mEscapeDepth != 1)
            return true;
        self.mEscapeDepth = 0;
    }

    switch (self.mState)
    {
    case STATE_ROOT:
        return self.on_json_root_key(key);
    case STATE_ASSET:
        return self.on_json_asset_key(key);
    case STATE_SCENE:
        return self.on_json_scene_key(key);
    case STATE_NODE:
        return self.on_json_node_key(key);
    case STATE_MESH:
        return self.on_json_mesh_key(key);
    case STATE_MESH_PRIMITIVE:
        return self.on_json_mesh_primitive_key(key);
    case STATE_MESH_PRIMITIVE_ATTRIBUTES:
        return self.on_json_mesh_primitive_attributes_key(key);
    case STATE_MATERIAL:
        return self.on_json_material_key(key);
    case STATE_MATERIAL_EMISSIVE_TEXTURE:
        return self.on_json_material_emissive_texture_key(key);
    case STATE_MATERIAL_NORMAL_TEXTURE:
        return self.on_json_material_normal_texture_key(key);
    case STATE_MATERIAL_OCCLUSION_TEXTURE:
        return self.on_json_material_occlusion_texture_key(key);
    case STATE_MATERIAL_PBR:
        return self.on_json_material_pbr_key(key);
    case STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE:
        return self.on_json_material_pbr_base_color_texture_key(key);
    case STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE:
        return self.on_json_material_pbr_metallic_roughness_texture_key(key);
    case STATE_TEXTURE:
        return self.on_json_texture_key(key);
    case STATE_SAMPLER:
        return self.on_json_sampler_key(key);
    case STATE_IMAGE:
        return self.on_json_image_key(key);
    case STATE_BUFFER:
        return self.on_json_buffer_key(key);
    case STATE_BUFFER_VIEW:
        return self.on_json_buffer_view_key(key);
    case STATE_ACCESSOR:
        return self.on_json_accessor_key(key);
    default:
        break; // not in a state to accept keys
    }

    return false;
}

bool GLTFEventParserObj::on_json_string(const View& string, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value())
        return true;

    // NOTE: The view from JSON event parser is transient,
    //       we must make a copy before this function returns.
    if (self.mStringSlot)
    {
        self.mStringSlot->clear();
        self.mStringSlot->write(string);
        self.mStringSlot = nullptr;
        return true;
    }

    return false;
}

bool GLTFEventParserObj::on_json_null(void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value())
        return true;

    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_bool(bool b, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value())
        return true;

    switch (self.mState)
    {
    case STATE_MATERIAL_DOUBLE_SIDED:
        self.mMaterialProp.doubleSided = b;
        self.mState = STATE_MATERIAL;
        return true;
    case STATE_ACCESSOR_NORMALIZED:
        self.mAccessorProp.normalized = b;
        self.mState = STATE_ACCESSOR;
        return true;
    default:
        break;
    }

    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_i64(int64_t i64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value())
        return true;

    return false; // not expecting signed integer
}

bool GLTFEventParserObj::on_json_u64(uint64_t u64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value() || self.on_json_u64_value(u64) || self.on_json_f64_value((double)u64))
        return true;

    return u64 <= UINT32_MAX && self.on_json_u32_value((uint32_t)u64);
}

bool GLTFEventParserObj::on_json_f64(double f64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.escape_json_value())
        return true;

    return self.on_json_f64_value(f64);
}

bool GLTFEventParserObj::escape_json_value()
{
    if (mEscapeDepth)
    {
        if (mEscapeDepth == 1)
            mEscapeDepth = 0; // escape single value

        return true;
    }

    return false;
}

bool GLTFEventParserObj::on_json_f64_value(double f64)
{
    switch (mState)
    {
    case STATE_NODE_MATRIX:
        if (mArrayCtr >= 16)
            return false;
        mNodeProp.matrix.element(mArrayCtr++) = (float)f64;
        return true;
    case STATE_NODE_TRANSLATION:
        if (mArrayCtr >= 3)
            return false;
        mNodeProp.TRS.position[mArrayCtr++] = (float)f64;
        return true;
    case STATE_NODE_ROTATION: // GLTF node rotation is a Quaternion with XYZW storage, same as our representation
        if (mArrayCtr >= 4)
            return false;
        mNodeProp.TRS.rotation[mArrayCtr++] = (float)f64;
        return true;
    case STATE_NODE_SCALE:
        if (mArrayCtr >= 3)
            return false;
        mNodeProp.TRS.scale[mArrayCtr++] = (float)f64;
        return true;
    case STATE_ACCESSOR_MIN:
        mAccessorProp.min.push_back((float)f64);
        mArrayCtr++;
        return true;
    case STATE_ACCESSOR_MAX:
        mAccessorProp.max.push_back((float)f64);
        mArrayCtr++;
        return true;
    case STATE_MATERIAL_ALPHA_CUTOFF:
        mMaterialProp.alphaCutoff = (float)f64;
        mState = STATE_MATERIAL;
        return true;
    case STATE_MATERIAL_NORMAL_TEXTURE_SCALE:
        LD_ASSERT(mMaterialProp.normalTexture.has_value());
        mMaterialProp.normalTexture->scale = (float)f64;
        return true;
    case STATE_MATERIAL_OCCLUSION_TEXTURE_STRENGTH:
        LD_ASSERT(mMaterialProp.occlusionTexture.has_value());
        mMaterialProp.occlusionTexture->strength = (float)f64;
        return true;
    case STATE_MATERIAL_EMISSIVE_FACTOR:
        if (mArrayCtr >= 3)
            return false;
        mMaterialProp.emissiveFactor[mArrayCtr++] = (float)f64;
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_FACTOR:
        if (mArrayCtr >= 4)
            return false;
        LD_ASSERT(mMaterialProp.pbr.has_value());
        mMaterialProp.pbr->baseColorFactor[mArrayCtr++] = (float)f64;
        return true;
    case STATE_MATERIAL_PBR_METALLIC_FACTOR:
        LD_ASSERT(mMaterialProp.pbr.has_value());
        mMaterialProp.pbr->metallicFactor = (float)f64;
        mState = STATE_MATERIAL_PBR;
        return true;
    case STATE_MATERIAL_PBR_ROUGHNESS_FACTOR:
        LD_ASSERT(mMaterialProp.pbr.has_value());
        mMaterialProp.pbr->roughnessFactor = (float)f64;
        mState = STATE_MATERIAL_PBR;
        return true;
    default:
        break;
    }

    return false; // not expecting floating point
}

bool GLTFEventParserObj::on_json_u64_value(uint64_t u64)
{
    switch (mState)
    {
    case STATE_BUFFER_BYTE_LENGTH:
        mBufferProp.byteLength = u64;
        mState = STATE_BUFFER;
        return true;
    case STATE_BUFFER_VIEW_BYTE_LENGTH:
        mBufferViewProp.byteLength = u64;
        mState = STATE_BUFFER_VIEW;
        return true;
    case STATE_BUFFER_VIEW_BYTE_OFFSET:
        mBufferViewProp.byteOffset = u64;
        mState = STATE_BUFFER_VIEW;
        return true;
    case STATE_BUFFER_VIEW_BYTE_STRIDE:
        mBufferViewProp.byteStride = u64;
        mState = STATE_BUFFER_VIEW;
        return true;
    case STATE_ACCESSOR_BYTE_OFFSET:
        mAccessorProp.byteOffset = u64;
        mState = STATE_ACCESSOR;
        return true;
    default:
        break;
    }

    return false; // not expecting u64
}

bool GLTFEventParserObj::on_json_u32_value(uint32_t u32)
{
    switch (mState)
    {
    case STATE_ROOT_SCENE_KEY:
        mSceneIndexProp = u32;
        if (mCallbacks.onSceneIndex)
            mCallbacks.onSceneIndex(mSceneIndexProp, mUser);
        mState = STATE_ROOT;
        return true;
    case STATE_SCENE_NODES:
        mSceneProp.nodes.push_back(u32);
        return true;
    case STATE_NODE_CHILDREN:
        mNodeProp.children.push_back(u32);
        return true;
    case STATE_NODE_MESH:
        mNodeProp.mesh = u32;
        mState = STATE_NODE;
        return true;
    case STATE_MESH_PRIMITIVE_INDICES:
        mMeshPrimitiveProp.indices = u32;
        mState = STATE_MESH_PRIMITIVE;
        return true;
    case STATE_MESH_PRIMITIVE_MATERIAL:
        mMeshPrimitiveProp.material = u32;
        mState = STATE_MESH_PRIMITIVE;
        return true;
    case STATE_MESH_PRIMITIVE_MODE:
        mMeshPrimitiveProp.mode = u32;
        mState = STATE_MESH_PRIMITIVE;
        return true;
    case STATE_MESH_PRIMITIVE_ATTRIBUTES_INDEX:
        LD_ASSERT(mPrimitiveAttributeKey.size() > 0);
        mMeshPrimitiveProp.attributes[mPrimitiveAttributeKey] = u32;
        mState = STATE_MESH_PRIMITIVE_ATTRIBUTES;
        return true;
    case STATE_MATERIAL_EMISSIVE_TEXTURE_INDEX:
        LD_ASSERT(mMaterialProp.emissiveTexture.has_value());
        mMaterialProp.emissiveTexture->index = u32;
        mState = STATE_MATERIAL_EMISSIVE_TEXTURE;
        return true;
    case STATE_MATERIAL_EMISSIVE_TEXTURE_TEXCOORD:
        LD_ASSERT(mMaterialProp.emissiveTexture.has_value());
        mMaterialProp.emissiveTexture->texCoord = u32;
        mState = STATE_MATERIAL_EMISSIVE_TEXTURE;
        return true;
    case STATE_MATERIAL_NORMAL_TEXTURE_INDEX:
        LD_ASSERT(mMaterialProp.normalTexture.has_value());
        mMaterialProp.normalTexture->index = u32;
        mState = STATE_MATERIAL_NORMAL_TEXTURE;
        return true;
    case STATE_MATERIAL_NORMAL_TEXTURE_TEXCOORD:
        LD_ASSERT(mMaterialProp.normalTexture.has_value());
        mMaterialProp.normalTexture->texCoord = u32;
        mState = STATE_MATERIAL_NORMAL_TEXTURE;
        return true;
    case STATE_MATERIAL_OCCLUSION_TEXTURE_INDEX:
        LD_ASSERT(mMaterialProp.occlusionTexture.has_value());
        mMaterialProp.occlusionTexture->index = u32;
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE;
        return true;
    case STATE_MATERIAL_OCCLUSION_TEXTURE_TEXCOORD:
        LD_ASSERT(mMaterialProp.occlusionTexture.has_value());
        mMaterialProp.occlusionTexture->texCoord = u32;
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE;
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_INDEX:
        LD_ASSERT(mMaterialProp.pbr.has_value() && mMaterialProp.pbr->baseColorTexture.has_value());
        mMaterialProp.pbr->baseColorTexture->index = u32;
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE;
        return true;
    case STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_TEXCOORD:
        LD_ASSERT(mMaterialProp.pbr.has_value() && mMaterialProp.pbr->baseColorTexture.has_value());
        mMaterialProp.pbr->baseColorTexture->texCoord = u32;
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE;
        return true;
    case STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_INDEX:
        LD_ASSERT(mMaterialProp.pbr.has_value() && mMaterialProp.pbr->metallicRoughnessTexture.has_value());
        mMaterialProp.pbr->metallicRoughnessTexture->index = u32;
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE;
        return true;
    case STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_TEXCOORD:
        LD_ASSERT(mMaterialProp.pbr.has_value() && mMaterialProp.pbr->metallicRoughnessTexture.has_value());
        mMaterialProp.pbr->metallicRoughnessTexture->texCoord = u32;
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE;
        return true;
    case STATE_TEXTURE_SAMPLER:
        mTextureProp.sampler = u32;
        mState = STATE_TEXTURE;
        return true;
    case STATE_TEXTURE_SOURCE:
        mTextureProp.source = u32;
        mState = STATE_TEXTURE;
        return true;
    case STATE_SAMPLER_MIN_FILTER:
        mSamplerProp.minFilter = u32;
        mState = STATE_SAMPLER;
        return true;
    case STATE_SAMPLER_MAG_FILTER:
        mSamplerProp.magFilter = u32;
        mState = STATE_SAMPLER;
        return true;
    case STATE_SAMPLER_WRAP_S:
        mSamplerProp.wrapS = u32;
        mState = STATE_SAMPLER;
        return true;
    case STATE_SAMPLER_WRAP_T:
        mSamplerProp.wrapT = u32;
        mState = STATE_SAMPLER;
        return true;
    case STATE_IMAGE_BUFFER_VIEW:
        mImageProp.bufferView = u32;
        mState = STATE_IMAGE;
        return true;
    case STATE_BUFFER_VIEW_BUFFER:
        mBufferViewProp.buffer = u32;
        mState = STATE_BUFFER_VIEW;
        return true;
    case STATE_BUFFER_VIEW_TARGET:
        mBufferViewProp.target = u32;
        mState = STATE_BUFFER_VIEW;
        return true;
    case STATE_ACCESSOR_BUFFER_VIEW:
        mAccessorProp.bufferView = u32;
        mState = STATE_ACCESSOR;
        return true;
    case STATE_ACCESSOR_COMPONENT_TYPE:
        mAccessorProp.componentType = u32;
        mState = STATE_ACCESSOR;
        return true;
    case STATE_ACCESSOR_COUNT:
        mAccessorProp.count = u32;
        mState = STATE_ACCESSOR;
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_root_key(const View& key)
{
    if (key == "asset")
        mState = STATE_ROOT_ASSET_KEY;
    else if (key == "scene")
        mState = STATE_ROOT_SCENE_KEY;
    else if (key == "scenes")
        mState = STATE_ROOT_SCENES_KEY;
    else if (key == "nodes")
        mState = STATE_ROOT_NODES_KEY;
    else if (key == "meshes")
        mState = STATE_ROOT_MESHES_KEY;
    else if (key == "materials")
        mState = STATE_ROOT_MATERIALS_KEY;
    else if (key == "textures")
        mState = STATE_ROOT_TEXTURES_KEY;
    else if (key == "samplers")
        mState = STATE_ROOT_SAMPLERS_KEY;
    else if (key == "images")
        mState = STATE_ROOT_IMAGES_KEY;
    else if (key == "buffers")
        mState = STATE_ROOT_BUFFERS_KEY;
    else if (key == "bufferViews")
        mState = STATE_ROOT_BUFFER_VIEWS_KEY;
    else if (key == "accessors")
        mState = STATE_ROOT_ACCESSORS_KEY;
    else
        mEscapeDepth = 1;

    return true;
}

bool GLTFEventParserObj::on_json_asset_key(const View& key)
{
    if (key == "version")
        mStringSlot = &mAssetProp.version;
    else if (key == "copyright")
        mStringSlot = &mAssetProp.copyright;
    else if (key == "generator")
        mStringSlot = &mAssetProp.generator;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_scene_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mSceneProp.name;
    else if (key == "nodes")
    {
        mState = STATE_SCENE_NODES;
        mSceneProp.nodes.clear();
    }
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_node_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mNodeProp.name;
    else if (key == "children")
    {
        mState = STATE_NODE_CHILDREN;
        mNodeProp.children.clear();
    }
    else if (key == "mesh")
        mState = STATE_NODE_MESH;
    else if (key == "matrix")
        mState = STATE_NODE_MATRIX;
    else if (key == "rotation")
        mState = STATE_NODE_ROTATION;
    else if (key == "scale")
        mState = STATE_NODE_SCALE;
    else if (key == "translation")
        mState = STATE_NODE_TRANSLATION;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_mesh_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mMeshProp.name;
    else if (key == "primitives")
        mState = STATE_MESH_PRIMITIVES_KEY;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_mesh_primitive_key(const View& key)
{
    if (key == "indices")
        mState = STATE_MESH_PRIMITIVE_INDICES;
    else if (key == "material")
        mState = STATE_MESH_PRIMITIVE_MATERIAL;
    else if (key == "mode")
        mState = STATE_MESH_PRIMITIVE_MODE;
    else if (key == "attributes")
        mState = STATE_MESH_PRIMITIVE_ATTRIBUTES_KEY;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_mesh_primitive_attributes_key(const View& key)
{
    // Attribute keys are UTF-8, hence the Buffer key instead of std::string key.
    // Common attribute keys are ascii "POSITION", "NORMAL", "TEXCOORD_*", but any key is valid.
    mPrimitiveAttributeKey = key;
    mState = STATE_MESH_PRIMITIVE_ATTRIBUTES_INDEX;

    return true;
}

bool GLTFEventParserObj::on_json_material_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mMaterialProp.name;
    else if (key == "doubleSided")
        mState = STATE_MATERIAL_DOUBLE_SIDED;
    else if (key == "alphaCutoff")
        mState = STATE_MATERIAL_ALPHA_CUTOFF;
    else if (key == "alphaMode")
        mStringSlot = &mMaterialProp.alphaMode;
    else if (key == "pbrMetallicRoughness")
        mState = STATE_MATERIAL_PBR;
    else if (key == "emissiveTexture")
        mState = STATE_MATERIAL_EMISSIVE_TEXTURE;
    else if (key == "emissiveFactor")
        mState = STATE_MATERIAL_EMISSIVE_FACTOR;
    else if (key == "normalTexture")
        mState = STATE_MATERIAL_NORMAL_TEXTURE;
    else if (key == "occlusionTexture")
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_emissive_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_EMISSIVE_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_EMISSIVE_TEXTURE_TEXCOORD;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_normal_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_NORMAL_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_NORMAL_TEXTURE_TEXCOORD;
    else if (key == "scale")
        mState = STATE_MATERIAL_NORMAL_TEXTURE_SCALE;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_occlusion_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE_TEXCOORD;
    else if (key == "strength")
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE_STRENGTH;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_pbr_key(const View& key)
{
    if (key == "baseColorFactor")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_FACTOR;
    else if (key == "metallicFactor")
        mState = STATE_MATERIAL_PBR_METALLIC_FACTOR;
    else if (key == "roughnessFactor")
        mState = STATE_MATERIAL_PBR_ROUGHNESS_FACTOR;
    else if (key == "baseColorTexture")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE;
    else if (key == "metallicRoughnessTexture")
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_pbr_base_color_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_TEXCOORD;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_material_pbr_metallic_roughness_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_TEXCOORD;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_texture_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mTextureProp.name;
    else if (key == "source")
        mState = STATE_TEXTURE_SOURCE;
    else if (key == "sampler")
        mState = STATE_TEXTURE_SAMPLER;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_sampler_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mSamplerProp.name;
    else if (key == "magFilter")
        mState = STATE_SAMPLER_MAG_FILTER;
    else if (key == "minFilter")
        mState = STATE_SAMPLER_MIN_FILTER;
    else if (key == "wrapS")
        mState = STATE_SAMPLER_WRAP_S;
    else if (key == "wrapT")
        mState = STATE_SAMPLER_WRAP_T;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_image_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mImageProp.name;
    else if (key == "uri")
        mStringSlot = &mImageProp.uri;
    else if (key == "mimeType")
        mStringSlot = &mImageProp.mimeType;
    else if (key == "bufferView")
        mState = STATE_IMAGE_BUFFER_VIEW;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_buffer_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mBufferProp.name;
    else if (key == "uri")
        mStringSlot = &mBufferProp.uri;
    else if (key == "byteLength")
        mState = STATE_BUFFER_BYTE_LENGTH;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_buffer_view_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mBufferViewProp.name;
    else if (key == "buffer")
        mState = STATE_BUFFER_VIEW_BUFFER;
    else if (key == "byteLength")
        mState = STATE_BUFFER_VIEW_BYTE_LENGTH;
    else if (key == "byteOffset")
        mState = STATE_BUFFER_VIEW_BYTE_OFFSET;
    else if (key == "byteStride")
        mState = STATE_BUFFER_VIEW_BYTE_STRIDE;
    else if (key == "target")
        mState = STATE_BUFFER_VIEW_TARGET;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::on_json_accessor_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mAccessorProp.name;
    else if (key == "type")
        mStringSlot = &mAccessorProp.type;
    else if (key == "bufferView")
        mState = STATE_ACCESSOR_BUFFER_VIEW;
    else if (key == "byteOffset")
        mState = STATE_ACCESSOR_BYTE_OFFSET;
    else if (key == "componentType")
        mState = STATE_ACCESSOR_COMPONENT_TYPE;
    else if (key == "count")
        mState = STATE_ACCESSOR_COUNT;
    else if (key == "normalized")
        mState = STATE_ACCESSOR_NORMALIZED;
    else if (key == "min")
        mState = STATE_ACCESSOR_MIN;
    else if (key == "max")
        mState = STATE_ACCESSOR_MAX;
    else
        mEscapeDepth++;

    return true;
}

bool GLTFEventParserObj::parse(const void* fileData, size_t fileSize, std::string& error)
{
    mState = STATE_ZERO;

    JSONEventCallback jsonEvents{};
    jsonEvents.onEnterObject = &GLTFEventParserObj::on_json_enter_object;
    jsonEvents.onLeaveObject = &GLTFEventParserObj::on_json_leave_object;
    jsonEvents.onEnterArray = &GLTFEventParserObj::on_json_enter_array;
    jsonEvents.onLeaveArray = &GLTFEventParserObj::on_json_leave_array;
    jsonEvents.onKey = &GLTFEventParserObj::on_json_key;
    jsonEvents.onString = &GLTFEventParserObj::on_json_string;
    jsonEvents.onNull = &GLTFEventParserObj::on_json_null;
    jsonEvents.onBool = &GLTFEventParserObj::on_json_bool;
    jsonEvents.onI64 = &GLTFEventParserObj::on_json_i64;
    jsonEvents.onU64 = &GLTFEventParserObj::on_json_u64;
    jsonEvents.onF64 = &GLTFEventParserObj::on_json_f64;

    bool isJSONValid = JSONEventParser::parse(fileData, fileSize, error, jsonEvents, this);
    if (!isJSONValid)
        return false;

    return true;
}

//
// Public API
//

/// @brief Leverages the GLTF event parser to generate a summarization string
class GLTFPrinter
{
public:
    GLTFPrinter() = delete;

    GLTFPrinter(const View& file)
        : mFile(file)
    {
    }

    bool print(std::string& str, std::string& err);

private:
    std::string fmt_indices(const Vector<uint32_t>& indices);
    std::string fmt_floats(const Vector<float>& floats);

    static bool on_asset(const GLTFAssetProp& asset, void*);
    static bool on_scene(const GLTFSceneProp& scene, void*);
    static bool on_node(const GLTFNodeProp& node, void*);
    static bool on_mesh(const GLTFMeshProp& mesh, void*);
    static bool on_mesh_primitive(const GLTFMeshPrimitiveProp& prim, void*);
    static bool on_material(const GLTFMaterialProp& mat, void*);
    static bool on_texture(const GLTFTextureProp& texture, void*);
    static bool on_sampler(const GLTFSamplerProp& sampler, void*);
    static bool on_image(const GLTFImageProp& image, void*);
    static bool on_buffer(const GLTFBufferProp& buf, void*);
    static bool on_buffer_view(const GLTFBufferViewProp& bufView, void*);
    static bool on_accessor(const GLTFAccessorProp& accessor, void*);

private:
    View mFile;
    std::string mAssetStr;
    std::string mScenesStr;
    std::string mNodesStr;
    std::string mMeshesStr;
    std::string mMeshPrimitivesStr;
    std::string mMaterialsStr;
    std::string mTexturesStr;
    std::string mSamplersStr;
    std::string mImagesStr;
    std::string mBuffersStr;
    std::string mBufferViewsStr;
    std::string mAccessorsStr;
};

bool GLTFPrinter::print(std::string& outStr, std::string& outErr)
{
    outStr.clear();
    outErr.clear();

    mAssetStr.clear();
    mScenesStr.clear();
    mNodesStr.clear();
    mMeshesStr.clear();
    mMeshPrimitivesStr.clear();
    mMaterialsStr.clear();
    mTexturesStr.clear();
    mSamplersStr.clear();
    mImagesStr.clear();
    mBuffersStr.clear();
    mBufferViewsStr.clear();
    mAccessorsStr.clear();

    GLTFEventCallback callbacks{};
    callbacks.onAsset = &GLTFPrinter::on_asset;
    callbacks.onScene = &GLTFPrinter::on_scene;
    callbacks.onNode = &GLTFPrinter::on_node;
    callbacks.onMesh = &GLTFPrinter::on_mesh;
    callbacks.onMeshPrimitive = &GLTFPrinter::on_mesh_primitive;
    callbacks.onMaterial = &GLTFPrinter::on_material;
    callbacks.onTexture = &GLTFPrinter::on_texture;
    callbacks.onSampler = &GLTFPrinter::on_sampler;
    callbacks.onImage = &GLTFPrinter::on_image;
    callbacks.onBuffer = &GLTFPrinter::on_buffer;
    callbacks.onBufferView = &GLTFPrinter::on_buffer_view;
    callbacks.onAccessor = &GLTFPrinter::on_accessor;

    if (!GLTFEventParser::parse(mFile, outErr, callbacks, this))
        return false;

    // GLTF top level properties may appear in arbitrary order,
    // here we canonicalize the order for printing.
    outStr = mAssetStr;
    outStr += mScenesStr;
    outStr += mNodesStr;
    outStr += mMeshesStr;
    outStr += mMaterialsStr;
    outStr += mTexturesStr;
    outStr += mSamplersStr;
    outStr += mImagesStr;
    outStr += mBuffersStr;
    outStr += mBufferViewsStr;
    outStr += mAccessorsStr;

    return true;
}

std::string GLTFPrinter::fmt_indices(const Vector<uint32_t>& indices)
{
    std::string str(1, '[');

    for (int i = 0; i < (int)indices.size(); i++)
    {
        if (i > 0)
            str += ", ";

        str += std::to_string(indices[i]);
    }

    str.push_back(']');
    return str;
}

std::string GLTFPrinter::fmt_floats(const Vector<float>& floats)
{
    std::string str(1, '[');

    for (int i = 0; i < (int)floats.size(); i++)
    {
        if (i > 0)
            str += ", ";

        str += std::format("{:.2f}", floats[i]);
    }

    str.push_back(']');
    return str;
}

bool GLTFPrinter::on_asset(const GLTFAssetProp& asset, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    self.mAssetStr = "asset:\n";

    if (asset.version.size() > 0)
        self.mAssetStr += std::format("- version: {}\n", asset.version.view());

    if (asset.generator.size() > 0)
        self.mAssetStr += std::format("- generator: {}\n", asset.generator.view());

    if (asset.copyright.size() > 0)
        self.mAssetStr += std::format("- copyright: {}\n", asset.copyright.view());

    return true;
}

bool GLTFPrinter::on_scene(const GLTFSceneProp& scene, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string sceneStr = std::format("scene:");

    if (scene.name.size() > 0)
        sceneStr += std::format(" {}", scene.name.view());
    sceneStr.push_back('\n');

    if (scene.nodes.size() > 0)
        sceneStr += std::format("- nodes: {}\n", self.fmt_indices(scene.nodes));

    self.mScenesStr += sceneStr;
    return true;
}

bool GLTFPrinter::on_node(const GLTFNodeProp& node, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string nodeStr = std::format("node:");

    if (node.name.size() > 0)
        nodeStr += std::format(" {}", node.name.view());
    nodeStr.push_back('\n');

    if (node.children.size() > 0)
        nodeStr += std::format("- children: {}\n", self.fmt_indices(node.children));

    if (node.mesh.has_value())
        nodeStr += std::format("- mesh: {}\n", node.mesh.value());

    self.mNodesStr += nodeStr;
    return true;
}

bool GLTFPrinter::on_mesh(const GLTFMeshProp& mesh, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string meshStr = std::format("mesh:");

    if (mesh.name.size() > 0)
        meshStr += std::format(" {}", mesh.name.view());
    meshStr.push_back('\n');

    meshStr += self.mMeshPrimitivesStr;
    self.mMeshPrimitivesStr.clear();

    self.mMeshesStr += meshStr;
    return true;
}

bool GLTFPrinter::on_mesh_primitive(const GLTFMeshPrimitiveProp& prim, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string primStr = "- primitive: attributes {";

    for (const auto& it : prim.attributes)
    {
        primStr += std::format(" {} : {}", it.first.view(), it.second);
    }

    primStr += " }";
    primStr += std::format(" mode {}", prim.mode);

    if (prim.indices.has_value())
        primStr += std::format(" indices {}", prim.indices.value());

    if (prim.material.has_value())
        primStr += std::format(" material {}", prim.material.value());

    primStr.push_back('\n');
    self.mMeshPrimitivesStr += primStr;
    return true;
}

bool GLTFPrinter::on_material(const GLTFMaterialProp& mat, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string matStr = std::format("material:");

    if (mat.name.size() > 0)
        matStr += std::format(" {}", mat.name.view());
    matStr.push_back('\n');

    matStr += std::format("- alphaMode: {}\n", mat.alphaMode.view());
    matStr += std::format("- alphaCutoff: {:.2f}\n", mat.alphaCutoff);
    matStr += std::format("- emissiveFactor: [{:.2f},{:.2f},{:.2f}]\n", mat.emissiveFactor.r, mat.emissiveFactor.g, mat.emissiveFactor.b);

    if (mat.emissiveTexture.has_value())
    {
        const GLTFTextureInfo& info = mat.emissiveTexture.value();
        matStr += std::format("- emissiveTexture:  index {}, texCoord {}\n", info.index, info.texCoord);
    }

    if (mat.normalTexture.has_value())
    {
        const GLTFNormalTextureInfo& info = mat.normalTexture.value();
        matStr += std::format("- normalTexture:    index {}, texCoord {}, scale {:.2f}\n", info.index, info.texCoord, info.scale);
    }

    if (mat.occlusionTexture.has_value())
    {
        const GLTFOcclusionTextureInfo& info = mat.occlusionTexture.value();
        matStr += std::format("- occlusionTexture: index {}, texCoord {}, strength {:.2f}\n", info.index, info.texCoord, info.strength);
    }

    if (mat.pbr.has_value())
    {
        const Vec4& clr = mat.pbr->baseColorFactor;
        matStr += "- pbrMetallicRoughness\n";
        matStr += std::format("  - baseColorFactor [{:.2f},{:.2f},{:.2f},{:.2f}]\n", clr.r, clr.g, clr.b, clr.a);
        if (mat.pbr->baseColorTexture.has_value())
        {
            const GLTFTextureInfo& info = mat.pbr->baseColorTexture.value();
            matStr += std::format("  - baseColorTexture: index {}, texCoord {}\n", info.index, info.texCoord);
        }
        matStr += std::format("  - metallicFactor  {:.2f}\n", mat.pbr->metallicFactor);
        matStr += std::format("  - roughnessFactor {:.2f}\n", mat.pbr->roughnessFactor);
        if (mat.pbr->metallicRoughnessTexture.has_value())
        {
            const GLTFTextureInfo& info = mat.pbr->metallicRoughnessTexture.value();
            matStr += std::format("  - metallicRoughnessTexture: index {}, texCoord {}\n", info.index, info.texCoord);
        }
    }

    self.mMaterialsStr += matStr;
    return true;
}

bool GLTFPrinter::on_texture(const GLTFTextureProp& texture, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string textureStr = "texture:";

    if (texture.source.has_value())
        textureStr += std::format(" source {}", texture.source.value());

    if (texture.sampler.has_value())
        textureStr += std::format(" sampler {}", texture.sampler.value());

    if (texture.name.size() > 0)
        textureStr += std::format(" name {}", texture.name.view());

    textureStr.push_back('\n');
    self.mTexturesStr += textureStr;
    return true;
}

bool GLTFPrinter::on_sampler(const GLTFSamplerProp& sampler, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string samplerStr = std::format("sampler: wrapS {} wrapT {}", sampler.wrapS, sampler.wrapT);

    if (sampler.minFilter.has_value())
        samplerStr += std::format(" minFilter {}", sampler.minFilter.value());

    if (sampler.magFilter.has_value())
        samplerStr += std::format(" magFilter {}", sampler.magFilter.value());

    if (sampler.name.size() > 0)
        samplerStr += std::format(" name {}", sampler.name.view());

    samplerStr.push_back('\n');
    self.mSamplersStr += samplerStr;
    return true;
}

bool GLTFPrinter::on_image(const GLTFImageProp& image, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string imageStr = "image:";

    if (image.uri.size() > 0)
        imageStr += std::format(" uri {}", image.uri.view());

    if (image.bufferView.has_value())
        imageStr += std::format(" bufferView {}", image.bufferView.value());

    if (image.mimeType.size() > 0)
        imageStr += std::format(" mimeType {}", image.mimeType.view());

    if (image.name.size() > 0)
        imageStr += std::format(" name {}", image.name.view());

    imageStr.push_back('\n');
    self.mImagesStr += imageStr;
    return true;
}

bool GLTFPrinter::on_buffer(const GLTFBufferProp& buf, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string bufStr = std::format("buffer: byteLength {}", buf.byteLength);

    if (buf.uri.size() > 0)
        bufStr += std::format(" uri {}", buf.uri.view());

    if (buf.name.size() > 0)
        bufStr += std::format(" name {}", buf.name.view());

    bufStr.push_back('\n');
    self.mBuffersStr += bufStr;
    return true;
}

bool GLTFPrinter::on_buffer_view(const GLTFBufferViewProp& bufView, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string bufViewStr = std::format("bufferView: buffer {:>2} byteOffset {:>6} byteLength {:>6}", bufView.buffer, bufView.byteOffset, bufView.byteLength);

    if (bufView.byteStride.has_value())
        bufViewStr += std::format(" byteStride {:>6}", bufView.byteStride.value());

    if (bufView.target.has_value())
        bufViewStr += std::format(" target {}", bufView.target.value());

    if (bufView.name.size() > 0)
        bufViewStr += std::format(" name {}", bufView.name.view());

    bufViewStr.push_back('\n');
    self.mBufferViewsStr += bufViewStr;
    return true;
}

bool GLTFPrinter::on_accessor(const GLTFAccessorProp& acc, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string accStr = "accessor:";

    if (acc.bufferView.has_value())
        accStr += std::format(" bufferView {:>2} byteOffset {:>6}", acc.bufferView.value(), acc.byteOffset);

    accStr += std::format(" count {:>6} type {} componentType {}", acc.count, acc.type.view(), acc.componentType);

    if (acc.normalized)
        accStr += " normalized";

    if (acc.min.size() > 0)
        accStr += std::format(" min {}", self.fmt_floats(acc.min));

    if (acc.max.size() > 0)
        accStr += std::format(" max {}", self.fmt_floats(acc.max));

    if (acc.name.size() > 0)
        accStr += std::format(" name {}", acc.name.view());

    accStr.push_back('\n');
    self.mAccessorsStr += accStr;
    return true;
}

bool GLTFEventParser::parse(const View& file, std::string& error, const GLTFEventCallback& callbacks, void* user)
{
    GLTFEventParserObj obj(callbacks, user);

    return obj.parse(file.data, file.size, error);
}

bool print_gltf_data(const View& file, std::string& str, std::string& err)
{
    GLTFPrinter printer(file);

    return printer.print(str, err);
}

} // namespace LD