#pragma once

#include <string>
#include <doctest.h>
#include "Core/Serialize/Include/MD.h"

using namespace LD;

// The testing of the markdown parser is alreadly done by MD4C,
// Here we are just doing some light-weight testing to check if
// the markdown DOM is constructed correctly.

TEST_CASE("Test Paragraph Block")
{
    MDParserConfig config{};
    MDParser parser(config);
   
    const char* md = R"(
This is a paragraph. With
    a second line

This is another paragraph
)";

    Ref<MDDocument> doc = parser.ParseString(md, strlen(md));
    MDBlock* root = doc->GetBlock();
    CHECK(root);
    CHECK(root->GetType() == MDBlockType::DOC);

    MDBlock* blk = root->GetChild();
    CHECK(blk);
    CHECK(blk->GetType() == MDBlockType::P);

    MDSpan* span = blk->GetSpans();
    CHECK(span);
    CHECK(Equal(span->Content, "This is a paragraph. With"));
    span = span->Next;
    CHECK(span);
    CHECK(Equal(span->Content, "\n"));
    span = span->Next;
    CHECK(span);
    CHECK(Equal(span->Content, "a second line"));

    blk = blk->GetNext();
    CHECK(blk);
    CHECK(blk->GetType() == MDBlockType::P);

    span = blk->GetSpans();
    CHECK(span);
    CHECK(Equal(span->Content, "This is another paragraph"));
    CHECK(!span->Next);
}

TEST_CASE("Test Heading Blocks")
{
    MDParserConfig config{};
    MDParser parser(config);

    const char* md = R"(
# Heading 1
##  Heading 2

###    Heading three

#### h4
##### 5
###### heading_6
)";

    Ref<MDDocument> doc = parser.ParseString(md, strlen(md));
    MDBlock* root = doc->GetBlock();
    MDBlock* blk = root->GetChild();
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 1);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "Heading 1"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(blk);
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 2);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "Heading 2"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(blk);
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 3);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "Heading three"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(blk);
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 4);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "h4"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(blk);
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 5);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "5"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(blk);
    {
        MDBlockH* h = blk->ToH();
        CHECK(h);
        CHECK(h->Level == 6);

        MDSpan* span = blk->GetSpans();
        CHECK(span);
        CHECK(Equal(span->Content, "heading_6"));
        CHECK(!span->Next);
    }
    blk = blk->GetNext();
    CHECK(!blk);
}