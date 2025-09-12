#include <GL/glut.h>
#include <iostream>
#include <cmath>
using namespace std;

int x1_in, y1_in, x2_in, y2_in;

// Function to plot a point
void plotPoint(int x, int y) {
    glBegin(GL_POINTS);
    glVertex2i(x, y);
    glEnd();
    glFlush();
}

// DDA Algorithm
void drawLineDDA(int x1, int y1, int x2, int y2) {
    int dx = x2 - x1;
    int dy = y2 - y1;

    int steps = max(abs(dx), abs(dy));

    float xInc = dx / (float) steps;
    float yInc = dy / (float) steps;

    float x = x1;
    float y = y1;

    glColor3f(1.0, 1.0, 1.0); // White line (DDA)

    for (int i = 0; i <= steps; i++) {
        plotPoint(round(x), round(y));
        x += xInc;
        y += yInc;
    }
}

// Bresenham Algorithm
void drawLineBresenham(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);

    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;

    int err = dx - dy;

    glColor3f(1.0, 1.0, 0.0); // Yellow line (Bresenham)

    while (true) {
        plotPoint(x1, y1);

        if (x1 == x2 && y1 == y2)
            break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw using DDA
    drawLineDDA(x1_in, y1_in, x2_in, y2_in);

    // Draw using Bresenham
    drawLineBresenham(x1_in, y1_in, x2_in, y2_in);

    glFlush();
}

void init() {
    glClearColor(0.0, 0.0, 0.0, 1.0); // Black background
    glColor3f(1.0, 1.0, 1.0);
    gluOrtho2D(0, 500, 0, 500); // 2D projection (coordinate system)
}

int main(int argc, char** argv) {
    cout << "Enter x1 y1: ";
    cin >> x1_in >> y1_in;
    cout << "Enter x2 y2: ";
    cin >> x2_in >> y2_in;

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("DDA vs Bresenham Line Drawing");

    init();
    glutDisplayFunc(display);
    glutMainLoop();

    return 0;
}
