#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/Stack.h>
#include <Ludens/DSA/Vector.h>
#include <Ludens/Media/Format/MD.h>

#include <cstdio>

using namespace LD;

enum MDExpectType
{
    MD_EXPECT_BLOCK,
    MD_EXPECT_SPAN,
    MD_EXPECT_TEXT,
};

struct MDExpect
{
    MDExpectType type;
    union
    {
        MDBlockType blockType;
        MDSpanType spanType;
        MDTextType textType;
    };
    const char* cstr;
};

struct MDValidator
{
    static int on_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        CHECK(self.expectedIndex < self.expected.size()); // more blocks than expected
        const MDExpect& expected = self.expected[self.expectedIndex++];

        REQUIRE(expected.type == MD_EXPECT_BLOCK);
        CHECK(expected.blockType == type);

        self.stack.push(expected);

        return 0;
    }

    static int on_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        const MDExpect& expected = self.stack.top();
        REQUIRE(expected.type == MD_EXPECT_BLOCK);
        CHECK(expected.blockType == type);

        self.stack.pop();

        return 0;
    }

    static int on_enter_span(MDSpanType type, const MDSpanDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        CHECK(self.expectedIndex < self.expected.size());
        const MDExpect& expected = self.expected[self.expectedIndex++];

        REQUIRE(expected.type == MD_EXPECT_SPAN);
        CHECK(expected.spanType == type);

        self.stack.push(expected);

        return 0;
    }

    static int on_leave_span(MDSpanType type, const MDSpanDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        const MDExpect& expected = self.stack.top();
        REQUIRE(expected.type == MD_EXPECT_SPAN);
        CHECK(expected.spanType == type);

        self.stack.pop();

        return 0;
    }

    static int on_text(MDTextType type, View text, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        CHECK(self.expectedIndex < self.expected.size());
        const MDExpect& expected = self.expected[self.expectedIndex++];
        REQUIRE(expected.type == MD_EXPECT_TEXT);
        CHECK(text == expected.cstr);

        return 0;
    }

    int expectedIndex = 0;
    Stack<MDExpect> stack;
    Vector<MDExpect> expected;
};

TEST_CASE("MD blocks")
{
    const char md[] = R"(
# h1
## h2
### h3
#### h4
##### h5
###### h6
)";
    MDValidator validator;
    validator.expected = {
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_DOC, .cstr = nullptr},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h1"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h2"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h3"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h4"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h5"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_H, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "h6"},
    };

    String err;
    MDCallback cb;
    cb.onEnterBlock = &MDValidator::on_enter_block;
    cb.onLeaveBlock = &MDValidator::on_leave_block;
    cb.onEnterSpan = &MDValidator::on_enter_span;
    cb.onLeaveSpan = &MDValidator::on_leave_span;
    cb.onText = &MDValidator::on_text;
    bool ok = MDParser::parse(View(md, sizeof(md) - 1), err, cb, &validator);
    CHECK(ok);
}

TEST_CASE("MD spans")
{
    const char md[] = R"(
paragraph with `code`

![alt text](/path/to/image "optional text")
)";
    MDValidator validator;
    validator.expected = {
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_DOC, .cstr = nullptr},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_P, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "paragraph with "},
        {.type = MD_EXPECT_SPAN, .spanType = MD_SPAN_TYPE_CODE, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "code"},
        {.type = MD_EXPECT_BLOCK, .blockType = MD_BLOCK_TYPE_P, .cstr = nullptr},
        {.type = MD_EXPECT_SPAN, .spanType = MD_SPAN_TYPE_IMG, .cstr = nullptr},
        {.type = MD_EXPECT_TEXT, .textType = MD_TEXT_TYPE_NORMAL, .cstr = "alt text"},
    };

    String err;
    MDCallback cb{};
    cb.onEnterBlock = &MDValidator::on_enter_block;
    cb.onLeaveBlock = &MDValidator::on_leave_block;
    cb.onEnterSpan = &MDValidator::on_enter_span;
    cb.onLeaveSpan = &MDValidator::on_leave_span;
    cb.onText = &MDValidator::on_text;
    bool ok = MDParser::parse(View(md, sizeof(md) - 1), err, cb, &validator);
    CHECK(ok);
}