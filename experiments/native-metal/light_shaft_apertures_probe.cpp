#include "light_shaft_apertures.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

void need(bool value, const char *message) {
  if (!value) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    std::exit(1);
  }
}

struct Mesh {
  std::vector<float> positions;
  std::vector<uint16_t> indices;

  uint16_t vertex(simd_float3 point) {
    const uint16_t index = static_cast<uint16_t>(positions.size() / 3);
    positions.push_back(point.x);
    positions.push_back(point.y);
    positions.push_back(point.z);
    return index;
  }
  void triangle(uint16_t a, uint16_t b, uint16_t c) {
    indices.push_back(a);
    indices.push_back(b);
    indices.push_back(c);
  }
  // Quad with its own vertices, as an exporter writes two separate openings.
  void quad(simd_float3 origin, simd_float3 right, simd_float3 up) {
    const uint16_t a = vertex(origin), b = vertex(origin + right);
    const uint16_t c = vertex(origin + right + up), d = vertex(origin + up);
    triangle(a, b, c);
    triangle(a, c, d);
  }
  // Quad sharing the given corner vertices, as one window split in two panes.
  void quad(uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
    triangle(a, b, c);
    triangle(a, c, d);
  }
  unsigned apertures(storm_metal::ShaftAperture *out, unsigned capacity) const {
    return storm_metal::deriveShaftApertures(positions.data(),
                                             uint32_t(positions.size() / 3),
                                             indices.data(),
                                             uint32_t(indices.size()), out,
                                             capacity);
  }
};

bool near(float value, float expected, float tolerance) {
  return std::fabs(value - expected) <= tolerance;
}

bool orthonormal(const storm_metal::ShaftAperture &aperture) {
  const float rightLength = simd_length(aperture.right);
  const float upLength = simd_length(aperture.up);
  if (!near(rightLength, 1.0f, 1e-3f) || !near(upLength, 1.0f, 1e-3f)) {
    return false;
  }
  return near(simd_dot(aperture.right, aperture.up), 0.0f, 1e-3f);
}

// The rectangle axes follow the longest authored edge, so a quad reports its
// two half extents in whichever order that edge implies.
struct Size {
  float small = 0.0f;
  float large = 0.0f;
};

Size sizes(const storm_metal::ShaftAperture &aperture) {
  return {std::fmin(aperture.halfWidth, aperture.halfHeight),
          std::fmax(aperture.halfWidth, aperture.halfHeight)};
}

}  // namespace

int main() {
  using storm_metal::ShaftAperture;
  ShaftAperture out[storm_metal::lightShaftOpeningLimit];

  // A LocationWindows object groups the glass of every window of a building.
  // The old bounding box of the whole object became one aperture the size of
  // the interior; each opening must keep its own rectangle.
  {
    Mesh mesh;
    const simd_float3 right = simd_make_float3(1.6f, 0, 0), up = simd_make_float3(0, 2.4f, 0);
    for (float x : {-7.0f, 0.0f, 7.0f}) {
      mesh.quad(simd_make_float3(x, 2.0f, 0), right, up);
    }
    const unsigned count = mesh.apertures(out, 16);
    need(count == 3, "three separated window quads stay three apertures");
    for (unsigned i = 0; i < count; ++i) {
      const Size size = sizes(out[i]);
      need(near(size.small, 0.8f, 1e-3f), "opening keeps its own width");
      need(near(size.large, 1.2f, 1e-3f), "opening keeps its own height");
      need(orthonormal(out[i]), "opening basis is orthonormal");
    }
    need(near(out[0].center.y, 3.2f, 1e-3f), "opening centre is the quad centre");
    for (unsigned i = 0; i < count; ++i) {
      need(sizes(out[i]).large < 1.5f,
           "no aperture inherits the object bounding box");
    }
  }

  // One authored window split into two panes that share their corner vertices
  // is one opening.
  {
    Mesh mesh;
    const uint16_t a = mesh.vertex({-1.0f, 0, 0});
    const uint16_t b = mesh.vertex({0.0f, 0, 0});
    const uint16_t c = mesh.vertex({0.0f, 2.0f, 0});
    const uint16_t d = mesh.vertex({-1.0f, 2.0f, 0});
    const uint16_t e = mesh.vertex({1.0f, 0, 0});
    const uint16_t f = mesh.vertex({1.0f, 2.0f, 0});
    mesh.quad(a, b, c, d);
    mesh.quad(b, e, f, c);
    const unsigned count = mesh.apertures(out, 16);
    need(count == 1, "panes sharing an edge form one aperture");
    need(near(out[0].halfWidth, 1.0f, 1e-3f), "shared pane keeps the full width");
    need(near(out[0].halfHeight, 1.0f, 1e-3f), "shared pane keeps the full height");
  }

  // Two coplanar windows without shared vertices are two openings even though
  // their planes are identical.
  {
    Mesh mesh;
    mesh.quad(simd_make_float3(-4.0f, 0, 0), simd_make_float3(1, 0, 0), simd_make_float3(0, 2, 0));
    mesh.quad(simd_make_float3(4.0f, 0, 0), simd_make_float3(1, 0, 0), simd_make_float3(0, 2, 0));
    const unsigned count = mesh.apertures(out, 16);
    need(count == 2, "coplanar openings without shared vertices stay separate");
  }

  // Both faces of a double-sided pane are one opening; the winding differs,
  // the plane does not.
  {
    Mesh mesh;
    const uint16_t a = mesh.vertex({0, 0, 3.0f});
    const uint16_t b = mesh.vertex({2.0f, 0, 3.0f});
    const uint16_t c = mesh.vertex({2.0f, 2.0f, 3.0f});
    const uint16_t d = mesh.vertex({0, 2.0f, 3.0f});
    mesh.triangle(a, b, c);
    mesh.triangle(a, c, d);
    mesh.triangle(a, c, b);
    mesh.triangle(a, d, c);
    const unsigned count = mesh.apertures(out, 16);
    need(count == 1, "double-sided pane is one aperture");
    need(near(out[0].halfWidth, 1.0f, 1e-3f), "double-sided pane keeps its width");
    need(near(out[0].halfHeight, 1.0f, 1e-3f), "double-sided pane keeps its height");
  }

  // Windows at different angles are different openings.
  {
    Mesh mesh;
    mesh.quad(simd_make_float3(0, 0, 0), simd_make_float3(2, 0, 0), simd_make_float3(0, 2, 0));
    mesh.quad(simd_make_float3(0, 0, 0), simd_make_float3(1.4f, 0, 1.4f), simd_make_float3(0, 2, 0));
    const unsigned count = mesh.apertures(out, 16);
    need(count == 2, "apertures on different planes stay separate");
  }

  // The capacity limit keeps the largest openings and stays deterministic.
  {
    Mesh mesh;
    for (unsigned i = 0; i < 20; ++i) {
      const float scale = 1.0f + float(i) * 0.1f;
      mesh.quad(simd_make_float3(float(i) * 6.0f, 0, 0), simd_make_float3(scale, 0, 0), simd_make_float3(0, scale, 0));
    }
    const unsigned count = mesh.apertures(out, 16);
    need(count == 16, "capacity limits the aperture count");
    float smallest = 1e9f;
    for (unsigned i = 0; i < count; ++i) {
      smallest = std::fmin(smallest, sizes(out[i]).large);
    }
    need(smallest > 0.65f, "the largest openings survive the capacity limit");
    ShaftAperture repeat[16];
    need(mesh.apertures(repeat, 16) == count, "capacity selection is deterministic");
    for (unsigned i = 0; i < count; ++i) {
      need(near(sizes(repeat[i]).large, sizes(out[i]).large, 0.0f), "selection order is stable");
    }
    need(mesh.apertures(out, 0) == 0, "a zero capacity derives nothing");
  }

  // Degenerate and non-finite input never fabricates an aperture.
  {
    Mesh mesh;
    mesh.quad(simd_make_float3(0, 0, 0), simd_make_float3(1, 0, 0), simd_make_float3(2, 0, 0));  // collinear
    need(mesh.apertures(out, 16) == 0, "a collinear strip is not an aperture");
    mesh.quad(simd_make_float3(0, 0, 5.0f), simd_make_float3(1, 0, 0), simd_make_float3(0, 2, 0));
    mesh.quad(simd_make_float3(0, 0, NAN), simd_make_float3(1, 0, 0), simd_make_float3(0, 2, 0));
    const unsigned count = mesh.apertures(out, 16);
    need(count == 1, "non-finite triangles are skipped, valid ones survive");
    need(near(out[0].halfWidth, 0.5f, 1e-3f), "surviving aperture keeps its size");
  }

  // The shader rebuilds the aperture normal as cross(right, up); it must agree
  // with the plane the triangles actually lie on.
  {
    Mesh mesh;
    mesh.quad(simd_make_float3(3.0f, 1.0f, 4.0f), simd_make_float3(1.7f, 0, 1.7f),
              simd_make_float3(0, 2.5f, 0));
    const unsigned count = mesh.apertures(out, 16);
    need(count == 1, "a rotated aperture is derived");
    const simd_float3 normal = simd_normalize(simd_cross(out[0].right, out[0].up));
    need(near(std::fabs(normal.z), 0.7071f, 1e-3f), "derived normal follows the plane");
    const Size size = sizes(out[0]);
    // The slant edge is 1.7*sqrt(2) long, so its half extent is 1.2021.
    need(near(size.small, 1.2021f, 1e-3f), "slanted opening keeps its authored width");
    need(near(size.large, 1.25f, 1e-3f), "slanted opening keeps its authored height");
  }

  std::printf("PASS: authored window apertures derive one rectangle per opening\n");
  return 0;
}
