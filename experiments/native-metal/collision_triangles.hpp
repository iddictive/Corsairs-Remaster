#pragma once
#include "c_vector.h"
#include <vector>

namespace storm::physics
{
// Native Clip has no callback context. Queries are synchronous on the game
// thread; a scoped sink also preserves nested callers and exception recovery.
class TriangleSink
{
    inline static thread_local std::vector<CVECTOR> *current_ = nullptr;
    std::vector<CVECTOR> *previous_;
  public:
    explicit TriangleSink(std::vector<CVECTOR> &out) : previous_(current_) { current_ = &out; }
    ~TriangleSink() { current_ = previous_; }
    static bool Add(const CVECTOR *vertices, int32_t count)
    {
        if (!current_ || !vertices || count < 3) return true;
        for (int32_t i = 1; i + 1 < count; ++i)
        {
            if (~((vertices[i]-vertices[0]) ^ (vertices[i+1]-vertices[0])) < 1.e-10f) continue;
            current_->insert(current_->end(), {vertices[0], vertices[i], vertices[i+1]});
        }
        return true;
    }
};
}
