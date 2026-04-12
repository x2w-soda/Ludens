#include <Ludens/DSA/URI.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

namespace LD {

const char* document_uri_default_page_path()
{
    return "Manual/GettingStarted";
}

std::string document_uri_normalized_path(const URI& uri)
{
    if (!document_uri_is_valid(uri))
        return {};

    View pathV = uri.path();
    std::string pathStr = std::string(pathV.data, pathV.size);
    if (pathStr.ends_with('/'))
        pathStr.pop_back();

    FS::Path path = pathStr;

    if (path.filename() == "index.md")
        path = path.parent_path();

    if (path.extension() == ".md")
        path.replace_extension("");

    /*
    if (path.ends_with("/"))
    {
        std::string pathStr = std::string(path.data, path.size);
        pathStr += "index.md";
        uri = URI("ld://Doc/" + pathStr);
    }
    else if (!path.ends_with(".md"))
    {
        std::string pathStr = std::string(path.data, path.size);
        pathStr += "/index.md";
        uri = URI("ld://Doc/" + pathStr);
    }
    */
    return path.string();
}

bool document_uri_is_valid(const char* uriCstr)
{
    return uriCstr && document_uri_is_valid(URI(uriCstr));
}

bool document_uri_is_valid(const URI& uri)
{
    return uri.scheme() == "ld" && uri.authority() == "Doc";
}

FS::Path document_md_path_from_uri_path(const char* uriPath)
{
    FS::Path path = uriPath;
    if (path.empty())
        return {};

    if (!path.has_extension())
        path /= "index.md";

    return path.lexically_normal();
}

bool document_uri_is_manual(const URI& uri)
{
    if (!document_uri_is_valid(uri))
        return false;

    View pathView = uri.path();
    return pathView.size >= 6 && !memcmp(pathView.data, "Manual", 6);
}

bool document_uri_is_lua_api(const URI& uri)
{
    if (!document_uri_is_valid(uri))
        return false;

    View pathView = uri.path();
    return pathView.size >= 6 && !memcmp(pathView.data, "LuaAPI", 6);
}

} // namespace LD