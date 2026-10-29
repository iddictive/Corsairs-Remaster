#include "core.h"

#include "geometry_r.h"
#include "gpu_skinning_bridge.hpp"
#include "string_compare.hpp"
#include "rdf.h"
#include <cmath>
#include <set>
#include <limits>
#include <vector>
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" bool StormMetalOutdoorSunShadowsReady(void*);
extern "C" bool StormMetalBakedStaticEnvironment(void*);
extern "C" bool StormMetalGeometryEmitter(void*);
extern "C" bool StormMetalBeginOffscreenLocalPointCaster(void*, const float*, float);
extern "C" void StormMetalEndOffscreenPointCaster(void*);
#endif

CREATE_SERVICE(GEOMETRY)

storm_render::VertexLayout *GEOM_SERVICE_R::vertexDecl_ = nullptr;

char technique[256] = "";
char RenderServiceName[] = "dx9render";
GEOM_SERVICE_R GSR;
char texturePath[256];

const int32_t SHIFT_VALUE = 999999999;
std::vector<VGEOMETRY::ANIMATION_VB> avb;

static VGEOMETRY::ANIMATION_VB *AnimationBuffer(int32_t id)
{
    if (id < SHIFT_VALUE || static_cast<size_t>(id - SHIFT_VALUE) >= avb.size())
        return nullptr;
    return &avb[id - SHIFT_VALUE];
}

GEOMETRY::GEOMETRY()
{
    strcpy_s(texturePath, "");
}

GEOMETRY::~GEOMETRY()
{
    GSR.SetLegacySeaModels(nullptr);
}

const char *GEOMETRY::GetTexturePath()
{
    return &texturePath[0];
}

void GEOMETRY::SetTexturePath(const char *path)
{
    strcpy_s(texturePath, path);
}

//=================================================================================================
// Block 1
//=================================================================================================
void GEOMETRY::SetTechnique(const char *name)
{
    strcpy_s(technique, name);
}

const char *GEOMETRY::GetTechnique() const
{
    return technique;
}

GEOMETRY::ANIMATION_VB GEOMETRY::GetAnimationVBDesc(int32_t vb)
{
    const auto *buffer = AnimationBuffer(vb);
    return buffer ? *buffer : ANIMATION_VB{};
}

VERTEX_TRANSFORM transform_func = nullptr;

void GEOMETRY::SetVBConvertFunc(VERTEX_TRANSFORM _transform_func)
{
    transform_func = _transform_func;
}

static bool geoLog = false;

bool GEOMETRY::Init()
{
    RenderService = static_cast<VDX9RENDER *>(core.GetService(RenderServiceName));
    if (!RenderService)
    {
        core.Trace("No service: %s", RenderServiceName);
    }
    GSR.SetRenderService(RenderService);

    auto ini = fio->OpenIniFile(core.EngineIniFileName());
    if (ini)
    {
        geoLog = ini->GetInt(nullptr, "geometry_log", 0) == 1;
    }

    return true;
}

void GEOMETRY::SetCausticMode(bool bSet)
{
    GSR.SetCausticMode(bSet);
}

bool GEOMETRY::LoadState(ENTITY_STATE *state)
{
    return true;
}

namespace
{
std::string LegacyModelKey(const char *name)
{
    std::string key = name ? name : "";
    for (auto &c : key)
        if (c == '/') c = '\\';
        else if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
    return key;
}
bool LegacyGeometryAvailable(const std::string &model)
{
    const auto name = "resource\\models\\" + model + ".gm";
    auto file = fio->_CreateFile(name.c_str(), std::ios::binary | std::ios::in);
    RDF_HEAD head{};
    return file.is_open() && fio->_ReadFile(file, &head, sizeof(head)) &&
        head.version == RDF_VERSION && head.nobjects >= 0 && head.ntriangles >= 0;
}
}

bool GEOMETRY::ConfigureLegacySeaGeometry(const ATTRIBUTES *frame)
{
    legacySeaGeometry = false;
    GSR.SetLegacySeaModels(nullptr);
    legacySeaModels.clear();
    legacySeaExposures.clear();
    if (!frame || frame->GetAttributeAsDword("version") != 1) return false;
    auto input = fio->OpenIniFile("RESOURCE\\INI\\legacy_sea_geometry.ini");
    if (!input || input->GetInt("LegacySea", "version", 0) != 1) return false;
    auto *regions = frame->GetAttributeClass("regions");
    const int count = input->GetInt("LegacySea", "regions", 0);
    const int models = input->GetInt("LegacySea", "models", 0);
    const int providers = input->GetInt("LegacySea", "providers", 0);
    const int exposures = input->GetInt("LegacySea", "exposures", 0);
    if (!regions || count < 1 || count > 128 || models < 1 || models > 512 ||
        providers < 1 || providers > 512 || exposures < 1 || exposures > 1024 || regions->GetAttributeAsDword("count") != unsigned(count) ||
        frame->GetAttributeAsFloat("scale") != input->GetFloat("LegacySea", "scale", 0)) return false;
    std::map<std::string, std::pair<float,float>> expected;
    char name[512];
    for (int i = 0; i < count; ++i)
    {
        const auto section = "Region" + std::to_string(i);
        if (!input->ReadString(section.c_str(), "id", name, sizeof(name), "") || !*name ||
            !expected.emplace(name, std::make_pair(input->GetFloat(section.c_str(), "mapX"),
                input->GetFloat(section.c_str(), "mapZ"))).second) return false;
    }
    std::set<std::string> seen;
    for (int i = 0; i < count; ++i)
    {
        auto *region = regions->GetAttributeClass("r" + std::to_string(i));
        const auto *id = region ? static_cast<const char *>(region->GetAttribute("id")) : nullptr;
        if (!id || !seen.insert(id).second) return false;
        const auto found = expected.find(id);
        if (found == expected.end()) return false;
        const float x = region->GetAttributeAsFloat("mapX"), z = region->GetAttributeAsFloat("mapZ");
        if (!std::isfinite(x) || !std::isfinite(z) || x != found->second.first || z != found->second.second) return false;
    }
    for (int i = 0; i < providers; ++i)
    {
        const auto section = "Provider" + std::to_string(i);
        if (!input->ReadString(section.c_str(), "source", name, sizeof(name), "") || !*name ||
            !LegacyGeometryAvailable(name)) return false;
    }
    for (int i = 0; i < models; ++i)
    {
        const auto section = "Model" + std::to_string(i);
        char target[512];
        if (!input->ReadString(section.c_str(), "source", name, sizeof(name), "") || !*name ||
            !input->ReadString(section.c_str(), "projection", target, sizeof(target), "") || !*target ||
            !LegacyGeometryAvailable(name) || !LegacyGeometryAvailable(target) ||
            !legacySeaModels.emplace(LegacyModelKey(name), target).second)
        { legacySeaModels.clear(); return false; }
    }
    std::vector<std::pair<std::string,std::string>> links;
    for (int i = 0; i < exposures; ++i)
    {
        const auto section = "Exposure" + std::to_string(i);
        char consumer[512];
        if (!input->ReadString(section.c_str(), "provider", name, sizeof(name), "") ||
            !input->ReadString(section.c_str(), "consumer", consumer, sizeof(consumer), "") ||
            !expected.count(name) || !expected.count(consumer)) return false;
        links.emplace_back(name, consumer);
    }
    legacySeaExposures = std::move(links);
    legacySeaGeometry = true;
    GSR.SetLegacySeaModels(&legacySeaModels);
    return true;
}

char lightPath[256];
int vrtSize;

GEOS *GEOMETRY::CreateGeometry(const char *file_name, const char *light_file_name, int32_t flags, const char *lmPath)
{
    char fnt[256];
    if (light_file_name != nullptr)
    {
        sprintf_s(lightPath, "%s\\%s", lmPath, file_name);
        // strcpy_s(lightPath, light_file_name);
        auto *const bs = strrchr(lightPath, '\\');
        if (bs != nullptr)
            *bs = 0;
    }

    static auto first = 0;
    FILE *fl;
    if (geoLog)
    {
        vrtSize = 0;
        if (first == 0)
            fl = fopen("geoLoad.txt", "w");
        else
            fl = fopen("geoLoad.txt", "a");
    }

    GEOS *gp;
    try
    {
        sprintf_s(fnt, "resource\\models\\%s.gm", file_name);
        const auto lighting = VGEOMETRY::LightFileName(file_name, light_file_name ? light_file_name : "");
        const auto projected = legacySeaGeometry ? legacySeaModels.find(LegacyModelKey(file_name)) : legacySeaModels.end();
        if (projected != legacySeaModels.end())
        {
            const auto resource = "resource\\models\\" + projected->second + ".gm";
            try { gp = ::CreateGeometry(fnt, lighting.empty() ? nullptr : lighting.c_str(), GSR, flags); }
            catch (...)
            {
                // Provider faces remain in both paths, so native fallback
                // retains coast after earlier copies transferred ownership.
                legacySeaGeometry = false;
                GSR.SetLegacySeaModels(nullptr);
                core.Trace("LEGACY_SEA_GEOMETRY: %s failed; original geometry retained, projection disabled for this scene", resource.c_str());
                gp = ::CreateGeometry(fnt, lighting.empty() ? nullptr : lighting.c_str(), GSR, flags);
            }
        }
        else gp = ::CreateGeometry(fnt, lighting.empty() ? nullptr : lighting.c_str(), GSR, flags);
    }
    catch (const std::exception &e)
    {
        core.Trace("%s: %s", fnt, e.what());
        return nullptr;
    }
    catch (...)
    {
        core.Trace("Invalid model: %s", fnt);
        return nullptr;
    }

    if (geoLog)
    {
        //---------------------------------------------------------------
        // print statistics
        //---------------------------------------------------------------
        GEOS::INFO gi;
        gp->GetInfo(gi);
        first += gi.ntriangles * 2 * 3 + vrtSize;
        fprintf(fl, "%.2f, trgSize: %d, vrtSize: %d, tex: %d. obj: %d, %s\n", first / 1024.0f / 1024.0f,
                gi.ntriangles * 2 * 3, vrtSize, gi.ntextures, gi.nobjects, file_name);
        fclose(fl);
    }

    return gp;
}

ANIMATION *GEOMETRY::LoadAnimation(const char *anim)
{
    return nullptr;
}

void GEOMETRY::DeleteGeometry(GEOS *gid)
{
    delete gid;
}

//=================================================================================================
// Block 2
//=================================================================================================

void GEOM_SERVICE_R::SetRenderService(VDX9RENDER *render_service)
{
    bCaustic = false;
    RenderService = render_service;
    CurentIndexBuffer = INVALID_BUFFER_ID;
    CurentVertexBuffer = INVALID_BUFFER_ID;
    CurentVertexBufferSize = 0;

    const storm_render::VertexElement VertexElements[] = {
        {0, 0, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Position, 0},
        {0, 12, storm_render::VertexComponent::Float3, storm_render::VertexMethod::Default, storm_render::VertexSemantic::TexCoord, 0},
        {0, 24, storm_render::VertexComponent::Color, storm_render::VertexMethod::Default, storm_render::VertexSemantic::Color, 0},
        {0, 28, storm_render::VertexComponent::Float2, storm_render::VertexMethod::Default, storm_render::VertexSemantic::TexCoord, 1},
        storm_render::vertexLayoutTerminator()};
    if (vertexDecl_ == nullptr)
        RenderService->CreateVertexDeclaration(VertexElements, &vertexDecl_);
}

std::fstream GEOM_SERVICE_R::OpenFile(const char *fname)
{
    if (RenderService)
    {
        RenderService->ProgressView();
    }
    std::string projected;
    if (legacySeaModels)
    {
        auto key = LegacyModelKey(fname);
        const std::string prefix = "resource\\models\\";
        if (key.compare(0, prefix.size(), prefix) == 0 && key.size() > prefix.size()+3 && key.compare(key.size()-3,3,".gm") == 0)
        {
            key = key.substr(prefix.size(),key.size()-prefix.size()-3);
            const auto found = legacySeaModels->find(key);
            if (found != legacySeaModels->end()) projected = prefix+found->second+".gm";
        }
    }
    auto fileS = fio->_CreateFile(projected.empty() ? fname : projected.c_str(), std::ios::binary | std::ios::in);
    if (!projected.empty() && !fileS.is_open())
        throw std::runtime_error("LEGACY_SEA_GEOMETRY: projected resource disappeared: " + projected);
    if (!fileS.is_open())
    {
        if (storm::iEquals(&fname[strlen(fname) - 4], ".col"))
        {
            if (fio->_FileOrDirectoryExists(fname))
                throw std::runtime_error("can't read geometry COL file");
        }
        else
        {
            throw std::runtime_error("can't open geometry file");
        }
    }

    return fileS;
}

int GEOM_SERVICE_R::FileSize(const char *fname)
{
    return fio->_GetFileSize(fname);
}

bool GEOM_SERVICE_R::ReadFile(std::fstream &fileS, void *data, int32_t bytes)
{
    return fio->_ReadFile(fileS, data, bytes);
}

void GEOM_SERVICE_R::CloseFile(std::fstream &fileS)
{
    fio->_CloseFile(fileS);
}

void *GEOM_SERVICE_R::malloc(int32_t bytes)
{
    return new char[bytes];
}

void GEOM_SERVICE_R::free(void *ptr)
{
    delete[] static_cast<char*>(ptr);
}

GEOS::ID GEOM_SERVICE_R::CreateTexture(const char *fname)
{
    char tex[256];
    if (storm::iEquals(fname, "shadow.tga"))
    {
        sprintf_s(tex, "lighting\\%s\\%s", lightPath, fname);
    }
    else
    {
        strcpy_s(tex, texturePath);
        strcat_s(tex, fname);
    }
    if (RenderService)
    {
        RenderService->ProgressView();
        return RenderService->TextureCreate(tex);
    }
    return INVALID_TEXTURE_ID;
}

void GEOM_SERVICE_R::SetCausticMode(bool bSet)
{
    bCaustic = bSet;
}

void GEOM_SERVICE_R::SetMaterial(const GEOS::MATERIAL &mt)
{
    RenderService->TextureSet(0, mt.texture[0]);

    if (bCaustic)
    {
        return;
    }

    if (mt.texture_type[1] != GEOS::TEXTURE_NONE)
    {
        // path BASE texture through
        RenderService->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpSelectArg2);

        // stage #2
        RenderService->TextureSet(1, mt.texture[1]);
        // COLOR = BASE_C + DETAIL_C - 0.5
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        const bool replaceBakedShadow = (mt.stormMetalBakedShadowMask & (1u << 1)) &&
            StormMetalOutdoorSunShadowsReady(RenderService->GetNativeDevice()) &&
            !StormMetalBakedStaticEnvironment(RenderService->GetNativeDevice());
#else
        const bool replaceBakedShadow = false;
#endif
        RenderService->SetTextureStageState(1, storm_render::StageColorOp, replaceBakedShadow ? storm_render::TextureOpSelectArg1 : storm_render::TextureOpModulate2X);

        // COLOR = CURRENT_C * VERTEX_C * 2
        RenderService->TextureSet(2, 0);
        RenderService->SetTextureStageState(2, storm_render::StageColorOp, storm_render::TextureOpModulate2X);
    }
    else
    {
        RenderService->SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpModulate2X);

        RenderService->SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
        RenderService->SetTextureStageState(2, storm_render::StageColorOp, storm_render::TextureOpDisable);
    }

    RenderService->SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    RenderService->SetTextureStageState(0, storm_render::StageColorArg2, storm_render::textureArg::texture);
    RenderService->SetTextureStageState(1, storm_render::StageColorArg1, storm_render::textureArg::current);
    RenderService->SetTextureStageState(1, storm_render::StageColorArg2, storm_render::textureArg::texture);
    RenderService->SetTextureStageState(2, storm_render::StageColorArg1, storm_render::textureArg::current);
    RenderService->SetTextureStageState(2, storm_render::StageColorArg2, storm_render::textureArg::diffuse);

    RenderService->SetRenderState(storm_render::RenderBlend, FALSE);

    storm_render::Material m;
    m.Diffuse.r = m.Diffuse.g = m.Diffuse.b = mt.diffuse;
    m.Diffuse.a = 1.0f;
    m.Ambient.r = m.Ambient.g = m.Ambient.b = m.Ambient.a = 0.0f;
    m.Specular.r = m.Specular.g = m.Specular.b = mt.specular;
    m.Specular.a = 1.0f;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    // RDF selfIllum also contains legacy weapon fill, not only luminous glass.
    // Only models registered as lamp fixtures may import it as emission.
    m.Emissive.r = m.Emissive.g = m.Emissive.b =
        StormMetalGeometryEmitter(RenderService->GetNativeDevice()) ? mt.selfIllum : 0.0f;
    m.Emissive.a = 0.0f;
#else
    m.Emissive.r = m.Emissive.g = m.Emissive.b = m.Emissive.a = 0.0f;
#endif
    m.Power = mt.gloss;

    RenderService->SetMaterial(m);
}

#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
bool GEOM_SERVICE_R::BeginOffscreenPointCaster(const GEOS::VERTEX &center, float radius)
{
    const float position[] = {center.x, center.y, center.z};
    return StormMetalBeginOffscreenLocalPointCaster(RenderService->GetNativeDevice(), position, radius);
}
void GEOM_SERVICE_R::EndOffscreenPointCaster()
{
    StormMetalEndOffscreenPointCaster(RenderService->GetNativeDevice());
}
#endif

void GEOM_SERVICE_R::ReleaseTexture(GEOS::ID tex)
{
    if (RenderService)
        RenderService->TextureRelease(tex);
}

GEOS::ID GEOM_SERVICE_R::CreateVertexBuffer(int32_t type, int32_t size)
{
    if (size == 0)
        return INVALID_BUFFER_ID;
    if (!RenderService)
        return INVALID_BUFFER_ID;

    int32_t texset[4] = {storm_render::vertexFormat::tex1, storm_render::vertexFormat::tex2, storm_render::vertexFormat::tex3, storm_render::vertexFormat::tex4};
    const int32_t fvf = storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::texCoord2;
    const auto FVF = fvf | texset[type & 3];

    // animated vertices
    if (type & 4)
    {
        size_t a;
        for (a = 0; a < avb.size(); a++)
            if (avb[a].buff == nullptr)
                break;
        if (a == avb.size())
        {
            if (a > static_cast<size_t>(std::numeric_limits<int32_t>::max() - SHIFT_VALUE))
                return INVALID_BUFFER_ID;
            avb.emplace_back();
        }
        avb[a].buff = new char[size];
        avb[a].stride = sizeof(GEOS::VERTEX0);
        avb[a].nvertices = size / sizeof(GEOS::AVERTEX0);
        avb[a].fvf = FVF;
        return static_cast<int32_t>(a) + SHIFT_VALUE;
    }

    vrtSize += size;
    return RenderService->CreateVertexBuffer(FVF, size, storm_render::usage::writeOnly);
}

void *GEOM_SERVICE_R::LockVertexBuffer(GEOS::ID vb)
{
    if (vb == INVALID_BUFFER_ID)
        return nullptr;
    if (RenderService == nullptr)
        return nullptr;
    if (vb >= SHIFT_VALUE)
    {
        const auto *buffer = AnimationBuffer(vb);
        return buffer ? buffer->buff : nullptr;
    }
    return RenderService->LockVertexBuffer(vb);
}

void GEOM_SERVICE_R::UnlockVertexBuffer(GEOS::ID vb)
{
    if (vb == INVALID_BUFFER_ID)
        return;
    if (vb >= SHIFT_VALUE)
        if (const auto *buffer = AnimationBuffer(vb))
            storm::metal::skinning::invalidateVertices(
                static_cast<const storm::metal::skinning::AnimatedVertex *>(buffer->buff));

    if (RenderService && vb < SHIFT_VALUE)
        RenderService->UnLockVertexBuffer(vb);
}

void GEOM_SERVICE_R::ReleaseVertexBuffer(GEOS::ID vb)
{
    if (vb == INVALID_BUFFER_ID)
        return;
    if (RenderService)
        if (vb >= SHIFT_VALUE)
        {
            if (auto *buffer = AnimationBuffer(vb))
            {
                storm::metal::skinning::invalidateVertices(
                    static_cast<const storm::metal::skinning::AnimatedVertex *>(buffer->buff));
                delete[] static_cast<char *>(buffer->buff);
                *buffer = {};
            }
        }
        else
            RenderService->ReleaseVertexBuffer(vb);
}

GEOS::ID GEOM_SERVICE_R::CreateIndexBuffer(int32_t size)
{
    if (RenderService)
        RenderService->ProgressView();
    if (size == 0)
        return INVALID_BUFFER_ID;
    if (RenderService)
        return RenderService->CreateIndexBuffer(size);
    return INVALID_BUFFER_ID;
}

void *GEOM_SERVICE_R::LockIndexBuffer(GEOS::ID ib)
{
    if (ib == INVALID_BUFFER_ID)
        return nullptr;
    if (RenderService)
        return RenderService->LockIndexBuffer(ib);
    return nullptr;
}

void GEOM_SERVICE_R::UnlockIndexBuffer(GEOS::ID ib)
{
    if (ib == INVALID_BUFFER_ID)
        return;
    if (RenderService)
        RenderService->UnLockIndexBuffer(ib);
}

void GEOM_SERVICE_R::ReleaseIndexBuffer(GEOS::ID ib)
{
    if (ib == INVALID_BUFFER_ID)
        return;
    if (RenderService)
        RenderService->ReleaseIndexBuffer(ib);
}

void GEOM_SERVICE_R::SetIndexBuffer(GEOS::ID ibuff)
{
    CurentIndexBuffer = ibuff;
}

void GEOM_SERVICE_R::SetVertexBuffer(int32_t vsize, GEOS::ID vbuff)
{
    CurentVertexBuffer = vbuff;
    CurentVertexBufferSize = vsize;
}

void GEOM_SERVICE_R::DrawIndexedPrimitive(int32_t minv, int32_t numv, int32_t vrtsize, int32_t startidx, int32_t numtrg)
{
    if (!RenderService)
        return;

    const auto submit = [&](const char *block, const auto &draw) {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        if (hullTwoSided) {
            uint32_t previous = storm_render::CullCounterClockwise;
            RenderService->GetRenderState(storm_render::RenderCullMode, &previous);
            struct CullRestore {
                VDX9RENDER *renderer;
                uint32_t previous;
                ~CullRestore() { renderer->SetRenderState(storm_render::RenderCullMode, previous); }
            } restore{RenderService, previous};
            // Techniques can publish cull state on every pass. Apply the hull
            // scope after publication, then submit that pass without reentry.
            const bool named = block && block[0];
            if (!named || RenderService->TechniqueExecuteStart(block))
                do {
                    RenderService->SetRenderState(storm_render::RenderCullMode, storm_render::CullNone);
                    draw(nullptr);
                } while (named && RenderService->TechniqueExecuteNext());
            return;
        }
#endif
        draw(block);
    };

    // uint32_t oldZBias;

    if (bCaustic)
    {
        CMatrix mWorld, mView, mProjection;
        RenderService->GetTransform(storm_render::TransformWorld, mWorld);
        RenderService->GetTransform(storm_render::TransformProjection, mProjection);
        RenderService->GetTransform(storm_render::TransformView, mView);

        auto mWVP = (mWorld * mView) * mProjection;

        mWVP.Transposition4x4();

        // constants
        // 0 - World * View * Projection
        // 4 - World

        RenderService->SetVertexShaderConstantF(0, mWVP, 4);
        RenderService->SetVertexShaderConstantF(4, mWorld, 4);

        RenderService->SetVertexDeclaration(vertexDecl_);

        // RenderService->GetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, &oldZBias);
        // float SSBias = -0.6f;
        // RenderService->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(int*)&SSBias);
    }

    // draw animation
    if (CurentVertexBuffer >= SHIFT_VALUE)
    {
        const auto *buffer = AnimationBuffer(CurentVertexBuffer);
        if (!buffer || !buffer->buff || !transform_func)
            return;
        const auto cavb = *buffer;

        auto *transformed_vb =
            static_cast<storm_render::VertexBuffer *>(transform_func(cavb.buff, minv, numv, cavb.nvertices));
        if (!bCaustic)
        {
            RenderService->SetStreamSource(0, transformed_vb, cavb.stride);
            RenderService->SetFVF(cavb.fvf);

            submit(technique, [&](const char *block) {
                RenderService->DrawBuffer(-1, cavb.stride, CurentIndexBuffer, minv, numv, startidx, numtrg, block);
            });
        }
        else
        {
            RenderService->SetStreamSource(0, transformed_vb, cavb.stride);
            submit("caustic", [&](const char *block) {
                RenderService->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, CurentVertexBuffer, 0, CurentIndexBuffer,
                                                            minv, numv, startidx, numtrg, block);
            });
        }
        return;
    }
    if (CurentIndexBuffer != INVALID_BUFFER_ID)
    {
        if (!bCaustic)
            submit(technique, [&](const char *block) {
                RenderService->DrawBuffer(CurentVertexBuffer, vrtsize, CurentIndexBuffer, minv, numv, startidx, numtrg, block);
            });
        else
            submit("caustic", [&](const char *block) {
                RenderService->DrawIndexedPrimitiveNoVShader(storm_render::PrimitiveTriangleList, CurentVertexBuffer, vrtsize,
                                                            CurentIndexBuffer, minv, numv, startidx, numtrg, block);
            });
    }

    // if (bCaustic)
    //{
    //    RenderService->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, oldZBias);
    //}
}

GEOS::ID GEOM_SERVICE_R::CreateLight(const GEOS::LIGHT)
{
    if (!RenderService)
        return INVALID_LIGHT_ID;
    return INVALID_LIGHT_ID;
}

void GEOM_SERVICE_R::ActivateLight(GEOS::ID n)
{
}
