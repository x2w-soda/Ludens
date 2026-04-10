#include <Ludens/DSA/URI.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>

namespace LD {

void document_uri_normalize(URI& uri)
{
    if (!document_uri_is_valid(uri))
        return;

    View path = uri.path();

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
}

bool document_uri_is_valid(const char* uriCstr)
{
    return uriCstr && document_uri_is_valid(URI(uriCstr));
}

bool document_uri_is_valid(const URI& uri)
{
    return uri.scheme() == "ld" && uri.authority() == "Doc";
}

FS::Path document_uri_md_path(const char* uriCstr)
{
    URI uri(uriCstr);

    if (!document_uri_is_valid(uri))
        return {};

    document_uri_normalize(uri);

    View pathView = uri.path();
    FS::Path path = std::string(pathView.data, pathView.size);
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