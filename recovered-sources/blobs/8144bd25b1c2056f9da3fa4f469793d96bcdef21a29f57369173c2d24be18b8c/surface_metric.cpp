// CPU-only independent sampled surface diagnostic. No renderer integration.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>
using V = std::array<double, 3>;
static V sub(V a,V b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
static V add(V a,V b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
static V mul(V a,double b){return {a[0]*b,a[1]*b,a[2]*b};}
static double dot(V a,V b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
static V position(const float* p,unsigned i){return {p[i*3],p[i*3+1],p[i*3+2]};}
static V closest(V p,V a,V b,V c){
    // Barycentric closest point (Ericson); degenerates fall back to edges.
    V ab=sub(b,a),ac=sub(c,a),ap=sub(p,a);
    double d1=dot(ab,ap),d2=dot(ac,ap);
    if(d1<=0&&d2<=0)return {1,0,0};
    V bp=sub(p,b);double d3=dot(ab,bp),d4=dot(ac,bp);
    if(d3>=0&&d4<=d3)return {0,1,0};
    double vc=d1*d4-d3*d2;
    if(vc<=0&&d1>=0&&d3<=0){double v=d1/(d1-d3);return {1-v,v,0};}
    V cp=sub(p,c);double d5=dot(ab,cp),d6=dot(ac,cp);
    if(d6>=0&&d5<=d6)return {0,0,1};
    double vb=d5*d2-d1*d6;
    if(vb<=0&&d2>=0&&d6<=0){double w=d2/(d2-d6);return {1-w,0,w};}
    double va=d3*d6-d5*d4;
    if(va<=0&&(d4-d3)>=0&&(d5-d6)>=0){double w=(d4-d3)/((d4-d3)+(d5-d6));return {0,1-w,w};}
    double den=va+vb+vc;
    if(std::abs(den)>1e-30){double v=vb/den,w=vc/den;return {1-v-w,v,w};}
    double best=std::numeric_limits<double>::infinity();V bary{1,0,0};V ps[]={a,b,c};
    for(int i=0;i<3;i++){int j=(i+1)%3;V e=sub(ps[j],ps[i]);double l=dot(e,e);double t=l?std::max(0.,std::min(1.,dot(sub(p,ps[i]),e)/l)):0.;V q=add(ps[i],mul(e,t));double d=dot(sub(q,p),sub(q,p));if(d<best){best=d;bary={0,0,0};bary[i]=1-t;bary[j]=t;}}
    return bary;
}
struct Node {V lo,hi;unsigned begin,end,left=0,right=0;};
struct Tree {
    const float* p;const float* a;const unsigned* ix;std::vector<unsigned> order;std::vector<Node> nodes;
    Tree(const float* p,const float* a,const unsigned* ix,size_t n):p(p),a(a),ix(ix),order(n/3){std::iota(order.begin(),order.end(),0);build(0,unsigned(order.size()));}
    unsigned build(unsigned begin,unsigned end){
        Node n;n.lo={INFINITY,INFINITY,INFINITY};n.hi={-INFINITY,-INFINITY,-INFINITY};n.begin=begin;n.end=end;
        for(unsigned j=begin;j<end;j++)for(unsigned c=0;c<3;c++){V v=position(p,ix[order[j]*3+c]);for(int k=0;k<3;k++){n.lo[k]=std::min(n.lo[k],v[k]);n.hi[k]=std::max(n.hi[k],v[k]);}}
        unsigned id=unsigned(nodes.size());nodes.push_back(n);
        if(end-begin>8){int axis=0;for(int k=1;k<3;k++)if(n.hi[k]-n.lo[k]>n.hi[axis]-n.lo[axis])axis=k;unsigned mid=(begin+end)/2;
            std::nth_element(order.begin()+begin,order.begin()+mid,order.begin()+end,[&](unsigned x,unsigned y){double cx=0,cy=0;for(int c=0;c<3;c++){cx+=p[ix[x*3+c]*3+axis];cy+=p[ix[y*3+c]*3+axis];}return cx<cy;});
            unsigned left=build(begin,mid),right=build(mid,end);nodes[id].left=left;nodes[id].right=right;
        }return id;
    }
    double bound(unsigned id,V q)const{double d=0;for(int k=0;k<3;k++){double v=std::max(0.,std::max(nodes[id].lo[k]-q[k],q[k]-nodes[id].hi[k]));d+=v*v;}return d;}
    void nearest(unsigned id,V q,const double* qa,double eps,double& best,double& score,unsigned& tri,V& bary)const{
        const Node& n=nodes[id];if(bound(id,q)>best+eps)return;
        if(n.left){double dl=bound(n.left,q),dr=bound(n.right,q);if(dl<dr){nearest(n.left,q,qa,eps,best,score,tri,bary);nearest(n.right,q,qa,eps,best,score,tri,bary);}else{nearest(n.right,q,qa,eps,best,score,tri,bary);nearest(n.left,q,qa,eps,best,score,tri,bary);}return;}
        for(unsigned j=n.begin;j<n.end;j++){unsigned t=order[j],i0=ix[t*3],i1=ix[t*3+1],i2=ix[t*3+2];V v0=position(p,i0),v1=position(p,i1),v2=position(p,i2);V w=closest(q,v0,v1,v2);V qp=add(add(mul(v0,w[0]),mul(v1,w[1])),mul(v2,w[2]));double d=dot(sub(q,qp),sub(q,qp));
            double s=0;for(int k=0;k<9;k++){double delta=a[i0*9+k]*w[0]+a[i1*9+k]*w[1]+a[i2*9+k]*w[2]-qa[k];s+=delta*delta*(k==3||k==4?100:1);}
            if(d<best-eps||(d<=best+eps&&s<score)){best=d;score=s;tri=t;bary=w;}
        }
    }
};
static void sample(const Tree& target,V q,const double* qa,double eps,std::array<std::vector<double>,5>& metrics){
    double best=INFINITY,score=INFINITY;unsigned t=0;V w;
    target.nearest(0,q,qa,eps,best,score,t,w);
    double a[9]={};for(int c=0;c<3;c++)for(int k=0;k<9;k++)a[k]+=target.a[target.ix[t*3+c]*9+k]*w[c];
    V n0{qa[0],qa[1],qa[2]},n1{a[0],a[1],a[2]};double den=std::sqrt(dot(n0,n0)*dot(n1,n1));
    double angle=den>1e-20?std::acos(std::max(-1.,std::min(1.,dot(n0,n1)/den)))*180/3.141592653589793:180;
    metrics[0].push_back(std::sqrt(best));metrics[1].push_back(angle);
    metrics[2].push_back(std::max(std::abs(a[3]-qa[3]),std::abs(a[4]-qa[4])));
    metrics[3].push_back(std::max(std::abs(a[5]-qa[5]),std::max(std::abs(a[6]-qa[6]),std::abs(a[7]-qa[7]))));metrics[4].push_back(std::abs(a[8]-qa[8]));
}
extern "C" void sampled_surface_metrics(const float* p,const float* a,size_t nv,const unsigned* original,size_t ni,const unsigned* candidate,size_t nc,double extent,double* out){
    Tree original_tree(p,a,original,ni),candidate_tree(p,a,candidate,nc);std::array<std::vector<double>,5> metrics;
    double eps=extent*extent*1e-14;
    for(size_t i=0;i<nv;i++){double qa[9];for(int k=0;k<9;k++)qa[k]=a[i*9+k];sample(candidate_tree,position(p,unsigned(i)),qa,eps,metrics);}
    for(int direction=0;direction<2;direction++){
        const unsigned* ix=direction?candidate:original;size_t n=direction?nc:ni;const Tree& target=direction?original_tree:candidate_tree;
        for(size_t i=0;i<n;i+=3){V q{};double qa[9]={};for(int c=0;c<3;c++){q=add(q,mul(position(p,ix[i+c]),1./3));for(int k=0;k<9;k++)qa[k]+=a[ix[i+c]*9+k]/3.;}sample(target,q,qa,eps,metrics);}
    }
    for(int k=0;k<5;k++){auto& v=metrics[k];std::sort(v.begin(),v.end());out[k*3]=v.back();out[k*3+1]=v[size_t((v.size()-1)*.99)];out[k*3+2]=v[size_t((v.size()-1)*.95)];}out[15]=double(metrics[0].size());
}
