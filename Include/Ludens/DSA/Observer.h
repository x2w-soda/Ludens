#pragma once

#include <Ludens/DSA/Vector.h>
#include <algorithm>
#include <utility>

namespace LD {

/// @brief A list of observer function callbacks. Not thread safe.
template <typename... TArgs>
class ObserverList
{
public:
    typedef void (*ObserverFn)(TArgs..., void*);

    void add_observer(ObserverFn fn, void* user)
    {
        mList.emplace_back(fn, user);
    }

    void remove_observer(ObserverFn fn, void* user)
    {
        Entry entry(fn, user);
        auto it = std::find(mList.begin(), mList.end(), entry);

        if (it != mList.end())
            mList.erase(it);
    }

    ///@brief Invokes all observer functions with payload.
    ///@warning Do not add or remove observers during callbacks.
    void notify(TArgs... args)
    {
        for (const Entry& entry : mList)
        {
            // NOTE: Observers should take parameters by value or const reference,
            //       otherwise a fn may end up observing mutated state by previous fn.
            entry.fn(std::forward<TArgs>(args)..., entry.user);
        }
    }

private:
    struct Entry
    {
        ObserverFn fn;
        void* user;

        Entry() = delete;
        Entry(ObserverFn fn, void* user)
            : fn(fn), user(user) {}

        inline bool operator==(const Entry& other) const
        {
            return fn == other.fn && user == other.user;
        }
    };

    Vector<Entry> mList;
};

} // namespace LD