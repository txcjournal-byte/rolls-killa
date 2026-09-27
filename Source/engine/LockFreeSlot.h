#pragma once

#include <algorithm>
#include <atomic>
#include <memory>
#include <vector>

namespace rk
{

/**
    Hands an immutable object from the message thread to the audio thread without locks or
    allocations on the audio side (single reader, hazard-pointer style).

    Audio thread:   auto* p = slot.acquire();  ...use p...  slot.release();
    Message thread: slot.publish (std::make_unique<T> (...));

    Retired objects are deleted on the message thread once the audio thread no longer uses them.
*/
template <typename T>
class LockFreeSlot
{
public:
    LockFreeSlot() = default;
    ~LockFreeSlot()
    {
        delete current.exchange (nullptr);
        retired.clear();
    }

    // ---- audio thread -------------------------------------------------------
    const T* acquire() noexcept
    {
        T* p = current.load (std::memory_order_seq_cst);
        for (;;)
        {
            hazard.store (p, std::memory_order_seq_cst);
            T* again = current.load (std::memory_order_seq_cst);
            if (again == p)
                return p;
            p = again;
        }
    }

    void release() noexcept { hazard.store (nullptr, std::memory_order_seq_cst); }

    // ---- message thread -----------------------------------------------------
    void publish (std::unique_ptr<T> next)
    {
        T* old = current.exchange (next.release(), std::memory_order_seq_cst);
        if (old != nullptr)
            retired.emplace_back (old);
        collectGarbage();
    }

    void collectGarbage()
    {
        const T* inUse = hazard.load (std::memory_order_seq_cst);
        retired.erase (std::remove_if (retired.begin(), retired.end(),
                                       [inUse] (const std::unique_ptr<T>& r) { return r.get() != inUse; }),
                       retired.end());
    }

    /** Message-thread read access to the latest published object. */
    const T* latest() const noexcept { return current.load(); }

private:
    std::atomic<T*> current { nullptr };
    std::atomic<const T*> hazard { nullptr };
    std::vector<std::unique_ptr<T>> retired;
};

} // namespace rk
