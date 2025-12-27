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
    bool on_json_u64_value(uint64_t u64);
    bool on_json_u32_value(uint32_t u32);

private:
    enum State
    {
        STATE_ZERO = 0,
        STATE_ROOT, // top level GLTF object
        STATE_ASSET_KEY,
        STATE_ASSET,
        STATE_SCENE_KEY,
        STATE_SCENES_KEY,
        STATE_SCENES_ARRAY,
        STATE_SCENES,
        STATE_SCENES_NODES,
    };

    State mState;
    Buffer mLastKey;
    void* mUser;
    Buffer* mStringSlot;
    GLTFEventCallback mCallbacks;
    GLTFAssetProp mAssetProp{};
    GLTFScenesProp mScenesProp{};
    uint32_t mSceneProp = 0;
};

bool GLTFEventParserObj::on_json_enter_object(void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    switch (self.mState)
    {
    case STATE_ZERO:
        self.mState = STATE_ROOT;
        return true;
    case STATE_ASSET_KEY:
        self.mState = STATE_ASSET;
        self.mAssetProp = {};
        return true;
    case STATE_SCENES_ARRAY:
        self.mState = STATE_SCENES;
        self.mScenesProp = {};
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_leave_object(size_t memberCount, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    switch (self.mState)
    {
    case STATE_ASSET:
        self.mState = STATE_ROOT;
        if (self.mCallbacks.onAsset)
            self.mCallbacks.onAsset(self.mAssetProp, self.mUser);
        return true;
    case STATE_SCENES:
        self.mState = STATE_SCENES_ARRAY;
        if (self.mCallbacks.onScenes)
            self.mCallbacks.onScenes(self.mScenesProp, self.mUser);
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_enter_array(void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    switch (self.mState)
    {
    case STATE_SCENES_KEY:
        self.mState = STATE_SCENES_ARRAY;
        return true;
    case STATE_SCENES:
        if (self.mLastKey.view() == "nodes")
        {
            self.mState = STATE_SCENES_NODES;
            return true;
        }
        return false;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_leave_array(size_t elementCount, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    switch (self.mState)
    {
    case STATE_SCENES_ARRAY:
        self.mState = STATE_ROOT;
        return true;
    case STATE_SCENES_NODES:
        self.mState = STATE_SCENES;
        return true;
    default:
        break;
    }

    return false;
}

bool GLTFEventParserObj::on_json_key(const View& key, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    self.mLastKey.clear();
    self.mLastKey.write(key);

    switch (self.mState)
    {
    case STATE_ROOT:
        if (key == "asset")
            self.mState = STATE_ASSET_KEY;
        else if (key == "scene")
            self.mState = STATE_SCENE_KEY;
        else if (key == "scenes")
            self.mState = STATE_SCENES_KEY;
        else
            return false; // unrecognized root object key
        return true;
    case STATE_ASSET:
        if (key == "version")
            self.mStringSlot = &self.mAssetProp.version;
        else if (key == "copyright")
            self.mStringSlot = &self.mAssetProp.copyright;
        else if (key == "generator")
            self.mStringSlot = &self.mAssetProp.generator;
        return true; // TODO: ignore prop code path
    case STATE_SCENES:
        if (key == "name")
            self.mStringSlot = &self.mScenesProp.name;
        else if (key == "nodes")
            self.mScenesProp.nodes.clear();
        return true;  // TODO: ignore prop code path
    default:
        break; // not in a state to accept keys
    }

    return false;
}

bool GLTFEventParserObj::on_json_string(const View& string, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

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
    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_bool(bool b, void* obj)
{
    return false; // not expecting boolean
}

bool GLTFEventParserObj::on_json_i64(int64_t i64, void* obj)
{
    return false; // not expecting signed integer
}

bool GLTFEventParserObj::on_json_u64(uint64_t u64, void* obj)
{
    auto& self = *(GLTFEventParserObj*)obj;

    if (self.on_json_u64_value(u64))
        return true;

    return u64 <= UINT32_MAX && self.on_json_u32_value((uint32_t)u64);
}

bool GLTFEventParserObj::on_json_f64(double f64, void* obj)
{
    return false; // not expecting floating point
}

bool GLTFEventParserObj::on_json_u64_value(uint64_t u64)
{
    return false; // not expecting u64
}

bool GLTFEventParserObj::on_json_u32_value(uint32_t u32)
{
    if (mState == STATE_SCENE_KEY)
    {
        mSceneProp = u32;

        if (mCallbacks.onSceneProp)
            mCallbacks.onSceneProp(mSceneProp, mUser);

        mState = STATE_ROOT;
        return true;
    }
    else if (mState == STATE_SCENES_NODES)
    {
        mScenesProp.nodes.push_back(u32);
        return true;
    }

    return false;
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
struct GLTFPrinter
{
    View file;
    std::string str;

    GLTFPrinter(const View& file)
        : file(file)
    {
    }

    bool print(std::string& str, std::string& err);

    static bool on_asset(const GLTFAssetProp& asset, void*);
    static bool on_scenes(const GLTFScenesProp& scene, void*);
};

bool GLTFPrinter::print(std::string& outStr, std::string& outErr)
{
    outStr.clear();
    outErr.clear();
    str.clear();

    GLTFEventCallback callbacks{};
    callbacks.onAsset = &GLTFPrinter::on_asset;
    callbacks.onScenes = &GLTFPrinter::on_scenes;

    if (!GLTFEventParser::parse(file, outErr, callbacks, this))
        return false;

    return true;
}

bool GLTFPrinter::on_asset(const GLTFAssetProp& asset, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    self.str += "asset:\n";

    // TODO:
    return true;
}

bool GLTFPrinter::on_scenes(const GLTFScenesProp& scene, void* user)
{
    auto& self = *(GLTFPrinter*)user;

    // TODO:
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