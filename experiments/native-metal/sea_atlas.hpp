#pragma once

#include "c_vector.h"
#include "matrix.h"
#include "attributes.h"
#include <array>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

class INIFILE;
class ATTRIBUTES;

namespace storm_sea_atlas
{
// x'=a*x+b*z+tx, z'=c*x+d*z+tz. The physical atlas keeps authored metres.
struct Frame
{
    std::array<double, 6> values{1., 0., 0., 1., 0., 0.};
    bool Rigid() const
    {
        for (const auto value : values) if (!std::isfinite(value)) return false;
        const auto &m = values;
        return std::abs(m[0]*m[0]+m[2]*m[2]-1.) <= 2e-6 &&
               std::abs(m[1]*m[1]+m[3]*m[3]-1.) <= 2e-6 &&
               std::abs(m[0]*m[1]+m[2]*m[3]) <= 2e-6 && m[0]*m[3]-m[1]*m[2] > 0.;
    }
    static Frame FromAttributes(const ATTRIBUTES &source)
    {
        Frame frame;
        static constexpr const char *keys[] = {"a", "b", "c", "d", "tx", "tz"};
        for (size_t i = 0; i < frame.values.size(); ++i)
        {
            if (!source.GetAttribute(keys[i]))
                throw std::runtime_error("SEA_ATLAS_LAYOUT: incomplete scene pose; reinstall matching sea assets");
            frame.values[i] = source.GetAttributeAsFloat(keys[i]);
        }
        if (!frame.Rigid())
            throw std::runtime_error("SEA_ATLAS_LAYOUT: invalid scene pose; reinstall matching sea assets");
        return frame;
    }
    Frame Compose(const Frame &inner) const
    {
        const auto &m = values; const auto &n = inner.values;
        return {{{m[0]*n[0]+m[1]*n[2], m[0]*n[1]+m[1]*n[3],
                  m[2]*n[0]+m[3]*n[2], m[2]*n[1]+m[3]*n[3],
                  m[0]*n[4]+m[1]*n[5]+m[4], m[2]*n[4]+m[3]*n[5]+m[5]}}};
    }
    Frame Inverse() const
    {
        if (!Rigid()) throw std::runtime_error("SEA_ATLAS_LAYOUT: cannot invert invalid physical frame");
        const auto &m = values; const double determinant = m[0]*m[3]-m[1]*m[2];
        return {{{m[3]/determinant, -m[1]/determinant, -m[2]/determinant, m[0]/determinant,
                  (m[1]*m[5]-m[3]*m[4])/determinant, (m[2]*m[4]-m[0]*m[5])/determinant}}};
    }
    CVECTOR Point(const CVECTOR &point) const
    {
        const auto &m = values;
        return {static_cast<float>(m[0]*point.x+m[1]*point.z+m[4]), point.y,
                static_cast<float>(m[2]*point.x+m[3]*point.z+m[5])};
    }
    CVECTOR Vector(const CVECTOR &vector) const
    {
        const auto &m = values;
        return {static_cast<float>(m[0]*vector.x+m[1]*vector.z), vector.y,
                static_cast<float>(m[2]*vector.x+m[3]*vector.z)};
    }
    float Heading(float heading) const { return heading+static_cast<float>(std::atan2(values[1], values[3])); }
    static std::optional<Frame> LegacyPose(const ATTRIBUTES *owner)
    {
        const auto *pose = owner ? owner->GetAttributeClass("SeaAtlasLegacyPose") : nullptr;
        return pose ? std::optional<Frame>(FromAttributes(*pose)) : std::nullopt;
    }
    CMatrix Matrix() const
    {
        CMatrix matrix;
        matrix.Vx() = CVECTOR(static_cast<float>(values[0]), 0.f, static_cast<float>(values[2]));
        matrix.Vy() = CVECTOR(0.f, 1.f, 0.f);
        matrix.Vz() = CVECTOR(static_cast<float>(values[1]), 0.f, static_cast<float>(values[3]));
        matrix.Pos() = CVECTOR(static_cast<float>(values[4]), 0.f, static_cast<float>(values[5]));
        return matrix;
    }
};

struct Body
{
    std::string id, sourceRegion, terrain, reflection;
    std::string chartTerrain, chartReflection, depthDirectory, depthName;
    Frame toAtlas, fromAtlas;
};

struct Region
{
    std::string id;
    size_t body = 0;
    Frame toBody, fromBody, toAtlas, fromAtlas;
    std::array<double, 2> mapOrigin{}, legacyMapOrigin{};
    double legacyMapMetres = 50.;
    std::string chartArea;
    std::vector<std::array<double, 2>> approaches;
};

struct RouteGoal
{
    std::array<double, 2> destination, approach;
};

class Layout
{
  public:
    // Absent/inactive input preserves the ordinary game path. Malformed active
    // data fails before a scene adopts the new frame, through native diagnostics.
    bool Load(INIFILE *input);
    bool Active() const { return active; }
    bool Publish(ATTRIBUTES &target) const;
    bool BindScene(ATTRIBUTES &frame, const std::string &region) const;
    const Region *FindRegion(const std::string &id) const;
    const Body *FindBody(const std::string &id) const;
    double MapMetres() const { return mapMetres; }
    const std::string &Identity() const { return recipeIdentity; }
    const std::vector<Body> &Bodies() const { return bodies; }
    const std::vector<Region> &Regions() const { return regions; }
    const std::vector<RouteGoal> &RouteGoals() const { return routeGoals; }
    const std::string &ChartBase() const { return chartBase; }
    const std::string &ChartPatch() const { return chartPatch; }

  private:
    bool active = false;
    double mapMetres = 50.;
    std::string recipeIdentity, inventoryIdentity;
    std::string chartBase, chartPatch;
    std::vector<Body> bodies;
    std::vector<Region> regions;
    std::vector<RouteGoal> routeGoals;
};
}
