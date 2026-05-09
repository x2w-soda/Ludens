#pragma once

#include <Ludens/System/FileSystem.h>

namespace LD {

class URI;

const char* document_uri_default_page_path();
String document_uri_normalized_path(const URI& uri);

/// @brief Check if a URI is a valid path.
///        Does not confirm if the uri references a valid document.
bool document_uri_is_valid(const char* uri);
bool document_uri_is_valid(const URI& uri);

/// @brief Get a relative path to markdown a file.
///        Does not confirm if the uri references a valid document.
FS::Path document_md_path_from_uri_path(const char* uriPath);

bool document_uri_is_manual(const URI& uri);
bool document_uri_is_lua_api(const URI& uri);

} // namespace LD