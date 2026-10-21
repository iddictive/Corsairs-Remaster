
#include "core_impl.h"
#include "sail.h"
#include "rope.h"
#include "shared/sail_msg.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
extern "C" bool StormMetalSetShipAuxiliaryDetail(void*,bool){return true;} // CPU fixture never draws
void need(bool b,int code){if(!b){fprintf(stderr,"FAIL %d\n",code);std::_Exit(code);}}
int main(int argc,char**argv){
 const char*mode=argc>1?argv[1]:"live";auto&s=static_cast<CoreImpl&>(core);
 auto*sails=new SAIL;auto*ropes=new ROPE;
 auto sid=s.entity_manager_.InsertEntity(sails,MakeHashValue("sail"));sails->data_.id=sid;
 auto rid=s.entity_manager_.InsertEntity(ropes,MakeHashValue("rope"));ropes->data_.id=rid;
 need(core.GetEntityId("sail")==sid&&core.GetEntityId("rope")==rid,2);
 sails->WindVect=new float[1024]{};sails->WINDVECTOR_QUANTITY=1024;sails->WINDVECTOR_TINCR=1;sails->WINDVECTOR_TADD=0;sails->TsailWindDepend=0;sails->fTHoleFlexDepend=0;sails->globalWind.base=0;
 sails->groupQuantity=2;sails->gdata=new SAIL::GROUPDATA[2]{};sails->gdata[0].modelEI=777;sails->gdata[1].modelEI=888;
 sails->sailQuantity=2;sails->slist=new SAILONE*[2];
 for(int i=0;i<2;++i){auto*so=sails->slist[i]=new SAILONE;so->pp=sails;so->HostNum=i;so->hostNode=reinterpret_cast<NODE*>(uintptr_t(0x1000+i));so->groupNum=0;so->ss.eSailType=SAIL_TREANGLE;so->m_dwRow=SAIL_ROW_MAX;so->m_dwCol=SAIL_COL_MAX;so->ss.boundSphere.r=1;for(int j=0;j<3;++j)so->ss.hardPoints[j]=CVECTOR(float(j*10+1),float(j*2+1),float(j*3+1));}
 ropes->groupQuantity=2;ropes->gdata=new ROPE::GROUPDATA[2]{};ropes->gdata[0].modelEI=777;ropes->gdata[0].ropeQuantity=2;ropes->gdata[0].ropeIdx=new int[2]{0,1};ropes->gdata[1].modelEI=888;ropes->gdata[1].ropeQuantity=1;ropes->gdata[1].ropeIdx=new int[1]{2};ropes->ropeQuantity=3;ropes->rlist=new ROPE::ROPEDATA*[3];
 for(int i=0;i<3;++i){auto*r=ropes->rlist[i]=new ROPE::ROPEDATA{};r->ropeNum=i==1?43:42;r->HostGroup=i==2?1:0;r->btie=true;auto*so=sails->slist[r->HostGroup];r->pBeg=so->ss.hardPoints[i==1?2:0];core.Send_Message(sid,"lplpl",int32_t(MSG_SAIL_ROPE_TIE),so->hostNode,int32_t(0),&r->pBeg,int32_t(r->ropeNum));}
 auto*dying=ropes->rlist[0];dying->etie=true;dying->pEnd=sails->slist[0]->ss.hardPoints[1];core.Send_Message(sid,"lplpl",int32_t(MSG_SAIL_ROPE_TIE),sails->slist[0]->hostNode,int32_t(0),&dying->pEnd,int32_t(-42));
 auto*keep=ropes->rlist[1];auto*other=ropes->rlist[2];
 std::vector<SAILVERTEX> vertices(SAIL_ROW_MAX*SAIL_COL_MAX);for(int i=0;i<2;++i)sails->slist[i]->goWave(vertices.data(),0);
 fprintf(stderr,"live_enrolled_two_models=PASS\n");
 bool retie=std::strcmp(mode,"retie")==0;
 if(retie){keep->pBeg=sails->slist[0]->ss.hardPoints[0];core.Send_Message(sid,"lplpl",int32_t(MSG_SAIL_ROPE_TIE),sails->slist[0]->hostNode,int32_t(0),&keep->pBeg,int32_t(43));}
 if(std::strcmp(mode,"remove")==0||retie){core.Send_Message(rid,"lil",int32_t(MSG_ROPE_DELETE),entid_t(777),int32_t(42));ropes->DoSTORM_DELETE();
 #ifdef EXPECT_FIXED
 need(sails->slist[0]->sailtrope.pnttie[0]==retie,3);need(!sails->slist[0]->sailtrope.pnttie[1],4);need(sails->slist[0]->sailtrope.pnttie[2],5);need(sails->slist[1]->sailtrope.pnttie[0],6);
 #endif
 }
 else if(std::strcmp(mode,"group")==0){core.Send_Message(rid,"li",int32_t(MSG_ROPE_DEL_GROUP),entid_t(777));ropes->DoSTORM_DELETE();
 #ifdef EXPECT_FIXED
 need(!sails->slist[0]->sailtrope.pnttie[0]&&!sails->slist[0]->sailtrope.pnttie[1]&&!sails->slist[0]->sailtrope.pnttie[2],7);need(sails->slist[1]->sailtrope.pnttie[0],8);
 #endif
 }
 else if(std::strcmp(mode,"destructor")==0){delete ropes;
 #ifdef EXPECT_FIXED
 for(int i=0;i<2;++i)for(int j=0;j<4;++j)need(!sails->slist[i]->sailtrope.pnttie[j],9);
 #endif
 }
 else if(std::strcmp(mode,"sail-removed")==0){s.entity_manager_.EraseEntity(sid);need(core.GetEntityId("sail")==invalid_entity,10);delete ropes;fprintf(stderr,"removed_receiver_shutdown=PASS\n");std::_Exit(0);}
 for(int i=0;i<2;++i)sails->slist[i]->goWave(vertices.data(),0);
 fprintf(stderr,"mode=%s transition_wave=PASS\n",mode);std::fflush(stderr);std::_Exit(0);
}
