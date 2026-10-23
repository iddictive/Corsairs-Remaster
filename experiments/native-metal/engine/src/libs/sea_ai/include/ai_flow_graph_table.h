#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

// Shared by AIFlowGraph and its offline asset producer. Keep the native
// relaxation order and float operations here; a different shortest-path
// algorithm need not select the same next hop at native near-ties.
namespace native_flow_graph
{
constexpr uint32_t invalid_index = 0xFFFFFFFFu;
constexpr uint32_t prepared_version = 1;
constexpr uint32_t prepared_endian = 0x01020304u;
constexpr char prepared_magic[8] = {'A', 'I', 'F', 'L', 'O', 'W', '0', '1'};
constexpr uint64_t prepared_header_bytes = 48;
constexpr uint32_t max_prepared_points = 2048;
constexpr uint32_t max_prepared_edges = 1000000;
constexpr uint32_t max_prepared_references = 2000000;
constexpr uint64_t max_prepared_bytes = 64ull * 1024 * 1024;

struct table_t
{
    uint32_t p;
    float d;
};

inline uint64_t PreparedBytes(uint32_t points, uint32_t edges, uint32_t references)
{
    return prepared_header_bytes + uint64_t(points) * 20 + uint64_t(edges) * 12 +
           uint64_t(references) * 4 + uint64_t(points) * points * 8;
}

inline uint64_t HashBytes(const void *data, size_t size, uint64_t hash = 14695981039346656037ull)
{
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (size_t i = 0; i < size; ++i)
        hash = (hash ^ bytes[i]) * 1099511628211ull;
    return hash;
}

template <class Point, class Edge, class OtherPoint>
void BuildTable(std::vector<Point> &aPoints, std::vector<Edge> &aEdges, table_t *pTable, OtherPoint GetOtherEdgePoint)
{
    uint32_t i, j, k, x, y;
    const auto dwNumPoints = aPoints.size();

    for (i = 0; i < dwNumPoints * dwNumPoints; i++)
    {
        pTable[i].p = invalid_index;
        pTable[i].d = 1e8f;
    }
    for (i = 0; i < dwNumPoints; i++)
    {
        Point *pP = &aPoints[i];
        table_t *pTableRow = &pTable[i * dwNumPoints];
        for (j = 0; j < pP->aEdges.size(); j++)
        {
            const uint32_t dwPnt = GetOtherEdgePoint(pP->aEdges[j], i);
            pTableRow[dwPnt].p = dwPnt;
            pTableRow[dwPnt].d = aEdges[pP->aEdges[j]].fLen;
        }
    }
    for (k = 0; k < dwNumPoints; k++)
    {
        bool bF = true;
        for (y = 0; y < dwNumPoints; y++)
        {
            for (x = 0; x < dwNumPoints; x++)
                if (x != y)
                {
                    Point *pP = &aPoints[y];
                    float d = pTable[x + y * dwNumPoints].d;
                    for (j = 0; j < pP->aEdges.size(); j++)
                    {
                        const uint32_t dwPnt = GetOtherEdgePoint(pP->aEdges[j], y);
                        const float d1 = pTable[dwPnt + y * dwNumPoints].d;
                        const float d2 = pTable[x + dwPnt * dwNumPoints].d;
                        if (d1 + d2 < d && fabsf((d1 + d2) - d) > 0.01f)
                        {
                            d = d1 + d2;
                            pTable[x + y * dwNumPoints].d = d;
                            pTable[x + y * dwNumPoints].p = dwPnt;
                            bF = false;
                        }
                    }
                }
        }
        if (bF)
            break;
    }
}
} // namespace native_flow_graph
