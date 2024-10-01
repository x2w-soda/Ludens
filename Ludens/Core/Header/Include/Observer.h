#pragma once

#include <unordered_set>
#include "Core/Header/Include/Hash.h"

namespace LD
{

template <typename TEvent>
class Observable;

//
// The most generic implementation of the Observer pattern, allowing many-to-many relationship
// between the observers and observed objects. The TEvent struct could be designed to contain
// private states of the Observable object without breaking encapsulation.
// Note that both Observer and Observable objects can not be copied or moved since they
// contain raw pointers of the opposite end.
//

template <typename TEvent>
class Observer
{
    using TObservable = Observable<TEvent>;
    friend TObservable;

public:
    Observer() = default;
    Observer(const Observer&) = delete;
    Observer(Observer&&) = delete;

    virtual ~Observer()
    {
        for (Observable<TEvent>* target : mTargets)
        {
            LD_DEBUG_ASSERT(target->mObservers.find(this) != target->mObservers.end());
            
            target->mObservers.erase(this);
        }
    }

    Observer& operator=(const Observer&) = delete;
    Observer& operator=(Observer&&) = delete;

protected:

    virtual void OnObserverNotify(Observable<TEvent>* observable, const TEvent& event) = 0;

private:

    std::unordered_set<TObservable*, PtrHash<TObservable>, PtrEqual<TObservable>> mTargets;
};

template <typename TEvent>
class Observable
{
    using TObserver = Observer<TEvent>;
    friend TObserver;

public:
    Observable() = default;
    Observable(const Observable&) = delete;
    Observable(Observable&&) = delete;

    ~Observable()
    {
        for (TObserver* observer : mObservers)
        {
            LD_DEBUG_ASSERT(observer->mTargets.find(this) != observer->mTargets.end());

            observer->mTargets.erase(this);
        }
    }

    Observable& operator=(const Observable&) = delete;
    Observable& operator=(Observable&&) = delete;

    void NotifyObservers(const TEvent& event)
    {
        for (TObserver* observer : mObservers)
        {
            observer->OnObserverNotify(this, event);
        }
    }

    void AddObserver(TObserver* observer)
    {
        LD_DEBUG_ASSERT(observer && mObservers.find(observer) == mObservers.end());

        // doubly linked
        mObservers.insert(observer);
        observer->mTargets.insert(this);
    }

private:

    std::unordered_set<TObserver*, PtrHash<TObserver>, PtrEqual<TObserver>> mObservers;
};

} // namespace LD