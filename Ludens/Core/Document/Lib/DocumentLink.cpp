#include "Core/Header/Include/Error.h"
#include "Core/Document/Include/DocumentLink.h"
#include "Core/Document/Include/DocumentItem.h"

namespace LD
{

DocumentLinkManager::~DocumentLinkManager()
{
    mLinks.clear();
}

void DocumentLinkManager::RegisterLink(const char* linkID)
{
    LD_DEBUG_ASSERT(mLinks.find(linkID) == mLinks.end());

    mLinks[linkID].LinkID = std::string{ linkID };
    mLinks[linkID].Item = nullptr;
}

void DocumentLinkManager::RemoveLink(const char* linkID)
{
    auto link = mLinks.find(linkID);

    if (link != mLinks.end())
        mLinks.erase(link);
}

void DocumentLinkManager::RemoveLinksToDocument(Document* document)
{
    for (auto ite = mLinks.begin(); ite != mLinks.end(); ite++)
    {
        DocumentItem* item = ite->second.Item;
        if (item && item->GetDocument() == document)
            mLinks.erase(ite);
    }
}

void DocumentLinkManager::ConnectLink(const char* linkID, DocumentItem* item)
{
    if (mLinks.find(linkID) == mLinks.end())
        RegisterLink(linkID);

    LD_DEBUG_ASSERT(item != nullptr);
    LD_DEBUG_ASSERT(mLinks[linkID].Item == nullptr);

    mLinks[linkID].Item = item;
}

bool DocumentLinkManager::GetLink(const char* linkID, DocumentLink& link)
{
    if (mLinks.find(linkID) == mLinks.end())
        return false;

    link = mLinks[linkID];
    return true;
}

bool DocumentLinkManager::IsEveryLinkConnected()
{
    for (auto ite = mLinks.begin(); ite != mLinks.end(); ite++)
        if (ite->second.Item == nullptr)
            return false;

    return true;
}

} // namespace LD