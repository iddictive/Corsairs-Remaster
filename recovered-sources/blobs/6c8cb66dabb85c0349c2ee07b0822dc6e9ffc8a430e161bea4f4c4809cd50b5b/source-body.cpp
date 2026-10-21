
#include <cassert>
#include <cstdint>
#include <map>
using entid_t=uint64_t;
struct Entity {virtual ~Entity()=default;};
struct GEOS {uint64_t generation=1;uint64_t GeometryGeneration()const{return generation;}};
struct Node {GEOS *geo=nullptr;};
struct Model {Node node;Node*GetNode(int){return &node;}};
struct SHIP_BASE:Entity {unsigned revision=0;Model model;bool retained=true;mutable unsigned queries=0;
 unsigned GetHullContourRevision()const{return revision;}Model*GetModel(){return &model;}
 bool IsHullPositionRetained(int)const{++queries;return retained;}};
struct Core {std::map<entid_t,Entity*>entities;Entity*GetEntityPointer(entid_t id){return entities[id];}} core;
struct AICannon {float fDamaged=0;entid_t eidParent=1;int vPos=0;
 mutable entid_t supportParent=0;mutable uint64_t supportGeneration=0;mutable uint32_t supportRevision=uint32_t(-1);mutable bool supportRetained=true;
 bool isDamaged()const;};
bool AICannon::isDamaged() const
{
    if (fDamaged >= 1.f) return true;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    auto *ship = dynamic_cast<SHIP_BASE *>(core.GetEntityPointer(eidParent));
    if (!ship || !ship->GetHullContourRevision()) return false;
    auto *model = ship->GetModel();
    auto *root = model ? model->GetNode(0) : nullptr;
    const uint64_t generation = root && root->geo ? root->geo->GeometryGeneration() : 0;
    const auto revision = ship->GetHullContourRevision();
    if (supportParent != eidParent || supportGeneration != generation || supportRevision != revision)
    {
        supportRetained = ship->IsHullPositionRetained(vPos);
        supportParent = eidParent; supportGeneration = generation; supportRevision = revision;
    }
    return !supportRetained;
#else
    return false;
#endif
}
int main(){GEOS geometry;SHIP_BASE ship;Entity fort;ship.model.node.geo=&geometry;core.entities[1]=&ship;core.entities[2]=&fort;AICannon gun;
 assert(!gun.isDamaged()&&ship.queries==0);ship.revision=1;
 assert(!gun.isDamaged()&&ship.queries==1);assert(!gun.isDamaged()&&ship.queries==1);
 ship.retained=false;++ship.revision;assert(gun.isDamaged()&&ship.queries==2);
 assert(gun.isDamaged()&&ship.queries==2);ship.retained=true;++geometry.generation;
 assert(!gun.isDamaged()&&ship.queries==3);gun.fDamaged=1;assert(gun.isDamaged()&&ship.queries==3);
 gun.eidParent=2;gun.fDamaged=0;assert(!gun.isDamaged());gun.fDamaged=1;assert(gun.isDamaged());
 gun.eidParent=1;gun.fDamaged=0;ship.retained=false;ship.revision=0;assert(!gun.isDamaged());
 ship.revision=1;assert(gun.isDamaged()&&ship.queries==4);
}
