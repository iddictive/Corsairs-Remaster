#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
struct VDATA{std::array<int,5> v{};void Get(int&x,int n){x=v[n];}};
struct Core{VDATA data;bool missing=false;VDATA*Event(const char*){return missing?nullptr:&data;}}core;
struct FRECT{float left,top,right,bottom;};
struct BI_ONETEXTURE_VERTEX{float tu=0,tv=0;bool operator==(const BI_ONETEXTURE_VERTEX&)const=default;};
struct RS{std::array<BI_ONETEXTURE_VERTEX,16> vertices{};int locks=0;void*LockVertexBuffer(int){++locks;return vertices.data();}void UnLockVertexBuffer(int){}};
struct Nav{int m_curCharge=-999,m_curSailState=-999,m_curWindPic=-999,m_curPowder=-999,m_bPowderRunOut=0,m_idCurChargeVBuf=1;struct Grid{int x=8,y=8;}m_ChargeGreed,m_WindGreed,m_SailGreed,m_PowderGreed;RS storage;RS*rs=&storage;void CalculateTextureRect(FRECT&r,int n,int x,int y){assert(n>=0);r={float(n%x)/x,float(n/x)/y,float(n%x+1)/x,float(n/x+1)/y};}};
struct Baseline:Nav{void UpdateCurrentCharge();};struct Candidate:Nav{void UpdateCurrentCharge();};
void Baseline::UpdateCurrentCharge()
{
    VDATA *pVD = core.Event("GetCurrentCharge");
    if (pVD == nullptr)
        return;
    int32_t curCharge = m_curCharge;
    int32_t curSailState = m_curSailState;
    int32_t curWindPic = m_curWindPic;
    int32_t curPowder = m_curPowder;
    int32_t isPowderRunOut = 0;
    pVD->Get(curCharge, 0);
    pVD->Get(curSailState, 1);
    pVD->Get(curWindPic, 2);
    pVD->Get(curPowder, 3);
    pVD->Get(isPowderRunOut, 4);
    m_bPowderRunOut = isPowderRunOut;
    if (curCharge == m_curCharge && curSailState == m_curSailState && curWindPic == m_curWindPic &&
        curPowder == m_curPowder)
        return;
    m_curCharge = curCharge;
    m_curSailState = curSailState;
    m_curWindPic = curWindPic;
    m_curPowder = curPowder;
    if (m_curCharge < 0)
        m_curCharge = 0;
    if (m_curSailState < 0)
        m_curSailState = 0;
    if (m_curWindPic < 0)
        m_curWindPic = 0;
    if (m_curPowder < 0)
        m_curPowder = 0;

    auto *pV = static_cast<BI_ONETEXTURE_VERTEX *>(rs->LockVertexBuffer(m_idCurChargeVBuf));
    if (pV != nullptr)
    {
        FRECT texRect;
        CalculateTextureRect(texRect, m_curCharge, m_ChargeGreed.x, m_ChargeGreed.y);
        pV[0].tu = texRect.left;
        pV[0].tv = texRect.top;
        pV[1].tu = texRect.left;
        pV[1].tv = texRect.bottom;
        pV[2].tu = texRect.right;
        pV[2].tv = texRect.top;
        pV[3].tu = texRect.right;
        pV[3].tv = texRect.bottom;

        CalculateTextureRect(texRect, m_curWindPic, m_WindGreed.x, m_WindGreed.y);
        pV[4].tu = texRect.left;
        pV[4].tv = texRect.top;
        pV[5].tu = texRect.left;
        pV[5].tv = texRect.bottom;
        pV[6].tu = texRect.right;
        pV[6].tv = texRect.top;
        pV[7].tu = texRect.right;
        pV[7].tv = texRect.bottom;

        CalculateTextureRect(texRect, m_curSailState, m_SailGreed.x, m_SailGreed.y);
        pV[8].tu = texRect.left;
        pV[8].tv = texRect.top;
        pV[9].tu = texRect.left;
        pV[9].tv = texRect.bottom;
        pV[10].tu = texRect.right;
        pV[10].tv = texRect.top;
        pV[11].tu = texRect.right;
        pV[11].tv = texRect.bottom;

        CalculateTextureRect(texRect, m_curPowder, m_PowderGreed.x, m_PowderGreed.y);
        pV[12].tu = texRect.left;
        pV[12].tv = texRect.top;
        pV[13].tu = texRect.left;
        pV[13].tv = texRect.bottom;
        pV[14].tu = texRect.right;
        pV[14].tv = texRect.top;
        pV[15].tu = texRect.right;
        pV[15].tv = texRect.bottom;

        rs->UnLockVertexBuffer(m_idCurChargeVBuf);
    }
}
void Candidate::UpdateCurrentCharge()
{
    VDATA *pVD = core.Event("GetCurrentCharge");
    if (pVD == nullptr)
        return;
    int32_t curCharge = m_curCharge;
    int32_t curSailState = m_curSailState;
    int32_t curWindPic = m_curWindPic;
    int32_t curPowder = m_curPowder;
    int32_t isPowderRunOut = 0;
    pVD->Get(curCharge, 0);
    pVD->Get(curSailState, 1);
    pVD->Get(curWindPic, 2);
    pVD->Get(curPowder, 3);
    pVD->Get(isPowderRunOut, 4);
    // Script uses the selected Phonk weapon as a negative icon sentinel. Its
    // name and real charge/reload use the existing Ammo/Powder text rows.
    const bool phonkWeapon = curCharge == -6 || curCharge == -7;
    if (phonkWeapon) curPowder = -1;
    m_bPowderRunOut = isPowderRunOut;
    if (curCharge == m_curCharge && curSailState == m_curSailState && curWindPic == m_curWindPic &&
        curPowder == m_curPowder)
        return;
    m_curCharge = curCharge;
    m_curSailState = curSailState;
    m_curWindPic = curWindPic;
    m_curPowder = curPowder;
    if (m_curCharge < 0 && !phonkWeapon)
        m_curCharge = 0;
    if (m_curSailState < 0)
        m_curSailState = 0;
    if (m_curWindPic < 0)
        m_curWindPic = 0;
    if (m_curPowder < 0 && !phonkWeapon)
        m_curPowder = 0;

    auto *pV = static_cast<BI_ONETEXTURE_VERTEX *>(rs->LockVertexBuffer(m_idCurChargeVBuf));
    if (pV != nullptr)
    {
        FRECT texRect;
        CalculateTextureRect(texRect, std::max(0, m_curCharge), m_ChargeGreed.x, m_ChargeGreed.y);
        pV[0].tu = texRect.left;
        pV[0].tv = texRect.top;
        pV[1].tu = texRect.left;
        pV[1].tv = texRect.bottom;
        pV[2].tu = texRect.right;
        pV[2].tv = texRect.top;
        pV[3].tu = texRect.right;
        pV[3].tv = texRect.bottom;

        CalculateTextureRect(texRect, m_curWindPic, m_WindGreed.x, m_WindGreed.y);
        pV[4].tu = texRect.left;
        pV[4].tv = texRect.top;
        pV[5].tu = texRect.left;
        pV[5].tv = texRect.bottom;
        pV[6].tu = texRect.right;
        pV[6].tv = texRect.top;
        pV[7].tu = texRect.right;
        pV[7].tv = texRect.bottom;

        CalculateTextureRect(texRect, m_curSailState, m_SailGreed.x, m_SailGreed.y);
        pV[8].tu = texRect.left;
        pV[8].tv = texRect.top;
        pV[9].tu = texRect.left;
        pV[9].tv = texRect.bottom;
        pV[10].tu = texRect.right;
        pV[10].tv = texRect.top;
        pV[11].tu = texRect.right;
        pV[11].tv = texRect.bottom;

        CalculateTextureRect(texRect, std::max(0, m_curPowder), m_PowderGreed.x, m_PowderGreed.y);
        pV[12].tu = texRect.left;
        pV[12].tv = texRect.top;
        pV[13].tu = texRect.left;
        pV[13].tv = texRect.bottom;
        pV[14].tu = texRect.right;
        pV[14].tv = texRect.top;
        pV[15].tu = texRect.right;
        pV[15].tv = texRect.bottom;

        rs->UnLockVertexBuffer(m_idCurChargeVBuf);
    }
}
int main(){int rows=0;for(int c:{-1,0,32,33,34,35,40})for(int sail:{-1,18,20})for(int wind:{-1,61})for(int powder:{-1,0,5}){
 core.data.v={c,sail,wind,powder,0};Baseline a;Candidate b;a.UpdateCurrentCharge();b.UpdateCurrentCharge();
 assert(a.m_curCharge==b.m_curCharge&&a.m_curPowder==b.m_curPowder&&a.m_curSailState==b.m_curSailState&&a.m_curWindPic==b.m_curWindPic&&a.storage.vertices==b.storage.vertices);++rows;}
 Candidate n;for(int c:{32,-6,-7,-6,33}){core.data.v={c,20,61,4,0};n.UpdateCurrentCharge();assert(n.m_curCharge==c&&n.m_curSailState==20&&n.m_curWindPic==61);assert(n.m_curPowder==(c<0?-1:4));const int locks=n.storage.locks;n.UpdateCurrentCharge();assert(n.storage.locks==locks);}
 core.missing=true;const int charge=n.m_curCharge;n.UpdateCurrentCharge();assert(n.m_curCharge==charge);
 std::printf("PASS native HUD owner: %d ordinary parity rows,6/7/OFF transitions,stable frame,null event\n",rows);}
