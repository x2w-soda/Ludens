#include <Extra/doctest/doctest.h>
#include <Ludens/DSA/RectSplit.h>
#include <Ludens/Header/Math/Rect.h>

using namespace LD;

namespace {

struct TestNode
{
    TestNode* parent;
    TestNode* lch;
    TestNode* rch;
    uint32_t nodeID;
    bool isLeaf;
    float splitRatio;
    Axis splitAxis;
    Rect splitRect;
    Rect rect;
};

} // namespace

TEST_CASE("RectSplit bottom right")
{
    RectSplit<TestNode> partition(Rect(0.0f, 0.0f, 100.0f, 100.0f), 0.0f);

    const Rect area1(0.0f, 0.0f, 25.0f, 100.0f);
    const Rect area2(25.0f, 0.0f, 75.0f, 25.0f);
    const Rect area3(25.0f, 25.0f, 75.0f, 75.0f);

    uint32_t node1 = partition.get_root_id();
    CHECK(node1);

    uint32_t node2 = partition.split_right(node1, 0.25f);
    CHECK(node2);

    uint32_t node3 = partition.split_bottom(node2, 0.25f);
    CHECK(node3);

    partition.visit_leaves(partition.get_root_id(), [&](TestNode* leaf) {
        if (leaf->nodeID == node1)
            CHECK(leaf->rect == area1);
        else if (leaf->nodeID == node2)
            CHECK(leaf->rect == area2);
        else if (leaf->nodeID == node3)
            CHECK(leaf->rect == area3);
    });
}

TEST_CASE("RectSplit top left")
{
    RectSplit<TestNode> partition(Rect(0.0f, 0.0f, 100.0f, 100.0f), 0.0f);

    const Rect area1(0.0f, 25.0f, 100.0f, 75.0f);
    const Rect area2(25.0f, 0.0f, 75.0f, 25.0f);
    const Rect area3(0.0f, 0.0f, 25.0f, 25.0f);

    uint32_t node1 = partition.get_root_id();
    CHECK(node1);

    uint32_t node2 = partition.split_top(node1, 0.25f);
    CHECK(node2);

    uint32_t node3 = partition.split_left(node2, 0.25f);
    CHECK(node3);

    partition.visit_leaves(partition.get_root_id(), [&](TestNode* leaf) {
        if (leaf->nodeID == node1)
            CHECK(leaf->rect == area1);
        else if (leaf->nodeID == node2)
            CHECK(leaf->rect == area2);
        else if (leaf->nodeID == node3)
            CHECK(leaf->rect == area3);
    });
}