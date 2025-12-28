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
    bool on_json_f64_value(double f64);
    bool on_json_u64_value(uint64_t u64);
    bool on_json_u32_value(uint32_t u32);
    bool on_json_root_key(const View& key);
    bool on_json_asset_key(const View& key);
    bool on_json_scene_key(const View& key);
    bool on_json_node_key(const View& key);
    bool on_json_material_key(const View& key);
    bool on_json_material_normal_texture_key(const View& key);
    bool on_json_material_occlusion_texture_key(const View& key);
    bool on_json_material_pbr_key(const View& key);
    bool on_json_material_pbr_base_color_texture_key(const View& key);
    bool on_json_material_pbr_metallic_roughness_texture_key(const View& key);

private:
    enum State
    {
        STATE_ZERO = 0,
        STATE_ROOT, // top level GLTF object
        STATE_ROOT_ASSET_KEY,
        STATE_ROOT_SCENE_KEY,
        STATE_ROOT_SCENES_KEY,
        STATE_ROOT_NODES_KEY,
        STATE_ROOT_MATERIALS_KEY,
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
        STATE_MATERIALS_ARRAY,
        STATE_MATERIAL,
        STATE_MATERIAL_DOUBLE_SIDED,
        STATE_MATERIAL_ALPHA_CUTOFF,
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
    };

    State mState;
    void* mUser;
    uint32_t mEscapeDepth = 0;
    Buffer* mStringSlot;
    GLTFEventCallback mCallbacks;
    GLTFAssetProp mAssetProp{};
    GLTFSceneProp mSceneProp{};
    GLTFNodeProp mNodeProp{};
    GLTFMaterialProp mMaterialProp{};
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
    case STATE_MATERIALS_ARRAY:
        self.mState = STATE_MATERIAL;
        self.mMaterialProp = {};
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
    case STATE_MATERIAL:
        self.mState = STATE_MATERIALS_ARRAY;
        if (self.mCallbacks.onMaterial)
            self.mCallbacks.onMaterial(self.mMaterialProp, self.mUser);
        return true;
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
    case STATE_ROOT_MATERIALS_KEY:
        self.mState = STATE_MATERIALS_ARRAY;
        return true;
    case STATE_SCENE_NODES:
    case STATE_NODE_CHILDREN:
    case STATE_NODE_MATRIX:
    case STATE_NODE_TRANSLATION:
    case STATE_NODE_ROTATION:
    case STATE_NODE_SCALE:
    case STATE_MATERIAL_PBR_BASE_COLOR_FACTOR:
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
    case STATE_MATERIALS_ARRAY:
        self.mState = STATE_ROOT;
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
        return true;

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
    case STATE_MATERIAL:
        return self.on_json_material_key(key);
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
    default:
        break; // not in a state to accept keys
    }

    return false;
}

bool GLTFEventParserObj::on_json_string(const View& string, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
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

    if (self.mEscapeDepth)
        return true;

    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_bool(bool b, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
        return true;

    switch (self.mState)
    {
    case STATE_MATERIAL_DOUBLE_SIDED:
        self.mMaterialProp.doubleSided = b;
        self.mState = STATE_MATERIAL;
        return true;
    default:
        break;
    }

    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_i64(int64_t i64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
        return true;

    return false; // not expecting signed integer
}

bool GLTFEventParserObj::on_json_u64(uint64_t u64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth || self.on_json_u64_value(u64) || self.on_json_f64_value((double)u64))
        return true;

    return u64 <= UINT32_MAX && self.on_json_u32_value((uint32_t)u64);
}

bool GLTFEventParserObj::on_json_f64(double f64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.mEscapeDepth)
        return true;

    return self.on_json_f64_value(f64);
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
    else if (key == "materials")
        mState = STATE_ROOT_MATERIALS_KEY;
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

    return true; // TODO: ignore prop code path
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

    return true; // TODO: ignore prop code path
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

bool GLTFEventParserObj::on_json_material_key(const View& key)
{
    if (key == "name")
        mStringSlot = &mMaterialProp.name;
    else if (key == "doubleSided")
        mState = STATE_MATERIAL_DOUBLE_SIDED;
    else if (key == "alphaCutoff")
        mState = STATE_MATERIAL_ALPHA_CUTOFF;
    else if (key == "pbrMetallicRoughness")
        mState = STATE_MATERIAL_PBR;
    else if (key == "normalTexture")
        mState = STATE_MATERIAL_NORMAL_TEXTURE;
    else if (key == "occlusionTexture")
        mState = STATE_MATERIAL_OCCLUSION_TEXTURE;
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

    return true; // TODO: ignore prop code path
}

bool GLTFEventParserObj::on_json_material_pbr_base_color_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_PBR_BASE_COLOR_TEXTURE_TEXCOORD;

    return true;
}

bool GLTFEventParserObj::on_json_material_pbr_metallic_roughness_texture_key(const View& key)
{
    if (key == "index")
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_INDEX;
    else if (key == "texCoord")
        mState = STATE_MATERIAL_PBR_METALLIC_ROUGHNESS_TEXTURE_TEXCOORD;

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
    std::string fmt_indices(const std::vector<uint32_t>& indices);

    static bool on_asset(const GLTFAssetProp& asset, void*);
    static bool on_scene(const GLTFSceneProp& scene, void*);
    static bool on_node(const GLTFNodeProp& node, void*);
    static bool on_material(const GLTFMaterialProp& mat, void*);

private:
    View mFile;
    std::string mAssetStr;
    std::string mScenesStr;
    std::string mNodesStr;
    std::string mMaterialsStr;
};

bool GLTFPrinter::print(std::string& outStr, std::string& outErr)
{
    outStr.clear();
    outErr.clear();

    mAssetStr.clear();
    mScenesStr.clear();
    mNodesStr.clear();
    mMaterialsStr.clear();

    GLTFEventCallback callbacks{};
    callbacks.onAsset = &GLTFPrinter::on_asset;
    callbacks.onScene = &GLTFPrinter::on_scene;
    callbacks.onNode = &GLTFPrinter::on_node;
    callbacks.onMaterial = &GLTFPrinter::on_material;

    if (!GLTFEventParser::parse(mFile, outErr, callbacks, this))
        return false;

    // GLTF top level properties may appear in arbitrary order,
    // here we canonicalize the order for printing.
    outStr = mAssetStr;
    outStr += mScenesStr;
    outStr += mNodesStr;
    outStr += mMaterialsStr;

    return true;
}

std::string GLTFPrinter::fmt_indices(const std::vector<uint32_t>& indices)
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

bool GLTFPrinter::on_material(const GLTFMaterialProp& mat, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    std::string matStr = std::format("material:");

    if (mat.name.size() > 0)
        matStr += std::format(" {}", mat.name.view());
    matStr.push_back('\n');

    if (mat.normalTexture.has_value())
    {
        const GLTFNormalTextureInfo& info = mat.normalTexture.value();
        matStr += std::format("- normalTexture: index {}, texCoord {}, scale {:.2f}\n", info.index, info.texCoord, info.scale);
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