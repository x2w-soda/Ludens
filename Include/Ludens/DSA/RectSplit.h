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
    { node.ratio } -> std::convertible_to<float>;
    { node.isLeaf } -> std::convertible_to<bool>;
    { node.area } -> std::same_as<Rect&>;
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
    RectSplit(const Rect& rootArea)
    {
        mRoot = heap_new<TNode>(TMemoryUsage);
        mRoot->isLeaf = true;
        mRoot->area = rootArea;
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

    ID get_root_id()
    {
        return mRoot->nodeID;
    }

    /// @brief Split a leaf node to make room for left area.
    ID split_left(ID nodeID, float ratio, float gap)
    {
        return split(get_node(nodeID), AXIS_Y, false, ratio, gap);
    }

    /// @brief Split a leaf node to make room for right area.
    ID split_right(ID nodeID, float ratio, float gap)
    {
        return split(get_node(nodeID), AXIS_Y, true, ratio, gap);
    }

    /// @brief Split a leaf node to make room for top area.
    ID split_top(ID nodeID, float ratio, float gap)
    {
        return split(get_node(nodeID), AXIS_X, false, ratio, gap);
    }

    /// @brief Split a leaf node to make room for bottom area.
    ID split_bottom(ID nodeID, float ratio, float gap)
    {
        return split(get_node(nodeID), AXIS_X, true, ratio, gap);
    }

    void visit(ID nodeID, const std::function<void(TNode*)>& onLeaf)
    {
        TNode* subtree = get_node(nodeID);

        if (!subtree)
            return;

        visit_node(subtree, onLeaf);
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
    ID split(TNode* target, Axis splitAxis, bool rotateLeft, float ratio, float gap)
    {
        if (!target || !target->isLeaf)
            return 0;

        ratio = std::clamp(ratio, 0.0f, 1.0f);
        TNode* parent = target->parent;
        Rect tlArea, brArea, splitArea;

        if (splitAxis == AXIS_X)
            Rect::split_h(ratio, gap, target->area, tlArea, brArea, splitArea);
        else
            Rect::split_v(ratio, gap, target->area, tlArea, brArea, splitArea);

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
        split->ratio = ratio;
        split->area = splitArea;
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

        lch->area = tlArea;
        lch->parent = split;
        lch->isLeaf = !lch->lch && !lch->rch;

        rch->area = brArea;
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

    void visit_node(TNode* node, const std::function<void(TNode*)>& onLeaf)
    {
        if (!node)
            return;

        if (node->lch)
            visit_node(node->lch, onLeaf);

        if (node->rch)
            visit_node(node->rch, onLeaf);

        if (!node->lch && !node->rch)
            onLeaf(node);
    }

private:
    Vector<TNode*> mNodes;
    TNode* mRoot = nullptr;
    ID mIDCounter = 0;
};

} // namespace LD