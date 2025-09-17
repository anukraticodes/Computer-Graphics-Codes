// main.cpp
// 3D Hybrid Transformations (Homogeneous Coordinates) Demo
// Compile: clang++ main.cpp -o main -std=c++17 -I/opt/homebrew/include -L/opt/homebrew/lib -framework OpenGL -lglfw

#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <array>
#include <iomanip>

using namespace std;

// ---- Matrix 4x4 definition ----
struct Mat4 {
    array<float,16> m;
    Mat4(){ m.fill(0.0f); }
    static Mat4 identity(){
        Mat4 I; for(int i=0;i<4;i++) I.m[i*4 + i] = 1.0f; return I;
    }
    float& at(int r,int c){ return m[c*4 + r]; }
    const float& at(int r,int c) const { return m[c*4 + r]; }
};

// ---- Matrix operations ----
Mat4 mul(const Mat4 &A, const Mat4 &B){
    Mat4 C;
    for(int r=0;r<4;r++){
        for(int c=0;c<4;c++){
            float sum = 0.0f;
            for(int k=0;k<4;k++) sum += A.at(r,k) * B.at(k,c);
            C.at(r,c) = sum;
        }
    }
    return C;
}

array<float,4> mulVec(const Mat4 &A, const array<float,4> &v){
    array<float,4> r{};
    for(int i=0;i<4;i++)
        r[i] = A.at(i,0)*v[0] + A.at(i,1)*v[1] + A.at(i,2)*v[2] + A.at(i,3)*v[3];
    return r;
}

// ---- Transform matrices ----
Mat4 translate(float tx,float ty,float tz){ Mat4 T=Mat4::identity(); T.at(0,3)=tx; T.at(1,3)=ty; T.at(2,3)=tz; return T; }
Mat4 scaleM(float sx,float sy,float sz){ Mat4 S=Mat4::identity(); S.at(0,0)=sx; S.at(1,1)=sy; S.at(2,2)=sz; return S; }
Mat4 rotateX(float a){ float r=a*M_PI/180.0f,c=cosf(r),s=sinf(r); Mat4 R=Mat4::identity(); R.at(1,1)=c;R.at(1,2)=-s;R.at(2,1)=s;R.at(2,2)=c;return R; }
Mat4 rotateY(float a){ float r=a*M_PI/180.0f,c=cosf(r),s=sinf(r); Mat4 R=Mat4::identity(); R.at(0,0)=c;R.at(0,2)=s;R.at(2,0)=-s;R.at(2,2)=c;return R; }
Mat4 rotateZ(float a){ float r=a*M_PI/180.0f,c=cosf(r),s=sinf(r); Mat4 R=Mat4::identity(); R.at(0,0)=c;R.at(0,1)=-s;R.at(1,0)=s;R.at(1,1)=c;return R; }
Mat4 reflect(char axis){ Mat4 M=Mat4::identity(); if(axis=='x') M.at(0,0)=-1; if(axis=='y') M.at(1,1)=-1; if(axis=='z') M.at(2,2)=-1; return M; }
Mat4 shear(float shxy,float shxz,float shyx,float shyz,float shzx,float shzy){
    Mat4 H=Mat4::identity(); H.at(0,1)=shxy;H.at(0,2)=shxz;H.at(1,0)=shyx;H.at(1,2)=shyz;H.at(2,0)=shzx;H.at(2,1)=shzy; return H;
}

// ---- Cube definition ----
vector<array<float,3>> cubeVertices = {
    {-0.5f,-0.5f,-0.5f},{0.5f,-0.5f,-0.5f},{0.5f,0.5f,-0.5f},{-0.5f,0.5f,-0.5f},
    {-0.5f,-0.5f,0.5f},{0.5f,-0.5f,0.5f},{0.5f,0.5f,0.5f},{-0.5f,0.5f,0.5f}
};
vector<array<int,4>> cubeFaces = { {0,1,2,3},{4,5,6,7},{0,1,5,4},{3,2,6,7},{1,2,6,5},{0,3,7,4} };

vector<array<float,3>> applyTransform(const Mat4 &T,const vector<array<float,3>> &in){
    vector<array<float,3>> out;
    for(auto &v:in){
        array<float,4> hv={v[0],v[1],v[2],1.0f};
        auto tv=mulVec(T,hv);
        float w=tv[3]==0?1.0f:tv[3];
        out.push_back({tv[0]/w,tv[1]/w,tv[2]/w});
    }
    return out;
}

void printMat(const Mat4 &M){
    cout<<fixed<<setprecision(3);
    for(int r=0;r<4;r++){ for(int c=0;c<4;c++) cout<<M.at(r,c)<<"\t"; cout<<"\n"; } cout<<"\n";
}

// ---- Global ----
Mat4 composite = Mat4::identity();

// ---- Key events ----
void keyCallback(GLFWwindow* w,int key,int scancode,int action,int mods){
    if(action!=GLFW_PRESS) return;
    bool shift = mods & GLFW_MOD_SHIFT;
    if(key==GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(w,GL_TRUE);
    if(key==GLFW_KEY_E){ composite=Mat4::identity(); cout<<"Reset\n"; }
    if(key==GLFW_KEY_T){ float step=shift?-0.2f:0.2f; composite=mul(translate(step,0,0),composite); cout<<"Translate\n"; printMat(composite);}
    if(key==GLFW_KEY_S){ float s=shift?0.8f:1.25f; composite=mul(scaleM(s,s,s),composite); cout<<"Scale\n"; printMat(composite);}
    if(key==GLFW_KEY_R){ composite=mul(rotateY(shift?-15:15),composite); cout<<"Rotate Y\n"; printMat(composite);}
    if(key==GLFW_KEY_X){ composite=mul(rotateX(shift?-15:15),composite); cout<<"Rotate X\n"; printMat(composite);}
    if(key==GLFW_KEY_Z){ composite=mul(rotateZ(shift?-15:15),composite); cout<<"Rotate Z\n"; printMat(composite);}
    if(key==GLFW_KEY_F){ static int mode=0; mode=(mode+1)%4; if(mode==1)composite=mul(reflect('x'),composite); if(mode==2)composite=mul(reflect('y'),composite); if(mode==3)composite=mul(reflect('z'),composite); cout<<"Reflect\n"; printMat(composite);}
    if(key==GLFW_KEY_H){ float sh=shift?-0.25f:0.25f; composite=mul(shear(sh,0,0,0,0,0),composite); cout<<"Shear\n"; printMat(composite);}
}

// ---- Drawing helpers ----
void drawAxes(float len=1.2f){
    glBegin(GL_LINES);
    glColor3f(1,0,0); glVertex3f(0,0,0); glVertex3f(len,0,0);
    glColor3f(0,1,0); glVertex3f(0,0,0); glVertex3f(0,len,0);
    glColor3f(0,0,1); glVertex3f(0,0,0); glVertex3f(0,0,len);
    glEnd();
}

int main(){
    if(!glfwInit()){ cerr<<"GLFW init failed\n"; return -1; }
    GLFWwindow* window=glfwCreateWindow(900,700,"3D Hybrid Transformations",NULL,NULL);
    if(!window){ cerr<<"Window creation failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window,keyCallback);

    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect=900.0f/700.0f;
    glFrustum(-aspect,aspect,-1,1,1,10);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0,0,-3);

    auto baseVerts=cubeVertices;
    while(!glfwWindowShouldClose(window)){
        glClearColor(0.9f,0.9f,0.95f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        drawAxes();

        // Original cube (wireframe)
        glColor3f(0.2f,0.2f,0.2f);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        glBegin(GL_QUADS);
        for(auto &f:cubeFaces) for(int i=0;i<4;i++){auto &v=baseVerts[f[i]]; glVertex3f(v[0],v[1],v[2]);}
        glEnd();

        // Transformed cube
        auto transformed=applyTransform(composite,baseVerts);
        glColor4f(0.8f,0.3f,0.3f,0.8f);
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        for(auto &f:cubeFaces){ glBegin(GL_QUADS); for(int i=0;i<4;i++){auto &v=transformed[f[i]]; glVertex3f(v[0],v[1],v[2]);} glEnd();}
        glfwSwapBuffers(window); glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
