#include "sail.h"
#include "rope.h"
#include "shared/sail_msg.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// CPU-only fixture. Access checking is disabled in this TU to seed a mounted
// tie without rendering; all message, deletion and wave methods are original.
int main(int argc, char **argv)
{
    const bool remove = argc > 1 && std::strcmp(argv[1], "remove") == 0;
    auto *sails = new SAIL;
    auto *sail = new SAILONE;
    auto *ropes = new ROPE;
    sails->WindVect = new float[1024]{};
    sails->WINDVECTOR_QUANTITY = 1024;
    sails->WINDVECTOR_TINCR = 1;
    sails->WINDVECTOR_TADD = 0;
    sails->TsailWindDepend = 0;
    sails->fTHoleFlexDepend = 0;
    sails->globalWind.base = 0;
    sails->sailQuantity = 1;
    sails->slist = new SAILONE *[1]{sail};
    sail->pp = sails;
    sail->ss.eSailType = SAIL_TREANGLE;
    sail->ss.hardPoints[0] = CVECTOR(1.f, 2.f, 3.f);
    sail->ss.hardPoints[1] = CVECTOR(4.f, 5.f, 6.f);
    sail->ss.hardPoints[2] = CVECTOR(7.f, 8.f, 9.f);
    sail->ss.boundSphere.r = 1;
    sail->m_dwRow = SAIL_ROW_MAX;
    sail->m_dwCol = SAIL_COL_MAX;
    ropes->ropeQuantity = ropes->groupQuantity = 1;
    ropes->rlist = new ROPE::ROPEDATA *[1]{new ROPE::ROPEDATA{}};
    ropes->gdata = new ROPE::GROUPDATA[1]{};
    ropes->gdata[0].modelEI = 777;
    ropes->gdata[0].ropeQuantity = 1;
    ropes->gdata[0].ropeIdx = new int[1]{0};
    auto *rope = ropes->rlist[0];
    rope->ropeNum = 42;
    rope->btie = true;
    rope->pBeg = sail->ss.hardPoints[0];
    MESSAGE tie;
    tie.Reset("lplpl", int32_t(MSG_SAIL_ROPE_TIE), uintptr_t(0), int32_t(0),
              reinterpret_cast<uintptr_t>(&rope->pBeg), int32_t(42));
    sails->ProcessMessage(tie);
    if (!sail->sailtrope.pnttie[0] || sail->sailtrope.pPos[0] != &rope->pBeg)
        return 2;
    std::vector<SAILVERTEX> vertices(SAIL_ROW_MAX * SAIL_COL_MAX);
    sail->goWave(vertices.data(), 0);
    std::fprintf(stderr, "live_tie_wave_pass=1\n");
    if (remove) {
        MESSAGE deletion;
        deletion.Reset("lil", int32_t(MSG_ROPE_DELETE), entid_t(777), int32_t(42));
        ropes->ProcessMessage(deletion);
        if (!rope->bDeleted) return 3;
        ropes->DoSTORM_DELETE();
        std::fprintf(stderr, "rope_deleted=%d retained_tie=%d\n", ropes->ropeQuantity == 0,
                     sail->sailtrope.pnttie[0]);
        sail->goWave(vertices.data(), 0);
    }
    std::fprintf(stderr, "wave_after_transition_pass=1\n");
    std::fflush(stderr);
    // Avoid unrelated renderer/core shutdown in this allocation-only fixture.
    std::_Exit(0);
}
