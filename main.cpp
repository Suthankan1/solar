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

// Initialize OpenGL state
void init() {
    // Set the clear color to black (background color of space)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Enable depth testing. This is crucial for 3D rendering so that objects 
    // closer to the camera correctly block objects behind them.
    glEnable(GL_DEPTH_TEST);
}

// Display/Render callback
void display() {
    // Clear both the color buffer (the screen pixels) and the depth buffer (Z-buffer)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Switch to Modelview Matrix mode to apply camera and object transformations
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Set the camera position and orientation using gluLookAt
    // Eye position: (0, 5, 15) - Above and back from the origin
    // LookAt point: (0, 0, 0) - Center of our solar system
    // Up vector:    (0, 1, 0) - Positive Y-axis is up
    gluLookAt(0.0, 5.0, 15.0, 
              0.0, 0.0, 0.0, 
              0.0, 1.0, 0.0);
    
    // Draw a placeholder wireframe sphere at the origin representing the Sun
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
    double aspectRatio = (double)width / (double)height;
    gluPerspective(45.0, aspectRatio, 0.1, 100.0);
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
