#pragma once

#include "RefBase.h"
#include <windows.h>


#define  SWAP_LOCK_COUNT  32U
static int32_t  _swap_locks[SWAP_LOCK_COUNT];

#define  SWAP_LOCK(addr)   \
   &_swap_locks[((unsigned)(void*)(addr) >> 3U) % SWAP_LOCK_COUNT]

#define android_atomic_cmpxchg android_atomic_release_cas

int32_t android_atomic_release_cas(int32_t oldValue, int32_t newvalue, volatile int32_t* addr) {
    int result;

    if (*addr == oldValue) {
        *addr = newvalue;
        result = 0;
    }
    else {
        result = 1;
    }
    return result;
}

int32_t android_atomic_acquire_load(volatile const int32_t* addr)
{
    return *addr;
}

int32_t android_atomic_release_load(volatile const int32_t* addr)
{
    return *addr;
}

void android_atomic_acquire_store(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, value, addr));
}

void android_atomic_release_store(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, value, addr));
}

int32_t android_atomic_inc(volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, oldValue + 1, addr));
    return oldValue;
}

int32_t android_atomic_dec(volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, oldValue - 1, addr));
    return oldValue;
}

int32_t android_atomic_add(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, oldValue + value, addr));
    return oldValue;
}

int32_t android_atomic_and(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, oldValue & value, addr));
    return oldValue;
}

int32_t android_atomic_or(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_release_cas(oldValue, oldValue | value, addr));
    return oldValue;
}

int32_t android_atomic_release_swap(int32_t value, volatile int32_t* addr) {
    int32_t oldValue;
    do {
        oldValue = *addr;
    } while (android_atomic_cmpxchg(oldValue, value, addr));
    return oldValue;
}

int32_t android_atomic_acquire_swap(int32_t value, volatile int32_t* addr) {
    return android_atomic_release_swap(value, addr);
}

int android_atomic_release_cmpxchg(int32_t oldvalue, int32_t newvalue,
    volatile int32_t* addr) {
    int result;

    if (*addr == oldvalue) {
        *addr = newvalue;
        result = 0;
    }
    else {
        result = 1;
    }
    return result;
}

int android_atomic_acquire_cmpxchg(int32_t oldvalue, int32_t newvalue,
    volatile int32_t* addr) {
    return android_atomic_release_cmpxchg(oldvalue, newvalue, addr);
}



namespace android {

#define INITIAL_STRONG_VALUE (1<<28)

    // ---------------------------------------------------------------------------

    class RefBase::weakref_impl : public RefBase::weakref_type
    {
    public:
        volatile int32_t    mStrong;
        volatile int32_t    mWeak;
        RefBase* const      mBase;
        volatile int32_t    mFlags;

        weakref_impl(RefBase* base)
            : mStrong(INITIAL_STRONG_VALUE)
            , mWeak(0)
            , mBase(base)
            , mFlags(0)
        {
        }
    };

    // ---------------------------------------------------------------------------

    void RefBase::incStrong(const void* id) const
    {
        weakref_impl* const refs = mRefs;
        refs->incWeak(id);

        const int32_t c = android_atomic_inc(&refs->mStrong);

        if (c != INITIAL_STRONG_VALUE) {
            return;
        }

        android_atomic_add(-INITIAL_STRONG_VALUE, &refs->mStrong);
        const_cast<RefBase*>(this)->onFirstRef();
    }

    void RefBase::decStrong(const void* id) const
    {
        weakref_impl* const refs = mRefs;
        const int32_t c = android_atomic_dec(&refs->mStrong);

        if (c == 1) {
            const_cast<RefBase*>(this)->onLastStrongRef(id);
            if ((refs->mFlags & OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK) {
                delete this;
            }
        }
        refs->decWeak(id);
    }

    void RefBase::forceIncStrong(const void* id) const
    {
        weakref_impl* const refs = mRefs;
        refs->incWeak(id);

        const int32_t c = android_atomic_inc(&refs->mStrong);


        switch (c) {
        case INITIAL_STRONG_VALUE:
            android_atomic_add(-INITIAL_STRONG_VALUE, &refs->mStrong);
            // fall through...
        case 0:
            const_cast<RefBase*>(this)->onFirstRef();
        }
    }

    int32_t RefBase::getStrongCount() const
    {
        return mRefs->mStrong;
    }



    RefBase* RefBase::weakref_type::refBase() const
    {
        return static_cast<const weakref_impl*>(this)->mBase;
    }

    void RefBase::weakref_type::incWeak(const void* id)
    {
        weakref_impl* const impl = static_cast<weakref_impl*>(this);
        const int32_t c = android_atomic_inc(&impl->mWeak);
    }

    void RefBase::weakref_type::decWeak(const void* id)
    {
        weakref_impl* const impl = static_cast<weakref_impl*>(this);
        const int32_t c = android_atomic_dec(&impl->mWeak);
        if (c != 1) return;

        if ((impl->mFlags & OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK) {
            if (impl->mStrong == INITIAL_STRONG_VALUE)
                delete impl->mBase;
            else {
                //            LOGV("Freeing refs %p of old RefBase %p\n", this, impl->mBase);
                delete impl;
            }
        }
        else {
            impl->mBase->onLastWeakRef(id);
            if ((impl->mFlags & OBJECT_LIFETIME_FOREVER) != OBJECT_LIFETIME_FOREVER) {
                delete impl->mBase;
            }
        }
    }

    bool RefBase::weakref_type::attemptIncStrong(const void* id)
    {
        incWeak(id);

        weakref_impl* const impl = static_cast<weakref_impl*>(this);

        int32_t curCount = impl->mStrong;
        while (curCount > 0 && curCount != INITIAL_STRONG_VALUE) {
            if (android_atomic_cmpxchg(curCount, curCount + 1, &impl->mStrong) == 0) {
                break;
            }
            curCount = impl->mStrong;
        }

        if (curCount <= 0 || curCount == INITIAL_STRONG_VALUE) {
            bool allow;
            if (curCount == INITIAL_STRONG_VALUE) {
                // Attempting to acquire first strong reference...  this is allowed
                // if the object does NOT have a longer lifetime (meaning the
                // implementation doesn't need to see this), or if the implementation
                // allows it to happen.
                allow = (impl->mFlags & OBJECT_LIFETIME_WEAK) != OBJECT_LIFETIME_WEAK
                    || impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id);
            }
            else {
                // Attempting to revive the object...  this is allowed
                // if the object DOES have a longer lifetime (so we can safely
                // call the object with only a weak ref) and the implementation
                // allows it to happen.
                allow = (impl->mFlags & OBJECT_LIFETIME_WEAK) == OBJECT_LIFETIME_WEAK
                    && impl->mBase->onIncStrongAttempted(FIRST_INC_STRONG, id);
            }
            if (!allow) {
                decWeak(id);
                return false;
            }
            curCount = android_atomic_inc(&impl->mStrong);

            // If the strong reference count has already been incremented by
            // someone else, the implementor of onIncStrongAttempted() is holding
            // an unneeded reference.  So call onLastStrongRef() here to remove it.
            // (No, this is not pretty.)  Note that we MUST NOT do this if we
            // are in fact acquiring the first reference.
            if (curCount > 0 && curCount < INITIAL_STRONG_VALUE) {
                impl->mBase->onLastStrongRef(id);
            }
        }




        if (curCount == INITIAL_STRONG_VALUE) {
            android_atomic_add(-INITIAL_STRONG_VALUE, &impl->mStrong);
            impl->mBase->onFirstRef();
        }

        return true;
    }

    bool RefBase::weakref_type::attemptIncWeak(const void* id)
    {
        weakref_impl* const impl = static_cast<weakref_impl*>(this);

        int32_t curCount = impl->mWeak;

        while (curCount > 0) {
            if (android_atomic_cmpxchg(curCount, curCount + 1, &impl->mWeak) == 0) {
                break;
            }
            curCount = impl->mWeak;
        }

        if (curCount > 0) {
        }

        return curCount > 0;
    }

    int32_t RefBase::weakref_type::getWeakCount() const
    {
        return static_cast<const weakref_impl*>(this)->mWeak;
    }

    RefBase::weakref_type* RefBase::createWeak(const void* id) const
    {
        mRefs->incWeak(id);
        return mRefs;
    }

    RefBase::weakref_type* RefBase::getWeakRefs() const
    {
        return mRefs;
    }

    RefBase::RefBase()
        : mRefs(new weakref_impl(this))
    {
        //    LOGV("Creating refs %p with RefBase %p\n", mRefs, this);
    }

    RefBase::~RefBase()
    {
        //    LOGV("Destroying RefBase %p (refs %p)\n", this, mRefs);
        if (mRefs->mWeak == 0) {
            //        LOGV("Freeing refs %p of old RefBase %p\n", mRefs, this);
            delete mRefs;
        }
    }

    void RefBase::extendObjectLifetime(int32_t mode)
    {
        android_atomic_or(mode, &mRefs->mFlags);
    }

    void RefBase::onFirstRef()
    {
    }

    void RefBase::onLastStrongRef(const void* /*id*/)
    {
    }

    bool RefBase::onIncStrongAttempted(uint32_t flags, const void* id)
    {
        return (flags & FIRST_INC_STRONG) ? true : false;
    }

    void RefBase::onLastWeakRef(const void* /*id*/)
    {
    }

}; // namespace android
