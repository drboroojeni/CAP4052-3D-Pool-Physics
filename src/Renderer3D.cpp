#include "Renderer3D.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <vector>

namespace pool {
namespace {
constexpr float pi=3.14159265358979323846f;
struct Vertex { glm::vec3 p,n; };
struct Mesh {
    GLuint vao=0,vbo=0;
    GLsizei count=0;
    void upload(const std::vector<Vertex>& v) {
        count=static_cast<GLsizei>(v.size());
        glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);
        glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(v.size()*sizeof(Vertex)),v.data(),GL_STATIC_DRAW);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(0));glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(sizeof(glm::vec3)));glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }
    void release(){if(vbo)glDeleteBuffers(1,&vbo);if(vao)glDeleteVertexArrays(1,&vao);vbo=vao=0;}
    void draw()const{glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,count);}
};
void tri(std::vector<Vertex>& v,glm::vec3 a,glm::vec3 b,glm::vec3 c,glm::vec3 n){v.push_back({a,n});v.push_back({b,n});v.push_back({c,n});}
std::vector<Vertex> cubeMesh(){
    std::vector<Vertex> v;
    const std::array<glm::vec3,8> p{{{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,.5f,-.5f},{-.5f,.5f,-.5f},
                                 {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f}}};
    const int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};
    const glm::vec3 n[6]={{0,0,-1},{0,0,1},{-1,0,0},{1,0,0},{0,-1,0},{0,1,0}};
    for(int j=0;j<6;++j){tri(v,p[f[j][0]],p[f[j][1]],p[f[j][2]],n[j]);tri(v,p[f[j][0]],p[f[j][2]],p[f[j][3]],n[j]);}
    return v;
}
std::vector<Vertex> sphereMesh(){
    std::vector<Vertex> v;
    constexpr int rings=24,sectors=40;
    auto point=[](float a,float b){return glm::vec3(std::sin(a)*std::cos(b),std::cos(a),std::sin(a)*std::sin(b));};
    for(int r=0;r<rings;++r)for(int s=0;s<sectors;++s){
        const auto a=point(pi*r/rings,2*pi*s/sectors), b=point(pi*(r+1)/rings,2*pi*s/sectors);
        const auto c=point(pi*(r+1)/rings,2*pi*(s+1)/sectors), d=point(pi*r/rings,2*pi*(s+1)/sectors);
        for(auto p:{a,b,c,a,c,d})v.push_back({p,p});
    }return v;
}
std::vector<Vertex> cylinderMesh(float upperRadius=1.f){
    std::vector<Vertex> v;constexpr int sectors=40;
    for(int s=0;s<sectors;++s){
        float a=2*pi*s/sectors,b=2*pi*(s+1)/sectors;
        glm::vec3 p0{std::cos(a),0,std::sin(a)},p1{std::cos(b),0,std::sin(b)};
        glm::vec3 p2{upperRadius*std::cos(b),1,upperRadius*std::sin(b)},p3{upperRadius*std::cos(a),1,upperRadius*std::sin(a)};
        glm::vec3 n0=glm::normalize(glm::vec3(std::cos(a),1-upperRadius,std::sin(a))),n1=glm::normalize(glm::vec3(std::cos(b),1-upperRadius,std::sin(b)));
        v.push_back({p0,n0});v.push_back({p1,n1});v.push_back({p2,n1});
        v.push_back({p0,n0});v.push_back({p2,n1});v.push_back({p3,n0});
        tri(v,{0,0,0},p1,p0,{0,-1,0});tri(v,{0,1,0},p3,p2,{0,1,0});
    }return v;
}
GLuint program(const char* vs,const char* fs,std::string& error){
    GLuint shaders[2]={glCreateShader(GL_VERTEX_SHADER),glCreateShader(GL_FRAGMENT_SHADER)};
    const char* sources[2]={vs,fs};
    for(int i=0;i<2;++i){
        glShaderSource(shaders[i],1,&sources[i],nullptr);glCompileShader(shaders[i]);
        GLint okay=0;glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&okay);
        if(!okay){char log[4096];glGetShaderInfoLog(shaders[i],sizeof(log),nullptr,log);error=log;for(auto sh:shaders)glDeleteShader(sh);return 0;}
    }
    GLuint p=glCreateProgram();for(auto sh:shaders)glAttachShader(p,sh);glLinkProgram(p);
    for(auto sh:shaders)glDeleteShader(sh);
    GLint okay=0;glGetProgramiv(p,GL_LINK_STATUS,&okay);
    if(!okay){char log[4096];glGetProgramInfoLog(p,sizeof(log),nullptr,log);error=log;glDeleteProgram(p);return 0;}
    return p;
}
const char* vertex3D=R"GLSL(#version 330 core
layout(location=0)in vec3 aPosition;layout(location=1)in vec3 aNormal;
uniform mat4 uModel,uView,uProjection;
out vec3 worldPosition,normal,localPosition;
void main(){vec4 p=uModel*vec4(aPosition,1);worldPosition=p.xyz;normal=mat3(transpose(inverse(uModel)))*aNormal;localPosition=aPosition;gl_Position=uProjection*uView*p;}
)GLSL";
const char* fragment3D=R"GLSL(#version 330 core
in vec3 worldPosition,normal,localPosition;out vec4 FragColor;
uniform vec3 uColor,uEye;uniform int uMaterial;uniform float uShininess;
void main(){
 vec3 color=uColor;
 if(uMaterial==1&&abs(localPosition.y)>.45)color=vec3(.94,.94,.89);
 if(uMaterial==2){float board=mod(floor(worldPosition.x*1.6)+floor(worldPosition.z*1.6),2.);color*=mix(.85,1.04,board);}
 if(uMaterial==3){float grain=sin(worldPosition.x*70.+sin(worldPosition.z*8.)*.8);color*=.94+.06*grain;}
 vec3 N=normalize(normal),L=normalize(vec3(-2.8,5.,-2.)-worldPosition),V=normalize(uEye-worldPosition),H=normalize(L+V);
 float diffuse=max(dot(N,L),0.);float spec=pow(max(dot(N,H),0.),uShininess);
 vec3 lit=color*(.40+.60*diffuse)+vec3(.40)*spec*(uShininess>20.?1.:.2);
 FragColor=vec4(lit,1.);
})GLSL";
const char* vertex2D=R"GLSL(#version 330 core
layout(location=0)in vec2 aPosition;layout(location=1)in vec4 aColor;out vec4 color;
void main(){gl_Position=vec4(aPosition,0,1);color=aColor;})GLSL";
const char* fragment2D=R"GLSL(#version 330 core
in vec4 color;out vec4 FragColor;void main(){FragColor=color;})GLSL";

// Original compact 5x7 bitmap glyphs. Font pixels become ordinary 2D triangles.
std::array<unsigned char,7> glyph(char ch){
    switch(static_cast<char>(std::toupper(static_cast<unsigned char>(ch)))){
    case 'A':return{14,17,17,31,17,17,17};case 'B':return{30,17,17,30,17,17,30};
    case 'C':return{14,17,16,16,16,17,14};case 'D':return{30,17,17,17,17,17,30};
    case 'E':return{31,16,16,30,16,16,31};case 'F':return{31,16,16,30,16,16,16};
    case 'G':return{14,17,16,23,17,17,15};case 'H':return{17,17,17,31,17,17,17};
    case 'I':return{14,4,4,4,4,4,14};case 'J':return{7,2,2,2,18,18,12};
    case 'K':return{17,18,20,24,20,18,17};case 'L':return{16,16,16,16,16,16,31};
    case 'M':return{17,27,21,21,17,17,17};case 'N':return{17,25,21,19,17,17,17};
    case 'O':return{14,17,17,17,17,17,14};case 'P':return{30,17,17,30,16,16,16};
    case 'Q':return{14,17,17,17,21,18,13};case 'R':return{30,17,17,30,20,18,17};
    case 'S':return{15,16,16,14,1,1,30};case 'T':return{31,4,4,4,4,4,4};
    case 'U':return{17,17,17,17,17,17,14};case 'V':return{17,17,17,17,17,10,4};
    case 'W':return{17,17,17,21,21,21,10};case 'X':return{17,17,10,4,10,17,17};
    case 'Y':return{17,17,10,4,4,4,4};case 'Z':return{31,1,2,4,8,16,31};
    case '0':return{14,17,19,21,25,17,14};case '1':return{4,12,4,4,4,4,14};
    case '2':return{14,17,1,2,4,8,31};case '3':return{30,1,1,14,1,1,30};
    case '4':return{2,6,10,18,31,2,2};case '5':return{31,16,16,30,1,1,30};
    case '6':return{14,16,16,30,17,17,14};case '7':return{31,1,2,4,8,8,8};
    case '8':return{14,17,17,14,17,17,14};case '9':return{14,17,17,15,1,1,14};
    case '.':return{0,0,0,0,0,12,12};case ',':return{0,0,0,0,4,4,8};
    case ':':return{0,12,12,0,12,12,0};case '-':return{0,0,0,31,0,0,0};
    case '/':return{1,1,2,4,8,16,16};case '+':return{0,4,4,31,4,4,0};
    case '=':return{0,0,31,0,31,0,0};case '(':return{2,4,8,8,8,4,2};
    case ')':return{8,4,2,2,2,4,8};case '%':return{17,2,4,4,8,16,17};
    case '[':return{14,8,8,8,8,8,14};case ']':return{14,2,2,2,2,2,14};
    case '#':return{10,31,10,10,31,10,0};case '!':return{4,4,4,4,4,0,4};
    default:return{0,0,0,0,0,0,0};
    }
}
struct HudVertex{float x,y;glm::vec4 color;};
}

struct Renderer3D::Impl {
    GLuint lit=0,hud=0,hudVAO=0,hudVBO=0;
    Mesh cube,sphere,cylinder,taper;
    std::vector<HudVertex> hudVertices;
    int width=1,height=1;
    void object(const Mesh& mesh,const glm::mat4& model,glm::vec3 color,int material=0,float shine=12){
        glUniformMatrix4fv(glGetUniformLocation(lit,"uModel"),1,GL_FALSE,glm::value_ptr(model));
        glUniform3fv(glGetUniformLocation(lit,"uColor"),1,glm::value_ptr(color));
        glUniform1i(glGetUniformLocation(lit,"uMaterial"),material);glUniform1f(glGetUniformLocation(lit,"uShininess"),shine);mesh.draw();
    }
    void box(glm::vec3 center,glm::vec3 size,glm::vec3 color,int material=0,float yaw=0){
        glm::mat4 m=glm::translate(glm::mat4(1),center);m=glm::rotate(m,yaw,{0,1,0});m=glm::scale(m,size);object(cube,m,color,material);
    }
    void disk(glm::vec3 center,float radius,float h,glm::vec3 color){
        glm::mat4 m=glm::translate(glm::mat4(1),center);m=glm::scale(m,{radius,h,radius});object(cylinder,m,color);
    }
    void rod(glm::vec3 a,glm::vec3 b,float radius,glm::vec3 color,bool tapered=false){
        glm::vec3 delta=b-a;float len=glm::length(delta);if(len<1e-6f)return;
        glm::vec3 y=delta/len,cross=glm::cross(glm::vec3(0,0,1),y);
        glm::vec3 x=glm::length(cross)<.001f?glm::vec3(1,0,0):glm::normalize(cross);
        glm::vec3 z=glm::cross(x,y);
        glm::mat4 basis(1);basis[0]=glm::vec4(x,0);basis[1]=glm::vec4(y,0);basis[2]=glm::vec4(z,0);
        glm::mat4 m=glm::translate(glm::mat4(1),a)*basis; m=glm::scale(m,{radius,len,radius});
        object(tapered?taper:cylinder,m,color,0,28);
    }
    void rect(float x,float y,float w,float h,glm::vec4 c){
        float a=2*x/width-1,b=1-2*y/height,r=2*(x+w)/width-1,d=1-2*(y+h)/height;
        hudVertices.insert(hudVertices.end(),{{a,b,c},{r,b,c},{r,d,c},{a,b,c},{r,d,c},{a,d,c}});
    }
    void text(float x,float y,const std::string& s,float pixel,glm::vec4 c){
        const float start=x;
        for(char ch:s){if(ch=='\n'){x=start;y+=9*pixel;continue;}const auto g=glyph(ch);
            for(int r=0;r<7;++r)for(int col=0;col<5;++col)if(g[r]&(1<<(4-col)))rect(x+col*pixel,y+r*pixel,pixel*.87f,pixel*.87f,c);
            x+=6*pixel;
        }
    }
};

Renderer3D::Renderer3D():impl(new Impl){}
Renderer3D::~Renderer3D(){
    if(!impl)return;
    impl->cube.release();impl->sphere.release();impl->cylinder.release();impl->taper.release();
    if(impl->lit)glDeleteProgram(impl->lit);
    if(impl->hud)glDeleteProgram(impl->hud);
    if(impl->hudVBO)glDeleteBuffers(1,&impl->hudVBO);
    if(impl->hudVAO)glDeleteVertexArrays(1,&impl->hudVAO);
}
bool Renderer3D::initialize(std::string& error){
    impl->lit=program(vertex3D,fragment3D,error);if(!impl->lit)return false;
    impl->hud=program(vertex2D,fragment2D,error);if(!impl->hud)return false;
    impl->cube.upload(cubeMesh());impl->sphere.upload(sphereMesh());impl->cylinder.upload(cylinderMesh());impl->taper.upload(cylinderMesh(.45f));
    glGenVertexArrays(1,&impl->hudVAO);glGenBuffers(1,&impl->hudVBO);glBindVertexArray(impl->hudVAO);glBindBuffer(GL_ARRAY_BUFFER,impl->hudVBO);
    glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(HudVertex),reinterpret_cast<void*>(0));glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(HudVertex),reinterpret_cast<void*>(2*sizeof(float)));glEnableVertexAttribArray(1);
    glBindVertexArray(0);return true;
}
glm::mat4 Renderer3D::view(double cameraAzimuth)const{
    const auto p=cameraOrbitPosition(cameraAzimuth);
    const glm::vec3 eye{static_cast<float>(p.x),static_cast<float>(p.y),static_cast<float>(p.z)};
    return glm::lookAt(eye,glm::vec3(0,static_cast<float>(kCameraTargetY),0),glm::vec3(0,1,0));
}
glm::mat4 Renderer3D::projection(int width,int height)const{
    // A wider view keeps the near rails above the HUD throughout the orbit.
    return glm::perspective(glm::radians(55.f),static_cast<float>(std::max(1,width))/std::max(1,height),.05f,30.f);
}

void Renderer3D::render(const PhysicsWorld& world,const ViewState& state,int framebufferW,int framebufferH){
    if(framebufferW<=0||framebufferH<=0)return;
    auto& r=*impl;r.width=framebufferW;r.height=framebufferH;
    glViewport(0,0,framebufferW,framebufferH);glEnable(GL_DEPTH_TEST);glDisable(GL_BLEND);
    glClearColor(.035f,.060f,.10f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glUseProgram(r.lit);
    glm::mat4 V=view(state.cameraAzimuth),P=projection(framebufferW,framebufferH);
    glm::vec3 eye=glm::vec3(glm::inverse(V)[3]);
    glUniformMatrix4fv(glGetUniformLocation(r.lit,"uView"),1,GL_FALSE,glm::value_ptr(V));
    glUniformMatrix4fv(glGetUniformLocation(r.lit,"uProjection"),1,GL_FALSE,glm::value_ptr(P));
    glUniform3fv(glGetUniformLocation(r.lit,"uEye"),1,glm::value_ptr(eye));

    const glm::vec3 wood{.24f,.105f,.045f},woodTop{.40f,.215f,.092f},gold{.70f,.53f,.22f};
    const glm::vec3 felt{.035f,.34f,.23f},cushion{.025f,.23f,.16f},black{.006f,.009f,.014f};
    // The floor, table apron, and legs provide depth cues throughout the orbit.
    r.box({0,-.835f,0},{12,.03f,12},{.10f,.135f,.18f},2);
    r.box({0,-.155f,0},{2.95f,.27f,1.66f},wood,3);
    r.box({0,-.025f,0},{2.68f,.050f,1.40f},felt);
    r.box({0,-.265f,0},{2.78f,.035f,1.50f},gold);
    for(float xx:{-1.04f,1.04f})for(float zz:{-.48f,.48f}){
        r.box({xx,-.565f,zz},{.16f,.61f,.16f},wood,3);r.disk({xx,-.825f,zz},.11f,.025f,gold);
    }
    // Rail segments leave six physical pocket mouths visible.
    for(float zz:{-.745f,.745f}){
        for(float sign:{-1.f,1.f}){
            r.box({sign*.685f,.020f,zz},{1.12f,.10f,.17f},woodTop,3);
            r.box({sign*.680f,.030f,zz<0?-.665f:.665f},{1.10f,.066f,.060f},cushion);
        }
    }
    for(float xx:{-1.382f,1.382f}){
        r.box({xx,.020f,0},{.17f,.10f,1.10f},woodTop,3);
        r.box({xx<0?-1.300f:1.300f,.030f,0},{.060f,.066f,1.09f},cushion);
    }
    for(const auto& p:PhysicsWorld::pocketCenters()){
        r.disk({static_cast<float>(p.x),.002f,static_cast<float>(p.z)},.082f,.0015f,black);
    }
    // White inlaid rail sights/diamonds are simple rotated boxes.
    for(float zz:{-.747f,.747f})for(float xx:{-.96f,-.64f,-.32f,.32f,.64f,.96f})
        r.box({xx,.074f,zz},{.018f,.003f,.018f},{.91f,.88f,.72f},0,pi/4);
    for(float xx:{-1.385f,1.385f})for(float zz:{-.32f,0.f,.32f})
        r.box({xx,.074f,zz},{.018f,.003f,.018f},{.91f,.88f,.72f},0,pi/4);
    // The head string and foot spot aid aiming without requiring texture assets.
    r.box({-.635f,.001f,0},{.0015f,.001f,1.13f},{.10f,.44f,.31f});
    r.disk({.45f,.001f,0},.006f,.001f,{.80f,.82f,.69f});

    static const glm::vec3 ballColors[8]={{.95f,.78f,.12f},{.055f,.18f,.77f},{.78f,.055f,.04f},{.42f,.10f,.65f},
                                       {.96f,.35f,.04f},{.02f,.43f,.22f},{.52f,.045f,.11f},{.015f,.018f,.025f}};
    for(const auto& b:world.balls){
        if(b.pocketed)continue;
        const glm::vec3 center{static_cast<float>(b.position.x),static_cast<float>(b.radius),static_cast<float>(b.position.z)};
        r.disk({center.x,.001f,center.z},static_cast<float>(b.radius)*.94f,.0008f,{.018f,.17f,.12f});
        glm::mat4 m=glm::translate(glm::mat4(1),center);
        m=glm::rotate(m,static_cast<float>(b.rollAngle),glm::vec3(0,0,1));m=glm::scale(m,glm::vec3(static_cast<float>(b.radius)));
        glm::vec3 color=b.id==0?glm::vec3(.95f,.95f,.91f):ballColors[(b.id-1)%8];
        r.object(r.sphere,m,color,b.id>=9?1:0,80.f);
    }
    const Ball* cue=world.cueBall();
    const bool activeCue=cue&&!cue->pocketed&&(world.allStopped()||state.strikeAnimation>0);
    Vec2 aim=normalized(state.aim);if(length(aim)<.001)aim={1,0};
    if(activeCue){
        Vec2 cp=state.strikeAnimation>0?state.strikePosition:cue->position;
        const float pull=static_cast<float>(std::clamp(state.cuePull,0.,.35));
        const float gap=state.strikeAnimation>0?static_cast<float>(state.strikeAnimation)*.09f:.018f+pull;
        glm::vec3 backward{-static_cast<float>(aim.x),.16f,-static_cast<float>(aim.z)};backward=glm::normalize(backward);
        glm::vec3 tip{static_cast<float>(cp.x-aim.x*(kBallRadius+gap)),.039f,static_cast<float>(cp.z-aim.z*(kBallRadius+gap))};
        r.rod(tip,tip+backward*.016f,.0065f,{.13f,.55f,.65f});
        r.rod(tip+backward*.016f,tip+backward*.045f,.007f,{.95f,.94f,.85f});
        // Build the tapered shaft from its wide end toward the narrow tip.
        r.rod(tip+backward*1.02f,tip+backward*.045f,.012f,{.79f,.60f,.32f},true);
        r.rod(tip+backward*1.43f,tip+backward*1.02f,.019f,{.13f,.075f,.045f});
        r.rod(tip+backward*1.45f,tip+backward*1.43f,.020f,black);
        // Dashed center-line sight is guidance only, not a predicted collision path.
        if(world.allStopped())for(int n=1;n<21;++n){
            Vec2 q=cp+aim*(.05*n);
            if(std::abs(q.x)>kHalfLength||std::abs(q.z)>kHalfWidth)break;
            r.disk({static_cast<float>(q.x),.002f,static_cast<float>(q.z)},.0038f,.001f,{.78f,.72f,.39f});
        }
    }

    // HUD uses framebuffer pixels, so text remains stable when the window resizes.
    r.hudVertices.clear();
    const float scale=std::max(1.2f,std::min(framebufferW/720.f,framebufferH/450.f));
    const float pad=12*scale,small=1.0f*scale,big=1.50f*scale;
    const glm::vec4 white{.91f,.94f,.98f,1},g{.91f,.73f,.34f,1},muted{.66f,.75f,.83f,1};
    r.rect(0,0,static_cast<float>(framebufferW),54*scale,{.025f,.06f,.105f,.95f});
    r.text(pad,9*scale,"CAP4052  /  3D POOL PHYSICS",big,g);
    std::ostringstream summary;summary<<"OBJECT BALLS LEFT: "<<world.objectBallsRemaining()<<"   SHOTS: "<<state.shots<<"   SCRATCHES: "<<world.scratches;
    r.text(pad,31*scale,summary.str(),small,white);
    std::ostringstream status;status<<"ORBIT "<<std::fixed<<std::setprecision(0)
        <<wrappedCameraAzimuth(state.cameraAzimuth)*180.0/kCameraPi<<" DEG / 40 DEG   "
        <<(state.paused?"PAUSED":(world.allStopped()?"AIM AND SHOOT":"BALLS MOVING"));
    r.text(framebufferW-310*scale,10*scale,status.str(),small,white);
    std::ostringstream speed;speed<<std::fixed<<std::setprecision(2)<<"SHOT SPEED "<<state.shotSpeed<<" M/S"<<(state.charging?"  CHARGING":"");
    r.text(framebufferW-310*scale,29*scale,speed.str(),small,g);
    float power=static_cast<float>(std::clamp(state.shotSpeed/std::max(.01,world.params.maxSpeed),0.,1.));
    r.rect(framebufferW-112*scale,41*scale,100*scale,4*scale,{.15f,.20f,.26f,1});
    r.rect(framebufferW-112*scale,41*scale,100*scale*power,4*scale,g);

    const float hudHeight=(state.showHelp?108.f:57.f)*scale;
    r.rect(0,framebufferH-hudHeight,static_cast<float>(framebufferW),hudHeight,{.025f,.06f,.105f,.95f});
    float yy=framebufferH-hudHeight+9*scale;
    std::ostringstream metrics;metrics<<std::fixed<<std::setprecision(3)<<"TICK "<<world.tick<<"   TIME "<<world.simulationTime<<" S   STEP 1/240 S   KE "<<world.kineticEnergy()<<" J   DECEL "<<world.params.rollingDeceleration<<" M/S2";
    r.text(pad,yy,metrics.str(),small,white);yy+=14*scale;
    if(cue){std::ostringstream c;c<<std::fixed<<std::setprecision(4)<<"CUE (X,Z)=("<<cue->position.x<<","<<cue->position.z<<")   V=("<<cue->velocity.x<<","<<cue->velocity.z<<")   CONTACTS "<<world.collisions<<(state.traceEnabled?"   CSV ON":"");r.text(pad,yy,c.str(),small,muted);}yy+=14*scale;
    std::string message=state.message;
    if(message.empty())message=world.objectBallsRemaining()==0?"TABLE CLEARED - R RESTARTS THE RACK":"GOAL: POCKET ALL OBJECT BALLS. NO SPIN OR FULL COMPETITION RULES.";
    r.text(pad,yy,message.substr(0,104),small,g);yy+=17*scale;
    if(state.showHelp){
        r.text(pad,yy,"MOUSE AIM   HOLD/RELEASE LEFT BUTTON: CHARGE/FIRE   A/D FINE AIM   W/S POWER   SPACE FIRE",small,white);yy+=14*scale;
        r.text(pad,yy,"HOLD C ORBIT   SHIFT+C REVERSE   P PAUSE   N ONE TICK   R RACK   1/2/3/4 SCENARIOS",small,white);yy+=14*scale;
        r.text(pad,yy,std::string("L FREE-ROLL   F3 FRICTION   F2 CSV   M SOUND ")+(state.soundEnabled?"ON":"OFF")+"   F1 HIDE HELP   ESC QUIT",small,white);
    }
    glDisable(GL_DEPTH_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glUseProgram(r.hud);
    glBindVertexArray(r.hudVAO);glBindBuffer(GL_ARRAY_BUFFER,r.hudVBO);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(r.hudVertices.size()*sizeof(HudVertex)),r.hudVertices.data(),GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(r.hudVertices.size()));glBindVertexArray(0);glDisable(GL_BLEND);
}
} // namespace pool
