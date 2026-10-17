#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace storm_geometry_work {
using Clock = std::chrono::steady_clock;
inline constexpr double CpuFraction = .10;
inline constexpr double QuantumSeconds = .001;

// All cut workers share one allowance. Active wall time is a conservative
// upper bound on a single worker's CPU time, including allocator contention;
// time spent in our own sleep is excluded. Main-thread callers never sleep.
struct Allowance {
    std::mutex mutex;
    Clock::time_point refreshed = Clock::now();
    double tokens = 0.;
    const double rate = CpuFraction * std::max(1u, std::thread::hardware_concurrency());

    double charge(double seconds) {
        std::lock_guard<std::mutex> lock(mutex);
        const auto now = Clock::now();
        tokens = std::min(QuantumSeconds, tokens + rate * std::chrono::duration<double>(now - refreshed).count());
        refreshed = now;
        tokens -= std::max(0., seconds);
        return std::max(0., -tokens / rate);
    }
};
inline std::shared_ptr<Allowance> allowance() {
    static const auto shared = std::make_shared<Allowance>();
    return shared;
}
struct Cancelled : std::exception {
    const char *what() const noexcept override { return "Geometry work cancelled"; }
};
class Scope;
inline thread_local Scope *active = nullptr;

class Scope {
    std::shared_ptr<Allowance> shared_;
    std::shared_ptr<std::atomic<bool>> cancel_;
    Scope *previous_ = nullptr;
    Clock::time_point charged_ = Clock::now();
    unsigned ticks_ = 0;

    bool cancelled() const { return cancel_ && cancel_->load(std::memory_order_relaxed); }
    void settle() {
        const auto now = Clock::now();
        const double delay = shared_->charge(std::chrono::duration<double>(now - charged_).count());
        const auto until = now + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(delay));
        while (!cancelled()) {
            const auto remaining = until - Clock::now();
            if (remaining <= Clock::duration::zero()) break;
            std::this_thread::sleep_for(std::min(remaining,
                std::chrono::duration_cast<Clock::duration>(std::chrono::milliseconds(5))));
        }
        charged_ = Clock::now();
    }
public:
    explicit Scope(std::shared_ptr<Allowance> shared, std::shared_ptr<std::atomic<bool>> cancel = {})
        : shared_(std::move(shared)), cancel_(std::move(cancel)), previous_(active) {
        settle();
        active = this;
    }
    ~Scope() noexcept {
        // Keep the allowance alive until the last retiring worker has left.
        try { settle(); } catch (...) {}
        active = previous_;
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
    void checkpoint(bool force) {
        if (!force && (++ticks_ & 63u)) return;
        if (cancelled()) throw Cancelled{};
        if (force || std::chrono::duration<double>(Clock::now() - charged_).count() >= QuantumSeconds)
            settle();
        if (cancelled()) throw Cancelled{};
    }
};
inline void checkpoint(bool force = false) {
    if (active) active->checkpoint(force);
}
} // namespace storm_geometry_work
