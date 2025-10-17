// mini_rt.cpp
// Minimal CPU ray tracer (C++17). Compile: g++ -O2 -std=c++17 mini_rt.cpp -o mini_rt
// Run: ./mini_rt   -> writes scene.ppm (open with any image viewer that supports PPM)

#include <cmath>
#include <limits>
#include <fstream>
#include <vector>
#include <iostream>

// small math helpers
struct Vec {
    double x,y,z;
    Vec() : x(0),y(0),z(0) {}
    Vec(double x_, double y_, double z_) : x(x_),y(y_),z(z_) {}
    Vec operator+(const Vec& b) const { return Vec(x+b.x,y+b.y,z+b.z); }
    Vec operator-(const Vec& b) const { return Vec(x-b.x,y-b.y,z-b.z); }
    Vec operator*(double s) const { return Vec(x*s,y*s,z*s); }
    Vec operator*(const Vec& b) const { return Vec(x*b.x,y*b.y,z*b.z); } // componentwise
    Vec operator/(double s) const { return Vec(x/s,y/s,z/s); }
};
inline double dot(const Vec& a, const Vec& b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec normalize(const Vec& v){ double n = std::sqrt(dot(v,v)); return v / (n>0? n:1); }
inline Vec reflect(const Vec& I, const Vec& N){ return I - N*(2*dot(I,N)); }
const double EPS = 1e-6;
const double INF = 1e20;

// ray
struct Ray { Vec o, d; Ray(const Vec&o_, const Vec&d_):o(o_),d(d_){} };

// material
struct Material {
    Vec color;      // base color
    double reflect; // 0..1 reflection strength
    Material(const Vec&c=Vec(1,1,1), double r=0) : color(c), reflect(r) {}
};

// sphere
struct Sphere {
    Vec c; double r;
    Material m;
    Sphere(const Vec&c_, double r_, const Material&m_) : c(c_), r(r_), m(m_) {}
    // returns t > 0 or INF
    double intersect(const Ray& ray) const {
        Vec oc = ray.o - c;
        double a = dot(ray.d, ray.d);
        double b = 2*dot(oc, ray.d);
        double cterm = dot(oc, oc) - r*r;
        double disc = b*b - 4*a*cterm;
        if (disc < 0) return INF;
        double sq = std::sqrt(disc);
        double t1 = (-b - sq) / (2*a);
        double t2 = (-b + sq) / (2*a);
        if (t1 > EPS) return t1;
        if (t2 > EPS) return t2;
        return INF;
    }
};

// scene global
std::vector<Sphere> spheres;

// find closest hit
bool scene_intersect(const Ray& ray, double &t, int &id) {
    t = INF; id = -1;
    for (int i=0;i<(int)spheres.size();++i){
        double ti = spheres[i].intersect(ray);
        if (ti < t) { t = ti; id = i; }
    }
    return id != -1;
}

// simple direct illumination with shadows and single reflection bounce
Vec trace(const Ray& ray, int depth=0) {
    if (depth > 3) return Vec(0,0,0); // limit recursion

    double t; int id;
    if (!scene_intersect(ray, t, id)) {
        // background gradient
        double tunit = 0.5*(normalize(ray.d).y + 1.0);
        return Vec(0.7,0.8,1.0)*(1.0 - tunit) + Vec(1.0,1.0,1.0)*tunit;
    }

    const Sphere &obj = spheres[id];
    Vec hit = ray.o + ray.d * t;
    Vec N = normalize(hit - obj.c);

    // simple ambient + diffuse + specular
    Vec col = obj.m.color * 0.05; // ambient

    // define a single point light
    Vec lightPos(5, 10, -2);
    Vec lightCol(1.0, 1.0, 1.0);
    Vec toLight = normalize(lightPos - hit);
    double nl = std::max(0.0, dot(N, toLight));

    // shadow check
    Ray shadowRay(hit + N * 1e-4, toLight);
    double ts; int sid;
    bool inShadow = scene_intersect(shadowRay, ts, sid) && ts < std::sqrt(dot(lightPos - hit, lightPos - hit)) - 1e-6;

    if (!inShadow) {
        // diffuse
        col = col + obj.m.color * (nl * 0.9) * lightCol;
        // simple Blinn-Phong specular
        Vec viewDir = normalize(ray.o - hit);
        Vec halfv = normalize(viewDir + toLight);
        double spec = pow(std::max(0.0, dot(N, halfv)), 64);
        col = col + lightCol * (spec * 0.6);
    } else {
        // slightly darken when in shadow
        col = col * 0.4;
    }

    // reflection
    if (obj.m.reflect > 1e-6) {
        Vec reflDir = normalize(reflect(ray.d, N));
        Ray reflRay(hit + N * 1e-4, reflDir);
        Vec reflCol = trace(reflRay, depth+1);
        col = col*(1.0 - obj.m.reflect) + reflCol * obj.m.reflect;
    }

    // clamp to [0,1]
    col.x = std::min(1.0, std::max(0.0, col.x));
    col.y = std::min(1.0, std::max(0.0, col.y));
    col.z = std::min(1.0, std::max(0.0, col.z));
    return col;
}

int main(){
    // build a simple scene
    spheres.push_back(Sphere(Vec(0.0, -10004, -20), 10000, Material(Vec(0.8,0.8,0.8), 0.0))); // ground as big sphere
    spheres.push_back(Sphere(Vec(0.0, 0.0, -6), 1.0, Material(Vec(0.9,0.1,0.1), 0.25))); // red sphere
    spheres.push_back(Sphere(Vec(2.0, 0.2, -7), 1.2, Material(Vec(0.1,0.3,0.9), 0.5)));  // blue reflective

    // image
    const int width = 800;
    const int height = 600;
    const double fov = M_PI/3.0; // 60 degrees

    std::vector<unsigned char> img(width * height * 3);

    Vec camPos(0, 0, 0);

    for (int j = 0; j < height; ++j) {
        for (int i = 0; i < width; ++i) {
            // normalized device coords
            double x = (2 * (i + 0.5) / (double)width  - 1) * tan(fov/2.0) * (width / (double)height);
            double y = (1 - 2 * (j + 0.5) / (double)height) * tan(fov/2.0);

            Ray ray(camPos, normalize(Vec(x, y, -1)));
            Vec color = trace(ray, 0);

            // gamma correction (approx)
            auto to8 = [](double c)->unsigned char {
                c = std::pow(std::max(0.0, std::min(1.0, c)), 1.0/2.2);
                return (unsigned char)(std::round(c * 255.0));
            };

            int idx = (j*width + i) * 3;
            img[idx+0] = to8(color.x);
            img[idx+1] = to8(color.y);
            img[idx+2] = to8(color.z);
        }
        if ((j%50)==0) std::cout << "scanline " << j << "/" << height << "\n";
    }

    // write PPM
    std::ofstream ofs("scene.ppm", std::ios::binary);
    ofs << "P6\n" << width << " " << height << "\n255\n";
    ofs.write(reinterpret_cast<char*>(img.data()), img.size());
    ofs.close();

    std::cout << "Wrote scene.ppm (" << width << "x" << height << ")\n";
    return 0;
}
