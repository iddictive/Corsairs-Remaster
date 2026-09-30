#pragma once

#include "indexed_geometry.hpp"

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <optional>
#include <vector>

namespace sm {

enum class BufferClass : std::uint8_t { revisionStable, dynamic, userPointer };

inline BufferClass bufferClass(bool userPointer, bool dynamicUsage)
{
    return userPointer ? BufferClass::userPointer
                       : dynamicUsage ? BufferClass::dynamic : BufferClass::revisionStable;
}

struct SourceSpan {
    std::size_t offset = 0;
    BufferClass classification = BufferClass::userPointer;
    bool resident() const { return classification == BufferClass::revisionStable; }
};

inline SourceSpan locateSourceSpan(const void *source, std::size_t length,
                                   const void *buffer, std::size_t bufferSize,
                                   std::uint64_t identity, std::uint64_t,
                                   bool dynamicUsage, bool locked)
{
    if (!source || !buffer || !identity || dynamicUsage || locked)
        return {};
    const auto *begin = static_cast<const std::uint8_t *>(buffer);
    const auto *value = static_cast<const std::uint8_t *>(source);
    if (value < begin || std::size_t(value - begin) > bufferSize ||
        length > bufferSize - std::size_t(value - begin))
        return {};
    return {std::size_t(value - begin), BufferClass::revisionStable};
}

struct SubmissionCounters {
    std::uint64_t legacyConvertedVertices = 0;
    std::uint64_t residentUploads = 0;
    std::uint64_t residentUploadBytes = 0;
    std::uint64_t dynamicUploads = 0;
    std::uint64_t dynamicUploadBytes = 0;
    std::uint64_t upUploads = 0;
    std::uint64_t upUploadBytes = 0;
    std::uint64_t upDraws = 0;
    std::uint64_t upBatches = 0;
};

inline void accountDynamicUpload(std::size_t bytes, SubmissionCounters &counters)
{
    if (!bytes)
        return;
    ++counters.dynamicUploads;
    counters.dynamicUploadBytes += bytes;
}

// The arena supplies the in-flight lifetime. This helper makes the dynamic
// route explicit and accounts only bytes copied into the returned slice.
template <class Arena, class Device>
auto uploadDynamicSlice(Arena &arena, Device device, const void *bytes,
                        std::size_t length, SubmissionCounters &counters)
    -> decltype(arena.allocate(device, length))
{
    using Slice = decltype(arena.allocate(device, length));
    if (!bytes || !length)
        return Slice{};
    Slice slice = arena.allocate(device, length);
    if (!slice.data || slice.length < length)
        return Slice{};
    std::memcpy(slice.data, bytes, length);
    accountDynamicUpload(length, counters);
    return slice;
}

struct BufferRevision {
    std::uint64_t identity = 0;
    std::uint64_t revision = 0;
    std::size_t bytes = 0;

    bool valid() const { return identity != 0 && bytes != 0; }
};

// Owns one GPU allocation per logical buffer identity. A stable revision is a
// lookup, not an upload; replacing a revision releases the previous handle.
// Handle may be id<MTLBuffer>, a smart pointer, or a deterministic probe token.
template <class Handle> class RevisionResidentBuffers {
public:
    struct Entry {
        std::uint64_t revision = 0;
        std::size_t bytes = 0;
        Handle handle{};
    };

    template <class Upload>
    std::optional<Handle> resolve(BufferRevision source, const void *bytes,
                                  Upload &&upload, SubmissionCounters &counters)
    {
        if (!source.valid() || !bytes)
            return std::nullopt;
        auto found = entries_.find(source.identity);
        if (found != entries_.end() && found->second.revision == source.revision &&
            found->second.bytes == source.bytes)
            return found->second.handle;

        Handle handle = upload(bytes, source.bytes);
        if (!handle)
            return std::nullopt;
        entries_[source.identity] = {source.revision, source.bytes, handle};
        ++counters.residentUploads;
        counters.residentUploadBytes += source.bytes;
        return handle;
    }

    void erase(std::uint64_t identity) { entries_.erase(identity); }
    void clear() { entries_.clear(); }
    std::size_t size() const { return entries_.size(); }

private:
    std::map<std::uint64_t, Entry> entries_;
};

struct IndexedDrawSpan {
    std::uint32_t sourceFirst = 0;
    std::uint32_t sourceLast = 0;
    std::int32_t baseVertex = 0;
    std::uint32_t indexCount = 0;
    std::size_t indexByteOffset = 0;
};

inline bool makeIndexedDrawSpan(const void *indices, bool wide, std::uint32_t count,
                                std::int32_t baseVertex, std::uint32_t minimum,
                                std::uint32_t declaredVertices,
                                std::size_t availableVertices,
                                std::size_t indexByteOffset, IndexedDrawSpan &out)
{
    IndexedSpan span;
    if (!indexedSpan(indices, wide, count, baseVertex, minimum, declaredVertices,
                     availableVertices, span))
        return false;
    out = {span.first, span.last, baseVertex, count, indexByteOffset};
    return true;
}

struct UpBatchKey {
    std::uint64_t pipeline = 0;
    std::uint32_t topology = 0;
    std::uint32_t stride = 0;

    auto operator<=>(const UpBatchKey &) const = default;
};

struct UpDraw {
    std::uint32_t firstIndex = 0;
    std::uint32_t indexCount = 0;
    std::int32_t baseVertex = 0;
};

// Coalesces compatible UP triangle-list payloads into one vertex upload and one
// 32-bit index upload. Source bytes are copied once in their original stride;
// the legacy 112-byte compatibility vertex is never materialized.
class UpBatch {
public:
    bool appendIndexedTriangles(UpBatchKey key, const void *vertices,
                                std::uint32_t vertexCount, const void *indices,
                                bool wideIndices, std::uint32_t indexCount,
                                std::uint32_t minimumVertex,
                                std::uint32_t declaredVertices)
    {
        if (!vertices || !indices || !key.stride || !vertexCount || !indexCount ||
            indexCount % 3 != 0 || minimumVertex > vertexCount ||
            declaredVertices > vertexCount - minimumVertex)
            return false;
        if (key_ && *key_ != key)
            return false;

        IndexedSpan span;
        if (!indexedSpan(indices, wideIndices, indexCount, 0, minimumVertex,
                         declaredVertices, vertexCount, span))
            return false;
        const std::size_t usedVertices = span.vertices();
        if (usedVertices > std::numeric_limits<std::uint32_t>::max() - vertexBase_)
            return false;
        const std::size_t firstByte = std::size_t(span.first) * key.stride;
        const std::size_t byteCount = usedVertices * key.stride;
        const std::size_t oldVertexBytes = vertexBytes_.size();
        if (byteCount > vertexBytes_.max_size() - oldVertexBytes)
            return false;
        if (indices_.size() > std::numeric_limits<std::uint32_t>::max() ||
            indexCount > indices_.max_size() - indices_.size() ||
            indexCount > std::numeric_limits<std::uint32_t>::max() - indices_.size())
            return false;

        key_ = key;
        const auto *source = static_cast<const std::uint8_t *>(vertices);
        vertexBytes_.insert(vertexBytes_.end(), source + firstByte,
                            source + firstByte + byteCount);
        const std::uint32_t firstIndex = static_cast<std::uint32_t>(indices_.size());
        indices_.reserve(indices_.size() + indexCount);
        for (std::uint32_t i = 0; i < indexCount; ++i)
            indices_.push_back(vertexBase_ + readIndex(indices, wideIndices, i) - span.first);
        draws_.push_back({firstIndex, indexCount, 0});
        vertexBase_ += static_cast<std::uint32_t>(usedVertices);
        return true;
    }

    void accountUpload(SubmissionCounters &counters) const
    {
        if (draws_.empty())
            return;
        ++counters.upBatches;
        counters.upDraws += draws_.size();
        counters.upUploads += 2;
        counters.upUploadBytes += vertexBytes_.size() + indices_.size() * sizeof(std::uint32_t);
    }

    const std::optional<UpBatchKey> &key() const { return key_; }
    const std::vector<std::uint8_t> &vertexBytes() const { return vertexBytes_; }
    const std::vector<std::uint32_t> &indices() const { return indices_; }
    const std::vector<UpDraw> &draws() const { return draws_; }

    void clear()
    {
        key_.reset();
        vertexBytes_.clear();
        indices_.clear();
        draws_.clear();
        vertexBase_ = 0;
    }

private:
    std::optional<UpBatchKey> key_;
    std::vector<std::uint8_t> vertexBytes_;
    std::vector<std::uint32_t> indices_;
    std::vector<UpDraw> draws_;
    std::uint32_t vertexBase_ = 0;
};

} // namespace sm
