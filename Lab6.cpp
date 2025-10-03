// circle_compare.cpp
// Compile: g++ circle_compare.cpp -o circle_compare -lGL -lGLU -lglut -std=c++17
// Run: ./circle_compare
//
// Press m -> toggle Midpoint
//       b -> toggle Bresenham
//       a -> show both
//       q / Esc -> quit

#include <GLUT/glut.h>
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>


using namespace std;
using namespace std::chrono;

int windowWidth = 800;
int windowHeight = 800;

int centerX = 400;
int centerY = 400;
int radiusR = 100;

bool showMidpoint = true;
bool showBresenham = true;

// For reporting
long long midpointPixels = 0;
long long bresenhamPixels = 0;
double midpointAvgMs = 0.0;
double bresenhamAvgMs = 0.0;

// Repeat count for timing (increase for more stable measurement)
const int TIMING_REPEATS = 200;

// Utility: plot point (pixel) in integer coords
inline void plotPoint(int x, int y) {
    glVertex2i(x, y);
}

// Plot 8-way symmetric points for circle center (xc,yc)
void plot8_symmetry(int xc, int yc, int x, int y) {
    // Each call should plot up to 8 distinct points (some may coincide if x==y or x==0 or y==0)
    plotPoint(xc + x, yc + y);
    plotPoint(xc - x, yc + y);
    plotPoint(xc + x, yc - y);
    plotPoint(xc - x, yc - y);
    plotPoint(xc + y, yc + x);
    plotPoint(xc - y, yc + x);
    plotPoint(xc + y, yc - x);
    plotPoint(xc - y, yc - x);
}

// Integer Midpoint Circle Algorithm (typical version)
long long midpointCircleDrawCount(int xc, int yc, int r, bool actuallyPlot) {
    int x = 0;
    int y = r;
    int d = 1 - r;
    long long count = 0;
    if (actuallyPlot) {
        glBegin(GL_POINTS);
        plot8_symmetry(xc, yc, x, y);
        glEnd();
    } else {
        // just counting - initial 8
        count += 8;
    }

    if (actuallyPlot) {
        glBegin(GL_POINTS);
    } else {
        // Nothing
    }

    while (x < y) {
        x++;
        if (d < 0) {
            d += 2 * x + 1;
        } else {
            y--;
            d += 2 * (x - y) + 1;
        }
        if (actuallyPlot) {
            plot8_symmetry(xc, yc, x, y);
        } else {
            // careful: when x==y or x==0 etc, duplicates may happen, but counting here approximates 8 per step.
            // We'll count exact number of distinct points by checking special cases:
            if (x == 0 || x == y) {
                count += 4; // degenerate symmetry
            } else {
                count += 8;
            }
        }
    }

    if (actuallyPlot) glEnd();
    // If counting branch started from initial 8 then added others, return count. To avoid double-counting initial,
    // we will compute precise count below instead of using the above approximations when not plotting.
    if (actuallyPlot) {
        return 0; // caller won't use this; we maintain global pixel counter separately when plotting
    } else {
        // More accurate distinct point count calculation:
        // We can simulate without calling gl functions to get exact count (simple integer simulation).
        int cx = 0, cy = r;
        int dec = 1 - r;
        long long exactCount = 0;
        // first point
        // handle the initial 8
        auto addSymCount = [&](int xx, int yy) {
            // compute distinct symmetric points count
            // if xx==0 && yy==0 -> 1
            // if xx==0 || yy==0 -> 4
            // if xx==yy -> 4 (since some duplicates between swapped)
            // else -> 8
            if (xx == 0 && yy == 0) return 1;
            if (xx == 0 || yy == 0) return 4;
            if (xx == yy) return 4;
            return 8;
        };
        exactCount += addSymCount(cx, cy);
        while (cx < cy) {
            cx++;
            if (dec < 0) {
                dec += 2 * cx + 1;
            } else {
                cy--;
                dec += 2 * (cx - cy) + 1;
            }
            exactCount += addSymCount(cx, cy);
        }
        return exactCount;
    }
}

// Bresenham's Circle Drawing (integer Bresenham variant)
// Implementation uses integer-only updates similar to mid-point; differences in increment structure.
long long bresenhamCircleDrawCount(int xc, int yc, int r, bool actuallyPlot) {
    int x = 0;
    int y = r;
    int d = 3 - 2 * r; // decision parameter initialization for Bresenham variant
    if (actuallyPlot) {
        glBegin(GL_POINTS);
        plot8_symmetry(xc, yc, x, y);
    }
    long long count = 0;
    if (!actuallyPlot) {
        // use exact counting like above
        auto addSymCount = [&](int xx, int yy) {
            if (xx == 0 && yy == 0) return 1;
            if (xx == 0 || yy == 0) return 4;
            if (xx == yy) return 4;
            return 8;
        };
        count += addSymCount(x, y);
        while (x < y) {
            x++;
            if (d <= 0) {
                d += 4 * x + 6;
            } else {
                y--;
                d += 4 * (x - y) + 10;
            }
            count += addSymCount(x, y);
        }
        return count;
    } else {
        while (x < y) {
            x++;
            if (d <= 0) {
                d += 4 * x + 6;
            } else {
                y--;
                d += 4 * (x - y) + 10;
            }
            plot8_symmetry(xc, yc, x, y);
        }
        glEnd();
        return 0;
    }
}

// Wrapper to draw midpoint algorithm and compute pixel count & timing
void drawMidpointAndMeasure() {
    // Measure exact count without plotting (fast)
    midpointPixels = midpointCircleDrawCount(centerX, centerY, radiusR, false);

    // Measure average time by repeating many times (but plot only once for visual)
    auto start = high_resolution_clock::now();
    for (int i = 0; i < TIMING_REPEATS; ++i) {
        // we will not call GL plotting inside timed loop because GL operations vary heavily.
        // Instead we measure algorithmic operations by a simulated run (no GL calls).
        midpointCircleDrawCount(centerX, centerY, radiusR, false);
    }
    auto end = high_resolution_clock::now();
    midpointAvgMs = duration_cast<duration<double, milli>>(end - start).count() / TIMING_REPEATS;

    // Finally plot once (if requested) inside display
}

// Wrapper to draw Bresenham algorithm and compute pixel count & timing
void drawBresenhamAndMeasure() {
    bresenhamPixels = bresenhamCircleDrawCount(centerX, centerY, radiusR, false);

    auto start = high_resolution_clock::now();
    for (int i = 0; i < TIMING_REPEATS; ++i) {
        bresenhamCircleDrawCount(centerX, centerY, radiusR, false);
    }
    auto end = high_resolution_clock::now();
    bresenhamAvgMs = duration_cast<duration<double, milli>>(end - start).count() / TIMING_REPEATS;
}

// Display callback
void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw axes for reference
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_LINES);
        glVertex2i(0, centerY);
        glVertex2i(windowWidth, centerY);
        glVertex2i(centerX, 0);
        glVertex2i(centerX, windowHeight);
    glEnd();

    // Midpoint circle
    if (showMidpoint) {
        glColor3f(1.0f, 0.0f, 0.0f); // red
        // Plot with GL_POINTS and count pixels by incrementing a local counter
        midpointPixels = 0;
        int x = 0, y = radiusR;
        int d = 1 - radiusR;
        glBegin(GL_POINTS);
        plot8_symmetry(centerX, centerY, x, y);
        midpointPixels += 8;
        while (x < y) {
            x++;
            if (d < 0) {
                d += 2 * x + 1;
            } else {
                y--;
                d += 2 * (x - y) + 1;
            }
            // Plot and update count precisely:
            auto addSymCount = [&](int xx, int yy) {
                if (xx == 0 && yy == 0) return 1;
                if (xx == 0 || yy == 0) return 4;
                if (xx == yy) return 4;
                return 8;
            };
            plot8_symmetry(centerX, centerY, x, y);
            midpointPixels += addSymCount(x, y);
        }
        glEnd();
    }

    // Bresenham circle
    if (showBresenham) {
        glColor3f(0.0f, 0.0f, 1.0f); // blue
        bresenhamPixels = 0;
        int x = 0, y = radiusR;
        int d = 3 - 2 * radiusR;
        glBegin(GL_POINTS);
        plot8_symmetry(centerX, centerY, x, y);
        bresenhamPixels += 8;
        while (x < y) {
            x++;
            if (d <= 0) {
                d += 4 * x + 6;
            } else {
                y--;
                d += 4 * (x - y) + 10;
            }
            auto addSymCount = [&](int xx, int yy) {
                if (xx == 0 && yy == 0) return 1;
                if (xx == 0 || yy == 0) return 4;
                if (xx == yy) return 4;
                return 8;
            };
            plot8_symmetry(centerX, centerY, x, y);
            bresenhamPixels += addSymCount(x, y);
        }
        glEnd();
    }

    // Measure timings (algorithmic simulation)
    drawMidpointAndMeasure();      // sets midpointAvgMs & midpointPixels (approx)
    drawBresenhamAndMeasure();     // sets bresenhamAvgMs & bresenhamPixels (approx)

    // Show results text (very basic) - using bitmap string at top-left
    glColor3f(0.0f, 0.0f, 0.0f);
    auto drawText = [&](int x, int y, const string &s) {
        glRasterPos2i(x, y);
        for (char c : s) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    };

    int yline = windowHeight - 20;
    drawText(10, yline, "Red = Midpoint | Blue = Bresenham");
    yline -= 16;

    // Display the measured values. Note the plotted counts were computed while plotting, timings are from simulation.
    stringstream ss;
    ss << "Midpoint: plotted pixels (approx) = " << midpointPixels << " | avg sim time = " << midpointAvgMs << " ms";
    drawText(10, yline, ss.str());
    yline -= 16;
    ss.str(""); ss.clear();
    ss << "Bresenham: plotted pixels (approx) = " << bresenhamPixels << " | avg sim time = " << bresenhamAvgMs << " ms";
    drawText(10, yline, ss.str());
    yline -= 20;

    ss.str(""); ss.clear();
    ss << "Center: (" << centerX << "," << centerY << ")  Radius: " << radiusR;
    drawText(10, yline, ss.str());
    yline -= 16;
    drawText(10, yline, "Keys: m toggle midpoint | b toggle bresenham | a show both | q/Esc quit");

    glFlush();
    glutSwapBuffers();
}

// Window reshape
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    // Setup orthographic projection with origin at bottom-left and units = pixels
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

// Keyboard input
void keyboard(unsigned char key, int x, int y) {
    switch (key) {
        case 'm':
        case 'M':
            showMidpoint = !showMidpoint;
            glutPostRedisplay();
            break;
        case 'b':
        case 'B':
            showBresenham = !showBresenham;
            glutPostRedisplay();
            break;
        case 'a':
        case 'A':
            showMidpoint = showBresenham = true;
            glutPostRedisplay();
            break;
        case 27: // Esc
        case 'q':
        case 'Q':
            exit(0);
            break;
    }
}

// Main
int main(int argc, char** argv) {
    cout << "Midpoint vs Bresenham Circle Drawing (OpenGL + GLUT)" << endl;
    cout << "Enter center X (pixel, 0.."<<windowWidth<<") [default " << centerX << "]: ";
    string tmp;
    getline(cin, tmp);
    if (!tmp.empty()) centerX = stoi(tmp);

    cout << "Enter center Y (pixel, 0.."<<windowHeight<<") [default " << centerY << "]: ";
    getline(cin, tmp);
    if (!tmp.empty()) centerY = stoi(tmp);

    cout << "Enter radius (pixels) [default " << radiusR << "]: ";
    getline(cin, tmp);
    if (!tmp.empty()) radiusR = stoi(tmp);

    if (radiusR < 0) radiusR = 0;
    if (radiusR > min(windowWidth, windowHeight)/2) {
        cerr << "Radius too big for default window; adjusting to fit." << endl;
        radiusR = min(windowWidth, windowHeight)/2 - 10;
    }

    // Initialize GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(windowWidth, windowHeight);
    glutCreateWindow("Midpoint vs Bresenham Circle Drawing");

    // background white
    glClearColor(1.0, 1.0, 1.0, 1.0);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

    cout << "Controls: m toggle midpoint | b toggle bresenham | a show both | q/Esc quit" << endl;
    cout << "Window will open now..." << endl;

    glutMainLoop();
    return 0;
}
