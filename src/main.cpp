#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "Physics.h"
#include "Renderer3D.h"
#include "OrbitCamera.h"
#include "CollisionAudio.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <vector>

namespace {
constexpr double kPi=3.14159265358979323846;
struct Options {
    int frames=0,camera=0,scenario=0;
    double autoShot=0,angle=0,frameDt=0;
    double cameraAngle=0;
    bool explicitCameraAngle=false,muted=false;
    std::string screenshot;
};
Options parseOptions(int argc,char** argv) {
    Options o;
    for(int i=1;i<argc;++i) {
        const std::string key=argv[i];
        auto value=[&](){if(i+1>=argc)throw std::runtime_error("Missing value for "+key);return std::string(argv[++i]);};
        if(key=="--frames")o.frames=std::stoi(value());
        else if(key=="--camera")o.camera=std::stoi(value());
        else if(key=="--camera-angle"){
            o.cameraAngle=std::stod(value());o.explicitCameraAngle=true;
            if(!std::isfinite(o.cameraAngle))throw std::runtime_error("Camera angle must be finite.");
        }
        else if(key=="--mute")o.muted=true;
        else if(key=="--scenario")o.scenario=std::stoi(value());
        else if(key=="--auto-shot")o.autoShot=std::stod(value());
        else if(key=="--aim-angle")o.angle=std::stod(value());
        else if(key=="--frame-dt")o.frameDt=std::stod(value());
        else if(key=="--screenshot")o.screenshot=value();
        else throw std::runtime_error("Unknown option: "+key);
    }
    return o;
}
// Picking uses logical window dimensions, not framebuffer pixels: GLFW's
// cursor positions use window coordinates even on a high-DPI monitor.
bool mouseOnBed(GLFWwindow* window,const pool::Renderer3D& renderer,double cameraAzimuth,
                double mouseX,double mouseY,pool::Vec2& point) {
    int w=0,h=0;glfwGetWindowSize(window,&w,&h);
    if(w<=0||h<=0)return false;
    const glm::mat4 inv=glm::inverse(renderer.projection(w,h)*renderer.view(cameraAzimuth));
    const float nx=static_cast<float>(2*mouseX/w-1),ny=static_cast<float>(1-2*mouseY/h);
    glm::vec4 nearP=inv*glm::vec4(nx,ny,-1,1),farP=inv*glm::vec4(nx,ny,1,1);
    nearP/=nearP.w;farP/=farP.w;
    const glm::vec3 direction=glm::vec3(farP-nearP);
    if(std::abs(direction.y)<1e-7)return false;
    const double t=(pool::kBallRadius-nearP.y)/direction.y;
    if(t<0)return false;
    const glm::vec3 hit=glm::vec3(nearP)+static_cast<float>(t)*direction;
    point={hit.x,hit.z};
    return std::abs(point.x)<pool::kHalfLength+.15&&std::abs(point.z)<pool::kHalfWidth+.15;
}
struct Input {
    std::array<bool,GLFW_KEY_LAST+1> previous{};
    bool oldMouse=false;
    double lastX=-1,lastY=-1;
    bool pressed(GLFWwindow* w,int key) {
        const bool down=glfwGetKey(w,key)==GLFW_PRESS;
        const bool edge=down&&!previous[static_cast<std::size_t>(key)];
        previous[static_cast<std::size_t>(key)]=down;return edge;
    }
};
class Trace {
    std::ofstream file;
public:
    bool active()const{return file.is_open();}
    std::string toggle() {
        if(file.is_open()){file.close();return "Trace saved. Open its CSV next to the working directory.";}
        const auto stamp=std::chrono::system_clock::now().time_since_epoch();
        const auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(stamp).count();
        const std::string path="pool_trace_"+std::to_string(ms)+".csv";
        file.open(path);if(!file)return "Could not open a CSV in the working directory.";
        file<<"tick,time_s,ball_id,x_m,z_m,vx_m_s,vz_m_s,pocketed,contacts,object_pockets,scratches,deceleration_m_s2\n";
        std::cout<<"Writing "<<path<<'\n';return "Recording "+path;
    }
    void record(const pool::PhysicsWorld& world) {
        if(!file.is_open())return;
        file<<std::setprecision(12);
        for(const auto& b:world.balls)
            file<<world.tick<<','<<world.simulationTime<<','<<b.id<<','<<b.position.x<<','<<b.position.z
                <<','<<b.velocity.x<<','<<b.velocity.z<<','<<b.pocketed<<','<<world.collisions<<','
                <<world.pockets<<','<<world.scratches<<','<<world.params.rollingDeceleration<<'\n';
    }
};
void writeScreenshot(const std::string& path,int width,int height) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    std::ofstream f(path,std::ios::binary);
    if(!f)throw std::runtime_error("Cannot write screenshot: "+path);
    f<<"P6\n"<<width<<' '<<height<<"\n255\n";
    for(int y=height-1;y>=0;--y)
        f.write(reinterpret_cast<const char*>(pixels.data()+static_cast<std::size_t>(y)*width*3),width*3);
}
void glfwError(int code,const char* description){std::cerr<<"GLFW "<<code<<": "<<description<<'\n';}
}

int main(int argc,char** argv) {
    try {
        const Options options=parseOptions(argc,argv);
        glfwSetErrorCallback(glfwError);
        if(!glfwInit())return 1;
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES,4);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
#endif
        GLFWwindow* window=glfwCreateWindow(1440,900,"CAP4052 DF5 | Pool Physics | F1: controls",nullptr,nullptr);
        if(!window){glfwTerminate();return 1;}
        glfwMakeContextCurrent(window);
        if(!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))){
            std::cerr<<"Could not load OpenGL 3.3. Update your graphics driver.\n";
            glfwDestroyWindow(window);glfwTerminate();return 1;
        }
        glfwSwapInterval(options.frames>0?0:1);
        std::cout<<"OpenGL: "<<glGetString(GL_VERSION)<<" / "<<glGetString(GL_RENDERER)<<'\n';
        {
            pool::Renderer3D renderer;
            std::string error;
            if(!renderer.initialize(error))throw std::runtime_error(error);
            pool::PhysicsWorld world;
            pool::ViewState ui;
            ui.cameraAzimuth=pool::wrappedCameraAzimuth(options.explicitCameraAngle?
                options.cameraAngle*kPi/180:(options.camera==1?kPi:0));
            ui.scenario=options.scenario;
            pool::CollisionAudio audio;
            if(!audio.initialize(error)){
                std::cerr<<"Collision audio unavailable: "<<error<<". Continuing without sound.\n";
                ui.message="No audio output found. The game remains playable.";
            }
            bool muted=options.muted;
            audio.setMuted(muted);
            ui.soundEnabled=audio.isAvailable()&&!muted;
            world.setupScenario(options.scenario);
            ui.aim={std::cos(options.angle*kPi/180),std::sin(options.angle*kPi/180)};
            if(options.scenario==2||options.scenario==3||options.scenario==4)ui.aim={0,1};
            Input input;Trace trace;
            double lastTime=glfwGetTime(),accumulator=0,titleTimer=0;
            int frame=0;
            auto fire=[&](double speed){
                if(ui.paused||!world.allStopped())return false;
                const auto* cue=world.cueBall();
                if(!cue||cue->pocketed)return false;
                const auto old=cue->position;
                if(!world.strike(ui.aim,speed))return false;
                ++ui.shots;ui.strikeAnimation=1;ui.strikePosition=old;
                ui.message="Shot committed. Physics now owns the ball positions.";return true;
            };
            auto scenario=[&](int index){
                world.setupScenario(index);ui.scenario=index;ui.shots=0;ui.charging=false;
                ui.aim=index>=2?pool::Vec2{0,1}:pool::Vec2{1,0};
                ui.strikeAnimation=0;accumulator=0;
                ui.message=index==0?"Pocket all 15 object balls. Scratches count as a penalty.":"Controlled scenario loaded. Aim and strike to inspect the response.";
            };
            auto tick=[&](){
                world.step(pool::kFixedDt);
                // Rules consume events after physics. A scratch is respotted
                // only after every remaining ball has stopped.
                for(const auto& event:world.lastEvents){
                    if(event.type==pool::EventType::BallContact||event.type==pool::EventType::CushionContact)
                        audio.playImpact(event.speed,event.type==pool::EventType::CushionContact);
                    if(event.type==pool::EventType::Pocket)ui.message="Object ball pocketed. Progress increased.";
                    if(event.type==pool::EventType::Scratch)ui.message="Scratch. Wait for rest, then the cue ball returns.";
                }
                if(world.respotCueBall())ui.message="Cue ball respotted in a free location. Aim for the next shot.";
                if(world.objectBallsRemaining()==0)ui.message="Table cleared! Press R to restart the rack.";
                trace.record(world);
            };
            while(!glfwWindowShouldClose(window)) {
                glfwPollEvents();
                const double now=glfwGetTime();
                // This teaching demo discards wall-clock time above 0.1 s.
                // It slows down after a stall instead of using a huge step.
                const double frameTime=options.frameDt>0?options.frameDt:std::clamp(now-lastTime,0.0,.1);
                lastTime=now;
                if(input.pressed(window,GLFW_KEY_ESCAPE))glfwSetWindowShouldClose(window,GLFW_TRUE);
                if(glfwGetKey(window,GLFW_KEY_C)==GLFW_PRESS){
                    const bool reverse=glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS||
                                       glfwGetKey(window,GLFW_KEY_RIGHT_SHIFT)==GLFW_PRESS;
                    ui.cameraAzimuth=pool::advanceCameraAzimuth(ui.cameraAzimuth,frameTime,reverse);
                }
                if(input.pressed(window,GLFW_KEY_M)){
                    muted=!muted;audio.setMuted(muted);
                    ui.soundEnabled=audio.isAvailable()&&!muted;
                    ui.message=audio.isAvailable()?(muted?"Collision sound muted.":"Collision sound enabled."):
                                                   "No audio output available. The game remains playable.";
                }
                if(input.pressed(window,GLFW_KEY_F1))ui.showHelp=!ui.showHelp;
                if(input.pressed(window,GLFW_KEY_F2))ui.message=trace.toggle();
                if(input.pressed(window,GLFW_KEY_F3)){
                    world.params.rollingDeceleration=world.params.rollingDeceleration<.3?.44:.22;
                    ui.message="Rolling resistance changed. Press L for the controlled 0.50 m/s test.";
                }
                if(input.pressed(window,GLFW_KEY_P)){ui.paused=!ui.paused;ui.charging=false;accumulator=0;}
                if(input.pressed(window,GLFW_KEY_R))scenario(0);
                for(int k=1;k<=4;++k)if(input.pressed(window,GLFW_KEY_0+k))scenario(k);
                if(input.pressed(window,GLFW_KEY_L)){
                    scenario(1);ui.paused=false;ui.shotSpeed=.5;ui.aim={1,0};fire(.5);
                    ui.message="Free roll: x starts at -0.75 m, speed 0.50 m/s. Observe the stop position.";
                }
                double mx=0,my=0;glfwGetCursorPos(window,&mx,&my);
                const bool mouseMoved=input.lastX<0||std::hypot(mx-input.lastX,my-input.lastY)>.3;
                input.lastX=mx;input.lastY=my;
                const bool ready=world.allStopped()&&world.cueBall()&&!world.cueBall()->pocketed;
                if(ready&&!ui.paused&&!ui.charging){
                    pool::Vec2 target;
                    if(options.frames==0&&mouseMoved&&mouseOnBed(window,renderer,ui.cameraAzimuth,mx,my,target)){
                        const auto d=target-world.cueBall()->position;
                        if(pool::length(d)>.04)ui.aim=pool::normalized(d);
                    }
                    const double turn=(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS?1.:0.)-
                                      (glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS?1.:0.);
                    if(turn!=0){const double angle=std::atan2(ui.aim.z,ui.aim.x)+turn*frameTime*.65;ui.aim={std::cos(angle),std::sin(angle)};}
                    const double change=(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS?1.:0.)-
                                        (glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS?1.:0.);
                    ui.shotSpeed=std::clamp(ui.shotSpeed+change*frameTime*.8,.1,world.params.maxSpeed);
                }
                const bool mouse=glfwGetMouseButton(window,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS;
                if(mouse&&!input.oldMouse&&ready&&!ui.paused){
                    pool::Vec2 hit;
                    if(mouseOnBed(window,renderer,ui.cameraAzimuth,mx,my,hit)){ui.charging=true;ui.shotSpeed=.1;}
                }
                if(ui.charging&&mouse)ui.shotSpeed=std::min(world.params.maxSpeed,ui.shotSpeed+frameTime*1.65);
                if(ui.charging&&!mouse){ui.charging=false;fire(ui.shotSpeed);}
                input.oldMouse=mouse;
                if(input.pressed(window,GLFW_KEY_SPACE)&&!ui.charging)fire(ui.shotSpeed);
                if(options.autoShot>0&&frame==1)fire(options.autoShot);
                if(ui.paused){if(input.pressed(window,GLFW_KEY_N))tick();}
                else {
                    input.pressed(window,GLFW_KEY_N);
                    accumulator+=std::min(frameTime,.1);
                    int steps=0;
                    while(accumulator>=pool::kFixedDt&&steps<24){tick();accumulator-=pool::kFixedDt;++steps;}
                    if(steps==24)accumulator=std::fmod(accumulator,pool::kFixedDt);
                }
                ui.cuePull=ui.charging?(ui.shotSpeed/world.params.maxSpeed)*.28:0;
                ui.strikeAnimation=std::max(0.0,ui.strikeAnimation-frameTime*7);
                ui.traceEnabled=trace.active();
                int fbw=0,fbh=0;glfwGetFramebufferSize(window,&fbw,&fbh);
                if(fbw>0&&fbh>0)renderer.render(world,ui,fbw,fbh);
                ++frame;
                if(options.frames>0&&frame>=options.frames){
                    if(!options.screenshot.empty()&&fbw>0&&fbh>0)writeScreenshot(options.screenshot,fbw,fbh);
                    glfwSetWindowShouldClose(window,GLFW_TRUE);
                }
                glfwSwapBuffers(window);
                titleTimer+=frameTime;
                if(titleTimer>.2){
                    std::ostringstream title;title<<"CAP4052 DF5 | Object balls "<<world.objectBallsRemaining()<<" | Shots "<<ui.shots
                        <<" | Scratches "<<world.scratches<<" | Camera "<<std::fixed<<std::setprecision(0)
                        <<ui.cameraAzimuth*180/kPi<<" deg | "<<(ui.paused?"PAUSED - N: one tick":"LIVE")<<" | F1: controls";
                    glfwSetWindowTitle(window,title.str().c_str());titleTimer=0;
                }
            }
        }
        glfwDestroyWindow(window);glfwTerminate();return 0;
    }catch(const std::exception& e){std::cerr<<"Pool demo: "<<e.what()<<'\n';glfwTerminate();return 1;}
}
