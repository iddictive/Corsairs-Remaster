//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmIslands
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_islands.h"

#include <algorithm>
#include <cmath>

#include "core.h"
#include "math_inlines.h"

#include "wdm_camera.h"
#include "wdm_label_layout.h"
#include "wdm_render_levels.h"
#include "wdm_ship.h"
#include "world_map.h"
#include "string_compare.hpp"

// ============================================================================================
// Collision data
// ============================================================================================

CVECTOR WdmIslands::centPos;
int32_t WdmIslands::numEdges;
CVECTOR WdmIslands::curPos;
bool WdmIslands::checkMode;
CMatrix WdmIslands::curMatrix;

namespace
{

enum class OptionalPositionState
{
    Absent,
    Valid,
    Invalid
};

OptionalPositionState ReadOptionalPosition(ATTRIBUTES *attributes, const char *name, CVECTOR &position)
{
    ATTRIBUTES *positionAttributes = attributes->FindAClass(attributes, name);
    if (!positionAttributes)
        return OptionalPositionState::Absent;

    if (!positionAttributes->GetAttribute("x") || !positionAttributes->GetAttribute("z"))
        return OptionalPositionState::Invalid;

    position.x = positionAttributes->GetAttributeAsFloat("x", 0.0f);
    position.y = positionAttributes->GetAttributeAsFloat("y", 0.0f);
    position.z = positionAttributes->GetAttributeAsFloat("z", 0.0f);
    return OptionalPositionState::Valid;
}

void AppendDashedLine(std::vector<RS_LINE2D> &lines, float startX, float startY, float endX, float endY,
                      uint32_t color)
{
    const float dx = endX - startX;
    const float dy = endY - startY;
    const float length = std::sqrt(dx * dx + dy * dy);
    if (length < 1.0f)
        return;

    constexpr float dash = 5.0f;
    constexpr float gap = 4.0f;
    for (float offset = 0.0f; offset < length; offset += dash + gap)
    {
        const float from = offset / length;
        const float to = std::min(offset + dash, length) / length;
        lines.push_back({CVECTOR(startX + dx * from, startY + dy * from, 1.0f), 0.5f, color});
        lines.push_back({CVECTOR(startX + dx * to, startY + dy * to, 1.0f), 0.5f, color});
    }
}

} // namespace

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmIslands::WdmIslands()
{
    labelSort.reserve(1024);
    icons.texture = -1;
    LabelsRelease();
    wdmObjects->islands = this;
    // Loading the base model
    baseModel = static_cast<WdmRenderModel *>(
        wdmObjects->wm->CreateModel(new WdmRenderModel(), "islands\\islands", false, false, false, 1));
    if (!baseModel || !baseModel->geo)
        return;
    // Geometry information
    GEOS::INFO ginfo;
    GEOS::LABEL label;
    baseModel->geo->GetInfo(ginfo);
    // Finding the size of the world and iterating over the locators
    CVECTOR vmin, vmax, center = 0.0f, vmn, vmx;
    auto isMin = false, isMax = false;
    for (int32_t i = 0; i < ginfo.nlabels; i++)
    {
        baseModel->geo->GetLabel(i, label);
        if (!label.group_name || !label.group_name[0])
            continue;
        if (storm::iEquals(label.group_name, "locators"))
        {
            if (storm::iEquals(label.name, "min"))
            {
                vmn = ((CMatrix *)label.m)->Pos();
                isMin = true;
            }
            if (storm::iEquals(label.name, "max"))
            {
                vmx = ((CMatrix *)label.m)->Pos();
                isMax = true;
            }
        }
        else if (storm::iEquals(label.group_name, "merchant"))
        {
            merchants.push_back(((CMatrix *)label.m)->Pos());
        }
        else if (label.name && label.name[0] && storm::iEquals(label.group_name, "quests"))
        {
            quests.push_back(Quest{((CMatrix *)label.m)->Pos(), label.name});
            // Quest & q = quests[quests.Add()];
            // q.pos = ((CMatrix *)label.m)->Pos();
            // q.name = label.name;
        }
    }
    if (isMin && isMax)
    {
        vmin.x = std::min(vmn.x, vmx.x);
        vmin.y = 0.0f;
        vmin.z = std::min(vmn.z, vmx.z);
        vmax.x = std::max(vmn.x, vmx.x);
        vmax.y = 0.0f;
        vmax.z = std::max(vmn.z, vmx.z);
        center = (vmin + vmax) * -0.5f;
        vmin += center;
        vmax += center;
        wdmObjects->SetWorldSize(vmax.x * 2.0f, vmax.z * 2.0f);
    }
    // Reading the islands
    std::string name;
    for (int32_t i = 0; i < ginfo.nlabels; i++)
    {
        // receive information
        baseModel->geo->GetLabel(i, label);
        if (!label.group_name || !label.group_name[0])
            continue;
        if (!label.name || !label.name[0])
            continue;
        if (!storm::iEquals(label.group_name, "islands"))
            continue;
        // Skip if added
        int32_t j;
        for (j = 0; j < islands.size(); j++)
        {
            if (islands[j].modelName == label.name)
                break;
        }
        if (j < islands.size())
            continue;
        name = "islands\\";
        name += label.name;
        // Loading
        auto *model = static_cast<WdmRenderModel *>(
            wdmObjects->wm->CreateModel(new WdmRenderModel(), name.c_str(), false, false, true, 2,
                                        wdm::render_levels::Island));
        if (model)
        {
            // General
            islands.push_back(Islands{});
            // Islands & isl = islands[islands.Add()];
            auto &isl = islands.back();
            isl.model = model;
            model->mtx = *((CMatrix *)label.m);
            model->mtx.Pos() += center;
            isl.toLocal = model->mtx;
            isl.toLocal.Transposition();
            isl.modelName = label.name;
            isl.worldPosition = model->mtx.Pos();
            // Model describing the area of the island
            name += "_area";
            isl.area = static_cast<WdmRenderModel *>(
                wdmObjects->wm->CreateModel(new WdmRenderModel(), name.c_str(), false, false, false, 3));
            if (!isl.area)
            {
                core.Trace("World map: can't load model of island's area: %s", name.c_str());
            }
            // Palm trees
            name = "islands\\";
            name += label.name;
            name += "_palms";
            isl.palms = static_cast<WdmRenderModel *>(
                wdmObjects->wm->CreateModel(new WdmRenderModel(), name.c_str(), false, false, true, 4, 800));
            if (isl.palms)
            {
                static const auto *techName = "WdmModelDrawStdAlphaTest";
                isl.palms->SetTech(techName, techName);
            }
            // Foam
            name = "islands\\";
            name += label.name;
            name += "_waves";
            isl.waves = static_cast<WdmIslandWaves *>(
                wdmObjects->wm->CreateModel(new WdmIslandWaves(), name.c_str(), false, false, true, 4,
                                            wdm::render_levels::Island));
        }
        else
        {
            core.Trace("World map: can't load model of island: %s", name.c_str());
        }
    }
    // Loading the patch
    patch = new PtcData();
    if (!patch->Load("RESOURCE\\MODELS\\WorldMap\\islands\\islands_patch.ptc"))
    {
        delete patch;
        patch = nullptr;
    }
}

WdmIslands::~WdmIslands()
{
    if (patch)
    {
        delete patch;
        patch = nullptr;
    }
    wdmObjects->islands = nullptr;
    for (int32_t i = 0; i < islands.size(); i++)
    {
        wdmObjects->wm->DeleteObject(islands[i].model);
        islands[i].model = nullptr;
    }
    if (baseModel)
        wdmObjects->wm->DeleteObject(baseModel);
    islands.clear();
    LabelsRelease();
}

// Check for possible collision
bool WdmIslands::CollisionTest(CMatrix &objMtx, float length, float width, bool heighTest)
{
    const auto maxHeightInTest = 0.5f;
    // Radius of a rectangle on a plane
    const auto boxRadius = sqrtf(length * length + width * width);
    if (boxRadius < 0.0000001f)
        return false;
    // Box radius
    float checkRadius = boxRadius + maxHeightInTest;
    if (!heighTest)
        checkRadius += 1000000.0f;
    // Test parameters
    numEdges = 0;
    centPos = 0.0f;
    checkMode = !heighTest;
    // walk through all the islands
    for (int32_t i = 0; i < islands.size(); i++)
    {
        // The island we work with
        Islands &island = islands[i];
        // Transform to the local coordinates system
        CMatrix locMtx(objMtx, island.toLocal);

        // Check hitting the island sphere
        const float dist2 = ~(locMtx.Pos() - island.model->center);
        const float maxDist = boxRadius + island.model->radius;
        if (dist2 >= maxDist * maxDist)
            continue;
        // test triangles
        GEOS::PLANE p[5];
        // Direction vector
        p[0].nrm.x = locMtx.Vz().x;
        p[0].nrm.y = 0.0f;
        p[0].nrm.z = locMtx.Vz().z;
        float radius = p[0].nrm.x * p[0].nrm.x + p[0].nrm.z * p[0].nrm.z;
        // Skip broken model
        if (radius < 0.000001f)
            continue;
        // Direction normal
        radius = 1.0f / sqrtf(radius);
        p[0].nrm.x *= radius;
        p[0].nrm.z *= radius;
        // Remaining normals
        p[1].nrm.x = -p[0].nrm.x;
        p[1].nrm.y = 0.0f;
        p[1].nrm.z = -p[0].nrm.z;
        p[2].nrm.x = -p[0].nrm.z;
        p[2].nrm.y = 0.0f;
        p[2].nrm.z = p[0].nrm.x;
        p[3].nrm.x = p[0].nrm.z;
        p[3].nrm.y = 0.0f;
        p[3].nrm.z = -p[0].nrm.x;
        p[4].nrm.x = 0.0f;
        p[4].nrm.y = 1.0f;
        p[4].nrm.z = 0.0f;
        // Distances to planes
        p[0].d =
            (p[0].nrm.x * length + locMtx.Pos().x) * p[0].nrm.x + (p[0].nrm.z * length + locMtx.Pos().z) * p[0].nrm.z;
        p[1].d =
            (p[1].nrm.x * length + locMtx.Pos().x) * p[1].nrm.x + (p[1].nrm.z * length + locMtx.Pos().z) * p[1].nrm.z;
        p[2].d =
            (p[2].nrm.x * width + locMtx.Pos().x) * p[2].nrm.x + (p[2].nrm.z * width + locMtx.Pos().z) * p[2].nrm.z;
        p[3].d =
            (p[3].nrm.x * width + locMtx.Pos().x) * p[3].nrm.x + (p[3].nrm.z * width + locMtx.Pos().z) * p[3].nrm.z;
        p[4].d = maxHeightInTest;
        // Sphere position
        GEOS::VERTEX v;
        v.x = locMtx.Pos().x;
        v.y = locMtx.Pos().y;
        v.z = locMtx.Pos().z;
        curPos = locMtx.Pos();
        // Conversion matrix to world coordinates
        curMatrix = island.model->mtx;
        // looking for hitting polygons
        island.model->geo->Clip(p, heighTest ? 5 : 4, v, checkRadius, AddEdges);
    }
    if (numEdges)
    {
        centPos *= 1.0f / numEdges;
        return true;
    }
    return false;
}

bool WdmIslands::AddEdges(const GEOS::VERTEX *vrt, int32_t numVrt)
{
    // Skip broken Data
    if (numVrt < 3)
        return true;
    if (checkMode)
    {
        centPos = curMatrix * CVECTOR(vrt[0].x, 0.0f, vrt[0].z);
        numEdges = 1;
        return false;
    }
    // try to determine the location of the point relative to the plane
    const CVECTOR v1(vrt[1].x - vrt[0].x, vrt[1].y - vrt[0].y, vrt[1].z - vrt[0].z);
    const CVECTOR v2(vrt[2].x - vrt[1].x, vrt[2].y - vrt[1].y, vrt[2].z - vrt[1].z);
    const CVECTOR v = v1 ^ v2;
    const float d = (v | curPos) - (v | CVECTOR(vrt[0].x, vrt[0].y, vrt[0].z));
    if (d < 0)
        return true;
    // Adding edges
    numEdges += numVrt;
    for (int32_t i = 0; i < numVrt; i++)
    {
        centPos += curMatrix * CVECTOR(vrt[i].x, 0.0f, vrt[i].z);
    }
    // continue
    return true;
}

// Check for the presence of triangles in this place
bool WdmIslands::ObstacleTest(float x, float z, float radius)
{
    if (radius <= 0.0f)
        return false;
    const CVECTOR wPos(x, 0.0f, z);
    // walk through all the islands
    for (int32_t i = 0; i < islands.size(); i++)
    {
        // The island we work with
        Islands &island = islands[i];
        // Converting to the local island system
        const CVECTOR pos = island.toLocal * wPos;
        GEOS::PLANE p[4];
        p[0].nrm.x = 0.0f;
        p[0].nrm.y = 0.0f;
        p[0].nrm.z = 1.0f;
        p[0].d = pos.z + radius;
        p[1].nrm.x = 0.0f;
        p[1].nrm.y = 0.0f;
        p[1].nrm.z = -1.0f;
        p[1].d = -(pos.z - radius);
        p[2].nrm.x = 1.0f;
        p[2].nrm.y = 0.0f;
        p[2].nrm.z = 0.0f;
        p[2].d = pos.x + radius;
        p[3].nrm.x = -1.0f;
        p[3].nrm.y = 0.0f;
        p[3].nrm.z = 0.0f;
        p[3].d = -(pos.x - radius);
        GEOS::VERTEX vrt;
        vrt.x = pos.x;
        vrt.y = 0.0f;
        vrt.z = pos.z;
        if (island.model->geo->Clip(p, 4, vrt, 50.0f, nullptr))
            return true;
    }
    return false;
}

// Read Island Data
void WdmIslands::SetIslandsData(ATTRIBUTES *apnt, bool isChange)
{
    if (!isChange)
    {
        LabelsRelease();
        // Reading the parameters of the pictures
        LabelsReadIconParams(apnt);
    }
    if (!apnt)
        return;
    // Get the description of the labels
    apnt = apnt->FindAClass(apnt, "labels");
    if (!apnt)
        return;
    // Number of labels
    const uint32_t numAttr = apnt->GetAttributesNum();
    // Looping through all labels
    for (uint32_t i = 0; i < numAttr; i++)
    {
        // get access to the description of the label
        ATTRIBUTES *a = apnt->GetAttributeClass(i);
        if (!a)
            continue;
        // Get the parameters of the label
        const char *id = a->GetAttribute("id");
        const char *locator = a->GetAttribute("locator");
        const char *text = a->GetAttribute("text");
        const int32_t icon = static_cast<int32_t>(a->GetAttributeAsDword("icon", static_cast<uint32_t>(-1)));
        const char *font = a->GetAttribute("font");
        const float pivotX = a->GetAttributeAsFloat("pivotX", -0.5f);
        const float pivotY = a->GetAttributeAsFloat("pivotY", -0.5f);
        const float heightView = a->GetAttributeAsFloat("heightView", 250.0);
        const uint32_t weight = a->GetAttributeAsDword("weight", 0);
        CVECTOR pos{};
        const OptionalPositionState anchorState = ReadOptionalPosition(a, "anchor", pos);
        // Check for sufficiency
        if (!id || !text || anchorState == OptionalPositionState::Invalid)
        {
            core.Trace("World map: label \"%s\" will be skipping...", apnt->GetAttributeName(i));
            continue;
        }
        if (anchorState == OptionalPositionState::Absent && (!locator || !locator[0] || !LabelsFindLocator(locator, pos)))
        {
            core.Trace("World map: locator \"%s\" in label \"%s\" not found...", locator ? locator : "",
                       apnt->GetAttributeName(i));
            continue;
        }
        // looking for a label among existing
        const uint32_t hash = MakeHashValue(id);
        int32_t index = LabelsFind(id, hash);
        if (index < 0)
        {
            // Adding a new label
            const int32_t iEntry = hash & (sizeof(labelsEntry) / sizeof(labelsEntry[0]) - 1);
            // index = labels.Add();
            labels.push_back(Label{});
            index = labels.size() - 1;
            labels[index].id = id;
            labels[index].idHash = hash;
            labels[index].next = labelsEntry[iEntry];
            labelsEntry[iEntry] = index;
            labels[index].alpha = 255.0f;
        }
        labels[index].locatorName = anchorState == OptionalPositionState::Valid ? "" : locator;
        // Text
        labels[index].text = text ? text : ""; // ~!~
        // Font
        labels[index].font = LabelsAddFont(font);
        // Picture
        labels[index].icon = icon < static_cast<int32_t>(icons.num) ? icon : icons.num - 1;
        // Position
        labels[index].pos = pos;
        // Blanking height
        labels[index].heightView = heightView;
        labels[index].alwaysVisible = a->GetAttributeAsDword("alwaysVisible", 0) != 0;
        labels[index].weight = weight;
        labels[index].dangerZone = a->GetAttributeAsDword("dangerZone", 0) != 0;
        labels[index].dangerRadius = std::max(a->GetAttributeAsFloat("dangerRadius", 18.0f), 0.0f);
        labels[index].dangerAlpha = std::clamp(a->GetAttributeAsFloat("dangerAlpha", 0.16f), 0.0f, 1.0f);
        labels[index].dangerPos = pos;
        CVECTOR dangerPos{};
        const OptionalPositionState dangerAnchorState = ReadOptionalPosition(a, "dangerAnchor", dangerPos);
        const char *dangerLocator = a->GetAttribute("dangerLocator");
        if (dangerAnchorState == OptionalPositionState::Valid)
        {
            labels[index].dangerPos = dangerPos;
        }
        else if (dangerAnchorState == OptionalPositionState::Invalid)
        {
            labels[index].dangerZone = false;
            core.Trace("World map: danger anchor in label \"%s\" is incomplete...", apnt->GetAttributeName(i));
        }
        else if (dangerLocator && dangerLocator[0])
        {
            if (LabelsFindLocator(dangerLocator, dangerPos))
                labels[index].dangerPos = dangerPos;
            else
            {
                labels[index].dangerZone = false;
                core.Trace("World map: danger locator \"%s\" in label \"%s\" not found...", dangerLocator,
                           apnt->GetAttributeName(i));
            }
        }
        labels[index].hasLeader = false;
        CVECTOR leaderPos{};
        const OptionalPositionState leaderAnchorState = ReadOptionalPosition(a, "leaderAnchor", leaderPos);
        const char *leaderLocator = a->GetAttribute("leaderLocator");
        if (leaderAnchorState == OptionalPositionState::Valid)
        {
            labels[index].leaderPos = leaderPos;
            labels[index].hasLeader = true;
        }
        else if (leaderAnchorState == OptionalPositionState::Invalid)
        {
            core.Trace("World map: leader anchor in label \"%s\" is incomplete...", apnt->GetAttributeName(i));
        }
        else if (leaderLocator && leaderLocator[0])
        {
            if (LabelsFindLocator(leaderLocator, leaderPos))
            {
                labels[index].leaderPos = leaderPos;
                labels[index].hasLeader = true;
            }
            else
            {
                core.Trace("World map: leader locator \"%s\" in label \"%s\" not found...", leaderLocator,
                           apnt->GetAttributeName(i));
            }
        }
        if (dangerAnchorState == OptionalPositionState::Absent && (!dangerLocator || !dangerLocator[0]) &&
            labels[index].hasLeader)
            labels[index].dangerPos = labels[index].leaderPos;
        // Line Sizes
        const float textWidth =
            static_cast<float>(wdmObjects->rs->StringWidth((char *)labels[index].text.c_str(), labels[index].font));
        float textHeight = static_cast<float>(wdmObjects->rs->CharHeight(labels[index].font));
        // Label dimensions
        const float labelWidth = textWidth + (labels[index].icon >= 0 ? icons.w + 4.0f : 0.0f);
        const float labelHeight = labels[index].icon >= 0 ? std::max(textHeight, icons.h) : textHeight;
        // Position of the label taking into account the offset
        const float labelX = pivotX * labelWidth;
        const float labelY = pivotY * labelHeight;
        // Offset rectangle
        labels[index].dl = labelX;
        labels[index].dt = labelY;
        labels[index].dr = labelX + labelWidth;
        labels[index].db = labelY + labelHeight;
        if (labels[index].icon >= 0)
        {
            // Picture position
            labels[index].iconX = 0.0f;
            labels[index].iconY = (labelHeight - icons.h) * 0.5f;
            // Text position
            labels[index].textX = icons.w + 4.0f;
            labels[index].textY = (labelHeight - textHeight) * 0.5f;
        }
        else
        {
            // Picture position
            labels[index].iconX = 0.0f;
            labels[index].iconY = 0.0f;
            // Text position
            labels[index].textX = 0.0f;
            labels[index].textY = 0.0f;
        }

        // Store locator position as label attribute
        auto* position_attr = a->VerifyAttributeClass("position");
        position_attr->SetAttributeUseFloat("x", pos.x);
        position_attr->SetAttributeUseFloat("y", pos.y);
        position_attr->SetAttributeUseFloat("z", pos.z);
    }
}

void WdmIslands::LabelsReadIconParams(ATTRIBUTES *apnt)
{
    if (!apnt)
        return;
    apnt = apnt->FindAClass(apnt, "icon");
    if (!apnt)
        return;
    icons.w = static_cast<float>(apnt->GetAttributeAsDword("width", 32));
    icons.h = static_cast<float>(apnt->GetAttributeAsDword("height", 32));
    icons.num = apnt->GetAttributeAsDword("num", 8);
    icons.frames = apnt->GetAttributeAsDword("frames", 1);
    icons.fps = apnt->GetAttributeAsFloat("fps", 0.0f);
    icons.frame = 0.0f;
    icons.f[0] = 0.0f;
    icons.f[1] = 0.0f;
    icons.blend = 0;
    const char *texName = apnt->GetAttribute("texture");
    if (!texName)
        texName = "";
    std::string name = "WorldMap\\Interfaces\\";
    name += texName;
    icons.texture = wdmObjects->rs->TextureCreate(name.c_str());
    // Dimensions uv
    const float tw = icons.w * icons.frames;
    const float th = icons.h * icons.num;
    if (tw)
    {
        icons.u = icons.w / tw;
    }
    else
    {
        icons.u = 0.0f;
    }
    if (th)
    {
        icons.v = icons.h / th;
    }
    else
    {
        icons.v = 0.0f;
    }
}

int32_t WdmIslands::LabelsFind(const char *id, uint32_t hash)
{
    if (!id)
        id = "";
    int32_t i = hash & (sizeof(labelsEntry) / sizeof(labelsEntry[0]) - 1);
    for (i = labelsEntry[i]; i >= 0; i = labels[i].next)
    {
        if (labels[i].idHash == hash)
        {
            if (storm::iEquals(labels[i].id, id))
                return i;
        }
    }
    return -1;
}

bool WdmIslands::LabelsFindLocator(const char *name, CVECTOR &pos) const
{
    if (!baseModel || !name || !name[0])
        return false;
    // Geometry information
    GEOS::INFO ginfo;
    GEOS::LABEL label;
    baseModel->geo->GetInfo(ginfo);
    for (int32_t i = 0; i < ginfo.nlabels; i++)
    {
        baseModel->geo->GetLabel(i, label);
        if (!label.group_name || !label.group_name[0])
            continue;
        if (!storm::iEquals(label.group_name, "labels"))
            continue;
        // if(!storm::iEquals(label.group_name, "geometry")) continue;
        if (storm::iEquals(label.name, name))
        {
            pos = ((CMatrix *)label.m)->Pos();
            return true;
        }
    }
    return false;
}

int32_t WdmIslands::LabelsAddFont(const char *name)
{
    if (!name)
        name = "";
    for (int32_t i = 0; i < fonts.size(); i++)
    {
        if (storm::iEquals(fonts[i].name, name))
        {
            return fonts[i].id;
        }
    }
    // Font & font = fonts[index = fonts.Add()];
    Font font;
    font.name = name;
    font.id = wdmObjects->rs->LoadFont((char *)name);
    if (font.id < 0)
        font.id = FONT_DEFAULT;
    fonts.push_back(font);
    return fonts.size() - 1;
}

void WdmIslands::LabelsRelease()
{
    // Remove all labels
    labels.clear();
    for (int32_t i = 0; i < sizeof(labelsEntry) / sizeof(labelsEntry[0]); i++)
    {
        labelsEntry[i] = -1;
    }
    // Remove all fonts
    for (int32_t i = 0; i < fonts.size(); i++)
    {
        if (fonts[i].id != FONT_DEFAULT)
        {
            wdmObjects->rs->UnloadFont(fonts[i].id);
        }
    }
    // Delete all pictures
    if (icons.texture >= 0)
    {
        wdmObjects->rs->TextureRelease(icons.texture);
    }
    memset(&icons, 0, sizeof(icons));
    icons.texture = -1;
}

void WdmIslands::Update(float dltTime)
{
    if (wdmObjects->playerShip)
    {
        CVECTOR pos;
        wdmObjects->playerShip->GetPosition(pos.x, pos.z, pos.y);
        pos.y = 0.0f;
        for (int32_t i = 0; i < islands.size(); i++)
        {
            if (IsShipInArea(i, pos))
            {
                if (wdmObjects->wm->AttributesPointer)
                {
                    wdmObjects->wm->AttributesPointer->SetAttribute("island", (char *)islands[i].modelName.c_str());
                    ATTRIBUTES *a =
                        wdmObjects->wm->AttributesPointer->FindAClass(wdmObjects->wm->AttributesPointer, "island");
                    if (a)
                    {
                        a->SetAttributeUseFloat("x", islands[i].worldPosition.x);
                        a->SetAttributeUseFloat("z", islands[i].worldPosition.z);
                    }
                    wdmObjects->curIsland = (char *)islands[i].modelName.c_str();
                }
                return;
            }
        }
    }
    wdmObjects->curIsland = nullptr;
    wdmObjects->wm->AttributesPointer->SetAttribute("island", "");
    ATTRIBUTES *a = wdmObjects->wm->AttributesPointer->FindAClass(wdmObjects->wm->AttributesPointer, "island");
    if (a)
    {
        a->SetAttributeUseFloat("x", 0.0f);
        a->SetAttributeUseFloat("z", 0.0f);
    }
}

void WdmIslands::LRender(VDX9RENDER *rs)
{
    // Draw a patch if needed
    if (wdmObjects->isDebug)
    {
        if (patch && core.Controls->GetDebugAsyncKeyState('1') < 0)
        {
            patch->DebugDraw(rs, core.GetDeltaTime() * 0.001f);
        }
    }
    // Update the state of the pictures
    icons.frame += core.GetDeltaTime() * 0.001f * icons.fps;
    icons.frame = (icons.frame / icons.frames - static_cast<int32_t>(icons.frame / icons.frames)) * icons.frames;
    if (icons.frame < 0.0f)
    {
        icons.frame += icons.frames;
    }
    icons.f[0] = static_cast<float>(static_cast<int32_t>(icons.frame));
    icons.blend = static_cast<int32_t>((icons.frame - icons.f[0]) * 255.0f);
    if (icons.blend < 0)
        icons.blend = 0;
    if (icons.blend > 255)
        icons.blend = 255;
    icons.blend |= (icons.blend << 24) | (icons.blend << 16) | (icons.blend << 8);
    if (icons.f[0] < 0)
        icons.f[0] = icons.frames - 1.0f;
    if (icons.f[0] >= icons.frames)
        icons.f[0] = 0.0f;
    icons.f[1] = icons.f[0] + 1;
    if (icons.f[1] < 0)
        icons.f[1] = icons.frames - 1.0f;
    if (icons.f[1] >= icons.frames)
        icons.f[1] = 0.0f;
    icons.f[0] *= icons.u;
    icons.f[1] *= icons.u;
    // get the current transformation matrix
    static CMatrix mtx, view, prj;
    rs->GetTransform(D3DTS_VIEW, view);
    rs->GetTransform(D3DTS_PROJECTION, prj);
    mtx.EqMultiply(view, prj);
    // Get the current vp sizes
    static D3DVIEWPORT9 vp;
    rs->GetViewport(&vp);
    const auto w = static_cast<float>(vp.Width);
    const auto h = static_cast<float>(vp.Height);
    // Get the height of the camera
    const float cameraHeight = wdmObjects->camera->realHeight;
    const float dAlpha = core.GetDeltaTime() * (0.001f * 1.5f * 255.0f);
    // Projecting to the screen
    labelSort.clear();
    MTX_PRJ_VECTOR prjVertex;
    for (int32_t i = 0; i < labels.size(); i++)
    {
        // Label
        Label &label = labels[i];
        // Label transparency
        if (label.alwaysVisible)
        {
            label.alpha = 255.0f;
        }
        else if (label.heightView > cameraHeight)
        {
            label.alpha += dAlpha;
            if (label.alpha > 255.0f)
                label.alpha = 255.0f;
        }
        else
        {
            label.alpha -= dAlpha;
            if (label.alpha < 0.0f)
                label.alpha = 0.0f;
        }
        // If the label is not visible, then do nothing
        if (label.alpha < 1.0f)
            continue;
        // calculate the projection point
        // rs->DrawSphere(label.pos, 1.0f, 0xff00ff00);
        mtx.Projection(&label.pos, &prjVertex, 1, w * 0.5f, h * 0.5f, 0, 0);
        // Rectangle on screen
        label.l = prjVertex.x + label.dl;
        label.t = prjVertex.y + label.dt;
        label.r = prjVertex.x + label.dr;
        label.b = prjVertex.y + label.db;
        // If the label is not visible, then delete the entry
        if (label.l >= w || label.t >= h || label.r < 0.0f || label.b < 0.0f)
        {
            continue;
        }
        labelSort.push_back(i);
    }

    std::vector<wdm::labels::LayoutItem> layout;
    layout.reserve(labelSort.size());
    for (const int32_t index : labelSort)
    {
        Label &label = labels[index];
        layout.push_back({index, label.weight, {label.l, label.t, label.r, label.b}, {}});
    }
    wdm::labels::ResolveCollisions(layout, w, h);
    for (const wdm::labels::LayoutItem &item : layout)
    {
        Label &label = labels[item.index];
        label.l = item.placed.left;
        label.t = item.placed.top;
        label.r = item.placed.right;
        label.b = item.placed.bottom;
    }

    std::vector<RS_LINE2D> leaderLines;
    for (const int32_t index : labelSort)
    {
        Label &label = labels[index];
        if (!label.hasLeader)
            continue;
        MTX_PRJ_VECTOR leaderVertex;
        mtx.Projection(&label.leaderPos, &leaderVertex, 1, w * 0.5f, h * 0.5f, 0, 0);
        const float startX = std::clamp(leaderVertex.x, 0.0f, w);
        const float startY = std::clamp(leaderVertex.y, 0.0f, h);
        const float endX = std::clamp(startX, label.l, label.r);
        const float endY = std::clamp(startY, label.t, label.b);
        const uint32_t alpha = static_cast<uint32_t>(std::min(label.alpha, 190.0f));
        AppendDashedLine(leaderLines, startX, startY, endX, endY, (alpha << 24) | 0x00c69c4a);
    }
    if (!leaderLines.empty())
        rs->DrawLines2D(leaderLines.data(), leaderLines.size() / 2, "Line");

    // Draw icons and write text
    for (int32_t i = 0; i < labelSort.size(); i++)
    {
        // Label
        Label &label = labels[labelSort[i]];
        // writing a text
        const uint32_t color = (static_cast<int32_t>(label.alpha) << 24) | 0xffffff;
        rs->Print(label.font, color, static_cast<int32_t>(label.l + label.textX), static_cast<int32_t>(label.t + label.textY),
                  (char *)label.text.c_str());
        // Draw a picture
        if (label.icon < 0)
            continue;
        rs->TextureSet(0, icons.texture);
        rs->TextureSet(1, icons.texture);
        rs->SetRenderState(D3DRS_TEXTUREFACTOR, static_cast<uint32_t>(icons.blend));
        // Filling the vertices
        static struct
        {
            float x, y, z, rhw;
            uint32_t c;
            float tu1, tv1;
            float tu2, tv2;
        } drawbuf[4];
        // 0
        drawbuf[0].x = label.l + label.iconX;
        drawbuf[0].y = label.t + label.iconY;
        drawbuf[0].z = 0.0f;
        drawbuf[0].rhw = 0.5f;
        drawbuf[0].c = color;
        drawbuf[0].tu1 = icons.f[0];
        drawbuf[0].tv1 = label.icon * icons.v;
        drawbuf[0].tu2 = icons.f[1];
        drawbuf[0].tv2 = drawbuf[0].tv1;
        // 1
        drawbuf[1].x = drawbuf[0].x;
        drawbuf[1].y = drawbuf[0].y + icons.h;
        drawbuf[1].z = 0.0f;
        drawbuf[1].rhw = 0.5f;
        drawbuf[1].c = color;
        drawbuf[1].tu1 = drawbuf[0].tu1;
        drawbuf[1].tv1 = drawbuf[0].tv1 + icons.v;
        drawbuf[1].tu2 = drawbuf[0].tu2;
        drawbuf[1].tv2 = drawbuf[1].tv1;
        // 2
        drawbuf[2].x = drawbuf[0].x + icons.w;
        drawbuf[2].y = drawbuf[0].y;
        drawbuf[2].z = 0.0f;
        drawbuf[2].rhw = 0.5f;
        drawbuf[2].c = color;
        drawbuf[2].tu1 = drawbuf[0].tu1 + icons.u;
        drawbuf[2].tv1 = drawbuf[0].tv1;
        drawbuf[2].tu2 = drawbuf[0].tu2 + icons.u;
        drawbuf[2].tv2 = drawbuf[2].tv1;
        // 3
        drawbuf[3].x = drawbuf[0].x + icons.w;
        drawbuf[3].y = drawbuf[0].y + icons.h;
        drawbuf[3].z = 0.0f;
        drawbuf[3].rhw = 0.5f;
        drawbuf[3].c = color;
        drawbuf[3].tu1 = drawbuf[2].tu1;
        drawbuf[3].tv1 = drawbuf[0].tv1 + icons.v;
        drawbuf[3].tu2 = drawbuf[2].tu2;
        drawbuf[3].tv2 = drawbuf[3].tv1;
        // draw the result
        rs->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2, 2, drawbuf,
                            sizeof(drawbuf[0]), "WdmDrawLabelIcon");
    }
}

void WdmIslands::RenderDangerZones(VDX9RENDER *rs) const
{
    struct DangerVertex
    {
        CVECTOR pos;
        uint32_t color;
    };

    constexpr int32_t segments = 48;
    constexpr float pi = 3.14159265358979323846f;
    DangerVertex vertices[segments + 2];
    const CMatrix identity;

    rs->TextureSet(0, -1);
    rs->TextureSet(1, -1);
    rs->SetTransform(D3DTS_WORLD, identity);

    for (const Label &label : labels)
    {
        if (!label.dangerZone || label.dangerRadius <= 0.0f || label.dangerAlpha <= 0.0f)
            continue;

        const uint32_t alpha = static_cast<uint32_t>(label.dangerAlpha * 255.0f);
        vertices[0].pos = CVECTOR(label.dangerPos.x, 0.35f, label.dangerPos.z);
        vertices[0].color = (alpha << 24) | 0x00b51f24;
        for (int32_t i = 0; i <= segments; ++i)
        {
            const float angle = 2.0f * pi * static_cast<float>(i) / static_cast<float>(segments);
            vertices[i + 1].pos = CVECTOR(label.dangerPos.x + std::sin(angle) * label.dangerRadius, 0.35f,
                                          label.dangerPos.z + std::cos(angle) * label.dangerRadius);
            vertices[i + 1].color = 0x00b51f24;
        }
        rs->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, D3DFVF_XYZ | D3DFVF_DIFFUSE, segments, vertices,
                            sizeof(vertices[0]), "WdmPortDangerZone");
    }
}

void WdmPortDangerZones::LRender(VDX9RENDER *rs)
{
    if (islands)
        islands->RenderDangerZones(rs);
}

// Find the direction to arrive at a given destination from the current
void WdmIslands::FindDirection(const CVECTOR &position, const CVECTOR &destination, CVECTOR &direction) const
{
    // If there is no patch, then go in a straight line
    if (!patch)
    {
        direction = Norm2D(destination - position);
        return;
    }
    // trying to determine the node on which we are standing and the node to which we need to sail
    float y = 0.0f;
    const int32_t from = patch->FindNode(position, y);
    y = 0.0f;
    int32_t to = patch->FindNode(destination, y);
    if (from < 0 || to < 0)
    {
        direction = Norm2D(destination - position);
        return;
    }
    // Getting directions
    if (!patch->FindPathDir(from, position, to, destination, to, direction))
    {
        direction = Norm2D(destination - position);
        return;
    }
    direction = Norm2D(direction - position);
}

// Find the repulsive force
void WdmIslands::FindReaction(const CVECTOR &position, CVECTOR &reaction) const
{
    if (patch)
    {
        float y = 0.0f;
        const int32_t n = patch->FindNode(position, y);
        if (n >= 0)
        {
            patch->FindForce(n, position, 20.0f, reaction);
        }
        else
        {
            reaction = 0.0f;
        }
    }
    else
    {
        reaction = 0.0f;
    }
}

// Find a random point for a merchant
bool WdmIslands::GetRandomMerchantPoint(CVECTOR &p)
{
    if (merchants.size() <= 0)
    {
        p = 0.0f;
        return false;
    }
    p = merchants[rand() % merchants.size()];
    return true;
}

// Get the coordinates of the quest locator
bool WdmIslands::GetQuestLocator(const char *locName, CVECTOR &p)
{
    if (!locName || !locName[0])
    {
        p = 0.0f;
        return false;
    }
    for (int32_t i = 0; i < quests.size(); i++)
    {
        if (storm::iEquals(quests[i].name, std::string_view(locName)))
        {
            p = quests[i].pos;
            return true;
        }
    }
    p = 0.0f;
    return false;
}

// Check if the ship is in the island zone
bool WdmIslands::CheckIslandArea(const char *islandName, float x, float z)
{
    for (int32_t i = 0; i < islands.size(); i++)
    {
        if (!islands[i].area)
            continue;
        if (islands[i].modelName == islandName)
        {
            return IsShipInArea(i, CVECTOR(x, 0.0f, z));
        }
    }
    return false;
}

// Get the closest point to the island zone
void WdmIslands::GetNearPointToArea(const char *islandName, float &x, float &z)
{
    // Looking for an area
    int32_t i;
    for (i = 0; i < islands.size(); i++)
    {
        if (!islands[i].area)
            continue;
        if (islands[i].modelName == islandName)
        {
            break;
        }
    }
    if (i >= islands.size())
    {
        return;
    }
    // iterate over the edges choosing the nearest point
    curPos.x = x;
    curPos.y = 0.0f;
    curPos.z = z;
    checkMode = false;
    GEOS::VERTEX v;
    v.x = 0.0f;
    v.y = 0.0f;
    v.z = 0.0f;
    islands[i].area->geo->Clip(nullptr, 0, v,
                               wdmObjects->worldSizeX * wdmObjects->worldSizeX +
                                   wdmObjects->worldSizeZ * wdmObjects->worldSizeZ,
                               FindNearPoint);
    if (checkMode)
    {
        x = centPos.x;
        z = centPos.z;
    }
}

bool WdmIslands::FindNearPoint(const GEOS::VERTEX *vrt, int32_t numVrt)
{
    // Skip broken Data
    for (int32_t i = 0; i < numVrt; i++)
    {
        const float dx = vrt[i].x - curPos.x;
        const float dz = vrt[i].z - curPos.z;
        const float dst = dx * dx + dz * dz;
        if (!checkMode || dst < centPos.y)
        {
            checkMode = true;
            centPos.x = vrt[i].x;
            centPos.z = vrt[i].z;
            centPos.y = dst;
        }
    }
    return true;
}

bool WdmIslands::IsShipInArea(int32_t islIndex, const CVECTOR &pos)
{
    static const float testRadius = 0.01f;
    // No area, no intersection
    if (!islands[islIndex].area)
        return false;
    // Checking the contact of the radii
    const float dist2 = ~(pos - islands[islIndex].area->center);
    const float rad = testRadius + islands[islIndex].area->radius;
    if (dist2 > rad * rad)
        return false;
    // Vertical planes
    GEOS::PLANE p[4];
    p[0].nrm.x = 0.0f;
    p[0].nrm.y = 0.0f;
    p[0].nrm.z = 1.0f;
    p[0].d = pos.z + testRadius;
    p[1].nrm.x = 0.0f;
    p[1].nrm.y = 0.0f;
    p[1].nrm.z = -1.0f;
    p[1].d = -(pos.z - testRadius);
    p[2].nrm.x = 1.0f;
    p[2].nrm.y = 0.0f;
    p[2].nrm.z = 0.0f;
    p[2].d = pos.x + testRadius;
    p[3].nrm.x = -1.0f;
    p[3].nrm.y = 0.0f;
    p[3].nrm.z = 0.0f;
    p[3].d = -(pos.x - testRadius);
    GEOS::VERTEX vrt;
    vrt.x = pos.x;
    vrt.y = 0.0f;
    vrt.z = pos.z;
    return islands[islIndex].area->geo->Clip(p, 4, vrt, 1000.0f, nullptr);
}

WdmIslandWaves::WdmIslandWaves()
{
    phase = rand() * (1.0f / RAND_MAX);
}

void WdmIslandWaves::Update(float dltTime)
{
    phase += dltTime * 0.1f;
    while (phase > 1.0f)
        phase -= 1.0f;
}

void WdmIslandWaves::LRender(VDX9RENDER *rs)
{
    const float fullPeriod = 0.8f;
    float k = phase * (1.0f / fullPeriod);
    if (k <= 1.0f)
    {
        Render(rs, k);
    }
    if (phase >= 0.5f)
    {
        k = phase - 0.5f;
    }
    else
    {
        k = phase + 0.5f;
    }
    k *= (1.0f / fullPeriod);
    if (k <= 1.0f)
    {
        Render(rs, k);
    }
}

void WdmIslandWaves::Render(VDX9RENDER *rs, float k)
{
    // Setting the matrix for the texture
    // k = 0.5f + 0.5f * sinf (k * 2.0f * PI);
    CMatrix mtx;
    mtx.m[1][1] = 0.3f + 1.0f * sinf(k * PI);
    mtx.m[2][1] = 0.0f; // 0.2f*sinf((k - 0.25f)*2.0f*PI) - 0.5f;
    rs->SetTransform(D3DTS_TEXTURE0, mtx);
    // set the overall transparency
    const float a = sinf(k * PI) * (1.0f - k * 0.5f) * 1.25f;
    // Draw the model
    SetTech("WdmIslandWaves", nullptr);
    rs->SetRenderState(D3DRS_TEXTUREFACTOR, (static_cast<int32_t>(a * a * a * a * 255.0f) << 24) | 0xffffff);
    WdmRenderModel::LRender(rs);
}
