#pragma once

#include "Core/OS/Include/Memory.h"
#include "Core/DSA/Include/View.h"

namespace LD
{

template <typename TVertex, typename TIndex>
class RBatch
{
public:
    /// @brief startup the vertex batching service
    void Startup(int vertexPerElement, View<int> indexSequence, int elementCapacity)
    {
        mVertexPerElement = vertexPerElement;
        mIndexPerElement = (int)indexSequence.Size();
        mElementCapacity = elementCapacity;
        mElementCtr = 0;

        mVertices = (TVertex*)MemoryAlloc(sizeof(TVertex) * mVertexPerElement * mElementCapacity);
        mIndices = (TIndex*)MemoryAlloc(sizeof(TIndex) * mIndexPerElement * mElementCapacity);

        // initialize index buffer data
        for (int element = 0; element < mElementCapacity; element++)
            for (int index = 0; index < mIndexPerElement; index++)
                mIndices[mIndexPerElement * element + index] =
                    static_cast<TIndex>(mVertexPerElement * element + indexSequence[index]);
    }

    /// @brief cleanup the vertex batching service.
    void Cleanup()
    {
        MemoryFree(mVertices);
        MemoryFree(mIndices);

        mVertices = nullptr;
        mIndices = nullptr;
    }

    int GetVertexCount() const
    {
        return mVertexPerElement * mElementCtr;
    }

    int GetIndexCount() const
    {
        return mIndexPerElement * mElementCtr;
    }

    int GetElementCount() const
    {
        return mElementCtr;
    }

    int GetElementCapacity() const
    {
        return mElementCapacity;
    }

    size_t GetVertexBufferSize() const
    {
        return sizeof(TVertex) * mVertexPerElement * mElementCapacity;
    }

    size_t GetIndexBufferSize() const
    {
        return sizeof(TIndex) * mIndexPerElement * mElementCapacity;
    }

    const TIndex* GetIndices() const
    {
        return (const TIndex*)mIndices;
    }

    const TVertex* GetVertices() const
    {
        return (const TVertex*)mVertices;
    }

    bool AddElement(const TVertex* src)
    {
        if (mElementCtr == mElementCapacity)
            return false;

        TVertex* dst = mVertices + mVertexPerElement * mElementCtr++;

        for (int i = 0; i < mVertexPerElement; i++)
            dst[i] = src[i];

        return true;
    }

    void Reset()
    {
        mElementCtr = 0;
    }

private:
    TVertex* mVertices;
    TIndex* mIndices;
    int mVertexPerElement;
    int mIndexPerElement;
    int mElementCapacity;
    int mElementCtr;
};

} // namespace LD