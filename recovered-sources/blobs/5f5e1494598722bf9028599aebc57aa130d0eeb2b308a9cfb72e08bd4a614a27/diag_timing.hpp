#pragma once
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>

// Private diagnostic only. The driver flushes after ending its CPU frame timer.
// No allocation or I/O occurs in a measured stage. Pending rows also capture the
// action frame's subsequent fragment execution before the driver's flush.
namespace corsairs_fracture_diag {
using Clock = std::chrono::steady_clock;
constexpr unsigned maxActions = 2, maxRows = 512;
struct Row {
    const char *stage = nullptr;
    const void *object = nullptr;
    char node[96]{};
    int parent = -1;
    uint64_t calls = 0, items = 0, bytes = 0;
    int64_t microseconds = 0;
};
struct State {
    std::array<Row, maxRows> rows{};
    unsigned count = 0, actions = 0, dropped = 0;
    bool active = false, pending = false;
    int parent = -1;
    const char *node = "<unbound>", *kind = "none";
    const void *owner = nullptr;
};
inline thread_local State state;
inline bool enabled() { return state.active || state.pending; }
inline int row(const char *stage, const void *object) {
    if (!enabled()) return -1;
    for (unsigned n = 0; n < state.count; ++n) {
        const auto &r = state.rows[n];
        if (r.object == object && r.parent == state.parent &&
            std::strcmp(r.stage, stage) == 0 && std::strcmp(r.node, state.node) == 0)
            return int(n);
    }
    if (state.count == maxRows) { ++state.dropped; return -1; }
    auto &r = state.rows[state.count];
    r = {}; r.stage = stage; r.object = object; r.parent = state.parent;
    std::snprintf(r.node, sizeof(r.node), "%s", state.node);
    return int(state.count++);
}
struct Action {
    bool owner = false;
    Action(const char *kind, const void *object) {
        if (state.active || state.pending || state.actions >= maxActions) return;
        ++state.actions; state.count = state.dropped = 0; state.parent = -1;
        state.node = "<unbound>"; state.kind = kind; state.owner = object;
        state.active = owner = true;
    }
    ~Action() { if (owner) { state.active = false; state.pending = true; state.parent = -1; } }
};
struct Node {
    const char *previous;
    explicit Node(const char *name) : previous(state.node) { state.node = name && *name ? name : "<root>"; }
    ~Node() { state.node = previous; }
};
struct Scope {
    int id = -1, previous = -1;
    Clock::time_point begin{};
    Scope(const char *stage, const void *object) : id(row(stage, object)), previous(state.parent) {
        if (id >= 0) { ++state.rows[id].calls; state.parent = id; begin = Clock::now(); }
    }
    void add(uint64_t items, uint64_t bytes = 0) { if (id >= 0) { state.rows[id].items += items; state.rows[id].bytes += bytes; } }
    void stop() {
        if (id < 0) return;
        state.rows[id].microseconds += std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - begin).count();
        state.parent = previous; id = -1;
    }
    ~Scope() { stop(); }
};
struct Interval {
    int id = -1;
    Clock::time_point begin{};
    explicit Interval(int rowId) : id(rowId) { if (id >= 0) begin = Clock::now(); }
    Interval(const Interval &) = delete;
    Interval &operator=(const Interval &) = delete;
    void stop() {
        if (id < 0) return;
        ++state.rows[id].calls;
        state.rows[id].microseconds += std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - begin).count();
        id = -1;
    }
    ~Interval() { stop(); }
};
struct Sum {
    int id = -1;
    Sum(const char *stage, const void *object) : id(row(stage, object)) {}
    Interval measure() { return Interval(id); }
    void add(uint64_t items, uint64_t bytes = 0) { if (id >= 0) { state.rows[id].items += items; state.rows[id].bytes += bytes; } }
};
inline void flush() {
    if (!state.pending || state.active) return;
    for (unsigned n = 0; n < state.count; ++n) {
        const auto &r = state.rows[n];
        std::fprintf(stderr, "FRACTURE_TRACE\taction=%u\ttype=%s\towner=%p\trow=%u\tparent=%d\tnode=%s\tobject=%p\tstage=%s\tcalls=%llu\tus=%lld\titems=%llu\tbytes=%llu\n",
            state.actions, state.kind, state.owner, n, r.parent, r.node, r.object, r.stage,
            (unsigned long long)r.calls, (long long)r.microseconds,
            (unsigned long long)r.items, (unsigned long long)r.bytes);
    }
    std::fprintf(stderr, "FRACTURE_TRACE_SUMMARY\taction=%u\trows=%u\tdropped=%u\n", state.actions, state.count, state.dropped);
    state.pending = false; state.count = state.dropped = 0; state.parent = -1;
}
} // namespace corsairs_fracture_diag
