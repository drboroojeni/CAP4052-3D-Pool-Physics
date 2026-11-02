#pragma once
#include "Physics.h"
#include "OrbitCamera.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace pool {
struct ViewState {
    Vec2 aim{1,0};
    double shotSpeed=1.5;
    bool charging=false, paused=false, showHelp=true, traceEnabled=false;
    bool soundEnabled=true;
    int scenario=0, shots=0;
    double cameraAzimuth=0.0; // Radians around the table's vertical Y axis.
    double cuePull=0;
    double strikeAnimation=0;
    Vec2 strikePosition{};
    std::string message;
};

// Owns GPU resources only. World state is const and rendering never advances it.
class Renderer3D {
public:
    Renderer3D();
    ~Renderer3D();
    Renderer3D(const Renderer3D&)=delete;
    Renderer3D& operator=(const Renderer3D&)=delete;
    bool initialize(std::string& error);
    void render(const PhysicsWorld&,const ViewState&,int framebufferW,int framebufferH);
    glm::mat4 view(double cameraAzimuth) const;
    glm::mat4 projection(int width,int height) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace pool
