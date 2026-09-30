#include "resource_submission.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>

struct ProbeArena {
    struct Slice { int buffer = 0; std::size_t offset = 0, length = 0; void *data = nullptr; };
    std::array<std::uint8_t, 128> bytes{};
    std::size_t used = 0;
    Slice allocate(int, std::size_t length)
    {
        if (length > bytes.size() - used) return {};
        Slice result{1, used, length, bytes.data() + used}; used += length; return result;
    }
};

static void check(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    sm::SubmissionCounters counters;
    check(sm::bufferClass(false, false) == sm::BufferClass::revisionStable &&
              sm::bufferClass(false, true) == sm::BufferClass::dynamic &&
              sm::bufferClass(true, false) == sm::BufferClass::userPointer,
          "only declared dynamic and UP inputs route through frame uploads");
    sm::RevisionResidentBuffers<std::uintptr_t> resident;
    std::array<std::uint8_t, 96> staticVertices{};
    std::uintptr_t nextHandle = 1;
    auto upload = [&](const void *, std::size_t) { return nextHandle++; };

    const sm::BufferRevision stable{7, 1, staticVertices.size()};
    auto first = resident.resolve(stable, staticVertices.data(), upload, counters);
    check(first && *first == 1, "first stable revision uploads");
    for (unsigned frame = 0; frame < 120; ++frame) {
        auto same = resident.resolve(stable, staticVertices.data(), upload, counters);
        check(same && *same == *first, "stable revision preserves one handle across frames");
    }
    check(counters.residentUploads == 1 &&
              counters.residentUploadBytes == staticVertices.size(),
          "stable VB/IB data has no per-frame upload");
    auto changed = resident.resolve({7, 2, staticVertices.size()}, staticVertices.data(),
                                    upload, counters);
    check(changed && *changed != *first && counters.residentUploads == 2,
          "only a changed revision uploads again");
    ProbeArena arena;
    std::array<std::uint8_t, 48> dynamic{};
    dynamic[17] = 0x5a;
    auto dynamicSlice = sm::uploadDynamicSlice(arena, 1, dynamic.data(), dynamic.size(), counters);
    check(counters.dynamicUploads == 1 && counters.dynamicUploadBytes == 48,
          "dynamic arena accounts only the written slice");
    check(dynamicSlice.buffer == 1 && dynamicSlice.length == dynamic.size() &&
              static_cast<std::uint8_t *>(dynamicSlice.data)[17] == 0x5a,
          "dynamic bytes are copied through the frame arena allocation");
    auto rejectedDynamic = sm::uploadDynamicSlice(arena, 1, dynamic.data(), 96, counters);
    check(!rejectedDynamic.data && counters.dynamicUploads == 1,
          "failed arena allocation is not counted as an upload");

    const std::uint16_t sourceIndices[] = {5, 7, 6, 6, 7, 8};
    sm::IndexedDrawSpan draw;
    check(sm::makeIndexedDrawSpan(sourceIndices, false, 6, -2, 5, 4, 16, 12, draw),
          "signed baseVertex and declared index span validate");
    check(draw.sourceFirst == 5 && draw.sourceLast == 8 && draw.baseVertex == -2 &&
              draw.indexByteOffset == 12,
          "draw span preserves source indices, baseVertex, and byte offset");
    check(!sm::makeIndexedDrawSpan(sourceIndices, false, 6, -6, 5, 4, 16, 0, draw),
          "negative resolved vertex address is rejected");
    const std::uint16_t escapedIndices[] = {5, 7, 9};
    check(!sm::makeIndexedDrawSpan(escapedIndices, false, 3, 0, 5, 4, 16, 0, draw),
          "index outside declared MinVertexIndex/NumVertices is rejected");

    struct Vertex { float xyz[3]; std::uint32_t color; };
    std::array<Vertex, 8> a{}, b{};
    const std::uint16_t ai[] = {2, 3, 4};
    const std::uint32_t bi[] = {1, 3, 2};
    sm::UpBatch batch;
    const sm::UpBatchKey key{99, 4, sizeof(Vertex)};
    check(batch.appendIndexedTriangles(key, a.data(), a.size(), ai, false, 3, 2, 3),
          "first UP triangle list batches its used span");
    check(batch.appendIndexedTriangles(key, b.data(), b.size(), bi, true, 3, 1, 3),
          "compatible UP draw joins the same upload");
    check(batch.vertexBytes().size() == 6 * sizeof(Vertex) &&
              batch.indices() == std::vector<std::uint32_t>({0, 1, 2, 3, 5, 4}) &&
              batch.draws().size() == 2,
          "UP indices rebase onto tightly packed original-stride vertices");
    check(!batch.appendIndexedTriangles({100, 4, sizeof(Vertex)}, a.data(), a.size(), ai,
                                        false, 3, 2, 3),
          "different pipeline cannot silently enter a batch");
    batch.accountUpload(counters);
    check(counters.upBatches == 1 && counters.upDraws == 2 && counters.upUploads == 2 &&
              counters.legacyConvertedVertices == 0,
          "one UP batch emits two arena uploads and zero legacy conversion");

    std::puts("PASS: stable revisions upload once; changed revisions upload once; indexed spans and UP rebasing are exact; legacy conversion remains zero");
}
