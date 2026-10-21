from pathlib import Path
import subprocess,hashlib
repo=Path.cwd(); source=repo/'experiments/native-metal/engine/src/libs/sea_ai/src/ai_balls.cpp'; text=source.read_text()
base=Path(__file__).with_name('probe.cpp').read_text().split('struct Scene {')[0]
base=base.replace('float Sqr(float x)', 'CVECTOR& operator+=(CVECTOR&a,CVECTOR b){a=a+b;return a;}\nfloat Sqr(float x)')
base=base.replace('float fSpeedV0=90,', 'CVECTOR vPos,vFirstPos;float fSpeedV0=90,')
step=text[text.index('            // update ball time'):text.index('            if (!pBall->sBallEvent.empty()',text.index('            // update ball time'))]
program=base+'''\n#define SQR(x) ((x)*(x))
CVECTOR Step(BALL_PARAMS&ball,float fDeltaTime,float fDeltaTimeMultiplier){auto*pBall=&ball;CVECTOR vDst;
'''+step+'''\nreturn vDst;}
int main(){
 BALL_PARAMS seed;seed.fTime=.9f;seed.vFirstPos={3,4,5};seed.fCosAngle=cosf(.4f);seed.fSinAngle=sinf(.4f);seed.fRawAng=.23f;seed.fHeightMultiply=1.7f;seed.fDirX=cosf(.7f);seed.fDirZ=sinf(.7f);seed.fTimeSpeedMultiply=.8f;
 auto paused=seed;auto p=Step(paused,0,2);assert(paused.fTime==seed.fTime);auto pausedAgain=paused;auto pp=Step(pausedAgain,0,2);assert(~(p-pp)==0);
 const float gameDt=.0005f,rate=2*seed.fTimeSpeedMultiply;
 auto before=seed,after=seed;auto from=Step(before,0,2),to=Step(after,gameDt,2);
 auto chord=(to-from)*(1/gameDt);auto analytic=BallWorldVelocity(seed,seed.fTime+gameDt*rate*.5f,rate);
 assert(~(chord-analytic)<.0025f);
 auto compressed=seed;Step(compressed,gameDt*4,2);assert(fabsf(compressed.fTime-seed.fTime-gameDt*4*rate)<1e-6f);
 // Same simulation instant has the same momentum at ordinary and compressed
 // frame rates; zero game delta changes neither ballistic time nor position.
 auto velocity=BallWorldVelocity(compressed,seed.fTime,rate);auto normal=BallWorldVelocity(seed,seed.fTime,rate);assert(~(velocity-normal)==0);
 puts("PASS: actual Execute trajectory update: pause preserves time/position; raw-angle/HeightMultiply/heading derivative matches game-time displacement; compression changes elapsed game time, not instantaneous velocity");
}
'''
path=Path(__file__).with_name('kinematics.cpp');path.write_text(program)
r=subprocess.run(['clang++','-std=c++20','-O1','-g','-fsanitize=address,undefined',str(path),'-o',str(path.with_suffix(''))],capture_output=True,text=True)
if r.returncode:print(r.stderr);raise SystemExit(r.returncode)
r=subprocess.run([str(path.with_suffix(''))],capture_output=True,text=True)
evidence=repo/'.local-archives/metal-renderer-evidence/ship-collision-20261016/secondary-projectiles'
(evidence/'kinematics.log').write_text('source_sha256='+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+r.stdout+r.stderr)
print(r.stdout+r.stderr);raise SystemExit(r.returncode)
