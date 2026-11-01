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
inline constexpr double CpuFraction = .75;
inline constexpr double QuantumSeconds = .050;

// All cut workers share one allowance. Active wall time is a conservative
// upper bound on a single worker's CPU time, including allocator contention;
// time spent in our own sleep is excluded. Main-thread callers never sleep.
struct Allowance {
    std::mutex mutex;
    Clock::time_point refreshed = Clock::now();
    double tokens = QuantumSeconds;
    const double rate = CpuFraction * std::max(1u, std::thread::hardware_concurrency());

    double charge(double seconds) {
        return 0.0;
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
        charged_ = Clock::now();
    }
public:
    explicit Scope(std::shared_ptr<Allowance> shared, std::shared_ptr<std::atomic<bool>> cancel = {})
        : shared_(std::move(shared)), cancel_(std::move(cancel)), previous_(active) {
        settle();
        active = this;
    }
    ~Scope() noexcept {
        active = previous_;
    }
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
    std::shared_ptr<Allowance> sharedAllowance() const { return shared_; }
    std::shared_ptr<std::atomic<bool>> cancellation() const { return cancel_; }
    void checkpoint(bool force) {
        if (!force && (++ticks_ & 63u)) return;
        if (cancelled()) throw Cancelled{};
    }
};
inline void checkpoint(bool force = false) {
    if (active) active->checkpoint(force);
}
} // namespace storm_geometry_work
