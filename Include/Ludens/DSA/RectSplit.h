#pragma once

#include <Ludens/DSA/Vector.h>
#include <Ludens/Header/Directional.h>
#include <Ludens/Header/Hash.h>
#include <Ludens/Header/Math/Rect.h>
#include <Ludens/System/Memory.h>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <type_traits>

namespace LD {

// clang-format off
template <typename T>
concept RectSplitNode = requires(T node)
{
    { node.nodeID } -> std::convertible_to<uint32_t>;
    { node.splitRatio } -> std::convertible_to<float>;
    { node.splitAxis } -> std::same_as<Axis&>;
    { node.splitRect } -> std::same_as<Rect&>;
    { node.isLeaf } -> std::convertible_to<bool>;
    { node.rect } -> std::same_as<Rect&>;
    { node.parent } -> std::same_as<T*&>;
    { node.lch } -> std::same_as<T*&>;
    { node.rch } -> std::same_as<T*&>;
};
// clang-format on

template <RectSplitNode TNode, MemoryUsage TMemoryUsage = MEMORY_USAGE_MISC>
class RectSplit
{
public:
    using ID = uint32_t;

    RectSplit() = delete;
    RectSplit(const Rect& rootRect, float gap)
        : mSplitGap(gap)
    {
        mRoot = heap_new<TNode>(TMemoryUsage);
        mRoot->isLeaf = true;
        mRoot->rect = rootRect;
        mRoot->parent = nullptr;
        mRoot->lch = nullptr;
        mRoot->rch = nullptr;
        mRoot->nodeID = get_id();

        mNodes.push_back(mRoot);
    }

    RectSplit(const RectSplit&) = delete;
    RectSplit(RectSplit&&) = delete;

    ~RectSplit()
    {
        for (TNode* node : mNodes)
            heap_delete<TNode>(node);
    }

    RectSplit& operator=(const RectSplit&) = delete;
    RectSplit& operator=(RectSplit&&) = delete;

    /// @brief Configure total root area, invalidates each node area recursively.
    void set_root_rect(const Rect& rootRect)
    {
        mRoot->rect = rootRect;
        invalidate(mRoot);
    }

    /// @brief Configure root area position, invalidates each node area recursively.
    void set_root_pos(const Vec2& rootPos)
    {
        mRoot->rect.x = rootPos.x;
        mRoot->rect.y = rootPos.y;
        invalidate(mRoot);
    }

    /// @brief Configure split ratio of a non-leaf node, invalidates subtree area recursively.
    void set_split_ratio(ID nodeID, float ratio)
    {
        TNode* node = get_node(nodeID);
        if (!node || (!node->lch && !node->rch))
            return;

        node->splitRatio = std::clamp(ratio, 0.0f, 1.0f);
        invalidate(node);
    }

    ID get_root_id()
    {
        return mRoot->nodeID;
    }

    /// @brief Split a leaf node to make room for left area.
    ID split_left(ID nodeID, float ratio)
    {
        return split(get_node(nodeID), AXIS_Y, false, ratio);
    }

    /// @brief Split a leaf node to make room for right area.
    ID split_right(ID nodeID, float ratio)
    {
        return split(get_node(nodeID), AXIS_Y, true, ratio);
    }

    /// @brief Split a leaf node to make room for top area.
    ID split_top(ID nodeID, float ratio)
    {
        return split(get_node(nodeID), AXIS_X, false, ratio);
    }

    /// @brief Split a leaf node to make room for bottom area.
    ID split_bottom(ID nodeID, float ratio)
    {
        return split(get_node(nodeID), AXIS_X, true, ratio);
    }

    /// @brief Visit all nodes in subtree.
    void visit_nodes(ID nodeID, const std::function<void(TNode*)>& onNode)
    {
        visit_node(get_node(nodeID), onNode);
    }

    /// @brief Visit leaves in subtree.
    void visit_leaves(ID nodeID, const std::function<void(TNode*)>& onLeaf)
    {
        visit_leaf(get_node(nodeID), onLeaf);
    }

    TNode* get_node(ID nodeID)
    {
        for (TNode* node : mNodes)
        {
            if (node->nodeID == nodeID)
                return node;
        }

        return nullptr;
    }

private:
    ID split(TNode* target, Axis splitAxis, bool rotateLeft, float splitRatio)
    {
        if (!target || !target->isLeaf)
            return 0;

        splitRatio = std::clamp(splitRatio, 0.0f, 1.0f);
        TNode* parent = target->parent;
        Rect tlArea, brArea, splitArea;

        if (splitAxis == AXIS_X)
            Rect::split_h(splitRatio, mSplitGap, target->rect, tlArea, brArea, splitArea);
        else
            Rect::split_v(splitRatio, mSplitGap, target->rect, tlArea, brArea, splitArea);

        TNode* split = heap_new<TNode>(TMemoryUsage);
        TNode* lch = nullptr;
        TNode* rch = nullptr;

        mNodes.push_back(split);

        if (rotateLeft) // split target becomes lch
        {
            lch = target;
            rch = heap_new<TNode>(TMemoryUsage);
            rch->nodeID = get_id();
            mNodes.push_back(rch);
        }
        else // split target becomes rch
        {
            lch = heap_new<TNode>(TMemoryUsage);
            lch->nodeID = get_id();
            rch = target;
            mNodes.push_back(lch);
        }

        split->nodeID = get_id();
        split->parent = parent;
        split->isLeaf = false;
        split->splitRatio = splitRatio;
        split->splitAxis = splitAxis;
        split->splitRect = splitArea;
        split->rect = target->rect;
        split->lch = lch;
        split->rch = rch;

        if (parent)
        {
            const bool wasLch = target == parent->lch;
            if (wasLch)
                parent->lch = split;
            else
                parent->rch = split;
        }
        else
            mRoot = split;

        lch->rect = tlArea;
        lch->parent = split;
        lch->isLeaf = !lch->lch && !lch->rch;

        rch->rect = brArea;
        rch->parent = split;
        rch->isLeaf = !rch->lch && !rch->rch;

        return rotateLeft ? rch->nodeID : lch->nodeID;
    }

    ID get_id()
    {
        if (mIDCounter == 0)
            mIDCounter = 1;

        return mIDCounter++;
    }

    void visit_node(TNode* node, const std::function<void(TNode*)>& onNode)
    {
        if (!node)
            return;

        onNode(node);

        if (node->lch)
            visit_node(node->lch, onNode);

        if (node->rch)
            visit_node(node->rch, onNode);
    }

    void visit_leaf(TNode* node, const std::function<void(TNode*)>& onLeaf)
    {
        if (!node)
            return;

        if (!node->lch && !node->rch)
        {
            onLeaf(node);
            return;
        }

        if (node->lch)
            visit_leaf(node->lch, onLeaf);

        if (node->rch)
            visit_leaf(node->rch, onLeaf);
    }

    /// @brief Invalidate subtree given root area.
    void invalidate(TNode* root)
    {
        if (!root || (!root->lch && !root->rch))
            return;

        Rect tlArea, brArea, splitArea;

        if (root->splitAxis == AXIS_X)
            Rect::split_h(root->splitRatio, mSplitGap, root->rect, tlArea, brArea, splitArea);
        else
            Rect::split_v(root->splitRatio, mSplitGap, root->rect, tlArea, brArea, splitArea);

        root->splitRect = splitArea;
        root->lch->rect = tlArea;
        root->rch->rect = brArea;

        invalidate(root->lch);
        invalidate(root->rch);
    }

private:
    Vector<TNode*> mNodes;
    TNode* mRoot = nullptr;
    ID mIDCounter = 0;
    float mSplitGap = 0.0f;
};

} // namespace LD