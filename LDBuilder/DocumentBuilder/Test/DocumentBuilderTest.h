#pragma once

#include <Ludens/DSA/HashSet.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Types.h>
#include <LudensBuilder/DocumentBuilder/Document.h>
#include <LudensBuilder/DocumentBuilder/DocumentRegistry.h>

#include <filesystem>

/// @brief Require valid document before test.
LD::Document require_document(const char* md, const char* uri = "doc://test.md");

/// @brief Require valid document for all files under Docs/ folder before test.
///        Since documents mostly rely on views, user keeps the stroage alive for testing.
bool require_documents(LD::DocumentRegistry reg, LD::HashSet<LD::Vector<LD::byte>*>& storage, const std::filesystem::path& docPath);

void release_document_storage(LD::HashSet<LD::Vector<LD::byte>*>& storage);