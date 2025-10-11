// polygon_fill.cpp
// Compile: g++ polygon_fill.cpp -o polygon_fill -lGL -lGLU -lglut

#include <GL/glut.h>
#include <vector>
#include <algorithm>
#include <stack>
#include <iostream>
#include <cmath>

using namespace std;

struct Point { int x, y; };
struct Color { unsigned char r, g, b; bool operator==(const Color &o) const { return r==o.r && g==o.g && b==o.b; } };

int winWidth = 800, winHeight = 600;

vector<Point> polygonPts;
bool polygonFinished = false;

enum Mode { IDLE, WAIT_SEED_FLOOD4, WAIT_SEED_FLOOD8, WAIT_SEED_BOUNDARY };
Mode currentMode = IDLE;

// Colors
Color backgroundColor = {255,255,255};
Color polygonColor = {0,0,0};      // boundary/outline color (black)
Color fillColor = {255,0,0};       // red fill

// Utility: convert GLUT mouse y to our coordinate system (origin bottom-left)
inline int convY(int y) { return winHeight - 1 - y; }

// Set a pixel (draw point)
void setPixel(int x, int y, const Color &c) {
    if(x<0 || x>=winWidth || y<0 || y>=winHeight) return;
    glBegin(GL_POINTS);
      glColor3ub(c.r, c.g, c.b);
      glVertex2i(x, y);
    glEnd();
}

// Get pixel color from framebuffer
Color getPixel(int x, int y) {
    Color c;
    if(x<0 || x>=winWidth || y<0 || y>=winHeight) return c;
    unsigned char pixel[3];
    glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    c.r = pixel[0]; c.g = pixel[1]; c.b = pixel[2];
    return c;
}

// Draw current polygon (outline and points)
void drawPolygonOutline() {
    // draw vertices as small points
    glPointSize(5.0f);
    glBegin(GL_POINTS);
      glColor3ub(0,0,255); // blue vertices
      for(auto &p: polygonPts) glVertex2i(p.x, p.y);
    glEnd();
    glPointSize(1.0f);

    if(polygonPts.size() >= 2) {
        glColor3ub(polygonColor.r, polygonColor.g, polygonColor.b);
        glBegin(GL_LINE_LOOP);
        for(auto &p: polygonPts) glVertex2i(p.x, p.y);
        if(!polygonFinished) glutSwapBuffers(); // no loop if not finished (helps see preview)
        glEnd();
    }
}

// Clear window to backgroundColor
void clearWindow() {
    glClearColor(backgroundColor.r/255.0f, backgroundColor.g/255.0f, backgroundColor.b/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glFlush();
}

// ---------- Scanline Fill Implementation ----------
struct EdgeEntry {
    int ymax;       // y coordinate where edge is no longer active
    float x;        // x at current scanline (float)
    float invSlope; // dx/dy
};

void scanlineFillPolygon() {
    if(polygonPts.size() < 3) return;

    // Find ymin and ymax
    int minY = polygonPts[0].y, maxY = polygonPts[0].y;
    for(auto &p: polygonPts) { minY = min(minY, p.y); maxY = max(maxY, p.y); }
    minY = max(minY, 0); maxY = min(maxY, winHeight-1);

    int height = winHeight;
    vector<vector<EdgeEntry>> ET(height); // Edge Table indexed by y

    int n = polygonPts.size();
    for(int i=0;i<n;i++) {
        Point p1 = polygonPts[i];
        Point p2 = polygonPts[(i+1)%n];
        // skip horizontal edges
        if(p1.y == p2.y) continue;
        // ensure p1.y < p2.y
        if(p1.y > p2.y) swap(p1, p2);

        EdgeEntry e;
        e.ymax = p2.y;
        e.x = p1.x;
        e.invSlope = (float)(p2.x - p1.x) / (float)(p2.y - p1.y);
        int yIndex = p1.y;
        if(yIndex < 0) yIndex = 0;
        if(yIndex >= height) continue;
        ET[yIndex].push_back(e);
    }

    vector<EdgeEntry> AET; // Active Edge Table

    // For each scanline from minY to maxY-1
    for(int y = minY; y <= maxY; ++y) {
        // 1) Add edges starting at this scanline
        if(y < height) {
            for(auto &e : ET[y]) AET.push_back(e);
        }

        // 2) Remove edges where ymax == y
        AET.erase(remove_if(AET.begin(), AET.end(),
                    [y](const EdgeEntry &e){ return e.ymax <= y; }), AET.end());

        // 3) Sort AET by x
        sort(AET.begin(), AET.end(), [](const EdgeEntry &a, const EdgeEntry &b){ return a.x < b.x; });

        // 4) Fill pixels between pairs
        for(size_t i=0; i+1 < AET.size(); i += 2) {
            int xStart = (int)ceil(AET[i].x);
            int xEnd   = (int)floor(AET[i+1].x);
            for(int x = xStart; x <= xEnd; ++x) {
                setPixel(x, y, fillColor);
            }
        }

        // 5) For each edge in AET, update x += invSlope
        for(auto &e : AET) e.x += e.invSlope;
    }

    glFlush();
}

// ---------- Flood Fill (iterative stack) ----------
void floodFillIterative(int seedX, int seedY, const Color &targetColor, bool eightConnected) {
    if(seedX<0||seedX>=winWidth||seedY<0||seedY>=winHeight) return;
    Color orig = getPixel(seedX, seedY);
    if(orig == targetColor) return; // nothing to do

    // If origin already equals target color, no-op
    if(orig == targetColor) return;

    // If the pixel is already the fillColor, nothing
    if(orig == targetColor) return;

    // Use stack
    stack<Point> st;
    st.push({seedX, seedY});

    while(!st.empty()) {
        Point p = st.top(); st.pop();
        Color cur = getPixel(p.x, p.y);
        if(!(cur == orig)) continue; // skip changed or boundary
        setPixel(p.x, p.y, targetColor);

        // Push neighbors
        // 4-connected
        if(p.x+1 < winWidth) st.push({p.x+1, p.y});
        if(p.x-1 >= 0) st.push({p.x-1, p.y});
        if(p.y+1 < winHeight) st.push({p.x, p.y+1});
        if(p.y-1 >= 0) st.push({p.x, p.y-1});
        if(eightConnected) {
            if(p.x+1 < winWidth && p.y+1 < winHeight) st.push({p.x+1, p.y+1});
            if(p.x-1 >= 0 && p.y+1 < winHeight) st.push({p.x-1, p.y+1});
            if(p.x+1 < winWidth && p.y-1 >= 0) st.push({p.x+1, p.y-1});
            if(p.x-1 >= 0 && p.y-1 >= 0) st.push({p.x-1, p.y-1});
        }
    }
    glFlush();
}

// ---------- Boundary Fill (iterative) ----------
void boundaryFillIterative(int seedX, int seedY, const Color &fillCol, const Color &boundaryCol) {
    if(seedX<0||seedX>=winWidth||seedY<0||seedY>=winHeight) return;
    Color cur = getPixel(seedX, seedY);
    if(cur == boundaryCol || cur == fillCol) return;

    stack<Point> st;
    st.push({seedX, seedY});

    while(!st.empty()) {
        Point p = st.top(); st.pop();
        Color c = getPixel(p.x, p.y);
        if(c == boundaryCol || c == fillCol) continue;
        setPixel(p.x, p.y, fillCol);

        // 4-connected neighbors (we'll use 8 if desired but classic boundary is 4)
        if(p.x+1 < winWidth) st.push({p.x+1, p.y});
        if(p.x-1 >= 0) st.push({p.x-1, p.y});
        if(p.y+1 < winHeight) st.push({p.x, p.y+1});
        if(p.y-1 >= 0) st.push({p.x, p.y-1});
    }
    glFlush();
}

// ---------- GLUT callbacks ----------
void display() {
    // Clear and redraw
    // Note: when we fill we have already drawn pixels into frame buffer.
    // We only redraw outline and vertices on top for clarity.
    glClear(GL_COLOR_BUFFER_BIT);
    // Redraw any existing filled pixels remain; OpenGL framebuffer keeps them
    // So we only redraw polygon outline for clarity:
    if(polygonPts.size() > 0) {
        glColor3ub(polygonColor.r, polygonColor.g, polygonColor.b);
        if(polygonFinished && polygonPts.size() >= 3) {
            glBegin(GL_LINE_LOOP);
            for(auto &p: polygonPts) glVertex2i(p.x, p.y);
            glEnd();
        } else if(!polygonFinished && polygonPts.size() >= 2) {
            glBegin(GL_LINE_STRIP);
            for(auto &p: polygonPts) glVertex2i(p.x, p.y);
            glEnd();
        }
    }
    // draw vertices
    glPointSize(5.0f);
    glBegin(GL_POINTS);
      glColor3ub(0,0,255);
      for(auto &p: polygonPts) glVertex2i(p.x, p.y);
    glEnd();
    glPointSize(1.0f);

    glFlush();
}

void reshape(int w, int h) {
    winWidth = w; winHeight = h;
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winWidth-1, 0, winHeight-1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    clearWindow();
    display();
}

// Mouse handler: Left click to add vertex (if polygon not finished), or used as seed after algorithm key
void mouse(int button, int state, int x, int y) {
    if(state != GLUT_DOWN) return;
    int cx = x;
    int cy = convY(y);

    if(!polygonFinished) {
        if(button == GLUT_LEFT_BUTTON) {
            polygonPts.push_back({cx, cy});
            // Draw vertex immediately
            glColor3ub(0,0,255);
            glBegin(GL_POINTS);
              glVertex2i(cx, cy);
            glEnd();
            glFlush();
        }
        return;
    }

    // If polygon finished, and waiting for seed for flood/boundary, use this click as seed
    if(currentMode == WAIT_SEED_FLOOD4) {
        cout << "Performing Flood Fill (4-connected) at seed (" << cx << ", " << cy << ")\n";
        floodFillIterative(cx, cy, fillColor, false);
        currentMode = IDLE;
    } else if(currentMode == WAIT_SEED_FLOOD8) {
        cout << "Performing Flood Fill (8-connected) at seed (" << cx << ", " << cy << ")\n";
        floodFillIterative(cx, cy, fillColor, true);
        currentMode = IDLE;
    } else if(currentMode == WAIT_SEED_BOUNDARY) {
        cout << "Performing Boundary Fill at seed (" << cx << ", " << cy << ")\n";
        boundaryFillIterative(cx, cy, fillColor, polygonColor);
        currentMode = IDLE;
    }
}

// Keyboard controls
void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case 'v': // finish polygon
        case 'V':
            if(polygonPts.size() >= 3) {
                polygonFinished = true;
                // draw outline
                glColor3ub(polygonColor.r, polygonColor.g, polygonColor.b);
                glBegin(GL_LINE_LOOP);
                  for(auto &p: polygonPts) glVertex2i(p.x, p.y);
                glEnd();
                glFlush();
                cout << "Polygon finished. Press:\n"
                     << "'s' => Scanline fill\n"
                     << "'f' => Flood fill (4-connected), then click seed inside polygon\n"
                     << "'g' => Flood fill (8-connected), then click seed\n"
                     << "'b' => Boundary fill, then click seed\n"
                     << "'r' => Reset polygon\n"
                     << "'c' => Clear window (keeps polygon outline)\n";
            } else {
                cout << "Need at least 3 vertices to finish polygon.\n";
            }
            break;

        case 's': // scanline
        case 'S':
            if(!polygonFinished) {
                cout << "Finish polygon first (press 'v').\n";
            } else {
                cout << "Running Scanline Fill...\n";
                scanlineFillPolygon();
            }
            break;

        case 'f': // flood 4-connected (need seed)
        case 'F':
            if(!polygonFinished) {
                cout << "Finish polygon first (press 'v').\n";
            } else {
                currentMode = WAIT_SEED_FLOOD4;
                cout << "Click inside polygon to choose seed point for Flood Fill (4-connected).\n";
            }
            break;

        case 'g': // flood 8-connected
        case 'G':
            if(!polygonFinished) {
                cout << "Finish polygon first (press 'v').\n";
            } else {
                currentMode = WAIT_SEED_FLOOD8;
                cout << "Click inside polygon to choose seed point for Flood Fill (8-connected).\n";
            }
            break;

        case 'b': // boundary fill
        case 'B':
            if(!polygonFinished) {
                cout << "Finish polygon first (press 'v').\n";
            } else {
                currentMode = WAIT_SEED_BOUNDARY;
                cout << "Click inside polygon to choose seed point for Boundary Fill.\n";
            }
            break;

        case 'r': // reset polygon
        case 'R':
            polygonPts.clear();
            polygonFinished = false;
            currentMode = IDLE;
            clearWindow();
            cout << "Polygon reset. Click to add new vertices.\n";
            break;

        case 'c': // clear window but keep polygon outline
        case 'C':
            // clear framebuffer
            clearWindow();
            // redraw polygon outline (if finished)
            if(polygonFinished) {
                glColor3ub(polygonColor.r, polygonColor.g, polygonColor.b);
                glBegin(GL_LINE_LOOP);
                  for(auto &p: polygonPts) glVertex2i(p.x, p.y);
                glEnd();
                // redraw vertices
                glPointSize(5.0f);
                glBegin(GL_POINTS);
                glColor3ub(0,0,255);
                for(auto &p: polygonPts) glVertex2i(p.x, p.y);
                glEnd();
                glPointSize(1.0f);
                glFlush();
            }
            cout << "Window cleared (polygon outline kept if finished).\n";
            break;

        case 27: // ESC
            exit(0);
            break;

        default:
            cout << "Unknown key. Controls:\n"
                 << "'v' finish polygon\n"
                 << "'s' scanline fill\n"
                 << "'f' flood fill 4-connected (then click seed)\n"
                 << "'g' flood fill 8-connected (then click seed)\n"
                 << "'b' boundary fill (then click seed)\n"
                 << "'r' reset polygon\n"
                 << "'c' clear window (keep outline)\n"
                 << "Esc to exit\n";
            break;
    }
    glutPostRedisplay();
}

// ---------- Initialization ----------
void initGL() {
    glClearColor(backgroundColor.r/255.0f, backgroundColor.g/255.0f, backgroundColor.b/255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, winWidth-1, 0, winHeight-1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPointSize(1.0f);
    glFlush();
}

// ---------- Main ----------
int main(int argc, char** argv) {
    cout << "Polygon Fill Demo (C++ / OpenGL GLUT)\n";
    cout << "Instructions:\n";
    cout << " - Left-click to add polygon vertices (while polygon not finished).\n";
    cout << " - Press 'v' to finish polygon (requires >=3 vertices).\n";
    cout << " - After finishing polygon:\n";
    cout << "     's' => Scanline Fill (fills immediately)\n";
    cout << "     'f' => Flood Fill (4-connected) — then click inside polygon to choose seed\n";
    cout << "     'g' => Flood Fill (8-connected) — then click inside polygon to choose seed\n";
    cout << "     'b' => Boundary Fill — then click inside polygon to choose seed\n";
    cout << " - 'r' => Reset and start a new polygon\n";
    cout << " - 'c' => Clear window (keeps outline if polygon finished)\n";
    cout << " - Esc => Exit\n";

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(winWidth, winHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("Polygon Fill Algorithms - Scanline / Flood / Boundary (GLUT)");

    initGL();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);

    glutMainLoop();
    return 0;
}
