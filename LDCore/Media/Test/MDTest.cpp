#include <Extra/doctest/doctest.h>
#include <Ludens/Media/Format/MD.h>
#include <cstdio>
#include <stack>
#include <vector>

using namespace LD;

struct MDExpect
{
    MDBlockType blockType;
    const char* cstr;
};

struct MDValidator
{
    static int on_enter_block(MDBlockType type, const MDBlockDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        CHECK(self.expectedIndex < self.expected.size()); // more blocks than expected
        const MDExpect& expected = self.expected[self.expectedIndex++];
        CHECK(expected.blockType == type);

        self.stack.push(expected);

        return 0;
    }

    static int on_leave_block(MDBlockType type, const MDBlockDetail& detail, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        const MDExpect& expected = self.stack.top();
        CHECK(expected.blockType == type);

        self.stack.pop();

        return 0;
    }

    static int on_text(MDTextType type, const MDString& text, void* user)
    {
        MDValidator& self = *(MDValidator*)user;

        const MDExpect& expected = self.stack.top();
        CHECK(text == expected.cstr);

        return 0;
    }

    int expectedIndex = 0;
    std::stack<MDExpect> stack;
    std::vector<MDExpect> expected;
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
        {MD_BLOCK_TYPE_DOC, nullptr},
        {MD_BLOCK_TYPE_H, "h1"},
        {MD_BLOCK_TYPE_H, "h2"},
        {MD_BLOCK_TYPE_H, "h3"},
        {MD_BLOCK_TYPE_H, "h4"},
        {MD_BLOCK_TYPE_H, "h5"},
        {MD_BLOCK_TYPE_H, "h6"},
    };

    std::string err;
    MDEventCallback cb;
    cb.onEnterBlock = &MDValidator::on_enter_block;
    cb.onLeaveBlock = &MDValidator::on_leave_block;
    cb.onText = &MDValidator::on_text;
    bool ok = MDEventParser::parse(View(md, sizeof(md) - 1), err, cb, &validator);
    CHECK(ok);
}