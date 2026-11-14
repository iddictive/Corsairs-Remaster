// Included after the extracted engine functions by matrix.py.
int main() {
 namespace air = storm::air_intercept;
 auto point=[](CVECTOR p){return air::Vec3{p.x,p.y,p.z};};
 const char* names[]={"Kinzhal","Kalibr","Shtorm","Oreshnik"};
 missile::Type types[]={missile::Type::Kinzhal,missile::Type::Kalibr,missile::Type::Shtorm,missile::Type::Oreshnik};
 constexpr float dt=.025f;
 for(float range:{800.f,1600.f,2400.f,4000.f,8000.f}) for(int typ=0;typ<4;typ++) for(int pat=0;pat<2;pat++) {
  unsigned acquired=0, launched=0, hits=0, evaded=0, armed=0;
  for(uint32_t serial=MATRIX_SEED;serial<MATRIX_SEED+MATRIX_TRIALS;serial++) {
   const uint32_t defenseSerial=serial*0x9e3779b9u;
   NuclearMissile m; m.active=true; m.type=types[typ];
   m.launch=CVECTOR((shotVariation(serial,2,100)*2.f-1.f)*1000.f,10.f,10000.f);
   m.target=CVECTOR((shotVariation(serial,2,102)*2.f-1.f)*80.f,5.5f,(shotVariation(serial,2,103)*2.f-1.f)*80.f);
   m.owner=2; m.shotId=serial; varyMissile(m,2);
   const CVECTOR ship(0.f,0.f,0.f), start(0.f,5.5f,0.f), forward(0.f,0.f,1.f);
   std::array<OrdnanceState::DefenseReaction,64> reaction{};
   bool detected=false, fired=false; float nextScan=.25f; OrdnanceState::Interceptor i{};
   for(m.age=.25f;m.age<m.duration;m.age+=dt) {
    advanceDefenseReaction(reaction,dt);
    if(m.age+1.e-5f<nextScan) continue;
    nextScan+=.1f;
    const auto p=nuclearPosition(m,m.age), relative=p-ship;
    CVECTOR v(0.f),a(0.f);
    if(observeBallistic(m.type)) observeShtorm(m,p,v,a); else v=nuclearVelocity(m,m.age);
    if((~relative)>range*range || (relative|v)>=0.f || !interceptorForwardSector(forward,ship,p)) continue;
    if(!detected) { detected=true; acquired++; }
    const auto stale=observeBallistic(m.type)?v:nuclearVelocity(m,std::max(0.f,m.age-.12f));
    const float limit=std::min(60.f,m.duration-m.age);
    float flight=0.f; CVECTOR target(0.f);
    bool ok=false;
    if(pat) {
     float speed=interceptorProfile(true).planningSpeed;
     ok=observeBallistic(m.type)?interceptorEstimateShtorm(start,p,v,a,speed,limit,flight,target):
       m.type==missile::Type::Kalibr?interceptorEstimateCruise(m,start,p,stale,speed,limit,flight,target):
       interceptorEstimate(start,p,stale,speed,limit,flight,target);
     if(!ok && m.type==missile::Type::Oreshnik) { target=p;flight=std::clamp(std::sqrt(~(p-start))/speed,.15f,15.f);ok=true; }
    } else {
     const auto accel=observeBallistic(m.type)?a:observedMissileAcceleration(m,stale);
     const auto vel=stale+(m.type==missile::Type::Kalibr?accel*std::min(.12f,std::max(0.f,m.age)):CVECTOR(0.f));
     ok=estimatePantsir(start,p,vel,accel,limit,defenseSerial,flight,target,observeBallistic(m.type));
    }
    if(!ok || !interceptorForwardSector(forward,start,target)) continue;
    target+=interceptorAimError(start,target,std::sqrt(~relative),flight,defenseSerial,1)+interceptorTrackingError(m,flight,defenseSerial,1);
    if(!interceptorForwardSector(forward,start,target)) continue;
    if(!defenseReactionReady(reaction,2,serial,0,defenseSerial,1)) continue;
    float distance=std::sqrt(~(target-start)); float arc=std::clamp(distance*.02f,3.f,20.f);
    i=OrdnanceState::Interceptor{start,target,0.f,flight,arc,0.f,defenseSerial,2,serial,0};
    i.blastRadius=15.f+15.f*shotVariation(defenseSerial,1,0x6c3874a5u);
    i.fuseDelay=MATRIX_FUSE_MIN_MS==1.f && MATRIX_FUSE_MAX_MS==3.f ? interceptorFuseDelay(defenseSerial,1) :
        .001f*(MATRIX_FUSE_MIN_MS+(MATRIX_FUSE_MAX_MS-MATRIX_FUSE_MIN_MS)*shotVariation(defenseSerial,1,0xa498731du));
    if(pat) initializePatriot(i,p); else i.duration=std::max(.05f,pantsirFlightTime(distance,defenseSerial));
    launched++; fired=true; break;
   }
   if(!fired) continue;
   bool ended=false, triggered=false;
   while(m.age<m.duration && i.age<i.duration && !ended) {
    float step=std::min(dt,m.duration-m.age);
    const float beforeAge=i.age; auto m0=nuclearPosition(m,m.age);
    auto i0=interceptorPosition(i,i.age/i.duration);
    m.age=std::min(m.duration,m.age+step);
    if(pat) advancePatriot(i,&m,step); else i.age=std::min(i.duration,i.age+step);
    auto i1=interceptorPosition(i,i.age/i.duration);
    if(!i.evasionTriggered && (~(nuclearPosition(m,m.age)-i1))<=300.f*300.f) {
     i.evasionTriggered=true; evadeInterceptor(m,ship,nullptr); if((~m.evasionOffset)>.001f)evaded++;
    }
    auto m1=nuclearPosition(m,m.age);
    const double available=pat?double(i.age-beforeAge):std::min(double(step),double(i.duration-beforeAge));
    if(!(available>0.)) continue;
    const double iExpiry=i.duration-beforeAge<=step+1.e-6f?double(i.duration-beforeAge):std::numeric_limits<double>::infinity();
    const double mExpiry=m.age>=m.duration?double(step):std::numeric_limits<double>::infinity();
    air::Body b{1,1,1,i.blastRadius,iExpiry,{{0.,available,point(i0),point(i1)}}};
    air::Body t{2,2,2,0.,mExpiry,{{0.,double(step),point(m0),point(m1)}}};
    const double pending=i.fuseAtAge>=0.f?std::max(0.,double(i.fuseAtAge-beforeAge)):std::numeric_limits<double>::infinity();
    const double sensorStart=pat?double(std::max(0.f,i.sensorEnableAge-beforeAge)):0.;
    auto events=air::resolve(std::vector<air::Interceptor>{{b,i.blastRadius,true,i.fuseDelay,pending,sensorStart}},std::vector<air::Body>{t},[](auto x,auto y){return x!=y;});
    for(const auto &event:events) {
     if(event.armed) { i.fuseAtAge=beforeAge+float(event.contact.time)+i.fuseDelay; if(!triggered){armed++;triggered=true;} }
     else { if(!event.splash.empty()) hits++; ended=true; break; }
    }
   }
  }
  std::printf("%.0f,%s,%s,%u,%u,%u,%u,%.2f,%u,%u\n",range,names[typ],pat?"Patriot":"Pantsir",unsigned(MATRIX_TRIALS),acquired,launched,hits,100.f*hits/MATRIX_TRIALS,armed,evaded);
 }
}
