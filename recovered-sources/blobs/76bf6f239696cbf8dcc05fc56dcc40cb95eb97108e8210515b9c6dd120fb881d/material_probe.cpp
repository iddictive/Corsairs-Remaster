#ifndef MATERIAL_HEADER
#define MATERIAL_HEADER "ship_hull_material.hpp"
#endif
#include MATERIAL_HEADER
#include <cstdio>
#include <cmath>
using namespace storm::ship::material;
Triangle tri(Vec3 a,Vec3 b,Vec3 c){Triangle t{};t[0].position=a;t[1].position=b;t[2].position=c;for(auto&v:t)v.normal={0,0,1};return t;}
void panel(Surface&s,float x){s.outer[0].push_back(tri({x,0,0},{x+1,0,0},{x+1,1,0}));s.outer[0].push_back(tri({x,0,0},{x+1,1,0},{x,1,0}));}
int main(){
  Surface before;panel(before,0);panel(before,1);panel(before,2);
  Surface after;panel(after,0);panel(after,2);
  std::vector<Surface> pieces;bool deferred=false;
  if(!releaseSections(&before,after,0,{{0,.1f}},pieces,deferred))return 10;
  std::printf("zero_allowance retained_area=%.6f emitted=%zu deferred=%d (expect 2,0,1)\n",area(renderMesh(after)),pieces.size(),deferred);
  const bool conservationPassed=std::abs(area(renderMesh(after))-2.)<=1.e-6&&pieces.empty()&&deferred;
  Surface noChange=before;deferred=false;
  if(!releaseSections(&before,noChange,0,{{0,.1f}},pieces,deferred)||std::abs(area(renderMesh(noChange))-3.)>1.e-6||deferred)return 2;
  Surface hull;
  const Vec3 corners[]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};
  const unsigned faces[][3]={{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},{3,7,6},{3,6,2},{0,4,7},{0,7,3},{1,2,6},{1,6,5}};
  for(const auto&f:faces)hull.outer[0].push_back(tri(corners[f[0]],corners[f[1]],corners[f[2]]));
  hull.supportOriginal=supportSource(hull.outer);
  auto source=std::make_shared<MaterialCellSource>();source->solids.push_back(std::make_shared<const Mesh>(hull.outer));source->bounds.push_back(bounds(hull.outer));source->index.build(source->bounds);
  auto cells=std::make_shared<MaterialCells>();cells->source=source;hull.cells=cells;
  auto layout=storm_ship_structure::build(hull,{{0,.1f}});
  auto state=storm_ship_structure::seed(layout,hull,{{0,.1f}});
  for(size_t n=0;n<state.alive.size();++n)if(layout->cells[n].bounds.lo.x>.7f){state.alive[n]=0;state.cellHealth[n]=0;}
  for(size_t n=0;n<layout->bonds.size();++n)if(!state.alive[layout->bonds[n].a]||!state.alive[layout->bonds[n].b])state.bondHealth[n]=0;
  Input input;input.surface=std::make_shared<const Surface>(hull);input.structure=std::make_shared<const storm_ship_structure::State>(state);input.wood={{0,.1f}};input.plane.normal={1,0,0};input.releaseOnly=true;input.hasDeferredSections=true;input.fragmentLimit=1;
  storm_ship_structure::DamageResult debug;debug.state=state;structure_geometry::includeDeferred(debug);structure_geometry::Realization realization;
  std::printf("layout=%zu valid=%d broken=%zu realize=%d\n",layout->cells.size(),storm_ship_structure::validState(state),debug.brokenCells.size(),structure_geometry::realize(input,debug,realization));
  Projection projection;if(!storm::ship::material::prepare(input,projection)){std::puts("fracture prepare rejected");return 4;}
  size_t stale=0;for(size_t n=0;n<state.alive.size();++n)if(!state.alive[n]&&projection.structure->physicalCellHealth[n]>1.e-6f){++stale;std::printf("stale cell %zu physical %.9f\n",n,projection.structure->physicalCellHealth[n]);}
  std::printf("released_bodies=%zu stale_failed_capacity=%zu deferred=%d\n",projection.result.fragments.size(),stale,projection.hasDeferredSections);
  if(stale||projection.hasDeferredSections||projection.result.fragments.empty())return 5;
  Input follow=input;follow.surface=projection.kept;follow.structure=projection.structure;follow.hasDeferredSections=projection.hasDeferredSections;Projection unchanged;if(!storm::ship::material::prepare(follow,unchanged)||unchanged.result.changed||unchanged.result.stateChanged)return 6;
  std::puts("material conservation / no-fracture / physical-capacity refresh / release-only continuation PASS");
  return conservationPassed?0:1;
}
