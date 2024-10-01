#pragma once

#include <string>
#include <unordered_map>
#include "Core/Header/Include/Singleton.h"

namespace LD
{

class Document;
class DocumentItem;

struct DocumentLink
{
    std::string LinkID;
    DocumentItem* Item;
};

/// Manages the link relationship across documents.
/// The basic unit that can be linked to is a document item.
/// While a document can declare any number of links, the links
/// are hollow until we connect it to an existing document item.
class DocumentLinkManager : public Singleton<DocumentLinkManager>
{
public:
    ~DocumentLinkManager();

    /// register a link with an identifier, the link is hollow until we connect it.
    void RegisterLink(const char* linkID);

    /// remove the link with the identifier
    void RemoveLink(const char* linkID);

    /// remove all links connected to a document
    void RemoveLinksToDocument(Document* document);

    /// connect a link to an existing document item
    void ConnectLink(const char* linkID, DocumentItem* item);

    /// @brief looks up a link
    /// @param linkID the link to search for
    /// @param link the resulting link, if found
    /// @return true if the link is found, false otherwise
    /// @note even if the link is found, the Item field may be null
    ///       if the link is not yet connected.
    bool GetLink(const char* linkID, DocumentLink& link);

    /// return true if all links are connected to a valid item 
    bool IsEveryLinkConnected();

private:
    DocumentLinkManager();

    // TODO: hashing can be improved
    std::unordered_map<std::string, DocumentLink> mLinks;
};

} // namespace LD