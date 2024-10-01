#pragma once

#include "Core/RenderBase/Include/RBinding.h"

namespace LD
{

/// base class for first-party binding groups
class PrefabBindingGroup
{
public:
    PrefabBindingGroup() = default;
    PrefabBindingGroup(const PrefabBindingGroup&) = delete;
    virtual ~PrefabBindingGroup()
    {
        LD_DEBUG_ASSERT(!mHandle);
    }

    PrefabBindingGroup& operator=(const PrefabBindingGroup&) = delete;

	operator bool() const
	{
        return (bool)mHandle;
	}

	explicit operator RBindingGroup() const
	{
		LD_DEBUG_ASSERT(mHandle);
		return mHandle;
	}

	virtual RBindingGroupLayoutData GetLayoutData() const = 0;

	virtual RBindingGroupLayout CreateLayout(RDevice device) = 0;

protected:
	RBindingGroup mHandle;
};

} // namespace LD