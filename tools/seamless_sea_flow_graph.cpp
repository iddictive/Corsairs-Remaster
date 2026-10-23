// Offline asset producer; never links or launches the game.
#include "../experiments/native-metal/engine/src/libs/sea_ai/include/ai_flow_graph_table.h"
#include "../experiments/native-metal/engine/src/libs/math/include/c_vector.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

using namespace native_flow_graph;

struct Point
{
    CVECTOR vPos;
    std::vector<size_t> aEdges;
};
struct Edge
{
    uint32_t dw1, dw2;
    float fLen;
};

std::string trim(const std::string &text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return {};
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

template <class T> void append(std::vector<unsigned char> &bytes, const T &value)
{
    const auto *data = reinterpret_cast<const unsigned char *>(&value);
    bytes.insert(bytes.end(), data, data + sizeof(value));
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 3)
            throw std::runtime_error("Usage: seamless_sea_flow_graph source.ini destination.aifg");
        const uint32_t endian = prepared_endian;
        if (*reinterpret_cast<const unsigned char *>(&endian) != 4 || sizeof(float) != 4)
            throw std::runtime_error("Prepared graph producer requires little-endian native32 floats");
        std::ifstream input(argv[1]);
        if (!input)
            throw std::runtime_error("Cannot read source INI");
        bool section = false;
        std::vector<std::string> records;
        std::string line;
        while (std::getline(input, line))
        {
            if (line.size() >= 32768)
                throw std::runtime_error("Graph line exceeds native Load buffer");
            line = trim(line);
            if (line.empty() || line[0] == ';')
                continue;
            if (line[0] == '[')
            {
                section = line == "[GraphPoints]";
                continue;
            }
            if (!section)
                continue;
            const auto equals = line.find('=');
            if (equals == std::string::npos || trim(line.substr(0, equals)) != "pnt" + std::to_string(records.size()))
                throw std::runtime_error("Graph points must retain native contiguous index order");
            records.push_back(trim(line.substr(equals + 1)));
            if (records.size() > max_prepared_points)
                throw std::runtime_error("Graph point budget exceeded");
        }

        // Native Load creates every point before AddEdge and preserves each
        // point's declared adjacency order. Dedup keeps the first orientation.
        std::vector<Point> points(records.size());
        std::vector<Edge> edges;
        std::vector<uint32_t> counts(records.size());
        uint32_t references = 0;
        for (size_t i = 0; i < records.size(); ++i)
        {
            auto &point = points[i];
            point.vPos.y = 0;
            int consumed = 0;
            if (std::sscanf(records[i].c_str(), "%f,%f,%u,%n", &point.vPos.x, &point.vPos.z, &counts[i], &consumed) != 3 ||
                consumed <= 0 || !std::isfinite(point.vPos.x) || !std::isfinite(point.vPos.z) ||
                counts[i] > max_prepared_references - references)
                throw std::runtime_error("Invalid native graph point");
            references += counts[i];
        }
        std::map<std::pair<uint32_t, uint32_t>, size_t> edgeIds;
        for (size_t i = 0; i < records.size(); ++i)
        {
            float x, z;
            unsigned count;
            int consumed = 0;
            std::sscanf(records[i].c_str(), "%f,%f,%u,%n", &x, &z, &count, &consumed);
            const char *cursor = records[i].c_str() + consumed;
            for (uint32_t j = 0; j < count; ++j)
            {
                uint32_t a, b;
                consumed = 0;
                if (std::sscanf(cursor, "%u,%u,%n", &a, &b, &consumed) != 2 || consumed <= 0 ||
                    a >= points.size() || b >= points.size() || (a != i && b != i))
                    throw std::runtime_error("Invalid native graph adjacency");
                cursor += consumed;
                const auto key = std::minmax(a, b);
                const auto found = edgeIds.find({key.first, key.second});
                size_t edge;
                if (found == edgeIds.end())
                {
                    edge = edges.size();
                    if (edge >= max_prepared_edges)
                        throw std::runtime_error("Graph edge budget exceeded");
                    const float length = sqrtf(~(points[a].vPos - points[b].vPos));
                    if (!std::isfinite(length))
                        throw std::runtime_error("Nonfinite native graph edge length");
                    edges.push_back({a, b, length});
                    edgeIds.emplace(std::make_pair(key.first, key.second), edge);
                }
                else
                    edge = found->second;
                points[i].aEdges.push_back(edge);
            }
            if (!trim(cursor).empty())
                throw std::runtime_error("Unexpected native graph trailing fields");
        }
        const uint64_t byteCount = PreparedBytes(uint32_t(points.size()), uint32_t(edges.size()), references);
        if (byteCount > max_prepared_bytes)
            throw std::runtime_error("Prepared graph file budget exceeded");
        std::vector<table_t> table(points.size() * points.size());
        BuildTable(points, edges, table.data(), [&](size_t edge, size_t point) -> size_t {
            if (edges[edge].dw1 == point)
                return edges[edge].dw2;
            return edges[edge].dw1;
        });

        std::vector<unsigned char> payload;
        payload.reserve(size_t(byteCount - prepared_header_bytes));
        for (size_t i = 0; i < points.size(); ++i)
        {
            append(payload, points[i].vPos.x); append(payload, points[i].vPos.y); append(payload, points[i].vPos.z);
            append(payload, uint32_t(0)); append(payload, counts[i]);
        }
        for (const auto &edge : edges)
        {
            append(payload, edge.dw1); append(payload, edge.dw2); append(payload, edge.fLen);
        }
        for (const auto &point : points)
            for (auto edge : point.aEdges)
                append(payload, uint32_t(edge));
        for (const auto &entry : table)
        {
            if ((entry.p != invalid_index && entry.p >= points.size()) || !std::isfinite(entry.d) ||
                entry.d < 0 || entry.d > 1e8f || (entry.p == invalid_index && entry.d != 1e8f))
                throw std::runtime_error("Native table exceeds prepared record contract");
            append(payload, entry.p); append(payload, entry.d);
        }
        std::vector<unsigned char> header;
        append(header, prepared_magic); append(header, prepared_version); append(header, prepared_endian);
        append(header, uint32_t(points.size())); append(header, uint32_t(edges.size())); append(header, references);
        append(header, uint32_t(0)); append(header, byteCount); append(header, HashBytes(payload.data(), payload.size()));
        if (header.size() != prepared_header_bytes || header.size() + payload.size() != byteCount)
            throw std::runtime_error("Prepared graph serialization size differs");
        std::ofstream output(argv[2], std::ios::binary | std::ios::trunc);
        output.exceptions(std::ios::failbit | std::ios::badbit);
        output.write(reinterpret_cast<const char *>(header.data()), header.size());
        output.write(reinterpret_cast<const char *>(payload.data()), payload.size());
        output.close();
        std::cout << "{\"points\":" << points.size() << ",\"edges\":" << edges.size()
                  << ",\"adjacency_references\":" << references << ",\"table_bytes\":" << table.size() * 8
                  << ",\"bytes\":" << byteCount << "}\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Prepared native graph: " << error.what() << "\n";
        return 1;
    }
}
