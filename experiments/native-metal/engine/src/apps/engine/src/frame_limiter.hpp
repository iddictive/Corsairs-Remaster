#pragma once

#include <chrono>
#include <cstdint>

namespace storm::frame
{

class Limiter
{
  public:
    using Clock = std::chrono::steady_clock;
    using Duration = Clock::duration;
    using TimePoint = Clock::time_point;

    explicit Limiter(uint32_t maximumFps)
        : interval_(maximumFps == 0
                        ? Duration::zero()
                        : std::chrono::duration_cast<Duration>(
                              std::chrono::duration<double>(1.0 / static_cast<double>(maximumFps))))
    {
    }

    Duration Interval() const { return interval_; }

    TimePoint NextFrame(TimePoint now)
    {
        if (interval_ == Duration::zero())
            return now;

        if (!armed_)
        {
            armed_ = true;
            deadline_ = now + interval_;
            return now;
        }

        if (now >= deadline_)
        {
            // Skip missed deadlines in one calculation after a stall. Never
            // spin through catch-up frames and never accumulate an old debt.
            const auto missed = (now - deadline_) / interval_;
            deadline_ += interval_ * (missed + 1);
            return now;
        }

        const auto target = deadline_;
        deadline_ += interval_;
        return target;
    }

  private:
    Duration interval_{};
    TimePoint deadline_{};
    bool armed_ = false;
};

} // namespace storm::frame
