import math,collections,sys
sub=lambda a,b:tuple(x-y for x,y in zip(a,b)); add=lambda a,b:tuple(x+y for x,y in zip(a,b)); mul=lambda a,s:tuple(x*s for x in a); dot=lambda a,b:sum(x*y for x,y in zip(a,b)); cross=lambda a,b:(a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]); norm=lambda a:math.sqrt(dot(a,a))
E=collections.defaultdict(list);tris=[]
for i,s in enumerate(open(sys.argv[1])):
 f=s.split(); vs=[tuple(map(float,f[2+j*3:5+j*3])) for j in range(3)];tris.append(vs);ks=[tuple(round(x*10000) for x in v) for v in vs]
 if cross(sub(ks[1],ks[0]),sub(ks[2],ks[0]))==(0,0,0):continue
 for j in range(3):
  a,b=ks[j],ks[(j+1)%3];E[tuple(sorted((a,b)))].append((i,1 if a<b else -1))
U={e:v for e,v in E.items() if len(v)!=2 or sum(s for _,s in v)!=0}
print('triangles',len(tris),'unmatched',len(U));fails=[]
for edge,refs in sorted(U.items()):
 a,b=[mul(k,.0001) for k in edge];line=sub(b,a);l2=dot(line,line);length=math.sqrt(l2);tol=.00015/length;ev=[];touching=[]
 for cand,rv in U.items():
  first,last=[sub(mul(k,.0001),a) for k in cand];x,y=dot(first,line)/l2,dot(last,line)/l2;lo,hi=max(0,min(x,y)),min(1,max(x,y))
  if hi<=lo or abs(y-x)<1e-14:continue
  st=add(first,mul(sub(last,first),(lo-x)/(y-x)));en=add(first,mul(sub(last,first),(hi-x)/(y-x)))
  if norm(cross(line,st))>.00015*length or norm(cross(line,en))>.00015*length:continue
  count=len(rv);bal=sum(s for _,s in rv)*(1 if y>=x else -1);ev.extend([(lo,count,bal),(hi,-count,-bal)]);touching.append((cand,rv,lo,hi))
 ev.sort();count=bal=0;end=0;n=0
 while n<len(ev):
  at=ev[n][0]
  if at>end+tol and (count!=2 or bal!=0):fails.append((edge,refs,(end,at,count,bal),touching));break
  last=n
  while last<len(ev) and ev[last][0]<=at+tol:count+=ev[last][1];bal+=ev[last][2];last+=1
  end=ev[last-1][0];n=last
for edge,refs,interval,touch in fails[:12]:
 print('\nFAIL',edge,'refs',refs,'interval',interval)
 for c,r,lo,hi in touch:print('candidate',c,'refs',r,'interval',lo,hi)
 for i,_ in refs:print('triangle',i,tris[i])
print('total failures',len(fails))
