#pragma once
#include <cstdint>
#include <map>
#include <string>
struct CVECTOR { float x=0,y=0,z=0; CVECTOR()=default; CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR operator-(CVECTOR b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator*(float b)const{return {x*b,y*b,z*b};} CVECTOR& operator+=(CVECTOR b){*this=*this+b;return *this;} float operator~()const{return x*x+y*y+z*z;} };
struct ATTRIBUTES { std::map<std::string,float> values; uint32_t GetAttributeAsDword(const char*n){return uint32_t(values[n]);} float GetAttributeAsFloat(const char*n,float d=0){auto p=values.find(n);return p==values.end()?d:p->second;} const char* GetAttribute(const char*){return "fake_ship";} };
enum class Stage {execute,realize};
struct Entity { ATTRIBUTES *AttributesPointer=nullptr; virtual ~Entity()=default; virtual bool Init(){return true;} virtual void ProcessStage(Stage,uint32_t){} int GetId(){return 1;} };
