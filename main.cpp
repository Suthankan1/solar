/**
 * main.cpp
 * Enhanced 3D Solar System simulation.
 * Features lighting, hierarchical transformations (Sun, Earth, Moon, Mars),
 * interactive camera controls, preset views, and a 2D HUD UI overlay.
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
#include <string>
#include <sstream>
#include <iomanip>

// Pi constant definition
const double PI = 3.14159265358979323846;

// Star structure for background
struct Star {
    float x;          // Normalized X coordinate [-1, 1]
    float y;          // Normalized Y coordinate [-1, 1]
    float brightness; // Brightness/color intensity factor [0.5, 1.0]
};

const int NUM_STARS = 500;
Star stars[NUM_STARS];
double g_aspectRatio = 1.3333; // Default 800/600 aspect ratio
int g_windowWidth = 800;
int g_windowHeight = 600;

// Animation state variables
float earthOrbitAngle = 0.0f;
float earthRotAngle = 0.0f;
float moonOrbitAngle = 0.0f;
float marsOrbitAngle = 0.0f;
float marsRotAngle = 0.0f;

bool isPaused = false;
float speedMultiplier = 1.0f;

// Camera state variables
float cameraZoom = 22.0f;       // Zoom level (distance from origin)
float cameraAngleX = 25.0f;     // Horizontal angle (azimuth) around Y axis
float cameraAngleY = 15.0f;     // Vertical angle (elevation) from XZ plane
int viewMode = 0;              // 0: Custom (interactive), 1: Front, 2: Top, 3: Side

// Initialize OpenGL state
void init() {
    // Set the clear color to black (background color of space)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    // Enable smooth shading
    glShadeModel(GL_SMOOTH);

    // Setup Lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Configure light source (Sun) properties
    GLfloat lightAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat lightDiffuse[] = { 1.2f, 1.2f, 1.2f, 1.0f }; // Bright light
    GLfloat lightSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

    // Enable color tracking for material properties
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    // Set specular material properties (gives planets realistic shiny highlights)
    GLfloat specReflectance[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, specReflectance);
    glMateriali(GL_FRONT, GL_SHININESS, 25); // Exponent for shiny highlights

    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Generate random star coordinates
    for (int i = 0; i < NUM_STARS; ++i) {
        stars[i].x = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].y = -1.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 2.0f);
        stars[i].brightness = 0.5f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) / 0.5f);
    }
}

// Draw a 3D coordinate axis system at the current local origin.
// Red represents the X-axis, Green represents the Y-axis, Blue represents the Z-axis.
void drawAxes() {
    glDisable(GL_LIGHTING); // Axes shouldn't be shaded by lighting
    glLineWidth(2.0f);
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
    glLineWidth(1.0f);
    glEnable(GL_LIGHTING);
}

// Draw a circular orbit in the XZ plane with a transparent outline
void drawOrbit(float radius) {
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Transparent light-grey color for orbit path
    glColor4f(0.5f, 0.5f, 0.6f, 0.35f);
    glLineWidth(1.0f);
    
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; ++i) {
        float theta = 2.0f * static_cast<float>(PI) * static_cast<float>(i) / 100.0f;
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        glVertex3f(x, 0.0f, z);
    }
    glEnd();
    
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// 2D Text Drawing Helper
void drawText2D(float x, float y, const std::string& text, void* font, float r, float g, float b) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text) {
        glutBitmapCharacter(font, c);
    }
}

// Render HUD overlay with interactive options and information
void renderHUD(int width, int height) {
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    
    // Switch to orthographic projection for 2D drawing
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, width, height, 0); // (0,0) is top-left
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    // 1. Draw semi-transparent background panel for controls legend
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.04f, 0.07f, 0.15f, 0.75f); // Rich dark blue tint
    glBegin(GL_QUADS);
        glVertex2f(15.0f, 15.0f);
        glVertex2f(340.0f, 15.0f);
        glVertex2f(340.0f, 255.0f);
        glVertex2f(15.0f, 255.0f);
    glEnd();
    glDisable(GL_BLEND);
    
    // Draw neon blue border around the panel
    glColor3f(0.0f, 0.6f, 1.0f);
    glLineWidth(1.5f);
    glBegin(GL_LINE_LOOP);
        glVertex2f(15.0f, 15.0f);
        glVertex2f(340.0f, 15.0f);
        glVertex2f(340.0f, 255.0f);
        glVertex2f(15.0f, 255.0f);
    glEnd();
    glLineWidth(1.0f);
    
    // 2. Draw Text Information inside Panel
    drawText2D(25.0f, 38.0f, "3D SOLAR SYSTEM SIMULATION", GLUT_BITMAP_HELVETICA_18, 0.1f, 0.8f, 1.0f);
    
    // Status text (Running / Paused)
    std::string statusStr = isPaused ? "PAUSED" : "RUNNING";
    float sr = isPaused ? 1.0f : 0.2f;
    float sg = isPaused ? 0.3f : 1.0f;
    float sb = isPaused ? 0.3f : 0.2f;
    drawText2D(25.0f, 65.0f, "Status: ", GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);
    drawText2D(75.0f, 65.0f, statusStr, GLUT_BITMAP_HELVETICA_12, sr, sg, sb);
    
    // Simulation stats
    std::stringstream speedSS;
    speedSS << "Speed: " << std::fixed << std::setprecision(1) << speedMultiplier << "x";
    drawText2D(25.0f, 85.0f, speedSS.str(), GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);
    
    std::stringstream zoomSS;
    zoomSS << "Zoom: " << std::fixed << std::setprecision(1) << cameraZoom;
    drawText2D(140.0f, 85.0f, zoomSS.str(), GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);
    
    std::string viewStr = "Custom (Interactive)";
    if (viewMode == 1) viewStr = "Front View (XZ Plane)";
    else if (viewMode == 2) viewStr = "Top-Down View (XY Plane)";
    else if (viewMode == 3) viewStr = "Side View (YZ Plane)";
    drawText2D(25.0f, 105.0f, "View Mode: " + viewStr, GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);
    
    // Keyboard Control Legend
    drawText2D(25.0f, 135.0f, "KEYBOARD CONTROLS:", GLUT_BITMAP_HELVETICA_12, 1.0f, 0.75f, 0.0f);
    drawText2D(25.0f, 155.0f, "[W / S]      Zoom In / Out", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText2D(25.0f, 172.0f, "[A / D]      Rotate Camera Left / Right", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText2D(25.0f, 189.0f, "[Space]      Pause / Resume Animation", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText2D(25.0f, 206.0f, "[+ / -]      Increase / Decrease Speed", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText2D(25.0f, 223.0f, "[1 / 2 / 3]  Front / Top / Side View Presets", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    drawText2D(25.0f, 240.0f, "[Esc]        Exit Simulation", GLUT_BITMAP_HELVETICA_12, 0.9f, 0.9f, 0.9f);
    
    // Group Member Credits at the bottom right
    drawText2D(width - 240.0f, height - 35.0f, "OpenGL Solar System Project", GLUT_BITMAP_HELVETICA_12, 0.8f, 0.8f, 0.8f);
    drawText2D(width - 240.0f, height - 20.0f, "Member 1 (Leader) - Scene Design", GLUT_BITMAP_HELVETICA_12, 0.0f, 0.8f, 1.0f);
    
    // Restore original matrix modes
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
}

// Display/Render callback
void display() {
    // Clear both the color buffer (the screen pixels) and the depth buffer (Z-buffer)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 1. Draw Star Background (Identity Modelview, Depth Testing Disabled)
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    
    glPointSize(1.5f);
    float zDepth = -95.0f;
    float halfHeight = -zDepth * std::tan(45.0f * static_cast<float>(PI) / 360.0f);
    float halfWidth = halfHeight * static_cast<float>(g_aspectRatio);
    
    glBegin(GL_POINTS);
    for (int i = 0; i < NUM_STARS; ++i) {
        float b = stars[i].brightness;
        if (i % 12 == 0) {
            glColor3f(b * 0.9f, b * 0.9f, b);     // Slightly bluish star
        } else if (i % 15 == 0) {
            glColor3f(b, b * 0.95f, b * 0.85f);  // Slightly yellowish star
        } else {
            glColor3f(b, b, b);                   // Pure white star
        }
        glVertex3f(stars[i].x * halfWidth, stars[i].y * halfHeight, zDepth);
    }
    glEnd();
    
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    
    // 2. Setup Camera position using spherical coordinates
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    float radX = cameraAngleX * static_cast<float>(PI) / 180.0f;
    float radY = cameraAngleY * static_cast<float>(PI) / 180.0f;
    
    float eyeX = cameraZoom * std::cos(radY) * std::sin(radX);
    float eyeY = cameraZoom * std::sin(radY);
    float eyeZ = cameraZoom * std::cos(radY) * std::cos(radX);
    
    float upX = 0.0f;
    float upY = 1.0f;
    float upZ = 0.0f;
    
    // Handle camera up-vector switch at vertical limits to prevent gimbal lock
    if (cameraAngleY > 89.0f) {
        upY = 0.0f;
        upZ = -1.0f;
    } else if (cameraAngleY < -89.0f) {
        upY = 0.0f;
        upZ = 1.0f;
    }
    
    gluLookAt(eyeX, eyeY, eyeZ, 
              0.0, 0.0, 0.0, 
              upX, upY, upZ);
    
    // 3. Position the light at the center (Sun position)
    // Needs to be called *after* camera transform (gluLookAt) is applied
    GLfloat lightPos[] = { 0.0f, 0.0f, 0.0f, 1.0f }; // Point light source
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    
    // 4. Draw a visible coordinate axis system (Red=X, Green=Y, Blue=Z)
    drawAxes();
    
    // 5. Draw Sun (Center)
    // The Sun itself is the light source, so we disable shading for it
    glPushMatrix();
        glDisable(GL_LIGHTING);
        glColor3f(1.0f, 0.65f, 0.0f); // Bright yellow-orange glowing Sun
        glutSolidSphere(2.0, 32, 32);  // Radius = 2.0
        glEnable(GL_LIGHTING);
    glPopMatrix();
    
    // 6. Draw Orbit Paths
    drawOrbit(6.0f);  // Earth orbit
    drawOrbit(10.0f); // Mars orbit
    
    // 7. Draw Earth & Moon System
    glPushMatrix();
        // Orbit around the Sun
        glRotatef(earthOrbitAngle, 0.0f, 1.0f, 0.0f);
        // Translate to Earth's orbit radius
        glTranslatef(6.0f, 0.0f, 0.0f);
        
        // Render Earth
        glPushMatrix();
            // Tilt Earth by 23.5 degrees
            glRotatef(23.5f, 0.0f, 0.0f, 1.0f);
            // Spin Earth on its own axis
            glRotatef(earthRotAngle, 0.0f, 1.0f, 0.0f);
            // Earth Color: Vivid Blue-Green
            glColor3f(0.12f, 0.55f, 0.85f);
            glutSolidSphere(0.6, 24, 24); // Radius = 0.6
        glPopMatrix();
        
        // Render Moon (Nested coordinate space, orbiting Earth)
        glPushMatrix();
            // Orbit around Earth
            glRotatef(moonOrbitAngle, 0.0f, 1.0f, 0.0f);
            // Distance from Earth
            glTranslatef(1.2f, 0.0f, 0.0f);
            // Moon Color: Light Grey
            glColor3f(0.75f, 0.75f, 0.75f);
            glutSolidSphere(0.18, 16, 16); // Radius = 0.18
        glPopMatrix();
    glPopMatrix();
    
    // 8. Draw Mars
    glPushMatrix();
        // Orbit around the Sun
        glRotatef(marsOrbitAngle, 0.0f, 1.0f, 0.0f);
        // Translate to Mars's orbit radius
        glTranslatef(10.0f, 0.0f, 0.0f);
        
        // Render Mars
        glPushMatrix();
            // Mars axial tilt (25 degrees)
            glRotatef(25.0f, 0.0f, 0.0f, 1.0f);
            // Spin Mars on its axis
            glRotatef(marsRotAngle, 0.0f, 1.0f, 0.0f);
            // Mars Color: Rusty Red
            glColor3f(0.85f, 0.35f, 0.18f);
            glutSolidSphere(0.45, 24, 24); // Radius = 0.45
        glPopMatrix();
    glPopMatrix();
    
    // 9. Draw 2D HUD UI Overlay
    renderHUD(g_windowWidth, g_windowHeight);
    
    // Swap front and back buffer for smooth double-buffered rendering
    glutSwapBuffers();
}

// Reshape/Resize callback
void reshape(int width, int height) {
    if (height == 0) height = 1;
    
    g_windowWidth = width;
    g_windowHeight = height;
    g_aspectRatio = static_cast<double>(width) / static_cast<double>(height);
    
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Set perspective camera parameters
    gluPerspective(45.0, g_aspectRatio, 0.1, 100.0);
}

// Keyboard input callback
void keyboard(unsigned char key, int x, int y) {
    (void)x; // Unused
    (void)y; // Unused
    
    switch (key) {
        case 'w':
        case 'W':
            cameraZoom -= 0.6f;
            if (cameraZoom < 5.0f) cameraZoom = 5.0f; // Zoom clamp
            viewMode = 0;
            break;
            
        case 's':
        case 'S':
            cameraZoom += 0.6f;
            if (cameraZoom > 50.0f) cameraZoom = 50.0f;
            viewMode = 0;
            break;
            
        case 'a':
        case 'A':
            cameraAngleX -= 3.0f;
            if (cameraAngleX < 0.0f) cameraAngleX += 360.0f;
            viewMode = 0;
            break;
            
        case 'd':
        case 'D':
            cameraAngleX += 3.0f;
            if (cameraAngleX >= 360.0f) cameraAngleX -= 360.0f;
            viewMode = 0;
            break;
            
        case ' ':
            isPaused = !isPaused;
            break;
            
        case '+':
        case '=':
            speedMultiplier += 0.1f;
            if (speedMultiplier > 5.0f) speedMultiplier = 5.0f;
            break;
            
        case '-':
        case '_':
            speedMultiplier -= 0.1f;
            if (speedMultiplier < 0.1f) speedMultiplier = 0.1f;
            break;
            
        case '1': // Front View
            viewMode = 1;
            cameraZoom = 20.0f;
            cameraAngleX = 0.0f;
            cameraAngleY = 5.0f; // Small offset for 3D depth perception
            break;
            
        case '2': // Top-Down View
            viewMode = 2;
            cameraZoom = 25.0f;
            cameraAngleX = 0.0f;
            cameraAngleY = 89.9f; // Gimbal-lock avoidance angle
            break;
            
        case '3': // Side View
            viewMode = 3;
            cameraZoom = 20.0f;
            cameraAngleX = 90.0f;
            cameraAngleY = 5.0f; // Small offset for 3D depth perception
            break;
            
        case 27: // ESC key
            std::cout << "Exiting OpenGL Solar System. Goodbye!" << std::endl;
            exit(0);
            break;
            
        default:
            break;
    }
    glutPostRedisplay();
}

// 60 FPS animation timer callback
void timer(int value) {
    (void)value;
    
    if (!isPaused) {
        // Orbit speeds (slower as distance from Sun increases)
        earthOrbitAngle += 0.45f * speedMultiplier;
        marsOrbitAngle += 0.24f * speedMultiplier;
        moonOrbitAngle += 2.50f * speedMultiplier;
        
        // Planet self-rotation speed
        earthRotAngle += 1.80f * speedMultiplier;
        marsRotAngle += 1.60f * speedMultiplier;
        
        // Wrap angles within [0, 360] to prevent floating point overflow
        if (earthOrbitAngle >= 360.0f) earthOrbitAngle -= 360.0f;
        if (earthRotAngle >= 360.0f) earthRotAngle -= 360.0f;
        if (moonOrbitAngle >= 360.0f) moonOrbitAngle -= 360.0f;
        if (marsOrbitAngle >= 360.0f) marsOrbitAngle -= 360.0f;
        if (marsRotAngle >= 360.0f) marsRotAngle -= 360.0f;
    }
    
    glutPostRedisplay();
    
    // Call timer again in ~16ms (60 FPS)
    glutTimerFunc(16, timer, 0);
}

int main(int argc, char** argv) {
    // Initialize GLUT
    glutInit(&argc, argv);
    
    // Set double buffering, RGB color space, and depth buffering
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    
    // Set initial window settings
    glutInitWindowSize(g_windowWidth, g_windowHeight);
    glutInitWindowPosition(100, 100);
    glutCreateWindow("3D Solar System");
    
    // Initialize custom OpenGL settings
    init();
    
    // Register callbacks
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    
    // Register the animation timer
    glutTimerFunc(16, timer, 0);
    
    // Print diagnostic system OpenGL versions
    std::cout << "OpenGL Window Initialized Successfully." << std::endl;
    std::cout << "OpenGL Vendor:   " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version:  " << glGetString(GL_VERSION) << std::endl;
    
    // Start GLUT main rendering loop
    glutMainLoop();
    
    return 0;
}
