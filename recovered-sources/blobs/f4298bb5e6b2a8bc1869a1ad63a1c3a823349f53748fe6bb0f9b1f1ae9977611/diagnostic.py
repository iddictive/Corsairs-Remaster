from pathlib import Path
root=Path('/tmp/corsairs-world-cut-kNlDBY')
s=Path('experiments/native-metal/world_cut_geometry.hpp').read_text().replace('#include <set>','#include <set>\n#include <iostream>')
s=s.replace('namespace storm_world_cut {','namespace storm_world_cut {\ninline bool diagnosticFailure(int line){std::cerr<<"world failure line="<<line<<"\\n";return false;}')
s=s.replace('return false;','return diagnosticFailure(__LINE__);').replace('std::cerr<<"world failure line="<<line<<"\\n";return diagnosticFailure(__LINE__);','std::cerr<<"world failure line="<<line<<"\\n";return false;')
s=s.replace('return capEdges==unique;','if(capEdges!=unique){std::cerr<<"cap mismatch expected="<<unique.size()<<" actual="<<capEdges.size()<<"\\n";return diagnosticFailure(__LINE__);}return true;')
old='if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4))return diagnosticFailure(__LINE__);'
new='''if(std::abs(measured-expected)>std::max(1.e-5,expected*1.e-4)){
std::cerr<<"cap area expected="<<expected<<" measured="<<measured<<" tolerance="<<positionTolerance<<" vertices="<<vertices.size()<<"\\n";
unsigned crossings=0;const auto cross=[](Point a,Point b,Point c){return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);};for(size_t ri=0;ri<polygon.size();++ri)for(size_t i=0;i<polygon[ri].size();++i)for(size_t rj=ri;rj<polygon.size();++rj)for(size_t j=0;j<polygon[rj].size();++j){if(ri==rj&&(i==j||(i+1)%polygon[ri].size()==j||(j+1)%polygon[ri].size()==i||j<i))continue;auto a=polygon[ri][i],b=polygon[ri][(i+1)%polygon[ri].size()],c=polygon[rj][j],d=polygon[rj][(j+1)%polygon[rj].size()];if(cross(a,b,c)*cross(a,b,d)<0&&cross(c,d,a)*cross(c,d,b)<0){++crossings;std::cerr<<"proper crossing rings="<<ri<<","<<rj<<" edges="<<i<<","<<j<<" pts="<<a[0]<<","<<a[1]<<" "<<b[0]<<","<<b[1]<<" "<<c[0]<<","<<c[1]<<" "<<d[0]<<","<<d[1]<<"\\n";}}std::cerr<<"crossings="<<crossings<<"\\n";
return diagnosticFailure(__LINE__);}'''
assert s.count(old)==1
s=s.replace(old,new)
old='if(!crossSection(original,planes[side],cap))return diagnosticFailure(__LINE__);'
new='if(!crossSection(original,planes[side],cap)){std::cerr<<"plane side="<<side<<" n="<<planes[side].normal.x<<","<<planes[side].normal.y<<","<<planes[side].normal.z<<" d="<<planes[side].distance<<"\\n";return diagnosticFailure(__LINE__);}'
assert s.count(old)==1
s=s.replace(old,new)
(root/'world_cut_diagnostic.hpp').write_text(s)
(root/'diagnostic.cpp').write_text((root/'probe.cpp').read_text().replace('#include "world_cut_geometry.hpp"','#include "world_cut_diagnostic.hpp"'))
