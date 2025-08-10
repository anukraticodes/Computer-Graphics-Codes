#include <stdio.h>
#include <math.h>
#include <graphics.h>
#include <conio.h>

// Function prototypes
void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color);
void translation(int x1, int y1, int x2, int y2, int x3, int y3, int tx, int ty);
void scaling(int x1, int y1, int x2, int y2, int x3, int y3, float sx, float sy);
void rotation(int x1, int y1, int x2, int y2, int x3, int y3, float angle);
void reflection(int x1, int y1, int x2, int y2, int x3, int y3, int axis);
void shearing(int x1, int y1, int x2, int y2, int x3, int y3, float shx, float shy);
void displayMenu();

int main() {
    int gd = DETECT, gm;
    int x1 = 100, y1 = 100, x2 = 200, y2 = 100, x3 = 150, y3 = 50;
    int choice, tx, ty, axis;
    float sx, sy, angle, shx, shy;
    
    // Initialize graphics mode
    initgraph(&gd, &gm, "C:\\TURBOC3\\BGI");
    
    while(1) {
        cleardevice();
        displayMenu();
        
        // Draw original triangle
        setcolor(WHITE);
        outtextxy(10, 10, "Original Triangle (White)");
        drawTriangle(x1, y1, x2, y2, x3, y3, WHITE);
        
        printf("Enter your choice (1-7): ");
        scanf("%d", &choice);
        
        switch(choice) {
            case 1: // Translation
                printf("Enter translation values (tx, ty): ");
                scanf("%d %d", &tx, &ty);
                translation(x1, y1, x2, y2, x3, y3, tx, ty);
                break;
                
            case 2: // Scaling
                printf("Enter scaling factors (sx, sy): ");
                scanf("%f %f", &sx, &sy);
                scaling(x1, y1, x2, y2, x3, y3, sx, sy);
                break;
                
            case 3: // Rotation
                printf("Enter rotation angle in degrees: ");
                scanf("%f", &angle);
                rotation(x1, y1, x2, y2, x3, y3, angle);
                break;
                
            case 4: // Reflection
                printf("Enter reflection axis (1-X axis, 2-Y axis, 3-Origin): ");
                scanf("%d", &axis);
                reflection(x1, y1, x2, y2, x3, y3, axis);
                break;
                
            case 5: // Shearing
                printf("Enter shearing factors (shx, shy): ");
                scanf("%f %f", &shx, &shy);
                shearing(x1, y1, x2, y2, x3, y3, shx, shy);
                break;
                
            case 6: // Reset triangle
                x1 = 100; y1 = 100; x2 = 200; y2 = 100; x3 = 150; y3 = 50;
                printf("Triangle reset to original position.\n");
                break;
                
            case 7: // Exit
                closegraph();
                return 0;
                
            default:
                printf("Invalid choice! Please try again.\n");
        }
        
        printf("Press any key to continue...");
        getch();
    }
}

void drawTriangle(int x1, int y1, int x2, int y2, int x3, int y3, int color) {
    setcolor(color);
    line(x1, y1, x2, y2);
    line(x2, y2, x3, y3);
    line(x3, y3, x1, y1);
    
    // Draw vertices as small circles
    circle(x1, y1, 2);
    circle(x2, y2, 2);
    circle(x3, y3, 2);
}

void translation(int x1, int y1, int x2, int y2, int x3, int y3, int tx, int ty) {
    int nx1, ny1, nx2, ny2, nx3, ny3;
    
    // Apply translation matrix
    nx1 = x1 + tx;
    ny1 = y1 + ty;
    nx2 = x2 + tx;
    ny2 = y2 + ty;
    nx3 = x3 + tx;
    ny3 = y3 + ty;
    
    setcolor(RED);
    outtextxy(10, 30, "Translated Triangle (Red)");
    drawTriangle(nx1, ny1, nx2, ny2, nx3, ny3, RED);
    
    printf("Translation completed: T(%d, %d)\n", tx, ty);
}

void scaling(int x1, int y1, int x2, int y2, int x3, int y3, float sx, float sy) {
    int nx1, ny1, nx2, ny2, nx3, ny3;
    
    // Apply scaling matrix (with respect to origin)
    nx1 = (int)(x1 * sx);
    ny1 = (int)(y1 * sy);
    nx2 = (int)(x2 * sx);
    ny2 = (int)(y2 * sy);
    nx3 = (int)(x3 * sx);
    ny3 = (int)(y3 * sy);
    
    setcolor(GREEN);
    outtextxy(10, 50, "Scaled Triangle (Green)");
    drawTriangle(nx1, ny1, nx2, ny2, nx3, ny3, GREEN);
    
    printf("Scaling completed: S(%.2f, %.2f)\n", sx, sy);
}

void rotation(int x1, int y1, int x2, int y2, int x3, int y3, float angle) {
    int nx1, ny1, nx2, ny2, nx3, ny3;
    float rad = angle * M_PI / 180.0; // Convert to radians
    float cosA = cos(rad);
    float sinA = sin(rad);
    
    // Apply rotation matrix (about origin)
    nx1 = (int)(x1 * cosA - y1 * sinA);
    ny1 = (int)(x1 * sinA + y1 * cosA);
    nx2 = (int)(x2 * cosA - y2 * sinA);
    ny2 = (int)(x2 * sinA + y2 * cosA);
    nx3 = (int)(x3 * cosA - y3 * sinA);
    ny3 = (int)(x3 * sinA + y3 * cosA);
    
    setcolor(BLUE);
    outtextxy(10, 70, "Rotated Triangle (Blue)");
    drawTriangle(nx1, ny1, nx2, ny2, nx3, ny3, BLUE);
    
    printf("Rotation completed: R(%.2f degrees)\n", angle);
}

void reflection(int x1, int y1, int x2, int y2, int x3, int y3, int axis) {
    int nx1, ny1, nx2, ny2, nx3, ny3;
    
    switch(axis) {
        case 1: // Reflection about X-axis
            nx1 = x1; ny1 = -y1;
            nx2 = x2; ny2 = -y2;
            nx3 = x3; ny3 = -y3;
            outtextxy(10, 90, "Reflected about X-axis (Yellow)");
            printf("Reflection about X-axis completed\n");
            break;
            
        case 2: // Reflection about Y-axis
            nx1 = -x1; ny1 = y1;
            nx2 = -x2; ny2 = y2;
            nx3 = -x3; ny3 = y3;
            outtextxy(10, 90, "Reflected about Y-axis (Yellow)");
            printf("Reflection about Y-axis completed\n");
            break;
            
        case 3: // Reflection about origin
            nx1 = -x1; ny1 = -y1;
            nx2 = -x2; ny2 = -y2;
            nx3 = -x3; ny3 = -y3;
            outtextxy(10, 90, "Reflected about Origin (Yellow)");
            printf("Reflection about origin completed\n");
            break;
            
        default:
            printf("Invalid reflection axis!\n");
            return;
    }
    
    setcolor(YELLOW);
    drawTriangle(nx1, ny1, nx2, ny2, nx3, ny3, YELLOW);
}

void shearing(int x1, int y1, int x2, int y2, int x3, int y3, float shx, float shy) {
    int nx1, ny1, nx2, ny2, nx3, ny3;
    
    // Apply shearing matrix
    nx1 = (int)(x1 + shx * y1);
    ny1 = (int)(y1 + shy * x1);
    nx2 = (int)(x2 + shx * y2);
    ny2 = (int)(y2 + shy * x2);
    nx3 = (int)(x3 + shx * y3);
    ny3 = (int)(y3 + shy * x3);
    
    setcolor(MAGENTA);
    outtextxy(10, 110, "Sheared Triangle (Magenta)");
    drawTriangle(nx1, ny1, nx2, ny2, nx3, ny3, MAGENTA);
    
    printf("Shearing completed: Sh(%.2f, %.2f)\n", shx, shy);
}

void displayMenu() {
    printf("\n========== 2D TRANSFORMATIONS MENU ==========\n");
    printf("1. Translation\n");
    printf("2. Scaling\n");
    printf("3. Rotation\n");
    printf("4. Reflection\n");
    printf("5. Shearing\n");
    printf("6. Reset Triangle\n");
    printf("7. Exit\n");
    printf("============================================\n");
}
