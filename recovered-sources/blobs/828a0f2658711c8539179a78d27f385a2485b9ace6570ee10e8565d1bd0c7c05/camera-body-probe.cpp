#include <cassert>
#include <chrono>
#include <algorithm>
#include <iostream>
struct ATTRIBUTES { int third=0; int GetAttributeAsDword(const char*) {return third;} };
struct SEA_BASE {};
struct Core { SEA_BASE* sea=nullptr; unsigned revision=7; int GetEntityId(const char*){return 1;} void* GetEntityPointer(int){return sea;} unsigned GetScenePauseRevision(){return revision;} } core;
struct Common { ATTRIBUTES* pACharacter=nullptr; int eid=0,next=0; int GetEID(){return eid;} bool FindShip(){if(!next)return false;eid=next;return true;} };
struct SHIP_CAMERA:Common { SEA_BASE* pSea=nullptr; void* pIsland=(void*)1; int lIlsInitCnt=10; float angle=1.2f,distance=.7f; void SetCharacter(ATTRIBUTES*); };
struct FREE_CAMERA:Common { void* pIslandBase=(void*)1; void SetCharacter(ATTRIBUTES*); };
struct Zoom { int resets=0; void reset(){resets++;} };
struct DECK_CAMERA:Common { void* pathNode=nullptr; void* pModel=nullptr; bool walkerLoadFailed=true,bLoad=false; float localPosition=3.2f; Zoom aimZoom; unsigned zoomPauseRevision=0; std::chrono::steady_clock::time_point zoomLastUpdate=std::chrono::steady_clock::now(); int releases=0; void ReleaseWalker(){releases++;} void SetCharacter(ATTRIBUTES*); };

void SHIP_CAMERA::SetCharacter(ATTRIBUTES *_pACharacter)
{
    const auto previousModel = GetEID();
    pACharacter = _pACharacter;
    auto *const currentSea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    if ((_pACharacter && FindShip() && previousModel != GetEID()) || pSea != currentSea)
    {
        pIsland = nullptr;
        lIlsInitCnt = 0;
    }
    pSea = currentSea;
}

void FREE_CAMERA::SetCharacter(ATTRIBUTES *_pACharacter)
{
    const auto previousModel = GetEID();
    pACharacter = _pACharacter;
    if (_pACharacter && FindShip() && previousModel != GetEID())
        pIslandBase = nullptr;
}

void DECK_CAMERA::SetCharacter(ATTRIBUTES *_pACharacter)
{
    const bool sameCharacter = pACharacter == _pACharacter;
    const bool hadPath = pathNode != nullptr;
    const auto previousModel = GetEID();
    pACharacter = _pACharacter;
    const bool replacedModel = _pACharacter && FindShip() && previousModel != GetEID();
    const bool retainedView = sameCharacter && hadPath && replacedModel;
    // Ordinary reselection resets aim. A retained view crosses the scene's
    // layer-freeze revision without turning a held aim into a fresh press.
    if (retainedView) zoomPauseRevision = core.GetScenePauseRevision();
    else aimZoom.reset();
    if (!sameCharacter || replacedModel)
    {
        ReleaseWalker();
        walkerLoadFailed = false;
        pathNode = nullptr;
        pModel = nullptr;
        bLoad = retainedView;
        zoomLastUpdate = {};
    }
}
float eye(bool walkMode,bool telescope,int third,float h_eye) { ATTRIBUTES attrs{third}; auto* AttributesPointer=&attrs; const float DEFAULT_HEIGHT=1.6f;
    const float eyeLift = walkMode && (telescope ||
                          AttributesPointer->GetAttributeAsDword("ThirdPerson") == 0) ? 0.5f : 0.f;
    const float eyeHeight = eyeLift > 0.f ? std::max(h_eye, DEFAULT_HEIGHT) + eyeLift : h_eye;
return eyeHeight;}
int main(){
 ATTRIBUTES player,other; SEA_BASE oldSea,newSea; core.sea=&newSea;
 SHIP_CAMERA ship; ship.eid=1;ship.next=2;ship.pACharacter=&player;ship.pSea=&oldSea;ship.SetCharacter(&player);
 assert(ship.eid==2 && ship.pSea==&newSea && !ship.pIsland && ship.lIlsInitCnt==0);assert(ship.angle==1.2f && ship.distance==.7f);
 ship.pIsland=(void*)3; ship.lIlsInitCnt=4;ship.SetCharacter(&player);assert(ship.pIsland==(void*)3 && ship.lIlsInitCnt==4);
 FREE_CAMERA free;free.eid=1;free.next=2;free.SetCharacter(&player);assert(!free.pIslandBase);
 free.pIslandBase=(void*)4;free.SetCharacter(&player);assert(free.pIslandBase==(void*)4);
 DECK_CAMERA deck;deck.pACharacter=&player;deck.eid=1;deck.next=2;deck.pathNode=(void*)1;deck.pModel=(void*)2;deck.SetCharacter(&player);
 assert(deck.bLoad && !deck.pathNode && !deck.pModel && deck.releases==1);assert(deck.localPosition==3.2f && deck.aimZoom.resets==0 && deck.zoomPauseRevision==7);assert(deck.zoomLastUpdate==std::chrono::steady_clock::time_point{});
 deck.pathNode=(void*)3;deck.pModel=(void*)4;deck.bLoad=false;deck.SetCharacter(&player);assert(deck.pathNode==(void*)3 && deck.releases==1 && deck.aimZoom.resets==1);
 deck.next=3;deck.SetCharacter(&other);assert(!deck.bLoad && !deck.pathNode && deck.aimZoom.resets==2);
 DECK_CAMERA fresh;fresh.next=4;fresh.SetCharacter(&player);assert(!fresh.bLoad);
 for(float h: {.5f,1.6f,2.f}) {for(int third:{0,1}) {float spy=eye(true,true,third,h);assert(spy>=2.1f && spy==eye(true,false,0,h));assert(eye(true,false,1,h)==h);assert(eye(false,true,third,h)==h);}}
 std::cout<<"camera_rebind=pass held_zoom=pass fresh/same-scene/changed-character=pass spyglass_height=pass third_person/legacy=unchanged\n";
}
