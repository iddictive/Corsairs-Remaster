// Standalone approximation: the runner prepends functions extracted from source.
int main() {
 namespace air=storm::air_intercept;
 auto point=[](CVECTOR p){return air::Vec3{p.x,p.y,p.z};};
 struct Shot { NuclearMissile m; bool alive=true, hit=false; };
 struct Defense { OrdnanceState::Interceptor i; size_t target; bool live=true; };
 const char *names[]={"Kinzhal","Kalibr","Shtorm","Oreshnik"};
 const missile::Type types[]={missile::Type::Kinzhal,missile::Type::Kalibr,missile::Type::Shtorm,missile::Type::Oreshnik};
 std::printf("scenario,type,defense,trials,carriers,hits,hit_pct,all_intercepted_trials,all_intercepted_pct,interceptors,pantsir_launches,patriot_launches,peak_airborne,max_assignments,unresolved\n");
 for(int scenario=0;scenario<2;scenario++) for(int kind=0;kind<4;kind++) for(int mode=0;mode<3;mode++) {
  unsigned total=0,hits=0,all=0,launches=0,patLaunches=0,peak=0,maxassigned=0,unresolved=0;
  for(unsigned trial=0;trial<TRIALS;trial++) {
   uint32_t attacker=2+(SEED+trial)*2, defender=attacker+1, shotSerial=0, ds=0;
   const CVECTOR ship(0.f,0.f,0.f),start(0.f,5.5f,0.f);
   std::vector<Shot> shots; std::vector<Defense> defenses;
   std::array<OrdnanceState::DefenseReaction,64> reaction{};
   const unsigned wanted=scenario?12:1; unsigned created=0, trialHits=0;
   float nextShot=0, cooldown=0, scan=0;
   for(unsigned tick=0;tick<unsigned(160.f/DT);tick++) {
    float time=tick*DT;
    cooldown=std::max(0.f,cooldown-DT); scan=std::max(0.f,scan-DT); advanceDefenseReaction(reaction,DT);
    if(created<wanted && time+1.e-5f>=nextShot) {
     NuclearMissile m; m.active=true; m.type=types[kind]; m.owner=attacker; m.shotId=++shotSerial;
     m.launch=CVECTOR(0.f,10.f,19000.f);
     m.target=CVECTOR((shotVariation(shotSerial,attacker,102)*2-1)*80,5.5f,(shotVariation(shotSerial,attacker,103)*2-1)*80);
     varyMissile(m,attacker); shots.push_back({m,true,false}); created++; total++;
     if(created%4==0) nextShot=float(created/4)*4.f;
     else nextShot=time+.2f+.15f*shotVariation(shotSerial,attacker,0xb192a437u);
    }
    if(!DISABLED && cooldown<=0.f && scan<=0.f && defenses.size()<32) {
     scan=.05f;
     bool pat=mode==1 || (mode==2 && shotVariation(ds+1,defender,0x729b483du)<.5f);
     uint32_t variation=(ds+1)*0x9e3779b9u; variation^=variation>>16;variation*=0x7feb352du;variation^=variation>>15;
     float scale=.96f+.08f*float(variation&65535u)/65535.f, speed=interceptorProfile(pat).planningSpeed;
     float best=std::numeric_limits<float>::infinity(); size_t selected=shots.size();
     // One attacker, under 128 candidates: full source-order scan each cycle.
     for(size_t n=0;n<shots.size();n++) {
      auto &s=shots[n]; auto &m=s.m; if(!s.alive)continue;
      auto p=nuclearPosition(m,m.age),rel=p-ship; CVECTOR v(0.f),a(0.f);
      if(observeBallistic(m.type))observeShtorm(m,p,v,a);else v=nuclearVelocity(m,m.age);
      float v2=v.x*v.x+v.z*v.z;
      if((~rel)>8000.f*8000.f || (rel|v)>=0.f || v2<1.f)continue;
      float approach=std::max(0.f,-(rel.x*v.x+rel.z*v.z)/v2);
      float mx=rel.x+v.x*approach,mz=rel.z+v.z*approach;
      float threat=approach+std::hypot(mx,mz)/std::max(100.f,std::sqrt(v2));
      if(!std::isfinite(threat)||threat>=best)continue;
      unsigned assigned=0;for(auto &d:defenses)if(d.live&&d.target==n)assigned++;
      if(assigned>=8)continue;
      auto stale=observeBallistic(m.type)?v:nuclearVelocity(m,std::max(0.f,m.age-.12f));
      float flight=0,limit=std::min(60.f,m.duration-m.age);CVECTOR target(0.f);bool ok=false;
      if(pat) ok=observeBallistic(m.type)?interceptorEstimateShtorm(start,p,v,a,speed/scale,limit,flight,target):m.type==missile::Type::Kalibr?interceptorEstimateCruise(m,start,p,stale,speed/scale,limit,flight,target):interceptorEstimate(start,p,stale,speed/scale,limit,flight,target);
      else {auto accel=observeBallistic(m.type)?a:observedMissileAcceleration(m,stale);auto observed=stale+(m.type==missile::Type::Kalibr?accel*std::min(.12f,std::max(0.f,m.age)):CVECTOR(0.f));ok=estimatePantsir(start,p,observed,accel,limit,ds+1,flight,target,observeBallistic(m.type));}
      if(!ok && !(pat && m.type==missile::Type::Oreshnik))continue;
      best=threat;selected=n;
     }
     if(selected<shots.size()) {
      auto &m=shots[selected].m; auto p=nuclearPosition(m,m.age);CVECTOR v(0.f),a(0.f);
      if(observeBallistic(m.type))observeShtorm(m,p,v,a);else v=nuclearVelocity(m,m.age);
      auto stale=observeBallistic(m.type)?v:nuclearVelocity(m,std::max(0.f,m.age-.12f));
      float flight=0,limit=std::min(60.f,m.duration-m.age);CVECTOR target(0.f);bool ok=false;
      ++ds;
      if(pat)ok=observeBallistic(m.type)?interceptorEstimateShtorm(start,p,v,a,speed/scale,limit,flight,target):m.type==missile::Type::Kalibr?interceptorEstimateCruise(m,start,p,stale,speed/scale,limit,flight,target):interceptorEstimate(start,p,stale,speed/scale,limit,flight,target);
      else {auto accel=observeBallistic(m.type)?a:observedMissileAcceleration(m,stale);auto observed=stale+(m.type==missile::Type::Kalibr?accel*std::min(.12f,std::max(0.f,m.age)):CVECTOR(0.f));ok=estimatePantsir(start,p,observed,accel,limit,ds,flight,target,observeBallistic(m.type));}
      if(!ok && pat && m.type==missile::Type::Oreshnik){target=p;flight=std::clamp(std::sqrt(~(target-start))/speed,.15f,15.f);ok=true;}
      if(ok && defenseReactionReady(reaction,attacker,m.shotId,0,ds,defender)) {
       target+=interceptorAimError(start,target,std::sqrt(~(p-start)),flight,ds,defender)+interceptorTrackingError(m,flight,ds,defender);
       float distance=std::sqrt(~(target-start)),arc=std::clamp(distance*.02f,3.f,20.f);
       float lateral=(float(variation>>16)/65535.f*2.f-1.f)*std::min(12.f,distance*.015f);
       OrdnanceState::Interceptor i{start,target,0.f,flight,arc,lateral,ds,attacker,m.shotId,0};
       i.blastRadius=15.f+15.f*shotVariation(ds,defender,0x6c3874a5u);i.fuseDelay=interceptorFuseDelay(ds,defender);
       if(pat)initializePatriot(i,p);else i.duration=std::max(.05f,pantsirFlightTime(distance,ds));
       defenses.push_back({i,selected,true}); cooldown=.05f; launches++;patLaunches+=pat;
       peak=std::max(peak,unsigned(defenses.size()));unsigned assigned=0;for(auto &d:defenses)if(d.target==selected)assigned++;maxassigned=std::max(maxassigned,assigned);
      }
     }
    }
    if(!defenses.empty()) {
     std::vector<air::Body> targets;std::vector<air::Interceptor> interceptors;
     std::vector<float> frameStart(defenses.size());
     for(size_t n=0;n<defenses.size();n++) {
      auto &d=defenses[n];auto &i=d.i;auto &tracked=shots[d.target];float before=i.age;frameStart[n]=before;
      auto from=interceptorPosition(i,i.age/i.duration);
      if(i.patriot)advancePatriot(i,tracked.alive?&tracked.m:nullptr,DT);else i.age=std::min(i.duration,i.age+DT);
      auto to=interceptorPosition(i,i.age/i.duration);
      if(tracked.alive&&!i.evasionTriggered&&(~(nuclearPosition(tracked.m,tracked.m.age)-to))<=300.f*300.f){i.evasionTriggered=true;evadeInterceptor(tracked.m,ship,nullptr);}
      double avail=i.patriot?double(i.age-before):std::min(double(DT),double(i.duration-before));if(!(avail>0))continue;
      double expires=i.duration-before<=DT+1e-6?double(i.duration-before):std::numeric_limits<double>::infinity();
      air::Body body{n+1,1,n+1,i.blastRadius,expires,{{0,avail,point(from),point(to)}}};
      double pending=i.fuseAtAge>=0?std::max(0.,double(i.fuseAtAge-before)):std::numeric_limits<double>::infinity();
      interceptors.push_back({body,i.blastRadius,true,i.fuseDelay,pending,i.patriot?double(std::max(0.f,i.sensorEnableAge-before)):0.});
     }
     for(size_t n=0;n<shots.size();n++)if(shots[n].alive){auto &m=shots[n].m;float step=std::min(DT,m.duration-m.age);if(step>0)targets.push_back({n+1,2,n+1,0,step<DT?double(step):std::numeric_limits<double>::infinity(),{{0,double(step),point(nuclearPosition(m,m.age)),point(nuclearPosition(m,m.age+step))}}});}
     auto events=air::resolve(interceptors,targets,[](auto x,auto y){return x!=y;});
     for(auto &e:events){size_t n=e.contact.interceptor-1;auto &d=defenses[n];if(e.armed)d.i.fuseAtAge=frameStart[n]+float(e.contact.time)+d.i.fuseDelay;else{d.live=false;for(auto id:e.splash){auto &s=shots[id-1];if(s.alive){s.alive=false;s.hit=true;hits++;trialHits++;}}}}
     for(auto &d:defenses)if(d.i.age>=d.i.duration)d.live=false;
     std::erase_if(defenses,[](auto &d){return !d.live;});
    }
    bool active=false;for(auto &s:shots)if(s.alive){s.m.age=std::min(s.m.duration,s.m.age+DT);if(s.m.age>=s.m.duration)s.alive=false;else active=true;}
    if(created==wanted&&!active)break;
    if(tick+1==unsigned(160.f/DT))for(auto &s:shots)unresolved+=s.alive;
   }
   if(trialHits==wanted)all++;
  }
  std::printf("%s,%s,%s,%u,%u,%u,%.3f,%u,%.3f,%u,%u,%u,%u,%u,%u\n",scenario?"three_x_four":"single",names[kind],mode==0?"Pantsir":mode==1?"Patriot":"Mixed",TRIALS,total,hits,total?100.*hits/total:0.,all,100.*all/TRIALS,launches,launches-patLaunches,patLaunches,peak,maxassigned,unresolved);
 }
}
