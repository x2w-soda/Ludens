#pragma once

#include <string>
#include "Core/Header/Include/Error.h"
#include "Core/UI/Include/Control/UITexture.h"
#include "Core/UI/Include/Control/UILabel.h"

namespace LD
{

class Document;

enum class DocumentItemType
{
    Text,
    Image,
};

/// The basic unit to display in a document. Each document item
/// is owned by exactly one document, and can be referenced via
/// multiple document links.
class DocumentItem
{
public:
    DocumentItem() = delete;
    DocumentItem(DocumentItemType type) : mType(type)
    {
    }
    DocumentItem(const DocumentItem&) = delete;
    virtual ~DocumentItem() = default;

    DocumentItem& operator=(const DocumentItem&) = delete;

    inline Document* GetDocument()
    {
        LD_DEBUG_ASSERT(mDocument != nullptr);
        return mDocument;
    }

    inline DocumentItemType GetType() const
    {
        return mType;
    }

private:
    Document* mDocument = nullptr;
    DocumentItemType mType;
};

/// a text paragraph in the document
class DocumentText : public DocumentItem
{
public:
    DocumentText(const char* md);
    DocumentText(const DocumentText&) = delete;
    ~DocumentText();

    DocumentText& operator=(const DocumentText&) = delete;

private:
    void ParseMD(const char* md);

    std::string mName;
    UILabel mText;
};

/// an image in the document
class DocumentImage : public DocumentItem
{
public:
    DocumentImage();
    DocumentImage(const DocumentImage&) = delete;
    ~DocumentImage();

    DocumentImage& operator=(const DocumentImage&) = delete;

private:
    UITexture mImage;
};

} // namespace LD