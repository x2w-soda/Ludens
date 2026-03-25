#include <Ludens/Header/Assert.h>
#include <Ludens/Profiler/Profiler.h>
#include <Ludens/UI/UIImmediate.h>
#include <LudensEditor/EditorWidget/EUIDocument.h>

#include "EUI.h"

namespace LD {

/// @brief Document imgui state
struct EUIDocumentState
{
    EUIDocumentStorage* storage;
    size_t itemIndex;
    EditorTheme theme;
    EditorDocumentTheme docTheme;
    UITheme uiTheme;
    float fontSize;
};

struct EUIDocumentMeta
{
    void (*build_fn)(DocumentItem* src, EUIDocumentItemStorage& dst);
    void (*imgui_fn)(EUIDocumentState& state, EUIDocumentItemStorage& item);
};

static std::string document_span_to_text_span(TView<DocumentSpan*> srcSpans, Vector<TextSpan>& dstSpans);
static void build_document_item_heading(DocumentItem* src, EUIDocumentItemStorage& dst);
static void build_document_item_paragraph(DocumentItem* src, EUIDocumentItemStorage& dst);
static void build_document_item_code_block(DocumentItem* src, EUIDocumentItemStorage& dst);
static void build_document_item_list_entry(DocumentItem* src, EUIDocumentItemStorage& dst);
static void eui_document_item_heading(EUIDocumentState& state, EUIDocumentItemStorage& storage);
static void eui_document_item_paragraph(EUIDocumentState& state, EUIDocumentItemStorage& storage);
static void eui_document_item_code_block(EUIDocumentState& state, EUIDocumentItemStorage& storage);
static void eui_document_item_list_entry(EUIDocumentState& state, EUIDocumentItemStorage& storage);

// clang-format off
static EUIDocumentMeta sEUIDocumentMeta[] = {
    { &build_document_item_heading,    &eui_document_item_heading },
    { &build_document_item_paragraph,  &eui_document_item_paragraph },
    { &build_document_item_code_block, &eui_document_item_code_block },
    { &build_document_item_list_entry, &eui_document_item_list_entry },
};
// clang-format on

static_assert(sizeof(sEUIDocumentMeta) / sizeof(*sEUIDocumentMeta) == (int)DOCUMENT_ITEM_ENUM_COUNT);

// Adapter to convert DocumentSpan to TextSpan for rendering.
static std::string document_span_to_text_span(TView<DocumentSpan*> srcSpans, Vector<TextSpan>& dstSpans)
{
    std::string str;
    uint32_t offset = 0;

    dstSpans.resize(srcSpans.size);

    for (size_t i = 0; i < srcSpans.size; i++)
    {
        DocumentSpan* srcSpan = srcSpans.data[i];
        std::string segment = std::string(srcSpan->text.data, srcSpan->text.size);

        dstSpans[i] = {};
        dstSpans[i].fgColor = 0xFFFFFFFF;
        dstSpans[i].offset = offset;
        dstSpans[i].length = (uint32_t)segment.size();

        str += segment;
        offset += (uint32_t)segment.size();
    }

    return str;
}

static void build_document_item_heading(DocumentItem* src, EUIDocumentItemStorage& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_HEADING);

    EditorTheme theme = eui_get_theme();
    Vector<TextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.fontSize = theme.get_font_size();
    dst.text.fgColor = theme.get_ui_theme().get_on_surface_color();
}

static void build_document_item_paragraph(DocumentItem* src, EUIDocumentItemStorage& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_PARAGRAPH);

    EditorTheme theme = eui_get_theme();
    auto* paragraph = (DocumentItemParagraph*)src;

    Vector<TextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.fgColor = theme.get_ui_theme().get_on_surface_color();
}

static void build_document_item_code_block(DocumentItem* src, EUIDocumentItemStorage& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_CODE_BLOCK);

    EditorTheme theme = eui_get_theme();
    auto* block = (DocumentItemCodeBlock*)src;

    // TODO: more decoration for code blocks,
    //       display language, copy button, etc.
    (void)block;

    Vector<TextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.fgColor = theme.get_ui_theme().get_on_surface_color();
}

static void build_document_item_list_entry(DocumentItem* src, EUIDocumentItemStorage& dst)
{
    LD_ASSERT(src->type == DOCUMENT_ITEM_LIST_ENTRY);

    EditorTheme theme = eui_get_theme();

    // TODO:
    Vector<TextSpan> textSpans;
    std::string value = document_span_to_text_span(src->spans, textSpans);
    dst.item = src;
    dst.text.set_value(value, textSpans);
    dst.text.fgColor = theme.get_ui_theme().get_on_surface_color();
}

static void eui_document_item_heading(EUIDocumentState& state, EUIDocumentItemStorage& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_HEADING);

    float headingFontSize = state.fontSize;
    auto* heading = (DocumentItemHeading*)storage.item;
    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_heading_layout(heading->level, headingFontSize));
    UITextStorage* text = ui_push_text(&storage.text);
    text->fontSize = headingFontSize;
    ui_pop();
    ui_pop();
}

static void eui_document_item_paragraph(EUIDocumentState& state, EUIDocumentItemStorage& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_PARAGRAPH);

    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_paragraph_layout());
    UITextStorage* text = ui_push_text(&storage.text);
    text->fontSize = state.fontSize;
    ui_pop();
    ui_pop();
}

static void eui_document_item_code_block(EUIDocumentState& state, EUIDocumentItemStorage& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_CODE_BLOCK);

    ui_push_panel(nullptr, state.uiTheme.get_field_color());
    ui_top_layout(state.docTheme.get_code_block_layout());
    UITextStorage* text = ui_push_text(&storage.text);
    text->fontSize = state.fontSize;
    ui_pop();
    ui_pop();
}

static void eui_document_item_list_entry(EUIDocumentState& state, EUIDocumentItemStorage& storage)
{
    LD_ASSERT(storage.item->type == DOCUMENT_ITEM_LIST_ENTRY);

    ui_push_panel(nullptr);
    ui_top_layout(state.docTheme.get_paragraph_layout());
    UITextStorage* text = ui_push_text(&storage.text);
    text->fontSize = state.fontSize; // TODO: list item font size?
    ui_pop();
    ui_pop();
}

void eui_document(EUIDocumentStorage* storage)
{
    LD_PROFILE_SCOPE;

    if (!storage || !storage->document)
        return;

    EUIDocumentState state{};
    state.storage = storage;
    state.theme = eui_get_theme();
    state.docTheme = state.theme.get_document_theme();
    state.uiTheme = state.theme.get_ui_theme();
    state.fontSize = state.theme.get_font_size();

    EditorContext ctx = eui_get_context();
    UIScrollStorage* scroll = ui_push_scroll(&storage->scroll);
    scroll->bgColor = state.uiTheme.get_surface_color();

    UILayoutInfo layoutI = state.docTheme.get_scroll_layout();
    ui_top_layout(layoutI);

    for (size_t i = 0; i < storage->items.size(); i++)
    {
        state.itemIndex = i;

        EUIDocumentItemStorage& itemS = storage->items[i];
        sEUIDocumentMeta[(int)itemS.item->type].imgui_fn(state, itemS);
    }

    ui_pop();
}

void EUIDocumentStorage::build(Document doc)
{
    LD_PROFILE_SCOPE;

    if (!doc)
        return;

    document = doc;

    TView<DocumentItem*> itemV = document.get_items();
    items.resize(itemV.size);

    for (size_t i = 0; i < itemV.size; i++)
    {
        DocumentItem* src = itemV.data[i];

        sEUIDocumentMeta[(int)src->type].build_fn(src, items[i]);
    }
}

} // namespace LD