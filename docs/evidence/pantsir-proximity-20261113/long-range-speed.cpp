// Included after functions extracted from the native source by long-range-speed.py.
int main() {
 const char *names[]={"Kinzhal","Kalibr","Shtorm","Oreshnik"};
 const missile::Type types[]={missile::Type::Kinzhal,missile::Type::Kalibr,missile::Type::Shtorm,missile::Type::Oreshnik};
 for (int k=0;k<4;k++) {
  unsigned count=0; double sum=0, minimum=1.e30,maximum=0, heights=0, peaks=0, fractions=0;
  for (uint32_t serial=1;serial<=300;serial++) {
   NuclearMissile m; m.active=true; m.type=types[k]; m.owner=2; m.shotId=serial;
   m.launch=CVECTOR(0.f,10.f,19000.f); m.target=CVECTOR(0.f,5.5f,0.f); varyMissile(m,2);
   float peak=0; bool detected=false;
   for(float t=0;t<m.duration;t+=.01f) {
    auto p=nuclearPosition(m,t), v=nuclearVelocity(m,t); peak=std::max(peak,p.y);
    if(!detected && (~p)<=8000.f*8000.f && (p|v)<0.f && v.x*v.x+v.z*v.z>=1.f) {
     double speed=std::sqrt(~v); count++; sum+=speed; minimum=std::min(minimum,speed); maximum=std::max(maximum,speed); heights+=p.y;
     fractions+=speed/missile::variedCruise(m.type,m.flightVariation); detected=true;
    }
   }
   peaks+=peak;
  }
  std::printf("%s,%u,%.1f,%.1f,%.1f,%.1f,%.1f,%.3f\n",names[k],count,count?minimum:0.,count?sum/count:0.,count?maximum:0.,count?heights/count:0.,peaks/300.,count?fractions/count:0.);
 }
}
