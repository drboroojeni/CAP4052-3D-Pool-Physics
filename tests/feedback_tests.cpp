#include "OrbitCamera.h"
#include "CollisionAudio.h"
#include "Physics.h"
#include <cmath>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
void require(bool ok,const std::string& message) {
    if(!ok)throw std::runtime_error(message);
}
void near(double a,double b,double tolerance=1e-9) {
    require(std::abs(a-b)<tolerance,"Unexpected numeric result");
}
double rms(const std::vector<float>& samples) {
    double sum=0;for(const float x:samples)sum+=x*x;
    return std::sqrt(sum/samples.size());
}
std::vector<float> impact(double speed,bool cushion=false) {
    pool::CollisionSoundMixer mixer;
    mixer.enqueueImpact(speed,cushion);
    std::vector<float> samples((pool::CollisionSoundMixer::soundFrames+128)*2);
    mixer.render(samples.data(),samples.size()/2);
    return samples;
}
pool::Ball ball(int id,pool::Vec2 p,pool::Vec2 v) {
    pool::Ball b;b.id=id;b.position=p;b.velocity=v;return b;
}
}

int main() {
    int passed=0,failed=0;
    const auto test=[&](const char* name,const std::function<void()>& body){
        try {body();++passed;std::cout<<"PASS "<<name<<'\n';}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<name<<": "<<e.what()<<'\n';}
    };
    test("orbit preserves radius, height, and forty-degree elevation",[]{
        for(int degrees=-720;degrees<=720;degrees+=7){
            const auto p=pool::cameraOrbitPosition(degrees*pool::kCameraPi/180);
            const double radius=std::hypot(p.x,p.z);
            near(radius,pool::kCameraOrbitRadius);
            near(std::atan2(p.y-pool::kCameraTargetY,radius)*180/pool::kCameraPi,40);
        }
    });
    test("held camera motion reverses and does not depend on frame rate",[]{
        const double start=.6;
        const double forward=pool::advanceCameraAzimuth(start,1,false);
        near(forward,start+pool::kCameraPi/4);
        near(pool::advanceCameraAzimuth(forward,1,true),start);
        for(const int fps:{30,60,144}){
            double angle=start;
            for(int frame=0;frame<fps;++frame)angle=pool::advanceCameraAzimuth(angle,1.0/fps,false);
            near(angle,forward);
        }
        near(pool::wrappedCameraAzimuth(-pool::kCameraPi),pool::kCameraPi);
    });
    test("impact gain rises with normal approach speed and rejects invalid input",[]{
        require(pool::impactGain(.2)<pool::impactGain(1),"Medium hit should be louder");
        require(pool::impactGain(1)<pool::impactGain(3),"Fast hit should be louder");
        near(pool::impactGain(0),0);near(pool::impactGain(-1),0);
        near(pool::impactGain(std::numeric_limits<double>::infinity()),0);
        require(pool::impactGain(1,true)<pool::impactGain(1,false),"Cushion should be softer");
    });
    test("synthesized sound is louder for stronger hits with bounded stereo samples",[]{
        const auto soft=impact(.2),medium=impact(1),hard=impact(3),cushion=impact(1,true);
        require(rms(soft)>0&&rms(soft)<rms(medium)&&rms(medium)<rms(hard),"Sound energy order");
        require(rms(cushion)>0,"Missing cushion sound");
        for(std::size_t i=0;i<hard.size();i+=2){
            require(std::isfinite(hard[i])&&std::abs(hard[i])<1,"Unbounded sample");
            near(hard[i],hard[i+1]);
        }
        for(std::size_t i=pool::CollisionSoundMixer::soundFrames*2;i<hard.size();++i)near(hard[i],0);
    });
    test("mute silences current voices and discards queued impacts",[]{
        pool::CollisionSoundMixer mixer;mixer.enqueueImpact(3);
        std::vector<float> samples(512);mixer.render(samples.data(),256);
        require(rms(samples)>0,"Missing initial sound");
        mixer.enqueueImpact(2);mixer.setMuted(true);mixer.render(samples.data(),256);near(rms(samples),0);
        mixer.setMuted(false);mixer.render(samples.data(),256);near(rms(samples),0);
    });
    test("rack-sized sound bursts remain finite and do not clip",[]{
        pool::CollisionSoundMixer mixer;
        for(int i=0;i<512;++i)mixer.enqueueImpact(6,i%2==0);
        std::vector<float> samples(pool::CollisionSoundMixer::soundFrames*2);
        mixer.render(samples.data(),samples.size()/2);
        require(rms(samples)>0,"Burst should be audible");
        for(const float x:samples)require(std::isfinite(x)&&std::abs(x)<1,"Burst clipped");
    });
    test("physics sound events report the incoming normal relative speed",[]{
        pool::PhysicsWorld world;world.params.rollingDeceleration=0;
        world.balls={ball(0,{-pool::kBallRadius,0},{1,.2}),
                     ball(1,{pool::kBallRadius,0},{-.5,.2})};
        world.step(pool::kFixedDt);
        int contacts=0;
        for(const auto& e:world.lastEvents)if(e.type==pool::EventType::BallContact){near(e.speed,1.5);++contacts;}
        require(contacts==1,"Expected one genuine approaching contact");
        world.balls={ball(0,{.5,pool::kHalfWidth-pool::kBallRadius-.001},{.3,1.2})};
        world.step(pool::kFixedDt);contacts=0;
        for(const auto& e:world.lastEvents)if(e.type==pool::EventType::CushionContact){near(e.speed,1.2);++contacts;}
        require(contacts==1,"Expected a cushion impact");
        world.balls={ball(0,{-.02,0},{-.5,0}),ball(1,{.02,0},{.5,0})};
        world.step(pool::kFixedDt);
        for(const auto& e:world.lastEvents)require(e.type!=pool::EventType::BallContact,"Separating contact emitted sound event");
    });
    std::cout<<passed<<" feedback checks passed, "<<failed<<" failed.\n";
    return failed?1:0;
}
