#include <iostream>
#include <vector>
using namespace std;

// Structure for 3D point
struct Point3D {
    float x, y, z;
};

// Structure for 2D point
struct Point2D {
    float x, y;
};

// Orthogonal Projection: ignore z
Point2D orthogonalProjection(const Point3D& p) {
    return {p.x, p.y};
}

// Perspective Projection: divide by z (assuming z != 0)
Point2D perspectiveProjection(const Point3D& p, float d = 1.0f) {
    return { (p.x * d) / (p.z + d), (p.y * d) / (p.z + d) };
}

int main() {
    // Define a cube in 3D space
    vector<Point3D> cube = {
        { -1, -1,  1 }, { 1, -1,  1 },
        {  1,  1,  1 }, { -1,  1,  1 },
        { -1, -1, -1 }, { 1, -1, -1 },
        {  1,  1, -1 }, { -1,  1, -1 }
    };

    cout << "Projection of 3D Cube Vertices:\n\n";

    cout << "Orthogonal Projection (x,y):\n";
    for (auto& p : cube) {
        Point2D proj = orthogonalProjection(p);
        cout << "(" << proj.x << ", " << proj.y << ")\n";
    }

    cout << "\nPerspective Projection (x',y'):\n";
    for (auto& p : cube) {
        Point2D proj = perspectiveProjection(p, 2.0f); // distance of camera
        cout << "(" << proj.x << ", " << proj.y << ")\n";
    }

    return 0;
}
