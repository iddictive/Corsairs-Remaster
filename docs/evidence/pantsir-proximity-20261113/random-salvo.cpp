// Source-derived experiment, not a copy of the engine update loop.
int main() {
 namespace air=storm::air_intercept;
 auto pt=[](CVECTOR p){return air::Vec3{p.x,p.y,p.z};};
 struct Shot { NuclearMissile m; bool alive=true; };
 struct Defense { OrdnanceState::Interceptor i; size_t target; bool live=true; };
 const char *names[]={"Kinzhal","Kalibr","Shtorm","Oreshnik"};
 const missile::Type types[]={missile::Type::Kinzhal,missile::Type::Kalibr,missile::Type::Shtorm,missile::Type::Oreshnik};
 constexpr float dt=.025f, horizon=45.f;
 for(int pat=0;pat<2;pat++) {
  unsigned volleys[4]={},shots[4]={},kills[4]={},launched=0,denied=0,peak=0;
  for(unsigned trial=0;trial<TRIALS;trial++) {
   std::mt19937 rng(SEED+trial); auto rnd=[&](unsigned n){return rng()%n;};
   unsigned ship=rnd(5), requested=ship==4?3+rnd(3):1+rnd(3);
   float low=2+rnd(5), high=low+3+rnd(4);
   float range=1200+100*rnd(25), interval=.2f+.1f*rnd(7); unsigned cap=2+rnd(7);
   if(DEFENSE_EXTREME==1) {range=1200.f; interval=.8f; cap=2;}
   if(DEFENSE_EXTREME==2) {range=3600.f; interval=.2f; cap=8;}
   if(pat) interval*=1.5f;
   // Surrogate stable entity IDs: source hashes the entity ID and initial serial.
   uint32_t attacker=2+trial*2, defender=3+trial*2, serial=0, ds=0;
   uint32_t bits=defender ^ 0x9e3779b9u; bits^=bits>>16;bits*=0x7feb352du;bits^=bits>>15;
   unsigned rate=5+bits%16, capacity=rate>=12?4:2; float charges=capacity;
   float cooldown=0, delay=0, dc=0, scan=0; unsigned remain=0, volleyIndex=0; int kind=0;
   std::vector<Shot> missiles; std::vector<Defense> defenders;
   std::array<OrdnanceState::DefenseReaction,64> reaction{};
   const CVECTOR shipPos(0.f,0.f,0.f), start(0.f,5.5f,0.f), forward(0.f,0.f,1.f);
   for(float t=0;t<horizon+60.f;t+=dt) {
    cooldown=std::max(0.f,cooldown-dt);delay=std::max(0.f,delay-dt);
    dc=std::max(0.f,dc-dt);scan=std::max(0.f,scan-dt);
    charges=std::min(float(capacity),charges+dt*rate/60.f); advanceDefenseReaction(reaction,dt);
    unsigned active=0; for(auto &s:missiles) active+=s.alive;
    if(t<horizon && !remain && cooldown<=0 && active<64) {
     uint32_t random=++serial*0x9e3779b9u ^ attacker; random^=random>>16;random*=0x7feb352du;random^=random>>15;
     cooldown=low+(high-low)*float(random&65535u)/65535.f;
     float choice=shotVariation(serial,attacker,0x5e10412du);
     kind=choice<.05f?0:choice<.10f?3:choice<.55f?1:2;
     if(kind==0) cooldown*=2;
     remain=std::min<unsigned>(kind==0?std::min(requested,2u):requested,64-active);
     volleyIndex=0;delay=0;volleys[kind]++;
    }
    if(remain && delay<=0 && active<64) {
     if(volleyIndex++) ++serial;
     NuclearMissile m; m.active=true;m.type=types[kind];m.owner=attacker;m.shotId=serial;
     m.launch=CVECTOR(0.f,10.f,10000.f);m.target=CVECTOR((shotVariation(serial,attacker,102)*2-1)*80,5.5f,(shotVariation(serial,attacker,103)*2-1)*80);
     varyMissile(m,attacker); missiles.push_back({m,true});shots[kind]++;remain--;
     delay=.2f+.15f*shotVariation(serial,attacker,0xb192a437u);
    }
    if(dc<=0 && scan<=0 && (UNLIMITED_CHARGES || charges>=1) && defenders.size()<cap) {
     scan=.1f;
     for(size_t n=0;n<missiles.size();n++) {
      auto &s=missiles[n];auto &m=s.m;if(!s.alive)continue;
      auto p=nuclearPosition(m,m.age), rel=p-shipPos;CVECTOR v(0.f),a(0.f);
      if(observeBallistic(m.type)) observeShtorm(m,p,v,a); else v=nuclearVelocity(m,m.age);
      if((~rel)>range*range || (rel|v)>=0 || !interceptorForwardSector(forward,shipPos,p))continue;
      auto stale=observeBallistic(m.type)?v:nuclearVelocity(m,std::max(0.f,m.age-.12f));
      float flight=0;CVECTOR target(0.f);float limit=std::min(2.f,m.duration-m.age);bool ok=false;
      if(pat) {
       float speed=interceptorProfile(true).planningSpeed;
       ok=observeBallistic(m.type)?interceptorEstimateShtorm(start,p,v,a,speed,limit,flight,target):m.type==missile::Type::Kalibr?interceptorEstimateCruise(m,start,p,stale,speed,limit,flight,target):interceptorEstimate(start,p,stale,speed,limit,flight,target);
       if(!ok && m.type==missile::Type::Oreshnik){target=p;flight=std::clamp(std::sqrt(~(p-start))/speed,.15f,15.f);ok=true;}
      } else {
       auto accel=observeBallistic(m.type)?a:observedMissileAcceleration(m,stale);
       auto vel=stale+(m.type==missile::Type::Kalibr?accel*std::min(.12f,std::max(0.f,m.age)):CVECTOR(0.f));
       ok=estimatePantsir(start,p,vel,accel,limit,ds+1,flight,target,observeBallistic(m.type));
      }
      if(!ok || !interceptorForwardSector(forward,start,target))continue;
      target+=interceptorAimError(start,target,std::sqrt(~rel),flight,ds+1,defender)+interceptorTrackingError(m,flight,ds+1,defender);
      if(!interceptorForwardSector(forward,start,target) || !defenseReactionReady(reaction,attacker,m.shotId,0,ds+1,defender))continue;
      float distance=std::sqrt(~(target-start)), arc=std::clamp(distance*.02f,3.f,20.f);
      OrdnanceState::Interceptor i{start,target,0.f,std::max(.05f,flight),arc,0.f,++ds,attacker,m.shotId,0};
      i.blastRadius=15.f+15.f*shotVariation(ds,defender,0x6c3874a5u);
      i.fuseDelay=.002f+.005f*shotVariation(ds,defender,0xa498731du);
      if(pat)initializePatriot(i,p);else i.duration=std::max(.05f,pantsirFlightTime(distance,ds));
      defenders.push_back({i,n,true});if(!UNLIMITED_CHARGES) charges-=1;dc=interval;launched++;peak=std::max(peak,unsigned(defenders.size()));break;
     }
    } else if((!UNLIMITED_CHARGES && charges<1) || defenders.size()>=cap) denied++;
    // One segment per 25 ms step; the source batch resolver sees all active targets.
    for(auto &d:defenders) {
     if(!d.live)continue;auto &i=d.i;auto &tracked=missiles[d.target];
     float before=i.age;auto i0=interceptorPosition(i,i.age/i.duration);
     if(pat) advancePatriot(i,tracked.alive?&tracked.m:nullptr,dt);else i.age=std::min(i.duration,i.age+dt);
     auto i1=interceptorPosition(i,i.age/i.duration);
     if(tracked.alive && !i.evasionTriggered && (~(nuclearPosition(tracked.m,tracked.m.age)-i1))<=300.f*300.f) {i.evasionTriggered=true;evadeInterceptor(tracked.m,shipPos,nullptr);}
     double avail=std::min(double(dt),double(i.duration-before));if(avail<=0){d.live=false;continue;}
     air::Body b{1,1,1,i.blastRadius,i.duration-before<=dt+1e-6?double(i.duration-before):std::numeric_limits<double>::infinity(),{{0,avail,pt(i0),pt(i1)}}};
     std::vector<air::Body> targets;
     for(size_t n=0;n<missiles.size();n++)if(missiles[n].alive) {auto &m=missiles[n].m;float step=std::min(dt,m.duration-m.age);if(step>0)targets.push_back({n+2,2,n+2,0,step<dt?double(step):std::numeric_limits<double>::infinity(),{{0,double(step),pt(nuclearPosition(m,m.age)),pt(nuclearPosition(m,m.age+step))}}});}
     double pending=i.fuseAtAge>=0?std::max(0.,double(i.fuseAtAge-before)):std::numeric_limits<double>::infinity();
     auto events=air::resolve(std::vector<air::Interceptor>{{b,i.blastRadius,true,i.fuseDelay,pending,pat?double(std::max(0.f,i.sensorEnableAge-before)):0.}},targets,[](auto x,auto y){return x!=y;});
     for(auto &e:events) {if(e.armed)i.fuseAtAge=before+float(e.contact.time)+i.fuseDelay;else {for(auto id:e.splash){size_t n=size_t(id)-2;if(n<missiles.size()&&missiles[n].alive){missiles[n].alive=false;auto type=missiles[n].m.type;for(int k=0;k<4;k++)if(type==types[k])kills[k]++;}}d.live=false;break;}}
     if(i.age>=i.duration)d.live=false;
    }
    for(auto &s:missiles)if(s.alive){s.m.age+=dt;if(s.m.age>=s.m.duration)s.alive=false;}
    std::erase_if(defenders,[](auto &d){return !d.live;});
   }
  }
  for(int k=0;k<4;k++)std::printf("%s,%s,%u,%u,%u,%.2f,%u,%u,%u\n",pat?"Patriot":"Pantsir",names[k],volleys[k],shots[k],kills[k],shots[k]?100.*kills[k]/shots[k]:0.,launched,denied,peak);
 }
}
