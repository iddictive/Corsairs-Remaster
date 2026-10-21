
#include <string>
#include <string_view>
#include <cctype>
#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include "before.hpp"
#include "after.hpp"
__attribute__((noinline)) int before(const char*a,const char*b,size_t n){return before_storm::ichar_traits<char>::compare(a,b,n);}
__attribute__((noinline)) int after(const char*a,const char*b,size_t n){return storm::ichar_traits<char>::compare(a,b,n);}
int main(){
 for(int a=0;a<256;++a)for(int b=0;b<256;++b){char x=char(a),y=char(b);if(before(&x,&y,1)!=after(&x,&y,1))return 2;}
 std::mt19937 rng(48213);for(int t=0;t<100000;++t){std::string a(rng()%96,0),b(a.size(),0);for(size_t i=0;i<a.size();++i){a[i]=char(rng()%256);b[i]=t%3? a[i]:char(rng()%256);}if(t%3==1)for(char&c:b)if(c>='a'&&c<='z')c-=32;if(before(a.data(),b.data(),a.size())!=after(a.data(),b.data(),a.size()))return 3;}
 printf("Parity: 65536 byte pairs + 100000 strings incl NUL/high bytes PASS\n");
 std::vector<std::string> keys={"Phonk","Active","MainCharacter","Ship","Stopped","SeaSurrender","Ship.Cannons.Borts","HullDamage","FireState","Sails","Speed","Tsunami","Ship.Position","sailors","ship","nation","index","BaseType","RotationSpeed"};
 for(int kind=0;kind<3;++kind){std::vector<std::pair<std::string,std::string>> rows;for(auto key:keys){auto b=key;if(kind==1)for(char&c:b)if(c>='a'&&c<='z')c-=32;if(kind==2&&b.size()>1)b.back()='z';rows.push_back({key,b});}
 auto measure=[&](auto fn){auto start=std::chrono::steady_clock::now();volatile int sum=0;for(int round=0;round<200000;++round)for(const auto&row:rows)sum=sum+fn(row.first.data(),row.second.data(),row.first.size());return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();};
 auto a=measure(before),b=measure(after),a2=measure(before),b2=measure(after);printf("kind=%d old_ms=%.3f/%.3f new_ms=%.3f/%.3f comparisons=%zu\n",kind,a,a2,b,b2,rows.size()*size_t(200000));
 }
}
