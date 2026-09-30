#pragma once

// Derives one light-shaft aperture rectangle per authored window opening.
//
// The engine bridge submits the exact triangles of every LocationWindows
// object.  A single object usually groups the stained glass of several
// separate openings, so a bounding box over the object describes the whole
// window band of a building instead of one window.  That box became an
// aperture prism the size of the interior: every view ray inside the room then
// crossed the beam for many units and the volume pass read as a lit fog with
// the character punched out of it, not as light falling through a window.
//
// The rectangles below are therefore grown from connected coplanar triangle
// groups: two quads belong to the same opening only when they share a vertex
// and lie on the same plane.  Coplanar openings separated by wall geometry
// stay separate apertures.

#include <simd/simd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace storm_metal {

inline constexpr unsigned lightShaftOpeningLimit = 16;

struct ShaftAperture {
  simd_float3 center{};
  simd_float3 right{};
  simd_float3 up{};
  float halfWidth = 0.0f;
  float halfHeight = 0.0f;
};

// Triangle count is bounded by the engine bridge contract (65536 indices), so
// the temporary vectors below stay under a few hundred kilobytes per object.
inline unsigned deriveShaftApertures(const float *positions, uint32_t vertexCount,
                                     const uint16_t *indices, uint32_t indexCount,
                                     ShaftAperture *out, unsigned capacity) {
  if (!positions || !indices || !out || capacity == 0 || vertexCount < 3 ||
      indexCount < 3) {
    return 0;
  }
  const uint32_t triangleCount = indexCount / 3;
  if (triangleCount == 0) return 0;

  auto vertex = [&](uint32_t index) {
    return simd_make_float3(positions[index * 3], positions[index * 3 + 1],
                            positions[index * 3 + 2]);
  };

  std::vector<simd_float3> normals(triangleCount, simd_make_float3(0, 0, 0));
  std::vector<float> offsets(triangleCount, 0.0f);
  std::vector<uint8_t> usable(triangleCount, 0);
  for (uint32_t t = 0; t < triangleCount; ++t) {
    const uint32_t a = indices[t * 3], b = indices[t * 3 + 1],
                   c = indices[t * 3 + 2];
    if (a >= vertexCount || b >= vertexCount || c >= vertexCount) continue;
    const simd_float3 p0 = vertex(a), p1 = vertex(b), p2 = vertex(c);
    const simd_float3 cross = simd_cross(p1 - p0, p2 - p0);
    const float length = simd_length(cross);
    if (!(length > 1e-6f) || !std::isfinite(length)) continue;
    normals[t] = cross / length;
    offsets[t] = simd_dot(normals[t], p0);
    usable[t] = 1;
  }

  std::vector<uint32_t> parent(triangleCount);
  for (uint32_t t = 0; t < triangleCount; ++t) parent[t] = t;
  auto find = [&parent](uint32_t value) {
    while (parent[value] != value) {
      parent[value] = parent[parent[value]];
      value = parent[value];
    }
    return value;
  };
  auto join = [&](uint32_t left, uint32_t right) {
    left = find(left);
    right = find(right);
    if (left != right) parent[right] = left;
  };

  // A vertex index is shared by every triangle touching it.  Comparing against
  // the first such triangle is enough: union-find propagates that relation.
  constexpr float normalTolerance = 0.9995f;  // about 1.8 degrees
  constexpr float planeTolerance = 0.02f;
  std::vector<uint32_t> vertexOwner(vertexCount, UINT32_MAX);
  for (uint32_t t = 0; t < triangleCount; ++t) {
    if (!usable[t]) continue;
    for (uint32_t corner = 0; corner < 3; ++corner) {
      const uint32_t index = indices[t * 3 + corner];
      const uint32_t owner = vertexOwner[index];
      if (owner == UINT32_MAX || !usable[owner]) {
        vertexOwner[index] = t;
        continue;
      }
      const float facing = simd_dot(normals[t], normals[owner]);
      // Both faces of a double-sided pane join: only the plane matters, not
      // the winding, otherwise one opening would produce two apertures.
      if (std::fabs(facing) < normalTolerance) continue;
      const float sign = facing > 0.0f ? 1.0f : -1.0f;
      if (std::fabs(offsets[t] - sign * offsets[owner]) > planeTolerance) {
        continue;
      }
      join(t, owner);
    }
  }

  struct Opening {
    unsigned triangle = 0;
    unsigned area = 0;
    ShaftAperture aperture{};
  };
  std::vector<Opening> openings;
  std::vector<unsigned> members;
  for (uint32_t t = 0; t < triangleCount; ++t) {
    if (!usable[t] || find(t) != t) continue;
    members.clear();
    for (uint32_t other = 0; other < triangleCount; ++other) {
      if (usable[other] && find(other) == t) members.push_back(other);
    }
    if (members.empty()) continue;

    const simd_float3 reference = normals[members.front()];
    simd_float3 normalSum{0, 0, 0};
    for (const unsigned triangle : members) {
      const float sign = simd_dot(normals[triangle], reference) < 0.0f ? -1.0f
                                                                       : 1.0f;
      normalSum += normals[triangle] * sign;
    }
    const float normalLength = simd_length(normalSum);
    if (!(normalLength > 1e-4f)) continue;
    const simd_float3 normal = normalSum / normalLength;

    // Distinct vertices of the group.
    std::vector<uint32_t> vertices;
    for (const unsigned triangle : members) {
      for (uint32_t corner = 0; corner < 3; ++corner) {
        vertices.push_back(indices[triangle * 3 + corner]);
      }
    }
    std::sort(vertices.begin(), vertices.end());
    vertices.erase(std::unique(vertices.begin(), vertices.end()),
                   vertices.end());
    simd_float3 center{0, 0, 0};
    for (const uint32_t index : vertices) center += vertex(index);
    center /= float(vertices.size());

    // The rectangle follows the group's own edges and the pair with the
    // smallest covering area wins.  The longest triangle edge is often the
    // shared diagonal of a two-triangle pane, and a principal-axis fit
    // degenerates on a square opening; both cases stay exact here.
    float bestArea = 0.0f;
    simd_float3 right{0, 0, 0}, up{0, 0, 0};
    for (const unsigned triangle : members) {
      for (uint32_t corner = 0; corner < 3; ++corner) {
        const simd_float3 start = vertex(indices[triangle * 3 + corner]);
        const simd_float3 end =
            vertex(indices[triangle * 3 + (corner + 1) % 3]);
        simd_float3 direction = end - start;
        direction -= normal * simd_dot(direction, normal);
        const float length = simd_length(direction);
        if (!(length > 1e-5f)) continue;
        direction /= length;
        const simd_float3 perpendicular = simd_cross(normal, direction);
        float along = 0.0f, across = 0.0f;
        for (const uint32_t index : vertices) {
          const simd_float3 local = vertex(index) - center;
          along = std::max(along, std::fabs(simd_dot(local, direction)));
          across = std::max(across, std::fabs(simd_dot(local, perpendicular)));
        }
        if (!(along > 1e-3f) || !(across > 1e-3f)) continue;
        const float area = along * across;
        if (bestArea == 0.0f || area < bestArea) {
          bestArea = area;
          right = direction;
          up = perpendicular;
        }
      }
    }
    if (bestArea == 0.0f) continue;

    float halfWidth = 0.0f, halfHeight = 0.0f;
    for (const uint32_t index : vertices) {
      const simd_float3 local = vertex(index) - center;
      halfWidth = std::max(halfWidth, std::fabs(simd_dot(local, right)));
      halfHeight = std::max(halfHeight, std::fabs(simd_dot(local, up)));
    }
    if (!(halfWidth > 1e-3f) || !(halfHeight > 1e-3f)) continue;

    Opening opening;
    opening.triangle = t;
    opening.aperture.center = center;
    opening.aperture.right = right;
    opening.aperture.up = up;
    opening.aperture.halfWidth = halfWidth;
    opening.aperture.halfHeight = halfHeight;
    opening.area = static_cast<unsigned>(halfWidth * halfHeight * 1000.0f);
    openings.push_back(opening);
  }
  if (openings.empty()) return 0;

  // Deterministic capacity handling: the largest openings win, ties fall back
  // to authored triangle order.
  if (openings.size() > capacity) {
    std::stable_sort(openings.begin(), openings.end(),
                     [](const Opening &left, const Opening &right) {
                       if (left.area != right.area) return left.area > right.area;
                       return left.triangle < right.triangle;
                     });
    openings.resize(capacity);
  }
  unsigned count = 0;
  for (const auto &opening : openings) out[count++] = opening.aperture;
  return count;
}

}  // namespace storm_metal
