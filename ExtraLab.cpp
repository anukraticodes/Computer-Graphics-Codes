// main.cpp
// Virtual environment with visual actors using OpenGL, GLFW, GLAD, GLM, and stb_image.
// Build: see instructions in the message.

#include "glad/glad.h"   // ✅ Use quotes, not < >
#include <GLUT/glut.h>   // macOS uses GLUT here
#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <map>
#include <functional>

// -------------------- Utility / Helpers --------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0,0,width,height);
}
float lastX=800/2.0f, lastY=600/2.0f;
bool firstMouse=true;
float yaw=-90.0f, pitch=0.0f;
float fov=45.0f;

struct Camera {
    glm::vec3 pos{0.0f, 2.0f, 6.0f};
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f,1.0f,0.0f};
    float speed = 5.0f;
} camera;

void processInput(GLFWwindow* w, float dt) {
    if(glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS) camera.pos += camera.speed*dt*camera.front;
    if(glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS) camera.pos -= camera.speed*dt*camera.front;
    glm::vec3 right = glm::normalize(glm::cross(camera.front, camera.up));
    if(glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS) camera.pos -= right * camera.speed * dt;
    if(glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS) camera.pos += right * camera.speed * dt;
    if(glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS) camera.pos += camera.speed*dt*camera.up;
    if(glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.pos -= camera.speed*dt*camera.up;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if(firstMouse) { lastX = xpos; lastY = ypos; firstMouse=false;}
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos; lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity; yoffset *= sensitivity;
    yaw += xoffset; pitch += yoffset;
    if(pitch>89.0f) pitch=89.0f;
    if(pitch<-89.0f) pitch=-89.0f;
    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw))*cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw))*cos(glm::radians(pitch));
    camera.front = glm::normalize(dir);
}

unsigned int loadTexture(const char* path) {
    int w,h,n;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w,&h,&n,0);
    if(!data) {
        std::cerr<<"Failed to load texture: "<<path<<"\n";
        return 0;
    }
    GLenum format = GL_RGB;
    if(n==1) format=GL_RED;
    else if(n==3) format=GL_RGB;
    else if(n==4) format=GL_RGBA;
    unsigned int tex; glGenTextures(1,&tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D,0,format,w,h,0,format,GL_UNSIGNED_BYTE,data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    stbi_image_free(data);
    return tex;
}

// -------------------- Shader helper --------------------
unsigned int compileShader(GLenum type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id,1,&src,nullptr);
    glCompileShader(id);
    int success; glGetShaderiv(id, GL_COMPILE_STATUS, &success);
    if(!success) {
        char info[1024]; glGetShaderInfoLog(id,1024,nullptr,info);
        std::cerr<<"Shader compile error: "<<info<<"\n";
    }
    return id;
}

unsigned int createProgram(const char* vs, const char* fs) {
    unsigned int p = glCreateProgram();
    unsigned int a = compileShader(GL_VERTEX_SHADER, vs);
    unsigned int b = compileShader(GL_FRAGMENT_SHADER, fs);
    glAttachShader(p,a); glAttachShader(p,b);
    glLinkProgram(p);
    int success; glGetProgramiv(p, GL_LINK_STATUS, &success);
    if(!success) {
        char info[1024]; glGetProgramInfoLog(p,1024,nullptr,info);
        std::cerr<<"Program link error: "<<info<<"\n";
    }
    glDeleteShader(a); glDeleteShader(b);
    return p;
}

// -------------------- Shaders --------------------
const char* vertexShaderSrc = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec2 aTex;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

out vec3 FragPos;
out vec3 Normal;
out vec2 Tex;

void main(){
    FragPos = vec3(model * vec4(aPos,1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    Tex = aTex;
    gl_Position = proj * view * vec4(FragPos,1.0);
}
)glsl";

const char* fragmentShaderSrc = R"glsl(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 Tex;

out vec4 FragColor;

struct DirLight { vec3 dir; vec3 ambient; vec3 diffuse; vec3 spec; };
uniform DirLight dlight;
uniform vec3 viewPos;

uniform sampler2D tex; 
uniform vec3 baseColor;
uniform float useTex; // 1.0 => use texture

void main(){
    vec3 color = baseColor;
    if(useTex>0.5) color = texture(tex, Tex).rgb;
    // ambient
    vec3 ambient = dlight.ambient * color;
    // diffuse
    vec3 n = normalize(Normal);
    vec3 lightDir = normalize(-dlight.dir);
    float diff = max(dot(n, lightDir), 0.0);
    vec3 diffuse = dlight.diffuse * diff * color;
    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, n);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = dlight.spec * spec;
    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
)glsl";

// -------------------- Mesh Generators --------------------
struct Mesh {
    unsigned int VAO=0, VBO=0, EBO=0;
    int indexCount=0;
};

Mesh createPlane(float size=20.0f) {
    float s = size;
    // positions, normals, texcoords
    float vertices[] = {
        // pos               normal           uv
         s, 0.0f,  s,   0,1,0,  s, 0,
        -s, 0.0f,  s,   0,1,0,  0, 0,
        -s, 0.0f, -s,   0,1,0,  0, s,
         s, 0.0f, -s,   0,1,0,  s, s
    };
    unsigned int indices[] = {0,1,2, 0,2,3};
    Mesh m;
    glGenVertexArrays(1,&m.VAO);
    glGenBuffers(1,&m.VBO);
    glGenBuffers(1,&m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    m.indexCount = 6;
    glBindVertexArray(0);
    return m;
}

Mesh createCube() {
    // 36 vertices (6 faces * 6 vertices) but we'll use a compact list with indices
    float verts[] = {
        // pos           normal          uv
        // front
        -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,
         0.5f,-0.5f, 0.5f,  0,0,1,  1,0,
         0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
        -0.5f, 0.5f, 0.5f,  0,0,1,  0,1,
        // back
        -0.5f,-0.5f,-0.5f,  0,0,-1, 0,0,
         0.5f,-0.5f,-0.5f,  0,0,-1, 1,0,
         0.5f, 0.5f,-0.5f,  0,0,-1, 1,1,
        -0.5f, 0.5f,-0.5f,  0,0,-1, 0,1,
        // left
        -0.5f,-0.5f,-0.5f, -1,0,0,  0,0,
        -0.5f,-0.5f, 0.5f, -1,0,0,  1,0,
        -0.5f, 0.5f, 0.5f, -1,0,0,  1,1,
        -0.5f, 0.5f,-0.5f, -1,0,0,  0,1,
        // right
         0.5f,-0.5f,-0.5f, 1,0,0,  0,0,
         0.5f,-0.5f, 0.5f, 1,0,0,  1,0,
         0.5f, 0.5f, 0.5f, 1,0,0,  1,1,
         0.5f, 0.5f,-0.5f, 1,0,0,  0,1,
        // top
        -0.5f, 0.5f, 0.5f, 0,1,0,  0,0,
         0.5f, 0.5f, 0.5f, 0,1,0,  1,0,
         0.5f, 0.5f,-0.5f, 0,1,0,  1,1,
        -0.5f, 0.5f,-0.5f, 0,1,0,  0,1,
        // bottom
        -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0,
         0.5f,-0.5f, 0.5f, 0,-1,0, 1,0,
         0.5f,-0.5f,-0.5f, 0,-1,0, 1,1,
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1
    };
    unsigned int idx[] = {
        0,1,2, 0,2,3,         // front
        4,5,6, 4,6,7,         // back
        8,9,10, 8,10,11,      // left
        12,13,14, 12,14,15,   // right
        16,17,18, 16,18,19,   // top
        20,21,22, 20,22,23    // bottom
    };
    Mesh m;
    glGenVertexArrays(1,&m.VAO);
    glGenBuffers(1,&m.VBO);
    glGenBuffers(1,&m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    m.indexCount = sizeof(idx)/sizeof(unsigned int);
    glBindVertexArray(0);
    return m;
}

Mesh createUVSphere(int rings = 16, int sectors = 32) {
    std::vector<float> verts;
    std::vector<unsigned int> idx;
    for(int r=0;r<=rings;++r){
        float v = (float)r / (float)rings;
        float phi = (v - 0.5f) * glm::pi<float>(); // -pi/2 .. pi/2
        for(int s=0;s<=sectors;++s){
            float u = (float)s / (float)sectors;
            float theta = u * 2.0f * glm::pi<float>();
            float x = cos(phi) * cos(theta);
            float y = sin(phi);
            float z = cos(phi) * sin(theta);
            // pos
            verts.push_back(x*0.5f);
            verts.push_back(y*0.5f);
            verts.push_back(z*0.5f);
            // normal
            verts.push_back(x); verts.push_back(y); verts.push_back(z);
            // tex
            verts.push_back(u); verts.push_back(v);
        }
    }
    for(int r=0;r<rings;++r){
        for(int s=0;s<sectors;++s){
            int a = r*(sectors+1) + s;
            int b = (r+1)*(sectors+1) + s;
            idx.push_back(a); idx.push_back(b); idx.push_back(a+1);
            idx.push_back(b); idx.push_back(b+1); idx.push_back(a+1);
        }
    }
    Mesh m;
    glGenVertexArrays(1,&m.VAO);
    glGenBuffers(1,&m.VBO);
    glGenBuffers(1,&m.EBO);
    glBindVertexArray(m.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size()*sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size()*sizeof(unsigned int), idx.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,8*sizeof(float),(void*)(6*sizeof(float)));
    m.indexCount = (int)idx.size();
    glBindVertexArray(0);
    return m;
}

// -------------------- Actor system --------------------
enum ActorKind { TREE, ROCK, NPC, CRATE };

struct Actor {
    ActorKind kind;
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 rotation; // euler degrees
    unsigned int tex; // optional
    glm::vec3 color;
    // model-specific params (e.g., animation)
    float radius = 0.0f;
    float speed = 0.0f;
};

std::vector<Actor> actors;

void populateActors(unsigned int barkTex, unsigned int leafTex, unsigned int crateTex, unsigned int npcTex) {
    // Trees (group)
    for(int i=0;i<8;i++){
        float x = -8.0f + i*2.2f + ((i%2)?0.6f:0.0f);
        float z = -6.0f + ((i%3)-1)*0.8f;
        Actor t;
        t.kind = TREE;
        t.position = {x, 0.0f, z};
        t.scale = {1.0f,1.0f,1.0f};
        t.tex = (i%2==0)?barkTex:leafTex; // trunk uses bark; foliage uses leaf; we'll bind accordingly
        actors.push_back(t);
    }
    // Rocks
    for(int i=0;i<6;i++){
        float x = -6.0f + i*2.6f;
        float z = 3.0f + (i%2?1.2f:-1.2f);
        Actor r;
        r.kind = ROCK;
        r.position = {x, 0.0f, z};
        r.scale = {0.6f + (i%3)*0.2f, 0.4f, 0.6f};
        r.rotation = {0.0f, (i*37.0f), 0.0f};
        r.color = {0.5f,0.45f,0.4f};
        actors.push_back(r);
    }
    // Crates (decor)
    for(int i=0;i<4;i++){
        Actor c;
        c.kind = CRATE;
        c.position = {4.0f + (i%2)*1.5f, 0.0f, -2.0f + (i/2)*1.5f};
        c.scale = {0.7f,0.7f,0.7f};
        c.tex = crateTex;
        actors.push_back(c);
    }
    // NPC moving in a circle
    Actor npc;
    npc.kind = NPC;
    npc.position = {0.0f, 0.5f, 0.0f};
    npc.scale = {0.6f,0.9f,0.6f};
    npc.tex = npcTex;
    npc.radius = 3.0f;
    npc.speed = 0.9f; // radians per second
    actors.push_back(npc);
}

// -------------------- Main --------------------
int main(){
    if(!glfwInit()) { std::cerr<<"GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(1280,720,"Virtual Environment - Actors", nullptr,nullptr);
    if(!window) { std::cerr<<"Window create failed\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr<<"GLAD init failed\n"; return -1;
    }
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    unsigned int program = createProgram(vertexShaderSrc, fragmentShaderSrc);

    Mesh plane = createPlane(20.0f);
    Mesh cube = createCube();
    Mesh sphere = createUVSphere(18, 32);

    // Load textures (optional) - place images in working directory
    unsigned int groundTex = loadTexture("ground.jpg");
    unsigned int barkTex = loadTexture("bark.jpg");
    unsigned int leafTex = loadTexture("leaf.jpg");
    unsigned int crateTex = loadTexture("crate.jpg");
    unsigned int npcTex = loadTexture("npc.jpg");

    populateActors(barkTex, leafTex, crateTex, npcTex);

    // Setup light
    glUseProgram(program);
    GLint dlightDirLoc = glGetUniformLocation(program, "dlight.dir");
    GLint dlightAmbLoc = glGetUniformLocation(program, "dlight.ambient");
    GLint dlightDiffLoc = glGetUniformLocation(program, "dlight.diffuse");
    GLint dlightSpecLoc = glGetUniformLocation(program, "dlight.spec");

    glUniform3f(dlightDirLoc, -0.3f, -1.0f, -0.4f);
    glUniform3f(dlightAmbLoc, 0.25f, 0.25f, 0.25f);
    glUniform3f(dlightDiffLoc, 0.9f, 0.9f, 0.9f);
    glUniform3f(dlightSpecLoc, 1.0f, 1.0f, 1.0f);

    // Uniform locations
    GLint modelLoc = glGetUniformLocation(program, "model");
    GLint viewLoc = glGetUniformLocation(program, "view");
    GLint projLoc = glGetUniformLocation(program, "proj");
    GLint viewPosLoc = glGetUniformLocation(program, "viewPos");
    GLint baseColorLoc = glGetUniformLocation(program, "baseColor");
    GLint useTexLoc = glGetUniformLocation(program, "useTex");
    GLint texLoc = glGetUniformLocation(program, "tex");
    glUniform1i(texLoc, 0);

    float lastTime = glfwGetTime();
    while(!glfwWindowShouldClose(window)){
        float now = glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        processInput(window, dt);

        glClearColor(0.6f, 0.85f, 1.0f, 1.0f); // sky color
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program);

        // Camera matrices
        glm::mat4 view = glm::lookAt(camera.pos, camera.pos + camera.front, camera.up);
        glm::mat4 proj = glm::perspective(glm::radians(fov), 1280.0f/720.0f, 0.1f, 100.0f);
        glUniformMatrix4fv(viewLoc,1,GL_FALSE,glm::value_ptr(view));
        glUniformMatrix4fv(projLoc,1,GL_FALSE,glm::value_ptr(proj));
        glUniform3fv(viewPosLoc,1,glm::value_ptr(camera.pos));

        // Draw ground
        glActiveTexture(GL_TEXTURE0);
        if(groundTex) glBindTexture(GL_TEXTURE_2D, groundTex);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(model));
        glUniform3f(baseColorLoc, 0.6f,0.7f,0.5f);
        glUniform1f(useTexLoc, groundTex?1.0f:0.0f);
        glBindVertexArray(plane.VAO);
        glDrawElements(GL_TRIANGLES, plane.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        // Draw actors
        for(auto &a : actors){
            glm::mat4 m = glm::mat4(1.0f);
            // animated NPC
            if(a.kind == NPC){
                float ang = now * a.speed;
                float x = a.radius * cos(ang);
                float z = a.radius * sin(ang);
                a.position.x = x;
                a.position.z = z;
                // make NPC face direction of motion
                float yawAngle = glm::degrees(atan2(-sin(ang), -cos(ang)));
                a.rotation.y = yawAngle;
            }
            m = glm::translate(m, a.position + glm::vec3(0.0f, a.scale.y*0.5f, 0.0f)); // lift by half height-ish
            m = glm::rotate(m, glm::radians(a.rotation.x), glm::vec3(1,0,0));
            m = glm::rotate(m, glm::radians(a.rotation.y), glm::vec3(0,1,0));
            m = glm::rotate(m, glm::radians(a.rotation.z), glm::vec3(0,0,1));
            m = glm::scale(m, a.scale);
            if(a.kind == TREE){
                // draw trunk (cube, scaled), then foliage (sphere)
                // trunk
                glm::mat4 trunkM = m;
                trunkM = glm::scale(trunkM, glm::vec3(0.4f,1.0f,0.4f)); // skinny trunk
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(trunkM));
                glUniform3f(baseColorLoc, 0.55f, 0.3f, 0.15f);
                glUniform1f(useTexLoc, a.tex?1.0f:0.0f);
                if(a.tex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, a.tex); }
                glBindVertexArray(cube.VAO);
                glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
                // foliage
                glm::mat4 folM = glm::translate(m, glm::vec3(0.0f, 0.9f, 0.0f));
                folM = glm::scale(folM, glm::vec3(1.4f,1.2f,1.4f));
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(folM));
                glUniform3f(baseColorLoc, 0.2f, 0.6f, 0.2f);
                // foliage uses leaf tex if present -> we stored leaf in same actor.tex sometimes
                glUniform1f(useTexLoc, a.tex?1.0f:0.0f);
                if(a.tex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, a.tex); }
                glBindVertexArray(sphere.VAO);
                glDrawElements(GL_TRIANGLES, sphere.indexCount, GL_UNSIGNED_INT, 0);
            } else if(a.kind == ROCK) {
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(m));
                glUniform3fv(baseColorLoc,1,glm::value_ptr(a.color));
                glUniform1f(useTexLoc, 0.0f);
                glBindVertexArray(cube.VAO);
                glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
            } else if(a.kind == CRATE){
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(m));
                glUniform3f(baseColorLoc, 1.0f,1.0f,1.0f);
                glUniform1f(useTexLoc, a.tex?1.0f:0.0f);
                if(a.tex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, a.tex); }
                glBindVertexArray(cube.VAO);
                glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
            } else if(a.kind == NPC){
                glUniformMatrix4fv(modelLoc,1,GL_FALSE,glm::value_ptr(m));
                glUniform3f(baseColorLoc, 0.8f, 0.25f, 0.3f);
                glUniform1f(useTexLoc, a.tex?1.0f:0.0f);
                if(a.tex) { glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, a.tex); }
                glBindVertexArray(cube.VAO);
                glDrawElements(GL_TRIANGLES, cube.indexCount, GL_UNSIGNED_INT, 0);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
