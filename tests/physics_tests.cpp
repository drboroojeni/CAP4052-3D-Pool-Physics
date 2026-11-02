#include "Physics.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace pool;
namespace {
void require(bool ok,const std::string& message) {
    if(!ok) throw std::runtime_error(message);
}
void near(double actual,double expected,double epsilon=1e-7) {
    require(std::abs(actual-expected)<=epsilon,
            "expected "+std::to_string(expected)+", got "+std::to_string(actual));
}
Ball ball(int id,Vec2 p,Vec2 v={}) { Ball b;b.id=id;b.position=p;b.velocity=v;return b; }
PhysicsWorld empty() {
    PhysicsWorld w;w.balls.clear();w.params.rollingDeceleration=0;return w;
}
void fixedSteps(PhysicsWorld& w,int n) {for(int i=0;i<n;++i)w.step(kFixedDt);}
PhysicsWorld renderedAt(int fps) {
    PhysicsWorld w;
    w.strike({1,.13},2.2);
    double accumulator=0;
    for(int frame=0;frame<fps*4;++frame) {
        accumulator+=1.0/fps;
        while(accumulator+1e-12>=kFixedDt) {
            w.step(kFixedDt);accumulator-=kFixedDt;
        }
    }
    return w;
}
}

int main() {
    int passed=0,failed=0;
    auto test=[&](const char* name,const std::function<void()>& body) {
        try {body();++passed;std::cout<<"PASS "<<name<<'\n';}
        catch(const std::exception& e) {++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
    };

    test("rack contains cue plus 15 non-overlapping object balls",[]{
        PhysicsWorld w;require(w.balls.size()==16,"rack size");
        require(w.objectBallsRemaining()==15,"object count");
        for(std::size_t i=0;i<w.balls.size();++i)for(std::size_t j=i+1;j<w.balls.size();++j)
            require(length(w.balls[i].position-w.balls[j].position)>=2*kBallRadius,"rack overlap");
        require(w.allStopped(),"rack should start stopped");
    });
    test("six pockets occupy corners and long-rail midpoints",[]{
        const auto& p=PhysicsWorld::pocketCenters();require(p.size()==6,"pocket count");
        near(p[1].x,0);near(p[1].z,-kHalfWidth);near(p[4].z,kHalfWidth);
    });
    test("strike normalizes direction and caps power",[]{
        PhysicsWorld w;require(w.strike({6,8},100),"strike rejected");
        near(length(w.cueBall()->velocity),3);near(w.cueBall()->velocity.x,1.8);
        near(w.cueBall()->velocity.z,2.4);
    });
    test("invalid or repeated strikes do not apply impulses",[]{
        PhysicsWorld w;require(!w.strike({0,0},2),"zero direction");
        require(!w.strike({1,0},-1),"negative power");
        require(!w.strike({1,0},std::numeric_limits<double>::infinity()),"infinite power");
        require(w.strike({1,0},1),"first shot");
        require(!w.strike({1,0},1),"moving cue can be struck twice");
        near(w.cueBall()->velocity.x,1);
    });
    test("semi-implicit rolling slowdown uses the new velocity",[]{
        auto w=empty();w.params.rollingDeceleration=.2;
        w.balls={ball(0,{-.5,0},{1,0})};w.step(.01);
        near(w.balls[0].velocity.x,.998);near(w.balls[0].position.x,-.49002);
    });
    test("rolling resistance never reverses direction",[]{
        auto w=empty();w.params.rollingDeceleration=5;
        w.balls={ball(0,{0,0},{.02,0})};w.step(.1);
        near(length(w.balls[0].velocity),0);near(w.balls[0].position.x,0);
    });
    test("rest threshold snaps tiny speed to zero",[]{
        auto w=empty();w.balls={ball(0,{0,0},{.005,0})};w.step(kFixedDt);
        near(length(w.balls[0].velocity),0);require(w.allStopped(),"not stopped");
    });
    test("equal-mass elastic head-on collision transfers normal velocity",[]{
        auto w=empty();w.params.ballRestitution=1;
        w.balls={ball(0,{-.05,0},{1,0}),ball(1,{.05,0})};
        fixedSteps(w,16);
        near(w.balls[0].velocity.x,0);near(w.balls[1].velocity.x,1);
        require(w.collisions==1,"duplicate contact impulses");
    });
    test("ball restitution controls relative separating normal speed",[]{
        auto w=empty();w.params.ballRestitution=.8;
        w.balls={ball(0,{-.03,0},{1,0}),ball(1,{.03,0})};w.step(.005);
        near(w.balls[1].velocity.x-w.balls[0].velocity.x,.8);
        near(w.balls[0].velocity.x+w.balls[1].velocity.x,1);
    });
    test("separating overlapping balls receive no second bounce",[]{
        auto w=empty();w.balls={ball(0,{-.02,0},{-.5,0}),ball(1,{.02,0},{.5,0})};
        w.step(.001);near(w.balls[0].velocity.x,-.5);near(w.balls[1].velocity.x,.5);
        require(w.collisions==0,"separating pair got impulse");
        require(length(w.balls[1].position-w.balls[0].position)>=2*kBallRadius,"overlap not fixed");
    });
    test("coincident stationary centers get deterministic separation",[]{
        auto w=empty();w.balls={ball(0,{0,0}),ball(1,{0,0})};w.step(kFixedDt);
        require(length(w.balls[1].position-w.balls[0].position)>=2*kBallRadius,"coincident centers remain");
        require(std::isfinite(w.balls[0].position.x),"nonfinite position");
    });
    test("elastic collision conserves momentum and kinetic energy",[]{
        auto w=empty();w.params.ballRestitution=1;
        auto a=ball(0,{-.03,0},{1,.2});auto b=ball(1,{.03,0},{-.5,.2});
        a.mass=.17;b.mass=.34;w.balls={a,b};
        const Vec2 momentum=a.velocity*a.mass+b.velocity*b.mass;
        const double energy=w.kineticEnergy();w.step(.003);
        const Vec2 after=w.balls[0].velocity*a.mass+w.balls[1].velocity*b.mass;
        near(after.x,momentum.x);near(after.z,momentum.z);near(w.kineticEnergy(),energy);
    });
    test("cushion changes the inward normal component only",[]{
        auto w=empty();w.params.cushionRestitution=.8;
        w.balls={ball(0,{.5,kHalfWidth-kBallRadius-.001},{1,1})};w.step(.005);
        near(w.balls[0].velocity.x,1);near(w.balls[0].velocity.z,-.8);
        require(w.balls[0].position.z<=kHalfWidth-kBallRadius,"penetration remains");
    });
    test("a ball already moving away from a cushion is not reflected",[]{
        auto w=empty();w.balls={ball(0,{kHalfLength-kBallRadius+.001,.2},{-.5,0})};
        w.step(.001);near(w.balls[0].velocity.x,-.5);require(w.collisions==0,"false cushion bounce");
    });
    test("side-pocket mouth admits an object ball",[]{
        auto w=empty();w.balls={ball(1,{0,.5},{0,1})};w.step(.20);
        require(w.balls[0].pocketed,"side pocket missed");near(length(w.balls[0].velocity),0);
        require(w.pockets==1&&w.scratches==0,"pocket counters");
        require(w.collisions==0,"cushion blocked the mouth");
    });
    test("corner pocket captures an approaching diagonal ball",[]{
        auto w=empty();w.balls={ball(1,{1.10,.465},{1,1})};w.step(.20);
        require(w.balls[0].pocketed,"corner pocket missed");require(w.pockets==1,"count");
    });
    test("a shot outside the mouth bounces off the solid rail",[]{
        auto w=empty();w.balls={ball(1,{.10,.58},{0,1})};w.step(.08);
        require(!w.balls[0].pocketed,"outside mouth pocketed");
        require(w.balls[0].velocity.z<0,"escaped rail");
    });
    test("pocketed balls stop interacting and do not count twice",[]{
        auto w=empty();auto b=ball(1,{0,.50},{0,2});w.balls={b};w.step(.1);
        require(w.balls[0].pocketed,"not pocketed");fixedSteps(w,500);
        require(w.pockets==1,"recounted pocket");near(w.kineticEnergy(),0);
    });
    test("cue pocket increments scratch rather than object counter",[]{
        auto w=empty();w.balls={ball(0,{0,.50},{0,2}),ball(1,{-.8,.2})};w.step(.1);
        require(w.cueBall()->pocketed,"cue not pocketed");
        require(w.scratches==1&&w.pockets==0,"scratch counters");
        require(!w.strike({1,0},1),"pocketed cue can shoot");
    });
    test("cue respot avoids an occupied preferred location",[]{
        auto w=empty();auto cue=ball(0,{0,kHalfWidth});cue.pocketed=true;
        w.balls={cue,ball(1,{-.65,0})};require(w.respotCueBall(),"no respot found");
        require(!w.cueBall()->pocketed,"cue still pocketed");
        require(length(w.balls[0].position-w.balls[1].position)>=2*kBallRadius+.002,"respot overlaps");
    });
    test("respot is disabled while object balls move",[]{
        auto w=empty();auto cue=ball(0,{0,kHalfWidth});cue.pocketed=true;
        w.balls={cue,ball(1,{-.65,0},{1,0})};require(!w.respotCueBall(),"respotted during motion");
    });
    test("speed guard caps directly edited velocities",[]{
        auto w=empty();w.balls={ball(0,{0,0},{100,0})};w.step(kFixedDt);
        near(length(w.balls[0].velocity),3);
    });
    test("adaptive substeps prevent fast head-on balls swapping places",[]{
        auto w=empty();w.params.ballRestitution=1;
        w.balls={ball(0,{-.3,.2},{3,0}),ball(1,{.3,.2},{-3,0})};w.step(.15);
        require(w.balls[0].velocity.x<0&&w.balls[1].velocity.x>0,"missed collision");
        require(w.balls[0].position.x<w.balls[1].position.x,"swapped centers");
    });
    test("30, 60 and 144 render FPS produce the same fixed-step world",[]{
        const auto a=renderedAt(30),b=renderedAt(60),c=renderedAt(144);
        require(a.tick==960&&b.tick==960&&c.tick==960,"different update counts");
        for(std::size_t i=0;i<a.balls.size();++i) {
            near(a.balls[i].position.x,b.balls[i].position.x,1e-12);
            near(a.balls[i].position.z,c.balls[i].position.z,1e-12);
            near(a.balls[i].velocity.x,c.balls[i].velocity.x,1e-12);
            require(a.balls[i].pocketed==c.balls[i].pocketed,"different pocket outcomes");
        }
    });
    test("invalid time intervals leave the clock unchanged",[]{
        PhysicsWorld w;w.step(0);w.step(-.1);w.step(10);
        w.step(std::numeric_limits<double>::quiet_NaN());
        require(w.tick==0,"invalid tick");near(w.simulationTime,0);
    });
    test("reset clears counters and restores the full rack",[]{
        PhysicsWorld w;w.setupScenario(1);w.strike({1,0},2);fixedSteps(w,100);
        w.resetRack();require(w.tick==0&&w.collisions==0&&w.pockets==0&&w.scratches==0,"reset counters");
        require(w.objectBallsRemaining()==15&&w.allStopped(),"reset rack state");
    });
    test("all deterministic teaching scenarios start at rest",[]{
        PhysicsWorld w;
        for(int i=0;i<=4;++i) {
            w.setupScenario(i);require(w.cueBall()!=nullptr,"cue missing");
            require(w.allStopped(),"scenario already moving");require(w.tick==0,"scenario clock");
        }
    });

    std::cout<<passed<<" passed, "<<failed<<" failed\n";
    return failed?1:0;
}
