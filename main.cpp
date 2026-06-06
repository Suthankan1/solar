/**
 * main.cpp
 * 3D Solar System Project - Member 1 Deliverables
 * Sets up basic OpenGL window, perspective viewing, coordinate axes with labels,
 * spatial XZ grid, polished star background, and a static Sun placeholder.
 */

#ifdef __APPLE__
/* Silence OpenGL deprecation warnings on macOS */
#define GL_SILENCE_DEPRECATION
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cmath>

// Pi constant
const double PI = 3.14159265358979323846;

// Star structure for background
struct Star {
    float x;          // Normalized X coordinate [-1, 1]
    float y;          // Normalized Y coordinate [-1, 1]
    float brightness; // Brightness/color intensity factor [0.4, 1.0]
};

const int NUM_STARS = 500;
Star stars[NUM_STARS];
double g_aspectRatio = 1.3333; // Default 800/600 aspect ratio

// Initialize OpenGL state
void init() {
    // Set clear color to black (space background)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Enable depth testing (for visible surface detection)
    glEnable(GL_DEPTH_TEST);
    
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Generate random star coordinates and brightness levels
    for (int i = 0; i < NUM_STARS; ++i) {
        stars[i].x = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].y = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].brightness = 0.4f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 0.6f);
    }
}

// Draw a 3D coordinate axis system at the origin
// Red represents the X-axis, Green represents the Y-axis, Blue represents the Z-axis
void drawAxes() {
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    
    // X Axis: Red, 10 units long
    glColor3f(1.0f, 0.2f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    
    // Y Axis: Green, 10 units long
    glColor3f(0.2f, 1.0f, 0.2f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    
    // Z Axis: Blue, 10 units long
    glColor3f(0.2f, 0.2f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    
    glEnd();
    glLineWidth(1.0f);
    
    // Axis text labels in 3D space
    glColor3f(1.0f, 1.0f, 1.0f); // White text
    
    // X Label
    glRasterPos3f(10.5f, 0.0f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
    
    // Y Label
    glRasterPos3f(0.0f, 10.5f, 0.0f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
    
    // Z Label
    glRasterPos3f(0.0f, 0.0f, 10.5f);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Z');
}

// Draw a spatial grid on the XZ plane representing the space coordinate coordinate system
void drawSpaceGrid(float size, float step) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Faint neon blue lines for a high-tech "fabric of space" grid
    glColor4f(0.0f, 0.5f, 1.0f, 0.2f);
    glLineWidth(1.0f);
    
    glBegin(GL_LINES);
    // Draw lines parallel to Z axis
    for (float x = -size; x <= size; x += step) {
        glVertex3f(x, 0.0f, -size);
        glVertex3f(x, 0.0f, size);
    }
    // Draw lines parallel to X axis
    for (float z = -size; z <= size; z += step) {
        glVertex3f(-size, 0.0f, z);
        glVertex3f(size, 0.0f, z);
    }
    glEnd();
    
    glDisable(GL_BLEND);
}

// Display/Render callback
void display() {
    // Clear both the color buffer and the depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 1. Draw Star Background (Identity Modelview, Depth Testing Disabled)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_POINT_SMOOTH); // Makes star points circular and smooth
    
    // Scale star placement depth to fit projection dimensions
    float zDepth = -95.0f;
    float halfHeight = -zDepth * std::tan(45.0f * static_cast<float>(PI) / 360.0f);
    float halfWidth = halfHeight * static_cast<float>(g_aspectRatio);
    
    // Draw stars in size batches to optimize performance and create depth layers
    
    // Batch 1: Small stars (distant)
    glPointSize(1.0f);
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; i += 3) {
        float b = stars[i].brightness;
        glColor4f(b * 0.9f, b * 0.9f, b, b); // Slightly bluish star tint
        glVertex3f(stars[i].x * halfWidth, stars[i].y * halfHeight, zDepth);
    }
    glEnd();
    
    // Batch 2: Medium stars
    glPointSize(1.8f);
    glBegin(GL_POINTS);
    for (int i = 1; i < NUM_STARS; i += 3) {
        float b = stars[i].brightness;
        glColor4f(b, b, b * 0.8f, b); // Slightly yellowish/warmer tint
        glVertex3f(stars[i].x * halfWidth, stars[i].y * halfHeight, zDepth);
    }
    glEnd();
    
    // Batch 3: Large stars (nearby)
    glPointSize(2.5f);
    glBegin(GL_POINTS);
    for (int i = 2; i < NUM_STARS; i += 3) {
        float b = stars[i].brightness;
        glColor4f(b * 0.8f, b * 0.85f, b, b); // Cooler white/blue tint
        glVertex3f(stars[i].x * halfWidth, stars[i].y * halfHeight, zDepth);
    }
    glEnd();
    
    glDisable(GL_BLEND);
    glDisable(GL_POINT_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // 2. Set static camera position & orientation
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // Position camera above and back to see grid and axes in 3D depth perspective
    gluLookAt(0.0, 6.0, 18.0, 
              0.0, 0.0, 0.0, 
              0.0, 1.0, 0.0);
    
    // 3. Draw coordinate elements
    drawSpaceGrid(15.0f, 1.5f); // 30x30 spatial grid
    drawAxes();                 // Red, Green, Blue axes with 3D labels
    
    // 4. Draw Sun Placeholder
    // Core orange solid sphere wrapped in a yellow mesh to look like a high-tech glowing body
    glPushMatrix();
        // Solid Orange Core
        glColor3f(1.0f, 0.5f, 0.0f);
        glutSolidSphere(1.98, 24, 24); // Size slightly smaller than wire shell to prevent z-fighting
        
        // Yellow Wireframe Shell
        glColor3f(1.0f, 0.85f, 0.0f);
        glutWireSphere(2.0, 24, 24);
    glPopMatrix();
    
    // Swap buffer to show completed drawing
    glutSwapBuffers();
}

// Reshape/Resize callback
void reshape(int width, int height) {
    if (height == 0) height = 1;
    
    glViewport(0, 0, width, height);
    
    g_aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Set perspective projection camera parameters
    gluPerspective(45.0, g_aspectRatio, 0.1, 100.0);
}

// Keyboard input callback
void keyboard(unsigned char key, int x, int y) {
    (void)x; // Unused
    (void)y; // Unused
    
    if (key == 27) { // ESC key
        std::cout << "Exiting Solar System. Goodbye!" << std::endl;
        exit(0);
    }
}

int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    
    // Double buffering, RGB color model, and Depth buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // Window configuration
    glutInitWindowSize(800, 600);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D Solar System - Member 1 Deliverables");
    
    // Custom initializations
    init();
    
    // Register event callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    
    // Print project setup confirmations
    std::cout << "OpenGL Window Initialized Successfully." << std::endl;
    std::cout << "Member 1 Workload Active: Labeled Coordinate Axes, Spatial Grid, Stars, static Sun." << std::endl;
    std::cout << "Press [Esc] to exit program." << std::endl;
    
    // Start GLUT main loop
    glutMainLoop();
    
    return 0;
}
