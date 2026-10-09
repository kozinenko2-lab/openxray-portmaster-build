#pragma once
#include <cmath>

namespace LegacyTransform {
struct Matrix34 {
    float m[9]{};
    float scalar=1.f;
};

struct Point3 { float x=0.f,y=0.f,z=0.f; };
// Exact AirXonix.wrp.exe:0x401490 application convention. The legacy matrix
// is consumed by columns: out.x=x*m0+y*m3+z*m6, etc.
inline Point3 transformPoint(const Matrix34& r,float x,float y,float z,float tx=0.f,float ty=0.f,float tz=0.f){
    x*=r.scalar; y*=r.scalar; z*=r.scalar;
    return {x*r.m[0]+y*r.m[3]+z*r.m[6]+tx,
            x*r.m[1]+y*r.m[4]+z*r.m[7]+ty,
            x*r.m[2]+y*r.m[5]+z*r.m[8]+tz};
}
inline Point3 transformNormal(const Matrix34& r,float x,float y,float z){
    return {x*r.m[0]+y*r.m[3]+z*r.m[6],
            x*r.m[1]+y*r.m[4]+z*r.m[7],
            x*r.m[2]+y*r.m[5]+z*r.m[8]};
}
inline Matrix34 identity(){ Matrix34 r{}; r.m[0]=r.m[4]=r.m[8]=1.f; r.scalar=1.f; return r; }
inline void setScale(Matrix34& r,float s){ r.scalar=s; }
inline float radians(int a){ return float(a & 0x7ff) * (6.2831853071795864769f/2048.f); }
inline void rotateX(Matrix34& r,int a){ const float c=std::cos(radians(a)),s=std::sin(radians(a));
    for(int row=0;row<3;++row){ float y=r.m[row*3+1],z=r.m[row*3+2]; r.m[row*3+1]=y*c-z*s; r.m[row*3+2]=y*s+z*c; }}
inline void rotateXRad(Matrix34& r,float angle){ const float c=std::cos(angle),s=std::sin(angle);
    for(int row=0;row<3;++row){ float y=r.m[row*3+1],z=r.m[row*3+2]; r.m[row*3+1]=y*c-z*s; r.m[row*3+2]=y*s+z*c; }}
inline void rotateY(Matrix34& r,int a){ const float c=std::cos(radians(a)),s=std::sin(radians(a));
    for(int row=0;row<3;++row){ float x=r.m[row*3+0],z=r.m[row*3+2]; r.m[row*3+0]=x*c-z*s; r.m[row*3+2]=x*s+z*c; }}
inline void rotateYRad(Matrix34& r,float angle){ const float c=std::cos(angle),s=std::sin(angle);
    for(int row=0;row<3;++row){ float x=r.m[row*3+0],z=r.m[row*3+2]; r.m[row*3+0]=x*c-z*s; r.m[row*3+2]=x*s+z*c; }}
inline void rotateZ(Matrix34& r,int a){ const float c=std::cos(radians(a)),s=std::sin(radians(a));
    for(int row=0;row<3;++row){ float x=r.m[row*3+0],y=r.m[row*3+1]; r.m[row*3+0]=x*c-y*s; r.m[row*3+1]=x*s+y*c; }}
inline void rotateZRad(Matrix34& r,float angle){ const float c=std::cos(angle),s=std::sin(angle);
    for(int row=0;row<3;++row){ float x=r.m[row*3+0],y=r.m[row*3+1]; r.m[row*3+0]=x*c-y*s; r.m[row*3+1]=x*s+y*c; }}
}
