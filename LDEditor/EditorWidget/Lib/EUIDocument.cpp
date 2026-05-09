#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensBuilder/DocumentBuilder/DocumentURI.h>
#include <LudensEditor/EditorWidget/EUIDocument.h>
#include <LudensEditor/EditorWidget/EditorWidget.h>

#include "EUI.h"

namespace LD {

/// @brief Document imgui state
struct EUIDocumentState
{
    EUIDocument* eui;
    size_t itemIndex;
    EditorTheme theme;
    EditorDocumentTheme docTheme;
    UITheme uiTheme;
    float fontSize;
};

struct EUIDocumentMeta
{
    void (*build_fn)(DocumentItem* src, EUIDocumentItem& dst);
    void (*imgui_fn)(EUIDocumentState& state, EUIDocumentItem& item);
};

static std::string document_span_to_text_span(TView<DocumentSpan*> srcSpans, Vector<UITextSpan>& dstSpans);
static void build_document_item_heading(DocumentItem* src, EUIDocumentItem& dst);
static void build_document_item_paragraph(DocumentItem* src, EUIDocumentItem& dst);
static void build_document_item_code_block(DocumentItem* src, EUIDocumentItem& dst);
static void build_document_item_list_entry(DocumentItem* src, EUIDocumentItem& dst);
static void eui_document_item_heading(EUIDocumentState& state, EUIDocumentItem& storage);
static void eui_document_item_paragraph(EUIDocumentState& state, EUIDocumentItem& storage);
static void eui_document_item_code_block(EUIDocumentState& state, EUIDocumentItem& storage);
static void eui_document_item_list_entry(EUIDocumentState& state, EUIDocumentItem& storage);
static void eui_document_spans(EUIDocumentState& state, EUIDocumentItem& storage, UITextData* testS);

// clang-format off
static EUIDocumentMeta sEUIDocumentMeta[] = {
    { &build_document_item_heading,    &eui_document_item_heading },
    { &build_document_item_paragraph,  &eui_document_item_paragraph },
    { &build_document_item_code_block, &eui_document_item_code_block },
    { &build_document_item_list_entry, &eui_document_item_list_entry },
};
// clang-format on

static_assert(sizeof(sEUIDocumentMeta) / sizeof(*sEUIDocumentMeta) == (int)DOCUMENT_ITEM_ENUM_COUNT);

// Adapter to convert DocumentSpan to UITextSpan for rendering.
static std::string document_span_to_text_span(TView<DocumentSpan*> srcSpans, Vector<UITextSpan>& dstSpans)
{
    std::string str;
    size_t offset = 0;

    dstSpans.resize(srcSpans.size);

    for (size_t i = 0; i < srcSpans.size; i++)
    {
        DocumentSpan* srcSpan = srcSpans.data[i];
        std::string segment = std::string((const char*)srcSpan->text.data, srcSpan->text.size);

        dstSpans[i] = {};
        dstSpans[i].text.fgColor = 0xFFFFFFFF;
        dstSpans[i].text.range = Range(offset, segment.size());

        str += segment;
        offset += segment.size();
    }

    return str;
}

static void build_document_item_heading(DocumentItem* src, EUIDocumentItem& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_HEADING);

    EditorTheme theme = eui_get_theme();
    Vector<UITextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.fontSize = theme.get_font_size();
    dst.text.set_fg_color(theme.get_ui_theme().get_on_surface_color());
}

static void build_document_item_paragraph(DocumentItem* src, EUIDocumentItem& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_PARAGRAPH);

    EditorTheme theme = eui_get_theme();
    auto* paragraph = (DocumentItemParagraph*)src;

    Vector<UITextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.set_fg_color(theme.get_ui_theme().get_on_surface_color());
}

static void build_document_item_code_block(DocumentItem* src, EUIDocumentItem& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_CODE_BLOCK);

    EditorTheme theme = eui_get_theme();
    auto* block = (DocumentItemCodeBlock*)src;

    // TODO: more decoration for code blocks,
    //       display language, copy button, etc.
    (void)block;

    Vector<UITextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.set_fg_color(theme.get_ui_theme().get_on_surface_color());
}

static void build_document_item_list_entry(DocumentItem* src, EUIDocumentItem& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_LIST_ENTRY);

    EditorTheme theme = eui_get_theme();

    // TODO:
    Vector<UITextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.set_fg_color(theme.get_ui_theme().get_on_surface_color());
}

static void eui_document_item_heading(EUIDocumentState& state, EUIDocumentItem& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_HEADING);

    float headingFontSize = state.fontSize;
    auto* heading = (DocumentItemHeading*)storage.item;
    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_heading_layout(heading->level, headingFontSize));
    storage.text.fontSize = headingFontSize;
    (void)ui_push_text(&storage.text);
    ui_pop();
    ui_pop();
}

static void eui_document_item_paragraph(EUIDocumentState& state, EUIDocumentItem& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_PARAGRAPH);

    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_paragraph_layout());
    storage.text.fontSize = state.fontSize;
    (void)ui_push_text(&storage.text);
    eui_document_spans(state, storage, &storage.text);
    ui_pop();
    ui_pop();
}

static void eui_document_item_code_block(EUIDocumentState& state, EUIDocumentItem& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_CODE_BLOCK);

    ui_push_panel(nullptr, state.uiTheme.get_field_color());
    ui_top_layout(state.docTheme.get_code_block_layout());
    storage.text.fontSize = state.fontSize;
    (void)ui_push_text(&storage.text);
    ui_pop();
    ui_pop();
}

static void eui_document_item_list_entry(EUIDocumentState& state, EUIDocumentItem& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_LIST_ENTRY);

    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_paragraph_layout());
    storage.text.fontSize = state.fontSize; // TODO: list item font size?
    (void)ui_push_text(&storage.text);
    ui_pop();
    ui_pop();
}

static void eui_document_spans(EUIDocumentState& state, EUIDocumentItem& item, UITextData* textS)
{
    TView<DocumentSpan*> spans = item.item->spans;
    Vector<UITextSpan>& uiSpans = textS->get_spans();

    LD_ASSERT(spans.size == uiSpans.size());

    for (int spanI = 0; spanI < (int)spans.size; spanI++)
    {
        const DocumentSpan* span = spans.data[spanI];

        if (span->type != DOCUMENT_SPAN_LINK)
            continue;

        Color spanTextColor = state.uiTheme.get_on_surface_color();
        if (ui_text_span_hovered(spanI))
        {
            spanTextColor = 0x20FFFFFF;
            eui_set_window_cursor(CURSOR_TYPE_HAND);
        }

        uiSpans[spanI].text.fgColor = spanTextColor;

        if (ui_text_span_pressed(spanI))
        {
            auto* link = (DocumentSpanLink*)span;
            state.eui->set_request_uri_path(document_uri_normalized_path(URI(link->href)));
        }
    }
}

void EUIDocument::push()
{
    LD_PROFILE_SCOPE;

    if (!mDocument)
        return;

    EUIDocumentState state{};
    state.eui = this;
    state.theme = eui_get_theme();
    state.docTheme = state.theme.get_document_theme();
    state.uiTheme = state.theme.get_ui_theme();
    state.fontSize = state.theme.get_font_size();

    UILayoutInfo layoutI = state.docTheme.get_scroll_layout();
    mScroll.bgColor = state.uiTheme.get_surface_color();
    mScroll.barColor = state.uiTheme.get_selection_color();
    mScroll.push(&layoutI);

    for (size_t i = 0; i < mItems.size(); i++)
    {
        state.itemIndex = i;

        EUIDocumentItem& itemS = mItems[i];
        sEUIDocumentMeta[(int)itemS.item->type].imgui_fn(state, itemS);
    }
}

void EUIDocument::pop()
{
    mScroll.pop();
}

bool EUIDocument::get_request_uri_path(String& outPath)
{
    if (!mRequestURIPath.empty())
    {
        outPath = mRequestURIPath;
        mRequestURIPath.clear(); // consume
        return true;
    }

    return false;
}

void EUIDocument::set_request_uri_path(const String& path)
{
    mRequestURIPath = path;
}

void EUIDocument::build(Document doc)
{
    LD_PROFILE_SCOPE;

    if (!doc)
        return;

    mDocument = doc;
    mRequestURIPath.clear();

    TView<DocumentItem*> itemV = mDocument.get_items();
    mItems.resize(itemV.size);

    for (size_t i = 0; i < itemV.size; i++)
    {
        DocumentItem* src = itemV.data[i];

        sEUIDocumentMeta[(int)src->type].build_fn(src, mItems[i]);
    }
}

} // namespace LD