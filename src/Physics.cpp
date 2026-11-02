#include "Physics.h"

#include <algorithm>
#include <limits>

namespace pool {
namespace {
double clamp(double value, double low, double high) {
    return std::max(low,std::min(high,value));
}
bool finite(Vec2 v) { return std::isfinite(v.x)&&std::isfinite(v.z); }
double segmentDistanceSquared(Vec2 p, Vec2 a, Vec2 b) {
    const Vec2 ab=b-a;
    const double denom=dot(ab,ab);
    const double u=denom>1e-18 ? clamp(dot(p-a,ab)/denom,0.0,1.0) : 0.0;
    const Vec2 offset=p-(a+ab*u);
    return dot(offset,offset);
}
Ball makeBall(int id, Vec2 p) {
    Ball b; b.id=id; b.position=p; return b;
}
}

PhysicsWorld::PhysicsWorld() { resetRack(); }

const std::vector<Vec2>& PhysicsWorld::pocketCenters() {
    static const std::vector<Vec2> centers{
        {-kHalfLength,-kHalfWidth},{0,-kHalfWidth},{kHalfLength,-kHalfWidth},
        {-kHalfLength, kHalfWidth},{0, kHalfWidth},{kHalfLength, kHalfWidth}
    };
    return centers;
}

void PhysicsWorld::clearCounters() {
    tick=0; simulationTime=0.0; collisions=0; pockets=0; scratches=0;
    lastEvents.clear();
}

void PhysicsWorld::resetRack() {
    balls.clear(); clearCounters();
    balls.push_back(makeBall(0,{-0.65,0}));
    int id=1;
    // Triangle of touching disks, separated by a tiny starting gap.
    const double spacing=2*kBallRadius+0.0005;
    for(int row=0;row<5;++row) {
        for(int col=0;col<=row;++col) {
            balls.push_back(makeBall(id++,{
                0.45+row*spacing*std::sqrt(3.0)/2.0,
                (col-row*0.5)*spacing
            }));
        }
    }
}

void PhysicsWorld::setupScenario(int index) {
    if(index<=0 || index>4) { resetRack(); return; }
    balls.clear(); clearCounters();
    switch(index) {
        case 1:
            balls.push_back(makeBall(0,{-0.75,0}));
            balls.push_back(makeBall(1,{0,0}));
            break;
        case 2:
            balls.push_back(makeBall(0,{0.45,-0.20}));
            balls.push_back(makeBall(1,{-0.80,0.30}));
            break;
        case 3:
            balls.push_back(makeBall(0,{0,-0.30}));
            balls.push_back(makeBall(1,{0,0.40}));
            break;
        case 4:
            balls.push_back(makeBall(0,{0,0.20}));
            balls.push_back(makeBall(1,{-0.75,0}));
            break;
    }
}

Ball* PhysicsWorld::cueBall() {
    for(auto& b:balls) if(b.id==0) return &b;
    return nullptr;
}
const Ball* PhysicsWorld::cueBall() const {
    for(const auto& b:balls) if(b.id==0) return &b;
    return nullptr;
}

bool PhysicsWorld::allStopped() const {
    for(const auto& b:balls) {
        if(!b.pocketed && length(b.velocity)>kStopSpeed) return false;
    }
    return true;
}

bool PhysicsWorld::strike(Vec2 direction, double speed) {
    Ball* cue=cueBall();
    if(!cue || cue->pocketed || !allStopped() || !finite(direction) ||
       !std::isfinite(speed) || speed<=0.0 || length(direction)<1e-12) return false;
    const double legalMax=std::max(0.01,params.maxSpeed);
    cue->velocity=normalized(direction)*clamp(speed,0.0,legalMax);
    lastEvents.push_back({EventType::Strike,0,-1,cue->position,length(cue->velocity)});
    return true;
}

bool PhysicsWorld::capturePocket(Ball& b, Vec2 previous) {
    if(b.pocketed) return false;
    for(const auto& center:pocketCenters()) {
        // Sweeping the center catches even a fast ball crossing the capture disk.
        if(segmentDistanceSquared(center,previous,b.position)<=kPocketRadius*kPocketRadius) {
            const double speed=length(b.velocity);
            b.pocketed=true; b.velocity={}; b.position=center;
            if(b.id==0) {
                ++scratches;
                lastEvents.push_back({EventType::Scratch,b.id,-1,center,speed});
            } else {
                ++pockets;
                lastEvents.push_back({EventType::Pocket,b.id,-1,center,speed});
            }
            return true;
        }
    }
    return false;
}

void PhysicsWorld::resolveCushions(Ball& b) {
    const double ex=clamp(params.cushionRestitution,0.0,1.0);
    const double xLimit=kHalfLength-b.radius, zLimit=kHalfWidth-b.radius;
    const bool nearCorner=std::abs(std::abs(b.position.z)-kHalfWidth)<=kPocketRadius;
    const bool zMouth=std::abs(b.position.x)<=kPocketRadius ||
                      std::abs(std::abs(b.position.x)-kHalfLength)<=kPocketRadius;
    auto bounce=[&](Vec2 normal) {
        const double incoming=dot(b.velocity,normal);
        if(incoming<0.0) {
            b.velocity-=normal*((1.0+ex)*incoming);
            ++collisions;
            lastEvents.push_back({EventType::CushionContact,b.id,-1,b.position,-incoming});
        }
    };
    // Normals point from each cushion into the playable area. The six pocket
    // mouths are open; the capture test handles a ball entering those openings.
    if(!nearCorner) {
        if(b.position.x>xLimit) { b.position.x=xLimit; bounce({-1,0}); }
        if(b.position.x< -xLimit) { b.position.x=-xLimit; bounce({1,0}); }
    }
    if(!zMouth) {
        if(b.position.z>zLimit) { b.position.z=zLimit; bounce({0,-1}); }
        if(b.position.z< -zLimit) { b.position.z=-zLimit; bounce({0,1}); }
    }
}

void PhysicsWorld::resolvePair(Ball& a, Ball& b) {
    if(a.pocketed||b.pocketed) return;
    Vec2 delta=b.position-a.position;
    const double d2=dot(delta,delta), target=a.radius+b.radius;
    if(d2>target*target) return;
    const double distance=std::sqrt(std::max(0.0,d2));
    const Vec2 n=distance>1e-12 ? delta/distance : Vec2{1,0};
    const double invA=a.mass>0 ? 1.0/a.mass : 0;
    const double invB=b.mass>0 ? 1.0/b.mass : 0;
    const double inverseMassSum=invA+invB;
    if(inverseMassSum<=0) return;
    // Correct positional overlap independently of the approach test. A small
    // separation epsilon prevents roundoff from recreating the same overlap.
    const double penetration=target-distance;
    if(penetration>0) {
        const Vec2 correction=n*((penetration+1e-8)/inverseMassSum);
        a.position-=correction*invA;
        b.position+=correction*invB;
    }
    const double relativeNormal=dot(b.velocity-a.velocity,n);
    if(relativeNormal>=0) return; // Never bounce objects already separating.
    const double restitution=clamp(params.ballRestitution,0.0,1.0);
    const double impulseMagnitude=-(1.0+restitution)*relativeNormal/inverseMassSum;
    const Vec2 impulse=n*impulseMagnitude;
    a.velocity-=impulse*invA;
    b.velocity+=impulse*invB;
    ++collisions;
    lastEvents.push_back({EventType::BallContact,a.id,b.id,
                          (a.position+b.position)*0.5,-relativeNormal});
}

void PhysicsWorld::step(double h) {
    lastEvents.clear();
    if(!std::isfinite(h)||h<=0) return;
    // The application supplies kFixedDt. Reject enormous intervals instead of
    // pretending that an unbounded backlog is a single safe physics step.
    if(h>0.25) return;
    double maxTravelSpeed=0.0, minimumRadius=kBallRadius;
    const double legalMax=std::max(0.01,params.maxSpeed);
    for(auto& b:balls) {
        if(b.pocketed) { b.velocity={}; continue; }
        if(!finite(b.velocity)) b.velocity={};
        const double speed=length(b.velocity);
        if(speed>legalMax) b.velocity*=legalMax/speed;
        maxTravelSpeed=std::max(maxTravelSpeed,length(b.velocity));
        if(b.radius>0) minimumRadius=std::min(minimumRadius,b.radius);
    }
    // No center travels more than 0.45 radii during one movement substep.
    // This substantially reduces missed disk contacts; it is not a general CCD
    // solver for every possible rigid body or arbitrary user-edited speed.
    const int substeps=std::max(1,static_cast<int>(std::ceil(
        maxTravelSpeed*h/std::max(0.001,minimumRadius*0.45))));
    const double dt=h/substeps;
    const double deceleration=std::max(0.0,params.rollingDeceleration);
    for(int s=0;s<substeps;++s) {
        for(auto& b:balls) {
            if(b.pocketed) continue;
            const Vec2 previous=b.position;
            const double oldSpeed=length(b.velocity);
            const double newSpeed=std::max(0.0,oldSpeed-deceleration*dt);
            if(newSpeed<=kStopSpeed) b.velocity={};
            else if(oldSpeed>0) b.velocity*=newSpeed/oldSpeed;
            // Semi-implicit update: the new velocity advances position.
            const Vec2 travel=b.velocity*dt;
            b.position+=travel;
            b.rollAngle+=length(travel)/std::max(1e-9,b.radius);
            capturePocket(b,previous);
        }
        // Several passes help resolve a tightly packed rack. This is a small
        // iterative impulse solver, not a claim of exact simultaneous impacts.
        for(int pass=0;pass<4;++pass) {
            for(std::size_t i=0;i<balls.size();++i)
                for(std::size_t j=i+1;j<balls.size();++j) resolvePair(balls[i],balls[j]);
            for(auto& b:balls) if(!b.pocketed) {
                capturePocket(b,b.position);
                if(!b.pocketed) resolveCushions(b);
            }
        }
    }
    ++tick;
    simulationTime+=h;
}

bool PhysicsWorld::spotIsFree(Vec2 p, double radius) const {
    if(std::abs(p.x)>kHalfLength-radius||std::abs(p.z)>kHalfWidth-radius) return false;
    for(const auto& b:balls) {
        if(b.pocketed||b.id==0) continue;
        if(length(b.position-p)<b.radius+radius+0.002) return false;
    }
    for(Vec2 pocket:pocketCenters())
        if(length(p-pocket)<kPocketRadius+radius) return false;
    return true;
}

bool PhysicsWorld::respotCueBall() {
    Ball* cue=cueBall();
    if(!cue||!cue->pocketed||!allStopped()) return false;
    Vec2 chosen{-0.65,0};
    bool found=spotIsFree(chosen,cue->radius);
    if(!found) {
        for(int row=0;row<13&&!found;++row) {
            for(int col=0;col<27&&!found;++col) {
                Vec2 candidate{-1.15+col*0.085,-0.51+row*0.085};
                if(spotIsFree(candidate,cue->radius)) { chosen=candidate; found=true; }
            }
        }
    }
    if(!found) return false;
    cue->position=chosen; cue->velocity={}; cue->pocketed=false; cue->rollAngle=0;
    lastEvents.push_back({EventType::Respot,0,-1,chosen,0});
    return true;
}

int PhysicsWorld::objectBallsRemaining() const {
    int count=0;
    for(const auto& b:balls) if(b.id!=0&&!b.pocketed) ++count;
    return count;
}
double PhysicsWorld::kineticEnergy() const {
    double energy=0;
    for(const auto& b:balls) if(!b.pocketed) energy+=0.5*b.mass*dot(b.velocity,b.velocity);
    return energy;
}

} // namespace pool
