/******************************************************************************
File:  geom_static.cpp

Author:   Nick Chirkov
Copyright (C) 2000, 2001 Nick Chirkov

Comments:
Import library main file
******************************************************************************/
#include "geom.h"
#ifdef STORM_METAL_ROCKK2
#include "rockk2_stochastic.hpp"
#endif
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
#include "island_geometry.hpp"
#include "weather_terrain.hpp"
#include <cstdlib>
#include <cctype>
#endif
#include <storm/config.hpp>

#include <cstdint>
#include <cstring>
#include <fmt/format.h>
#include <vector>
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
#include <unordered_map>
#endif

#include "../../util/include/string_compare.hpp"

namespace
{
std::vector<uint32_t> getColData(GEOM_SERVICE &srv, const std::string_view &file_name)
{
    std::vector<uint32_t> result;

    auto ltfl = srv.OpenFile(file_name.data());
    if (ltfl.is_open())
    {
        const auto file_size = srv.FileSize(file_name.data());
        if (file_size > 0)
        {
            result.resize(file_size / sizeof(uint32_t));
            srv.ReadFile(ltfl, result.data(), file_size);
        }
    }
    srv.CloseFile(ltfl);

    return result;
}
} // namespace

// create geometry func
GEOS *CreateGeometry(const char *fname, const char *lightname, GEOM_SERVICE &srv, int32_t flags)
{
    return new GEOM(fname, lightname, srv, flags);
}

// geometry constructor does all init
GEOM::GEOM(const char *fname, const char *lightname, GEOM_SERVICE &_srv, int32_t flags) : srv(_srv)
{
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
    std::string islandPath = fname ? fname : "";
    for (char& c : islandPath) c = c == '\\' ? '/' : char(std::tolower(static_cast<unsigned char>(c)));
    const char* islandMode = std::getenv("STORM_METAL_ISLAND_GEOMETRY");
    const bool captureIsland = island_geometry::islandModelPath(islandPath);
    const bool refineIsland = islandMode && !std::strcmp(islandMode,"1") && captureIsland;
    std::vector<RDF_TRIANGLE> islandTriangles;
    std::vector<std::vector<uint8_t>> islandVertices;
#endif
    std::vector<uint32_t> colData;
    if (lightname != nullptr)
    {
        colData = getColData(srv, lightname);
    }

    auto file = srv.OpenFile(fname);
    // read header
    srv.ReadFile(file, &rhead, sizeof(RDF_HEAD));
    if (rhead.version != RDF_VERSION)
        throw std::runtime_error("invalid version");

    // read names
    globname = static_cast<char *>(srv.malloc(rhead.name_size));
    srv.ReadFile(file, globname, rhead.name_size);

    names = static_cast<int32_t *>(srv.malloc(rhead.names * sizeof(int32_t)));
    srv.ReadFile(file, names, rhead.names * sizeof(int32_t));

    // load textures
    tname = static_cast<int32_t *>(srv.malloc(rhead.ntextures * sizeof(int32_t)));
    tlookup = static_cast<int32_t *>(srv.malloc(rhead.ntextures * sizeof(int32_t)));
    srv.ReadFile(file, tname, rhead.ntextures * sizeof(int32_t));

    // read materials
    auto *rmat = static_cast<RDF_MATERIAL *>(srv.malloc(sizeof(RDF_MATERIAL) * rhead.nmaterials));
    srv.ReadFile(file, rmat, sizeof(RDF_MATERIAL) * rhead.nmaterials);
    material = static_cast<MATERIAL *>(srv.malloc(sizeof(MATERIAL) * rhead.nmaterials));

    // read lights
    auto *rlig = static_cast<RDF_LIGHT *>(srv.malloc(sizeof(RDF_LIGHT) * rhead.nlights));
    srv.ReadFile(file, rlig, sizeof(RDF_LIGHT) * rhead.nlights);
    light = static_cast<LIGHT *>(srv.malloc(sizeof(LIGHT) * rhead.nlights));
    for (int32_t l = 0; l < rhead.nlights; l++)
    {
        light[l].flags = rlig[l].flags;
        light[l].type = static_cast<LIGHT_TYPE>(rlig[l].type);
        light[l].name = &globname[rlig[l].name];
        light[l].r = rlig[l].r;
        light[l].g = rlig[l].g;
        light[l].b = rlig[l].b;
        light[l].range = rlig[l].range;
        light[l].pos.x = rlig[l].pos.x;
        light[l].pos.y = rlig[l].pos.y;
        light[l].pos.z = rlig[l].pos.z;
        light[l].atten[0] = rlig[l].atten[0];
        light[l].atten[1] = rlig[l].atten[1];
        light[l].atten[2] = rlig[l].atten[2];
        light[l].inner = rlig[l].inner;
        light[l].outer = rlig[l].outer;
        light[l].falloff = rlig[l].falloff;
        light[l].dir.x = rlig[l].dir.x;
        light[l].dir.y = rlig[l].dir.y;
        light[l].dir.z = rlig[l].dir.z;
        light[l].id = srv.CreateLight(light[l]);
        srv.ActivateLight(light[l].id);
    }
    srv.free(rlig);

    // read labels
    auto *lab = static_cast<RDF_LABEL *>(srv.malloc(sizeof(RDF_LABEL) * rhead.nlabels));
    srv.ReadFile(file, lab, sizeof(RDF_LABEL) * rhead.nlabels);
    label = static_cast<LABEL *>(srv.malloc(sizeof(LABEL) * rhead.nlabels));
    for (int32_t lb = 0; lb < rhead.nlabels; lb++)
    {
        label[lb].flags = lab[lb].flags;
        label[lb].name = &globname[lab[lb].name];
        label[lb].group_name = &globname[lab[lb].group_name];
        memcpy(&label[lb].m[0][0], &lab[lb].m[0][0], sizeof(lab[lb].m));
        memcpy(&label[lb].bones[0], &lab[lb].bones[0], sizeof(lab[lb].bones));
        memcpy(&label[lb].weight[0], &lab[lb].weight[0], sizeof(lab[lb].weight));
    }
    srv.free(lab);

    // read objects
    auto *obj = static_cast<RDF_OBJECT *>(srv.malloc(sizeof(RDF_OBJECT) * rhead.nobjects));
    atriangles = static_cast<int32_t *>(srv.malloc(sizeof(int32_t) * rhead.nobjects));
    srv.ReadFile(file, obj, sizeof(RDF_OBJECT) * rhead.nobjects);
    object = static_cast<OBJECT *>(srv.malloc(sizeof(OBJECT) * rhead.nobjects));
    for (int32_t o = 0; o < rhead.nobjects; o++)
    {
        object[o].flags = obj[o].flags;
        object[o].center.x = obj[o].center.x;
        object[o].center.y = obj[o].center.y;
        object[o].center.z = obj[o].center.z;
        object[o].radius = obj[o].radius;
        object[o].material = obj[o].material;
        object[o].ntriangles = obj[o].ntriangles;
        object[o].striangle = obj[o].striangle;
        object[o].name = &globname[obj[o].name];
        object[o].group_name = &globname[obj[o].group_name];
        object[o].vertex_buff = obj[o].vertex_buff;
        object[o].start_vertex = obj[o].svertex;
        object[o].num_vertices = obj[o].nvertices;
        atriangles[o] = obj[o].atriangles;
    }
    srv.free(obj);

    idx_buff = -1;
    const auto abortLoad = [&](const std::string &reason) {
        if (vbuff)
        {
            for (int32_t v = 0; v < rhead.nvrtbuffs; v++)
                srv.ReleaseVertexBuffer(vbuff[v].dev_buff);
            srv.free(vbuff);
            vbuff = nullptr;
        }
        srv.ReleaseIndexBuffer(idx_buff);
        idx_buff = -1;

        srv.free(label);
        label = nullptr;
        srv.free(object);
        object = nullptr;
        srv.free(atriangles);
        atriangles = nullptr;
        srv.free(light);
        light = nullptr;
        srv.free(material);
        material = nullptr;
        srv.free(tlookup);
        tlookup = nullptr;
        srv.free(tname);
        tname = nullptr;
        srv.free(names);
        names = nullptr;
        srv.free(globname);
        globname = nullptr;
        srv.free(rmat);
        rmat = nullptr;

        throw std::runtime_error(fmt::format("{} while loading '{}'", reason, fname));
    };

    // read triangles
    idx_buff = srv.CreateIndexBuffer(rhead.ntriangles * sizeof(RDF_TRIANGLE));
    auto *trg = static_cast<RDF_TRIANGLE *>(srv.LockIndexBuffer(idx_buff));
    const auto triangleDataSize = sizeof(RDF_TRIANGLE) * rhead.ntriangles;
    if (triangleDataSize > 0)
    {
        if (!trg)
            abortLoad(fmt::format("Failed to lock index buffer ({} bytes)", triangleDataSize));
        if (!srv.ReadFile(file, trg, triangleDataSize))
        {
            srv.UnlockIndexBuffer(idx_buff);
            abortLoad(fmt::format("Failed to read index buffer ({} bytes)", triangleDataSize));
        }
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
        if (captureIsland) islandTriangles.assign(trg, trg + rhead.ntriangles);
#endif
        srv.UnlockIndexBuffer(idx_buff);
    }

    auto nvertices = 0;
    // read vertex buffers
    auto *rvb = static_cast<RDF_VERTEXBUFF *>(srv.malloc(rhead.nvrtbuffs * sizeof(RDF_VERTEXBUFF)));
    srv.ReadFile(file, rvb, rhead.nvrtbuffs * sizeof(RDF_VERTEXBUFF));
    vbuff = static_cast<VERTEX_BUFFER *>(srv.malloc(rhead.nvrtbuffs * sizeof(VERTEX_BUFFER)));
    int32_t v;
    for (v = 0; v < rhead.nvrtbuffs; v++)
    {
        vbuff[v].type = rvb[v].type;
        vbuff[v].stride = sizeof(RDF_VERTEX0) + (rvb[v].type & 3) * 8 + (rvb[v].type >> 2) * 8;
        vbuff[v].size = rvb[v].size;
        vbuff[v].nverts = vbuff[v].size / vbuff[v].stride;
        vbuff[v].dev_buff = vbuff[v].nverts > 0 ? srv.CreateVertexBuffer(rvb[v].type, rvb[v].size) : -1;
        nvertices += vbuff[v].nverts;
    }
    srv.free(rvb);
    // read vertices
    auto itColData = colData.begin();
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
    if (captureIsland) islandVertices.resize(rhead.nvrtbuffs);
#endif
    for (v = 0; v < rhead.nvrtbuffs; v++)
    {
        if (vbuff[v].size == 0)
            continue;
        auto *vrt = static_cast<RDF_VERTEX0 *>(srv.LockVertexBuffer(vbuff[v].dev_buff));
        if (!vrt)
            abortLoad(fmt::format("Failed to lock vertex buffer {} ({} bytes)", v, vbuff[v].size));
        if (!srv.ReadFile(file, vrt, vbuff[v].size))
        {
            srv.UnlockVertexBuffer(vbuff[v].dev_buff);
            abortLoad(fmt::format("Failed to read vertex buffer {} ({} bytes)", v, vbuff[v].size));
        }
        for (int32_t vr = 0; vr < vbuff[v].nverts; vr++)
        {
            auto *prv = (RDF_VERTEX0 *)((uint8_t *)(vrt) + vbuff[v].stride * vr);
            if (colData.size() == nvertices)
            {
                prv->color = *itColData;
                ++itColData;
            }
        }

#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
        if (captureIsland && !(vbuff[v].type >> 2)) {
            auto* bytes = reinterpret_cast<uint8_t*>(vrt);
            islandVertices[v].assign(bytes, bytes + vbuff[v].size);
        }
#endif
        srv.UnlockVertexBuffer(vbuff[v].dev_buff);
    }

    // read BSP
    // rhead.flags &= ~FLAGS_BSP_PRESENT;
    if (rhead.flags & FLAGS_BSP_PRESENT)
    {
        RDF_BSPHEAD bhead;
        srv.ReadFile(file, &bhead, sizeof(RDF_BSPHEAD));

        sroot = std::vector<BSP_NODE>(bhead.nnodes);
        srv.ReadFile(file, sroot.data(), sroot.size() * sizeof(BSP_NODE));

        vrt = std::vector<CVECTOR>(bhead.nvertices);
        srv.ReadFile(file, vrt.data(), vrt.size() * sizeof(RDF_BSPVERTEX));

        btrg = std::vector<RDF_BSPTRIANGLE>(bhead.ntriangles);
        srv.ReadFile(file, btrg.data(), btrg.size() * sizeof(RDF_BSPTRIANGLE));

        if constexpr (storm::kValidateCollisionData)  {
            const bool valid = std::all_of(std::begin(btrg), std::end(btrg), [this](const auto &triangle) {
                return triangle.getIndex(0) < vrt.size() && triangle.getIndex(1) < vrt.size() &&
                       triangle.getIndex(2) < vrt.size();
            });
            if (!valid)
            {
                throw std::runtime_error(fmt::format("Detected invalid collision data while loading file '{}'", fname));
            }
        }
    }

    srv.CloseFile(file);

    for (int32_t t = 0; t < rhead.ntextures; t++) {
        const char* textureName = &globname[tname[t]];
#ifdef STORM_METAL_ROCKK2
        const char* rockMode = std::getenv("STORM_METAL_ROCKK2");
        if ((!rockMode || std::strcmp(rockMode, "0")) && storm::iEquals(textureName, "rockK2.tga")) {
            const auto alias = storm_metal::rockk2::aliases[storm_metal::rockk2::candidate(fname ? fname : "")];
            std::string selected(alias);
            tlookup[t] = srv.CreateTexture(selected.c_str());
            continue;
        }
#endif
        tlookup[t] = srv.CreateTexture(textureName);
    }
    for (int32_t m = 0; m < rhead.nmaterials; m++)
    {
        material[m].group_name = &globname[rmat[m].group_name];
        material[m].name = &globname[rmat[m].name];
        material[m].diffuse = rmat[m].diffuse;
        material[m].specular = rmat[m].specular;
        material[m].gloss = rmat[m].gloss;
        material[m].selfIllum = rmat[m].selfIllum;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        material[m].stormMetalBakedShadowMask = 0;
        material[m].stormMetalBakedBaseShadow = false;
#endif

        // relink textures
        for (int32_t tl = 0; tl < 4; tl++)
        {
            material[m].texture_type[tl] = static_cast<TEXTURE_TYPE>(rmat[m].texture_type[tl]);
            if (rmat[m].texture_type[tl] != TEXTURE_NONE) {
                material[m].texture[tl] = tlookup[rmat[m].texture[tl]];
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
                const std::string_view textureName = &globname[tname[rmat[m].texture[tl]]];
                const auto shadow = std::string_view("shadow");
                if (std::search(textureName.begin(), textureName.end(), shadow.begin(), shadow.end(),
                                [](char lhs, char rhs) {
                                    return std::tolower(static_cast<unsigned char>(lhs)) ==
                                           std::tolower(static_cast<unsigned char>(rhs));
                                }) != textureName.end())
                    material[m].stormMetalBakedShadowMask |= uint8_t(1u << tl);
                if (tl == 0 && storm::iEquals(textureName, "shadow.tga"))
                    material[m].stormMetalBakedBaseShadow = true;
#endif
            } else
                material[m].texture[tl] = -1;
        }
    }
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    for (int32_t o = 0; o < rhead.nobjects; ++o) {
        const auto materialIndex = object[o].material;
        if ((object[o].flags & VISIBLE) && materialIndex >= 0 && materialIndex < rhead.nmaterials &&
            material[materialIndex].stormMetalBakedBaseShadow) {
            stormMetalBakedBaseShadow = true;
            break;
        }
    }
#endif
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
    if (captureIsland && !islandTriangles.empty()) {
        std::vector<island_geometry::Face> faces;
        for (int o=0; o<rhead.nobjects; ++o) {
            const auto& ob=object[o];
            if (!(ob.flags & VISIBLE) || ob.vertex_buff<0 || ob.vertex_buff>=rhead.nvrtbuffs) continue;
            const int materialIndex=ob.material;
            if(materialIndex<0 || materialIndex>=rhead.nmaterials)continue;
            const int textureIndex=rmat[materialIndex].texture[0];
            if(textureIndex<0 || textureIndex>=rhead.ntextures ||
               !island_geometry::refinableTerrainTexture(&globname[tname[textureIndex]]))continue;
            const auto& bytes=islandVertices[ob.vertex_buff]; const auto& vb=vbuff[ob.vertex_buff];
            if (bytes.empty() || vb.stride<36 || vb.stride>60) continue;
            for (int t=0; t<ob.ntriangles; ++t) {
                const int triangle=ob.striangle+t;
                if (triangle<0 || size_t(triangle)>=islandTriangles.size()) continue;
                island_geometry::Face face{}; face.object=o;face.material=ob.material;bool valid=true;
                for (int j=0;j<3;j++) {
                    const int64_t vertex=int64_t(ob.start_vertex)+islandTriangles[triangle].vindex[j];
                    if(vertex<0 || uint64_t(vertex)*vb.stride+vb.stride>bytes.size()){valid=false;break;}
                    std::memcpy(&face.v[j],bytes.data()+size_t(vertex)*vb.stride,vb.stride);
                }
                if(valid)faces.push_back(face);
            }
        }
        storm_metal::weatherTerrain.capture(this,faces);
        islandDraws.resize(rhead.nobjects);
        size_t changedFaces=0,outputTriangles=0;
        for(int o=0;refineIsland && o<rhead.nobjects;o++) {
            std::vector<island_geometry::Face> objectFaces;for(const auto& face:faces)if(face.object==o)objectFaces.push_back(face);
            if(objectFaces.empty())continue;
            island_geometry::Surface surface(objectFaces);
            bool changed=false;for(const auto& face:objectFaces)if(surface.refine(face).size()>3){changed=true;changedFaces++;}
            if(!changed)continue;
            const auto& original=vbuff[object[o].vertex_buff];
            bool valid=true;
            for(const auto& chunk:island_geometry::chunks(surface,objectFaces,o)) {
                IslandDraw draw;draw.stride=original.stride;draw.vertices=int(chunk.vertices.size());draw.triangles=int(chunk.indices.size()/3);
                draw.vb=srv.CreateVertexBuffer(original.type,draw.vertices*draw.stride);
                draw.ib=srv.CreateIndexBuffer(int(chunk.indices.size()*sizeof(uint16_t)));
                auto* vertices=static_cast<uint8_t*>(srv.LockVertexBuffer(draw.vb));
                auto* indices=srv.LockIndexBuffer(draw.ib);
                if(!vertices||!indices){if(vertices)srv.UnlockVertexBuffer(draw.vb);if(indices)srv.UnlockIndexBuffer(draw.ib);srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);valid=false;break;}
                for(size_t i=0;i<chunk.vertices.size();i++)std::memcpy(vertices+i*draw.stride,&chunk.vertices[i],draw.stride);
                std::memcpy(indices,chunk.indices.data(),chunk.indices.size()*sizeof(uint16_t));
                srv.UnlockVertexBuffer(draw.vb);srv.UnlockIndexBuffer(draw.ib);islandDraws[o].push_back(draw);
            }
            if(!valid){for(const auto&draw:islandDraws[o]){srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);}islandDraws[o].clear();}
            else {islandRenderHorizontalMargin=16;for(const auto&draw:islandDraws[o])outputTriangles+=draw.triangles;}
        }
        if(islandRenderHorizontalMargin>0)std::fprintf(stderr,"[StormMetal] island PN terrain geometry %s: %zu curved faces, %zu triangles in replaced opaque terrain objects; authored vertical envelope and BSP retained\n",fname,changedFaces,outputTriangles);
    }
#endif
    srv.free(rmat);
}

// delete all textures, buffers, memory
GEOM::~GEOM()
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    ReleaseHullDraws(hullDraws);
#endif
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
    storm_metal::weatherTerrain.clear(this);
    for(const auto& objectDraws:islandDraws)for(const auto& draw:objectDraws){srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);}
#endif
    for (int32_t v = 0; v < rhead.nvrtbuffs; v++)
        srv.ReleaseVertexBuffer(vbuff[v].dev_buff);
    srv.free(vbuff);

    srv.ReleaseIndexBuffer(idx_buff);

    srv.free(label);
    srv.free(object);
    srv.free(atriangles);
    srv.free(light);

    srv.free(material);

    for (int32_t t = 0; t < rhead.ntextures; t++)
        srv.ReleaseTexture(tlookup[t]);
    srv.free(tlookup);
    srv.free(tname);

    srv.free(names);
    srv.free(globname);
}

// visible analyze and draw all objects
void GEOM::Draw(const PLANE *pl, int32_t np, MATERIAL_FUNC mtf) const
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    // A cut exposes the inside of the opposite, potentially uncut material.
    // Scope the complete node without duplicating any geometry or closing holes.
    struct HullSurfaceScope {
        GEOM_SERVICE &service;
        bool active, previous;
        HullSurfaceScope(GEOM_SERVICE &service, bool active)
            : service(service), active(active), previous(active ? service.SetHullTwoSided(true) : false) {}
        ~HullSurfaceScope() { if (active) service.SetHullTwoSided(previous); }
    } hullSurface(srv, !hullDraws.empty());
#endif
    srv.SetIndexBuffer(idx_buff);
    for (int32_t o = 0; o < rhead.nobjects; o++)
    {
        if (!(object[o].flags & VISIBLE))
            continue;
        // clip by external planes
        int32_t cp;
        for (cp = 0; cp < np; cp++)
        {
            const auto dist = object[o].center.x * pl[cp].nrm.x + object[o].center.y * pl[cp].nrm.y +
                              object[o].center.z * pl[cp].nrm.z - pl[cp].d;
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
            if (dist > object[o].radius + islandRenderHorizontalMargin)
#else
            if (dist > object[o].radius)
#endif
                break;
            // if(dist<-object[o].radius)    break;
        }
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        // Room objects have their own culling, even when the room node is visible.
        struct CasterScope {
            GEOM_SERVICE &service;
            bool active = false;
            ~CasterScope() { if (active) service.EndOffscreenPointCaster(); }
        } caster{srv};
        if (cp < np && !(caster.active = srv.BeginOffscreenPointCaster(object[o].center, object[o].radius)))
#else
        if (cp < np)
#endif
            continue;

        auto *const vb = &vbuff[object[o].vertex_buff];
        srv.SetVertexBuffer(vb->stride, vb->dev_buff);
        srv.SetMaterial(material[object[o].material]);
        if (mtf != nullptr)
            mtf(material[object[o].material]);
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        const auto damaged=hullDraws.find(o);
        if(damaged!=hullDraws.end()){
            for(const auto&draw:damaged->second){srv.SetIndexBuffer(draw.ib);srv.SetVertexBuffer(draw.stride,draw.vb);srv.DrawIndexedPrimitive(0,draw.vertices,draw.stride,0,draw.triangles);}
            srv.SetIndexBuffer(idx_buff);continue;
        }
#endif
#ifdef STORM_METAL_ISLAND_GEOMETRY_BRIDGE
        if(size_t(o)<islandDraws.size()&&!islandDraws[o].empty()) {
            for(const auto& draw:islandDraws[o]) {
                srv.SetIndexBuffer(draw.ib);srv.SetVertexBuffer(draw.stride,draw.vb);
                srv.DrawIndexedPrimitive(0,draw.vertices,draw.stride,0,draw.triangles);
            }
            srv.SetIndexBuffer(idx_buff);
            continue;
        }
#endif
        srv.DrawIndexedPrimitive(object[o].start_vertex, object[o].num_vertices, vb->stride, object[o].striangle * 3,
                                 object[o].ntriangles);
    }
}

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
void GEOM::ReleaseHullDraws(std::map<int,std::vector<HullDraw>>&draws)
{
    for(auto&entry:draws)for(auto&draw:entry.second){srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);}draws.clear();
}
void GEOM::ClearHullCuts(uint64_t owner)
{
    if(owner!=hullOwner)return;ReleaseHullDraws(hullDraws);hullOwner=hullSignature=0;
}
bool GEOM::CaptureHullObject(int objectIndex)
{
    if(hullOriginals.count(objectIndex))return true;
    if(objectIndex<0||objectIndex>=rhead.nobjects)return false;const auto&ob=object[objectIndex];
    if(ob.vertex_buff<0||ob.vertex_buff>=rhead.nvrtbuffs||ob.num_vertices<=0||ob.ntriangles<=0||ob.start_vertex<0||ob.striangle<0)return false;
    const auto&vb=vbuff[ob.vertex_buff];if((vb.type>>2)||vb.stride<36||vb.stride>60||ob.start_vertex+ob.num_vertices>vb.nverts||ob.striangle+ob.ntriangles>rhead.ntriangles)return false;
    auto*vertices=static_cast<uint8_t*>(srv.LockVertexBuffer(vb.dev_buff));if(!vertices)return false;
    HullOriginal source;source.type=vb.type;source.stride=vb.stride;source.vertices.assign(vertices+size_t(ob.start_vertex)*vb.stride,vertices+size_t(ob.start_vertex+ob.num_vertices)*vb.stride);srv.UnlockVertexBuffer(vb.dev_buff);
    auto*indices=static_cast<uint16_t*>(srv.LockIndexBuffer(idx_buff));if(!indices)return false;source.indices.assign(indices+ob.striangle*3,indices+(ob.striangle+ob.ntriangles)*3);srv.UnlockIndexBuffer(idx_buff);
    for(auto index:source.indices)if(index>=ob.num_vertices)return false;
    source.broadphase.build(source.vertices,source.stride,source.indices);hullOriginals.emplace(objectIndex,std::move(source));return true;
}
bool GEOM::SetHullCuts(uint64_t owner,const std::vector<storm_ship_hull::Impact>&cuts)
{
    if(!owner||(hullOwner&&hullOwner!=owner)||cuts.size()>256)return false;
    if(cuts.empty()){ClearHullCuts(owner);return true;}const auto signature=storm_ship_hull::signature(cuts);if(owner==hullOwner&&signature==hullSignature)return true;
    std::map<int,std::vector<storm_ship_hull::Impact>>groups;
    for(const auto&cut:cuts){if(!storm_ship_hull::valid(cut)||!CaptureHullObject(cut.object))return false;groups[cut.object].push_back(cut);}
    std::map<int,std::vector<HullDraw>>replacement;
    for(const auto&group:groups){const auto&source=hullOriginals.at(group.first);
        struct PreparedCut {storm_ship_hull::Impact impact;std::array<storm_ship_hull::Plane,10>planes;};std::vector<PreparedCut>prepared;prepared.reserve(group.second.size());
        std::vector<std::vector<const PreparedCut*>>localCuts(source.indices.size()/3);std::vector<uint32_t>candidates;
        for(const auto&cut:group.second){prepared.push_back({cut,storm_ship_hull::prism(cut)});source.broadphase.query(cut,candidates,.055f);for(auto triangle:candidates)localCuts[triangle].push_back(&prepared.back());}
        std::vector<uint8_t>vertices;std::vector<uint16_t>indices;vertices.reserve(source.vertices.size());indices.reserve(source.indices.size());
        std::unordered_map<std::string,uint16_t>weld;std::vector<int32_t>originalLookup(source.vertices.size()/source.stride,-1);bool changed=false,failed=false;
        const auto upload=[&](){if(indices.empty())return true;HullDraw draw;draw.vertices=int(vertices.size()/source.stride);draw.triangles=int(indices.size()/3);draw.stride=source.stride;draw.vb=srv.CreateVertexBuffer(source.type,int(vertices.size()));draw.ib=srv.CreateIndexBuffer(int(indices.size()*2));
            auto*v=draw.vb>=0?srv.LockVertexBuffer(draw.vb):nullptr;auto*i=draw.ib>=0?srv.LockIndexBuffer(draw.ib):nullptr;
            if(!v||!i){if(v)srv.UnlockVertexBuffer(draw.vb);if(i)srv.UnlockIndexBuffer(draw.ib);srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);return false;}
            std::memcpy(v,vertices.data(),vertices.size());std::memcpy(i,indices.data(),indices.size()*2);srv.UnlockVertexBuffer(draw.vb);srv.UnlockIndexBuffer(draw.ib);replacement[group.first].push_back(draw);vertices.clear();indices.clear();weld.clear();std::fill(originalLookup.begin(),originalLookup.end(),-1);return true;};
        const auto appendOriginal=[&](size_t triangle){if(vertices.size()/source.stride+3>=65535&&!upload()){failed=true;return;}
            for(unsigned corner=0;corner<3;corner++){auto original=source.indices[triangle*3+corner];auto&mapped=originalLookup[original];if(mapped<0){mapped=int32_t(vertices.size()/source.stride);auto*start=source.vertices.data()+size_t(original)*source.stride;vertices.insert(vertices.end(),start,start+source.stride);}indices.push_back(uint16_t(mapped));}};
        const auto append=[&](const storm_ship_hull::Triangle&triangle){if(vertices.size()/source.stride+3>=65535&&!upload()){failed=true;return;}
            for(const auto&vertex:triangle){std::string bytes(size_t(source.stride),'\0');storm_ship_hull::encode(vertex,reinterpret_cast<uint8_t*>(bytes.data()),source.stride);auto found=weld.find(bytes);uint16_t index;
                if(found==weld.end()){index=uint16_t(vertices.size()/source.stride);vertices.insert(vertices.end(),bytes.begin(),bytes.end());weld.emplace(std::move(bytes),index);}else index=found->second;indices.push_back(index);}};
        for(size_t triangle=0;triangle<localCuts.size();triangle++){if(localCuts[triangle].empty()){appendOriginal(triangle);if(failed)break;continue;}storm_ship_hull::Triangle original;for(unsigned corner=0;corner<3;corner++)original[corner]=storm_ship_hull::decode(source.vertices.data()+size_t(source.indices[triangle*3+corner])*source.stride,source.stride);
            struct GeneratedRim {storm_ship_hull::Triangle triangle;const PreparedCut*owner;};std::vector<storm_ship_hull::Triangle>pieces{original};std::vector<GeneratedRim>rims;bool triangleChanged=false;
            for(const auto*cut:localCuts[triangle]){std::vector<storm_ship_hull::Triangle>next;
                for(const auto&piece:pieces){auto result=storm_ship_hull::subtract(piece,cut->impact,cut->planes);triangleChanged|=result.removedArea>0;next.insert(next.end(),result.surface.begin(),result.surface.end());for(const auto&rim:result.rim)rims.push_back({rim,cut});}pieces=std::move(next);}
            changed|=triangleChanged;if(!triangleChanged)appendOriginal(triangle);else{for(const auto&piece:pieces)append(piece);const auto receiverNormal=storm_ship_hull::normalized(original[0].normal+original[1].normal+original[2].normal);
                for(const auto&rim:rims){std::vector<storm_ship_hull::Triangle>rimPieces{rim.triangle};for(const auto*cut:localCuts[triangle]){if(cut==rim.owner||storm_ship_hull::dot(receiverNormal,cut->impact.normal)<.30f)continue;std::vector<storm_ship_hull::Triangle>outside;
                        for(const auto&piece:rimPieces){auto clipped=storm_ship_hull::clipRim(piece,cut->impact,cut->planes);outside.insert(outside.end(),clipped.begin(),clipped.end());}rimPieces=std::move(outside);if(rimPieces.empty())break;}for(const auto&piece:rimPieces)append(piece);}}
            if(failed)break;
        }
        if(failed||!upload()){ReleaseHullDraws(replacement);return false;}if(changed)replacement.try_emplace(group.first);
        if(!changed){auto found=replacement.find(group.first);if(found!=replacement.end()){for(auto&draw:found->second){srv.ReleaseVertexBuffer(draw.vb);srv.ReleaseIndexBuffer(draw.ib);}replacement.erase(found);}}
    }
    // Only geometry instances own these buffers. File names are never cache keys.
    // Existing resource identities/revisions invalidate ordinary and shadow draws.
    ReleaseHullDraws(hullDraws);hullDraws=std::move(replacement);hullOwner=owner;hullSignature=signature;return true;
}
#endif

bool GEOM::GetCollisionDetails(TRACE_INFO &ti) const
{
    if (!(rhead.flags & FLAGS_BSP_PRESENT) || traceid == -1)
    {
        ti.a = ti.b = -1.0;
        ti.obj = ti.trg = -1;
        memset(&ti.plane, 0, sizeof(PLANE));
        return false;
    }

    // triangle-based coord
    int32_t vindex[3];
    vindex[0] =
        (btrg[traceid].vindex[0][0] << 0) | (btrg[traceid].vindex[0][1] << 8) | (btrg[traceid].vindex[0][2] << 16);
    vindex[1] =
        (btrg[traceid].vindex[1][0] << 0) | (btrg[traceid].vindex[1][1] << 8) | (btrg[traceid].vindex[1][2] << 16);
    vindex[2] =
        (btrg[traceid].vindex[2][0] << 0) | (btrg[traceid].vindex[2][1] << 8) | (btrg[traceid].vindex[2][2] << 16);

    const auto ve = dst - src;
    const DVECTOR a = vrt[vindex[1]] - vrt[vindex[0]];
    const DVECTOR b = vrt[vindex[2]] - vrt[vindex[0]];
    const auto pvec = ve ^ b;
    const double det = a | pvec;
    const double invdet = 1.0 / det;
    const DVECTOR c = src - vrt[vindex[0]];

    ti.a = static_cast<float>((c | pvec) * invdet);
    ti.b = static_cast<float>((ve | (c ^ a)) * invdet);

    // plane info
    const DVECTOR nrm = !(a ^ b);
    const double pldist = nrm | vrt[vindex[0]];
    ti.plane.nrm.x = static_cast<float>(nrm.x);
    ti.plane.nrm.y = static_cast<float>(nrm.y);
    ti.plane.nrm.z = static_cast<float>(nrm.z);
    ti.plane.d = static_cast<float>(pldist);

    // object and triangle
    for (int32_t o = 0; o < rhead.nobjects; o++)
        if (atriangles[o] > traceid)
        {
            ti.obj = o;
            ti.trg = traceid;
            if (o > 0)
                ti.trg -= atriangles[o - 1];
            break;
        }

    // vertices
    for (int32_t v = 0; v < 3; v++)
    {
        ti.vrt[v].x = vrt[vindex[v]].x;
        ti.vrt[v].y = vrt[vindex[v]].y;
        ti.vrt[v].z = vrt[vindex[v]].z;
    }
    return true;
}

void GEOM::GetMaterial(int32_t m, MATERIAL &mt) const
{
    mt = material[m];
}

void GEOM::SetMaterial(int32_t m, const MATERIAL &mt)
{
    material[m].diffuse = mt.diffuse;
    material[m].gloss = mt.gloss;
    material[m].selfIllum = mt.selfIllum;
    material[m].specular = mt.specular;

    for (int32_t t = 0; t < 4; t++)
    {
        material[m].texture_type[t] = mt.texture_type[t];
        material[m].texture[t] = mt.texture[t];
    }
}

void GEOM::SetObj(int32_t o, const OBJECT &ob)
{
    object[o].flags = ob.flags;
    object[o].material = ob.material;
}

void GEOM::GetObj(int32_t o, OBJECT &ob) const
{
    ob = object[o];
    ob.vertex_buff = vbuff[object[o].vertex_buff].dev_buff;
}

void GEOM::GetInfo(INFO &i) const
{
    i.ntextures = rhead.ntextures;
    i.nmaterials = rhead.nmaterials;
    i.nlights = rhead.nlights;
    i.nobjects = rhead.nobjects;
    i.ntriangles = rhead.ntriangles;
    i.nvrtbuffs = rhead.nvrtbuffs;
    i.nlabels = rhead.nlabels;
    i.boxcenter.x = rhead.bbox_center.x;
    i.boxcenter.y = rhead.bbox_center.y;
    i.boxcenter.z = rhead.bbox_center.z;
    i.boxsize.x = rhead.bbox_size.x;
    i.boxsize.y = rhead.bbox_size.y;
    i.boxsize.z = rhead.bbox_size.z;
    i.radius = rhead.radius;
}

int32_t GEOM::GetVertexBuffer(int32_t vb) const
{
    return vbuff[vb].dev_buff;
}

int32_t GEOM::GetIndexBuffer() const
{
    return idx_buff;
}

int32_t GEOM::GetTexture(int32_t tx) const
{
    return tlookup[tx];
}

const char *GEOM::GetTextureName(int32_t tx) const
{
    return &globname[tname[tx]];
}

// ~!~
// leave it like that for now
// will think later
auto unbelievable_workaround(void *ptr)
{
    return reinterpret_cast<uint64_t>(ptr) & 0x7FFFFFFF;
}

int32_t GEOM::FindName(const char *name) const
{
    for (int32_t n = 0; n < rhead.names; n++)
        if (storm::iEquals(&globname[names[n]], name))
            return unbelievable_workaround(&globname[names[n]]);
    return -1;
}

//--------------------------------------------------
// label functions
//--------------------------------------------------
int32_t GEOM::FindLabelN(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nlabels; start_index++)
        if (unbelievable_workaround(label[start_index].name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindLabelG(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nlabels; start_index++)
        if (unbelievable_workaround(label[start_index].group_name) == name_id)
            return start_index;
    return -1;
}

void GEOM::GetLabel(int32_t l, LABEL &lb) const
{
    lb.flags = label[l].flags;
    lb.group_name = label[l].group_name;
    lb.name = label[l].name;
    memcpy(&lb.bones[0], &label[l].bones[0], sizeof(lb.bones));
    memcpy(&lb.weight[0], &label[l].weight[0], sizeof(lb.weight));
    memcpy(&lb.m[0][0], &label[l].m[0][0], sizeof(lb.m));
}

void GEOM::SetLabel(int32_t l, const LABEL &lb)
{
    label[l].flags = lb.flags;
    memcpy(&label[l].bones[0], &lb.bones[0], sizeof(lb.bones));
    memcpy(&label[l].weight[0], &lb.weight[0], sizeof(lb.weight));
    memcpy(&label[l].m[0][0], &lb.m[0][0], sizeof(lb.m));
}

//--------------------------------------------------
// object functions
//--------------------------------------------------
int32_t GEOM::FindObjN(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nobjects; start_index++)
        if (unbelievable_workaround(object[start_index].name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindObjG(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nobjects; start_index++)
        if (unbelievable_workaround(object[start_index].group_name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindMaterialN(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nmaterials; start_index++)
        if (unbelievable_workaround(material[start_index].name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindMaterialG(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nmaterials; start_index++)
        if (unbelievable_workaround(material[start_index].group_name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindLightN(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nlights; start_index++)
        if (unbelievable_workaround(light[start_index].name) == name_id)
            return start_index;
    return -1;
}

int32_t GEOM::FindLightG(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.nlights; start_index++)
        if (unbelievable_workaround(light[start_index].group_name) == name_id)
            return start_index;
    return -1;
}

void GEOM::SetLight(int32_t l, const LIGHT &lt)
{
    light[l].atten[0] = lt.atten[0];
    light[l].atten[1] = lt.atten[1];
    light[l].atten[2] = lt.atten[2];
    light[l].r = lt.r;
    light[l].g = lt.g;
    light[l].b = lt.b;
    light[l].dir = lt.dir;
    light[l].falloff = lt.falloff;
    light[l].flags = lt.flags;
    light[l].inner = lt.inner;
    light[l].outer = lt.outer;
    light[l].pos = lt.pos;
    light[l].range = lt.range;
    light[l].type = lt.type;
}

int32_t GEOM::FindTexture(int32_t start_index, int32_t name_id)
{
    for (; start_index < rhead.ntextures; start_index++)
        if (globname[tlookup[start_index]] == name_id)
            return start_index;
    return -1;
}

void GEOM::GetLight(int32_t l, LIGHT &lt) const
{
    lt = light[l];
}
