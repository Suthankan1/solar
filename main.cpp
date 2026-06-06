/**
 * main.cpp
 * Base skeleton for a 3D Solar System simulation.
 * Sets up OpenGL window, GLUT event loop, camera projection, and depth testing.
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

// Star structure for background
struct Star {
    float x;          // Normalized X coordinate [-1, 1]
    float y;          // Normalized Y coordinate [-1, 1]
    float brightness; // Brightness/color intensity factor [0.5, 1.0]
};

const int NUM_STARS = 500;
Star stars[NUM_STARS];
double g_aspectRatio = 1.3333; // Default 800/600 aspect ratio


// Initialize OpenGL state
void init() {
    // Set the clear color to black (background color of space)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Enable depth testing. This is crucial for 3D rendering so that objects 
    // closer to the camera correctly block objects behind them.
    glEnable(GL_DEPTH_TEST);

    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Generate random star coordinates in the normalized range [-1.0, 1.0]
    // with randomized brightness levels.
    for (int i = 0; i < NUM_STARS; ++i) {
        stars[i].x = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].y = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].brightness = 0.5f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 0.5f);
    }
}

// Draw a 3D coordinate axis system at the current local origin.
// Red represents the X-axis, Green represents the Y-axis, Blue represents the Z-axis.
void drawAxes() {
    glBegin(GL_LINES);
    
    // X Axis: Red, 10 units long
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(10.0f, 0.0f, 0.0f);
    
    // Y Axis: Green, 10 units long
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 10.0f, 0.0f);
    
    // Z Axis: Blue, 10 units long
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 10.0f);
    
    glEnd();
}


// Display/Render callback
void display() {
    // Clear both the color buffer (the screen pixels) and the depth buffer (Z-buffer)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 1. Draw Star Background
    // Reset modelview to identity so camera transformations do not move the stars
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Temporarily disable depth testing/writing so stars are always drawn in the background
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    // Set star point size
    glPointSize(1.5f);
    
    // Place stars at a depth of -95.0 (near the far plane of 100.0)
    // Scale coordinates by perspective projection dimensions at this depth
    float zDepth = -95.0f;
    float halfHeight = -zDepth * std::tan(45.0f * 3.14159265f / 360.0f);
    float halfWidth = halfHeight * static_cast<float>(g_aspectRatio);
    
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; ++i) {
        float b = stars[i].brightness;
        // Introduce subtle color variation for astronomical realism
        if (i % 12 == 0) {
            glColor3f(b * 0.9f, b * 0.9f, b);     // Slightly bluish star
        } else if (i % 15 == 0) {
            glColor3f(b, b * 0.95f, b * 0.85f);  // Slightly yellowish/warmer star
        } else {
            glColor3f(b, b, b);                   // Pure white star
        }
        glVertex3f(stars[i].x * halfWidth, stars[i].y * halfHeight, zDepth);
    }
    glEnd();
    
    // Restore depth buffer state for 3D rendering
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // 2. Set the camera position and orientation using gluLookAt
    // Eye position: (0, 5, 15) - Above and back from the origin
    // LookAt point: (0, 0, 0) - Center of our solar system
    // Up vector:    (0, 1, 0) - Positive Y-axis is up
    gluLookAt(0.0, 5.0, 15.0, 
              0.0, 0.0, 0.0, 
              0.0, 1.0, 0.0);
    
    // 3. Draw a visible coordinate axis system (for development purposes)
    drawAxes();
    
    // 4. Draw a placeholder wireframe sphere at the origin representing the Sun
    // This allows us to visually verify that the 3D projection, depth test, and camera are working.
    glColor3f(1.0f, 0.8f, 0.0f); // Warm yellow/golden color for the Sun
    glutWireSphere(2.0, 24, 24); // Radius = 2.0, slices = 24, stacks = 24
    
    // Swap the back buffer (where we just drew) with the front buffer (what the user sees)
    // This prevents flickering during rendering.
    glutSwapBuffers();
}

// Reshape/Resize callback
void reshape(int width, int height) {
    // Avoid division by zero
    if (height == 0) height = 1;
    
    // Define the viewport mapping normalized coordinates to window pixels
    glViewport(0, 0, width, height);
    
    // Switch to Projection Matrix mode to define the viewing volume (camera lens parameters)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Set up a perspective projection
    // fieldOfView = 45 degrees
    // aspectRatio = width / height
    // nearVal     = 0.1 (nearest visible distance)
    // farVal      = 100.0 (farthest visible distance)
    g_aspectRatio = (double)width / (double)height;
    gluPerspective(45.0, g_aspectRatio, 0.1, 100.0);
}

int main(int argc, char** argv) {
    // Initialize the GLUT library
    glutInit(&argc, argv);
    
    // Set up display mode:
    // GLUT_DOUBLE - Enable double buffering to prevent screen flickering
    // GLUT_RGB    - Enable red, green, blue color model
    // GLUT_DEPTH  - Allocate a depth buffer for 3D depth testing
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // Set initial window size to 800x600 pixels
    glutInitWindowSize(800, 600);
    
    // Position the window on the user's screen
    glutInitWindowPosition(100, 100);
    
    // Create the window with a title
    glutCreateWindow("3D Solar System");
    
    // Initialize our OpenGL states (depth test, clear color)
    init();
    
    // Register event callback functions with GLUT
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    
    std::cout << "OpenGL Window Initialized Successfully." << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    
    // Start the GLUT main event loop. This function never returns.
    glutMainLoop();
    
    return 0;
}
