#include <stdio.h> 
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include "dependente/glew/glew.h"
#include "dependente/glfw/glfw3.h"
#include "shader.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GLFWwindow* window;
const int WINDOW_WIDTH = 728, WINDOW_HEIGHT = 728;

struct Bullet { float x, y, speed; bool isAIBullet; };
std::vector<Bullet> bullets;

struct Tumbleweed {
    float x, y, radius, speed, angle, rotationSpeed;
};
Tumbleweed tumbleweed = { -1.1f, -0.45f, 0.05f, 0.25f, 0.0f, 4.0f };

bool isNight = false;

// --- Cowboy + Horse position ---
float cowboyX = 0.0f, cowboyY = 0.0f;
bool isJumping = false;
float jumpStartTime = 0.0f;
bool playerFacingRight = true; // NEW: player's facing direction

// --- Gun/Ammo System ---
int bulletCount = 8;
bool showAmmoBar = false;
double ammoBarShowTime = 0.0;
const double AMMO_BAR_DURATION = 2.0; // Show for 2 seconds

// --- Lives System ---
int playerLives = 3;
bool gameOver = false;
bool aiDefeated = false;

// --- AI Lives System ---
int aiLives = 3;
bool aiDead = false;

// --- AI Death Animation / Disappear ---
bool aiFalling = false;
float aiFallRotation = 0.0f;   // degrees
float aiFallY = 0.0f;          // vertical sink offset
double aiDeathStartTime = 0.0; // time when death animation started
const float AI_FALL_DURATION = 2.0f; // seconds to complete fall and disappear
bool aiGone = false; // when true, AI is removed from drawing entirely

// --- Muzzle Flash ---
bool muzzleFlashActive = false;
double muzzleFlashStartTime = 0.0;
const double MUZZLE_FLASH_DURATION = 0.15; // Flash for 0.15 seconds (a few frames)

// --- Second Cowboy (AI) ---
struct AICowboy {
    float x, y;
    float speed;
    float direction; // 1 = right, -1 = left
    float minX, maxX;
    double lastShotTime;
    bool muzzleFlashActive;
    double muzzleFlashStartTime;
} aiCowboy = { -0.5f, -0.35f, 0.3f, 1.0f, -0.8f, 0.8f, 0.0, false, 0.0 };

const double AI_SHOOT_INTERVAL = 3.0; // AI shoots every 3 seconds

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        int width, height;
        glfwGetCursorPos(window, &mx, &my);
        glfwGetWindowSize(window, &width, &height);
        float xNDC = (float)((mx / width) * 2.0 - 1.0);
        float yNDC = (float)(1.0 - (my / height) * 2.0);

        // Toggle day/night when clicking on sun/moon
        if (xNDC < -0.4f && yNDC > 0.4f) {
            isNight = !isNight;
        }
        else if (!gameOver) {
            // Show ammo bar when attempting to shoot (only if game not over)
            showAmmoBar = true;
            ammoBarShowTime = glfwGetTime();

            // Only shoot if we have bullets
            if (bulletCount > 0) {
                // spawn player bullet from gun barrel depending on facing
                float bx = cowboyX + (playerFacingRight ? 0.15f : -0.13f);
                float speed = (playerFacingRight ? 0.8f : -0.8f);
                bullets.push_back({ bx, cowboyY - 0.05f, speed, false }); // false = player bullet
                bulletCount--;
                // Activate muzzle flash
                muzzleFlashActive = true;
                muzzleFlashStartTime = glfwGetTime();
            }
        }
    }
}

void addQuad(std::vector<GLfloat>& v, float x1, float y1, float x2, float y2, float r, float g, float b) {
    v.insert(v.end(), {
        x1,y1,0.0f,r,g,b,
        x2,y1,0.0f,r,g,b,
        x2,y2,0.0f,r,g,b,
        x1,y1,0.0f,r,g,b,
        x2,y2,0.0f,r,g,b,
        x1,y2,0.0f,r,g,b
        });
}

// drawCowboy now accepts facingRight to mirror horizontally
void drawCowboy(std::vector<GLfloat>& verts, float baseX, float baseY, float hatBobOffset = 0.0f, bool facingRight = true) {
    float dir = facingRight ? 1.0f : -1.0f;
    auto ox = [&](float offset)->float { return baseX + offset * dir; };

    // Horse body (symmetric)
    addQuad(verts, ox(-0.2f), baseY - 0.1f, ox(0.2f), baseY + 0.1f, 0.4f, 0.2f, 0.0f);
    // Horse legs
    addQuad(verts, ox(-0.18f), baseY - 0.1f, ox(-0.15f), baseY - 0.25f, 0.35f, 0.2f, 0.0f);
    addQuad(verts, ox(0.15f), baseY - 0.1f, ox(0.18f), baseY - 0.25f, 0.35f, 0.2f, 0.0f);
    // Horse head (mirror automatically via ox)
    verts.insert(verts.end(), {
        ox(0.2f),baseY + 0.1f,0.0f,0.4f,0.2f,0.0f,
        ox(0.3f),baseY + 0.1f,0.0f,0.4f,0.2f,0.0f,
        ox(0.3f),baseY + 0.2f,0.0f,0.4f,0.2f,0.0f
        });
    // Cowboy body
    addQuad(verts, ox(-0.04f), baseY + 0.1f, ox(0.04f), baseY + 0.22f, 0.1f, 0.1f, 0.1f);
    // Cowboy head
    addQuad(verts, ox(-0.03f), baseY + 0.22f, ox(0.03f), baseY + 0.28f, 0.9f, 0.8f, 0.6f);
    // Cowboy hat (with bobbing offset applied)
    float hatY1 = baseY + 0.28f + hatBobOffset;
    float hatY2 = baseY + 0.32f + hatBobOffset;
    verts.insert(verts.end(), {
        ox(-0.05f),hatY1,0.0f,0.3f,0.1f,0.0f,
        ox(0.05f),hatY1,0.0f,0.3f,0.1f,0.0f,
        ox(0.0f),hatY2,0.0f,0.3f,0.1f,0.0f
        });
    // Cowboy gun: place to the right if facingRight, left when not (mirrored by ox)
    addQuad(verts, ox(0.04f), baseY + 0.14f, ox(0.11f), baseY + 0.18f, 0.2f, 0.15f, 0.1f);
}

void drawCharacter(std::vector<GLfloat>& verts, float x, float y, char c, float size, float r, float g, float b) {
    // Simple bitmap font - draws basic characters as rectangles
    float charWidth = size * 0.6f;
    float charHeight = size;

    switch (c) {
    case 'N':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b); // Left vertical
        addQuad(verts, x + charWidth - size * 0.15f, y, x + charWidth, y + charHeight, r, g, b); // Right vertical
        // Diagonal
        verts.insert(verts.end(), {
            x,y + charHeight,0.0f,r,g,b,
            x + charWidth,y,0.0f,r,g,b,
            x + size * 0.15f,y + charHeight,0.0f,r,g,b,
            x + charWidth,y,0.0f,r,g,b,
            x + charWidth - size * 0.15f,y,0.0f,r,g,b,
            x + size * 0.15f,y + charHeight,0.0f,r,g,b
            });
        break;
    case 'O':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x + charWidth - size * 0.15f, y, x + charWidth, y + charHeight, r, g, b);
        addQuad(verts, x + size * 0.15f, y, x + charWidth - size * 0.15f, y + size * 0.15f, r, g, b);
        addQuad(verts, x + size * 0.15f, y + charHeight - size * 0.15f, x + charWidth - size * 0.15f, y + charHeight, r, g, b);
        break;
    case 'B':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x, y, x + charWidth * 0.8f, y + size * 0.15f, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth * 0.8f, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth * 0.8f, y + charHeight, r, g, b);
        addQuad(verts, x + charWidth * 0.65f, y, x + charWidth * 0.8f, y + charHeight * 0.5f, r, g, b);
        addQuad(verts, x + charWidth * 0.65f, y + charHeight * 0.5f, x + charWidth * 0.8f, y + charHeight, r, g, b);
        break;
    case 'U':
        addQuad(verts, x, y + size * 0.15f, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x + charWidth - size * 0.15f, y + size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        addQuad(verts, x + size * 0.15f, y, x + charWidth - size * 0.15f, y + size * 0.15f, r, g, b);
        break;
    case 'L':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x, y, x + charWidth, y + size * 0.15f, r, g, b);
        break;
    case 'E':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x, y, x + charWidth, y + size * 0.15f, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth * 0.7f, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        break;
    case 'T':
        addQuad(verts, x + charWidth * 0.5f - size * 0.075f, y, x + charWidth * 0.5f + size * 0.075f, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        break;
    case 'S':
        addQuad(verts, x, y, x + charWidth, y + size * 0.15f, r, g, b);
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight * 0.5f, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        addQuad(verts, x + charWidth - size * 0.15f, y + charHeight * 0.5f, x + charWidth, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        break;
    case 'F':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth * 0.7f, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        break;
    case 'I':
        addQuad(verts, x + charWidth * 0.5f - size * 0.075f, y, x + charWidth * 0.5f + size * 0.075f, y + charHeight, r, g, b);
        addQuad(verts, x, y, x + charWidth, y + size * 0.15f, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        break;
    case 'G':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x + size * 0.15f, y, x + charWidth, y + size * 0.15f, r, g, b);
        addQuad(verts, x + size * 0.15f, y + charHeight - size * 0.15f, x + charWidth, y + charHeight, r, g, b);
        addQuad(verts, x + charWidth - size * 0.15f, y, x + charWidth, y + charHeight * 0.5f, r, g, b);
        addQuad(verts, x + charWidth * 0.5f, y + charHeight * 0.4f, x + charWidth, y + charHeight * 0.5f, r, g, b);
        break;
    case 'H':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x + charWidth - size * 0.15f, y, x + charWidth, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        break;
    case 'R':
        addQuad(verts, x, y, x + size * 0.15f, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight - size * 0.15f, x + charWidth * 0.8f, y + charHeight, r, g, b);
        addQuad(verts, x, y + charHeight * 0.5f - size * 0.075f, x + charWidth * 0.8f, y + charHeight * 0.5f + size * 0.075f, r, g, b);
        addQuad(verts, x + charWidth * 0.65f, y + charHeight * 0.5f, x + charWidth * 0.8f, y + charHeight, r, g, b);
        verts.insert(verts.end(), {
            x + charWidth * 0.65f,y + charHeight * 0.5f,0.0f,r,g,b,
            x + charWidth,y,0.0f,r,g,b,
            x + charWidth * 0.8f,y + charHeight * 0.5f,0.0f,r,g,b,
            x + charWidth,y,0.0f,r,g,b,
            x + charWidth * 0.8f,y,0.0f,r,g,b,
            x + charWidth * 0.8f,y + charHeight * 0.5f,0.0f,r,g,b
            });
        break;
    case ' ':
        // Space - no drawing
        break;
    }
}

void drawText(std::vector<GLfloat>& verts, float startX, float startY, const char* text, float size, float r, float g, float b) {
    float x = startX;
    for (int i = 0; text[i] != '\0'; i++) {
        drawCharacter(verts, x, startY, text[i], size, r, g, b);
        x += size * 0.7f; // spacing between characters
    }
}

int main(void) {
    if (!glfwInit()) { fprintf(stderr, "Failed to initialize GLFW\n"); return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Wild West", NULL, NULL);
    if (!window) { fprintf(stderr, "Failed to open GLFW window\n"); glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) { fprintf(stderr, "Failed to initialize GLEW\n"); glfwTerminate(); return -1; }

    GLuint programID = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");
    if (programID == 0) { std::cerr << "Failed to load shaders.\n"; glfwTerminate(); return -1; }

    // Enable blending for text rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Scene base ---
    std::vector<GLfloat> sceneVerts;
    addQuad(sceneVerts, -1.0f, -0.5f, 1.0f, -1.0f, 0.83f, 0.72f, 0.49f); // ground
    // mountains
    sceneVerts.insert(sceneVerts.end(), {
        -0.9f,-0.5f,0.0f,0.4f,0.3f,0.2f,
        -0.5f,0.4f,0.0f,0.4f,0.3f,0.2f,
        -0.1f,-0.5f,0.0f,0.4f,0.3f,0.2f,
         0.2f,-0.5f,0.0f,0.6f,0.4f,0.2f,
         0.6f,0.5f,0.0f,0.6f,0.4f,0.2f,
         1.0f,-0.5f,0.0f,0.6f,0.4f,0.2f
        });
    // cacti
    addQuad(sceneVerts, -0.75f, -0.5f, -0.7f, -0.1f, 0.0f, 0.8f, 0.0f);
    addQuad(sceneVerts, 0.6f, -0.5f, 0.65f, -0.15f, 0.0f, 0.9f, 0.0f);

    // cacti (duplicate in original - kept)
    addQuad(sceneVerts, -0.75f, -0.5f, -0.7f, -0.1f, 0.0f, 0.8f, 0.0f);
    addQuad(sceneVerts, 0.6f, -0.5f, 0.65f, -0.15f, 0.0f, 0.9f, 0.0f);

    // === SALOON BUILDING ===
    float saloonLeft = -0.2f;
    float saloonRight = 0.4f;
    float saloonBottom = -0.5f;
    float saloonTop = 0.2f;

    // Main building body (wooden brown)
    addQuad(sceneVerts, saloonLeft, saloonBottom, saloonRight, saloonTop, 0.55f, 0.27f, 0.07f);

    // Roof (darker brown)
    addQuad(sceneVerts, saloonLeft - 0.05f, saloonTop, saloonRight + 0.05f, saloonTop + 0.05f, 0.35f, 0.16f, 0.05f);

    // Door
    addQuad(sceneVerts, 0.05f, saloonBottom, 0.15f, -0.2f, 0.25f, 0.13f, 0.03f);

    // Windows (light yellow, as if lights are inside)
    addQuad(sceneVerts, saloonLeft + 0.05f, -0.1f, saloonLeft + 0.15f, 0.05f, 0.9f, 0.8f, 0.5f);
    addQuad(sceneVerts, saloonRight - 0.15f, -0.1f, saloonRight - 0.05f, 0.05f, 0.9f, 0.8f, 0.5f);

    // Balcony/Sign base
    addQuad(sceneVerts, saloonLeft + 0.05f, 0.15f, saloonRight - 0.05f, 0.2f, 0.4f, 0.2f, 0.05f);

    // “SALOON” text sign (big letters)
    drawText(sceneVerts, saloonLeft + 0.1f, 0.18f, "SALOON", 0.05f, 0.9f, 0.9f, 0.2f);

    // === PORCH ADDITION ===
    float porchDepth = 0.05f;
    float porchTop = saloonBottom + 0.05f;
    float porchFront = saloonBottom - porchDepth;

    // Porch deck (lighter wood)
    addQuad(sceneVerts, saloonLeft - 0.05f, porchFront, saloonRight + 0.05f, porchTop, 0.65f, 0.43f, 0.20f);

    // Support columns
    float columnWidth = 0.03f;
    float columnTop = saloonTop;
    float columnBottom = porchTop;

    // Left column
    addQuad(sceneVerts, saloonLeft + 0.05f, columnBottom, saloonLeft + 0.05f + columnWidth, columnTop, 0.45f, 0.25f, 0.08f);

    // Right column
    addQuad(sceneVerts, saloonRight - 0.05f - columnWidth, columnBottom, saloonRight - 0.05f, columnTop, 0.45f, 0.25f, 0.08f);

    // Optional: porch shadow
    addQuad(sceneVerts, saloonLeft - 0.05f, porchFront - 0.02f, saloonRight + 0.05f, porchFront, 0.25f, 0.20f, 0.10f);

    // === END SALOON ===


    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sceneVerts.size() * sizeof(GLfloat), sceneVerts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        tumbleweed.x += tumbleweed.speed * deltaTime;
        tumbleweed.angle += tumbleweed.rotationSpeed * deltaTime;
        if (tumbleweed.x - tumbleweed.radius > 1.2f) tumbleweed.x = -1.2f;

        // === AI Cowboy Movement ===
        if (!aiDead) {
            aiCowboy.x += aiCowboy.direction * aiCowboy.speed * deltaTime;
            // Change direction when reaching boundaries
            if (aiCowboy.x >= aiCowboy.maxX) {
                aiCowboy.x = aiCowboy.maxX;
                aiCowboy.direction = -1.0f;
            }
            else if (aiCowboy.x <= aiCowboy.minX) {
                aiCowboy.x = aiCowboy.minX;
                aiCowboy.direction = 1.0f;
            }
        }

        // === AI Death Animation update (if active) ===
        if (aiFalling) {
            double t = currentTime - aiDeathStartTime;
            float progress = (float)std::min(t / AI_FALL_DURATION, 1.0);
            aiFallRotation = progress * 90.0f;            // rotate up to 90 degrees
            aiFallY = progress * -0.25f;                 // sink down up to -0.25
            if (t >= AI_FALL_DURATION) {
                // animation finished -> mark fully gone
                aiFalling = false;
                aiGone = true;
            }
        }

        // === Movement: WASD and Arrow keys (disabled when game over) ===
        if (!gameOver) {
            // Player facing updates: pressing A makes him face left; D makes him face right.
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
                playerFacingRight = false; // flip to left when pressing A
                cowboyX -= 0.4f * deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
                playerFacingRight = true; // face right when pressing D
                cowboyX += 0.4f * deltaTime;
            }
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
                cowboyY += 0.4f * deltaTime;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
                cowboyY -= 0.4f * deltaTime;

            // === Jumping ===
            if (!isJumping && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                isJumping = true;
                jumpStartTime = currentTime;
            }
        }
        float jumpY = 0.0f;
        if (isJumping) {
            float t = (float)(currentTime - jumpStartTime);
            if (t < 1.0f)
                jumpY = 0.25f * sin(t * M_PI);
            else
                isJumping = false;
        }

        // Player’s torso anchor matches drawCowboy()
        float playerBaseX = cowboyX;
        float playerBaseY = cowboyY + jumpY - 0.2f;

        // Treat the saloon as a rectangle with a small buffer for easy triggering
        bool nearSaloon =
            playerBaseX >= saloonLeft - 0.1f && playerBaseX <= saloonRight + 0.1f &&
            playerBaseY >= saloonBottom - 0.05f && playerBaseY <= saloonTop + 0.05f;

        static bool reloadHeld = false;  // prevents holding R from spamming reloads
        int reloadKeyState = glfwGetKey(window, GLFW_KEY_R);

        if (!gameOver && nearSaloon && reloadKeyState == GLFW_PRESS && !reloadHeld) {
            bulletCount = 8;
            showAmmoBar = true;
            ammoBarShowTime = currentTime;
            reloadHeld = true;
        }
        if (reloadKeyState == GLFW_RELEASE) {
            reloadHeld = false;
        }

        // Update bullets
        for (auto& b : bullets) b.x += b.speed * deltaTime;

        // === Collision Detection: AI Bullets vs Player Cowboy Body ===
        if (!gameOver && playerLives > 0) {
            float playerBaseX = cowboyX;
            float playerBaseY = cowboyY + jumpY - 0.2f;
            // Player cowboy body bounds (from drawCowboy function)
            float playerBodyLeft = playerBaseX - 0.04f;
            float playerBodyRight = playerBaseX + 0.04f;
            float playerBodyBottom = playerBaseY + 0.1f;
            float playerBodyTop = playerBaseY + 0.22f;

            // Check collision with AI bullets
            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                [&](const Bullet& b) {
                    if (b.isAIBullet) {
                        // Bullet bounds
                        float bulletLeft = b.x;
                        float bulletRight = b.x + 0.02f * (b.speed >= 0 ? 1.0f : -1.0f);
                        // normalize min/max
                        if (bulletRight < bulletLeft) std::swap(bulletLeft, bulletRight);
                        float bulletBottom = b.y;
                        float bulletTop = b.y + 0.01f;

                        // Check if bullet overlaps with player body
                        if (bulletRight >= playerBodyLeft && bulletLeft <= playerBodyRight &&
                            bulletTop >= playerBodyBottom && bulletBottom <= playerBodyTop) {
                            // Collision! Player loses a life
                            playerLives--;
                            if (playerLives <= 0) {
                                gameOver = true;
                            }
                            return true; // Remove bullet
                        }
                    }
                    return false;
                }), bullets.end());
        }

        // === Collision Detection: Player Bullets vs AI Cowboy Body (NEW) ===
        // Only process collisions if AI is not already gone/fully dead
        if (!aiGone && !aiDead) {
            float aiBaseX = aiCowboy.x;
            float aiBaseY = aiCowboy.y;
            // AI cowboy body bounds (same as drawCowboy)
            float aiBodyLeft = aiBaseX - 0.04f;
            float aiBodyRight = aiBaseX + 0.04f;
            float aiBodyBottom = aiBaseY + 0.1f;
            float aiBodyTop = aiBaseY + 0.22f;

            bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
                [&](const Bullet& b) {
                    if (!b.isAIBullet) {
                        // Player bullet bounds
                        float bulletLeft = b.x;
                        float bulletRight = b.x + 0.02f * (b.speed >= 0 ? 1.0f : -1.0f);
                        if (bulletRight < bulletLeft) std::swap(bulletLeft, bulletRight);
                        float bulletBottom = b.y;
                        float bulletTop = b.y + 0.01f;

                        // Check overlap with AI body
                        if (bulletRight >= aiBodyLeft && bulletLeft <= aiBodyRight &&
                            bulletTop >= aiBodyBottom && bulletBottom <= aiBodyTop) {
                            // Collision! AI loses a life
                            aiLives--;
                            if (aiLives <= 0) {
                                // trigger death/fall animation (do not immediately remove)
                                aiDead = true; // stop movement/shooting immediately
                                aiFalling = true;
                                aiDeathStartTime = currentTime;
                                aiCowboy.muzzleFlashActive = false;
                            }
                            return true; // remove bullet
                        }
                    }
                    return false;
                }), bullets.end());
        }

        // Remove bullets that go off screen (left or right)
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(),
            [](const Bullet& b) {return b.x > 1.2f || b.x < -1.2f; }), bullets.end());

        // === AI Cowboy Shooting (disabled when game over or AI dead) ===
        if (!gameOver && !aiDead && currentTime - aiCowboy.lastShotTime >= AI_SHOOT_INTERVAL) {
            // AI cowboy shoots (from gun barrel position)
            bool aiFacingRight = (aiCowboy.direction > 0.0f);
            float aiGunX = aiCowboy.x + (aiFacingRight ? 0.11f : -0.11f); // Gun barrel X position depending on facing
            float aiGunY = aiCowboy.y + 0.15f; // Bullet Y position (matches player's relative offset)
            float aiBulletSpeed = (aiFacingRight ? 0.8f : -0.8f);
            bullets.push_back({ aiGunX, aiGunY, aiBulletSpeed, true }); // true = AI bullet
            aiCowboy.lastShotTime = currentTime;
            // Activate AI muzzle flash
            aiCowboy.muzzleFlashActive = true;
            aiCowboy.muzzleFlashStartTime = currentTime;
        }

        // Update AI muzzle flash state
        if (aiCowboy.muzzleFlashActive && (currentTime - aiCowboy.muzzleFlashStartTime) > MUZZLE_FLASH_DURATION) {
            aiCowboy.muzzleFlashActive = false;
        }

        // Hide ammo bar after duration
        if (showAmmoBar && (currentTime - ammoBarShowTime) > AMMO_BAR_DURATION) {
            showAmmoBar = false;
        }

        // Update muzzle flash state
        if (muzzleFlashActive && (currentTime - muzzleFlashStartTime) > MUZZLE_FLASH_DURATION) {
            muzzleFlashActive = false;
        }

        // --- Background ---
        if (!isNight) glClearColor(0.55f, 0.75f, 1.0f, 1.0f);
        else glClearColor(0.05f, 0.05f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(programID);

        // --- Draw Scene ---
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, sceneVerts.size() / 6);

        // --- Sun/Moon ---
        float sunX = -0.7f, sunY = 0.7f, sunR = 0.15f;
        float sr = isNight ? 0.8f : 1.0f, sg = isNight ? 0.8f : 0.9f, sb = isNight ? 0.8f : 0.2f;
        std::vector<GLfloat> sunVerts;
        int segs = 30;
        for (int i = 0; i < segs; i++) {
            float a1 = 2 * M_PI * i / segs, a2 = 2 * M_PI * (i + 1) / segs;
            sunVerts.insert(sunVerts.end(), {
                sunX,sunY,0.0f,sr,sg,sb,
                sunX + sunR * cos(a1),sunY + sunR * sin(a1),0.0f,sr,sg,sb,
                sunX + sunR * cos(a2),sunY + sunR * sin(a2),0.0f,sr,sg,sb
                });
        }
        GLuint sVAO, sVBO;
        glGenVertexArrays(1, &sVAO); glBindVertexArray(sVAO);
        glGenBuffers(1, &sVBO); glBindBuffer(GL_ARRAY_BUFFER, sVBO);
        glBufferData(GL_ARRAY_BUFFER, sunVerts.size() * sizeof(GLfloat), sunVerts.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, sunVerts.size() / 6);
        glDeleteBuffers(1, &sVBO); glDeleteVertexArrays(1, &sVAO);

        // --- Tumbleweed ---
        std::vector<GLfloat> tVerts;
        int segCount = 14;
        for (int i = 0; i < segCount; i++) {
            float a1 = 2 * M_PI * i / segCount;
            float a2 = 2 * M_PI * (i + 1) / segCount;
            tVerts.insert(tVerts.end(), {
                tumbleweed.x,tumbleweed.y,0.0f,0.6f,0.4f,0.1f,
                tumbleweed.x + tumbleweed.radius * cos(a1 + tumbleweed.angle),tumbleweed.y + tumbleweed.radius * sin(a1 + tumbleweed.angle),0.0f,0.6f,0.4f,0.1f,
                tumbleweed.x + tumbleweed.radius * cos(a2 + tumbleweed.angle),tumbleweed.y + tumbleweed.radius * sin(a2 + tumbleweed.angle),0.0f,0.6f,0.4f,0.1f
                });
        }
        GLuint tVAO, tVBO;
        glGenVertexArrays(1, &tVAO);
        glBindVertexArray(tVAO);
        glGenBuffers(1, &tVBO);
        glBindBuffer(GL_ARRAY_BUFFER, tVBO);
        glBufferData(GL_ARRAY_BUFFER, tVerts.size() * sizeof(GLfloat), tVerts.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, tVerts.size() / 6);
        glDeleteBuffers(1, &tVBO); glDeleteVertexArrays(1, &tVAO);

        // --- Hat Bobbing Animation ---
        const float HAT_BOB_FREQUENCY = 2.0f; // Slow frequency for smooth bobbing
        const float HAT_BOB_AMPLITUDE = 0.015f; // Small amplitude for subtle movement
        float playerHatBob = sin((float)currentTime * HAT_BOB_FREQUENCY) * HAT_BOB_AMPLITUDE;
        float aiHatBob = sin((float)currentTime * HAT_BOB_FREQUENCY + (float)M_PI / 2.0f) * HAT_BOB_AMPLITUDE; // 90 degrees out of phase

        // --- Player Cowboy + Horse ---
        std::vector<GLfloat> hcVerts;
        float baseX = cowboyX;
        float baseY = cowboyY + jumpY - 0.2f;
        drawCowboy(hcVerts, baseX, baseY, playerHatBob, playerFacingRight);

        GLuint hcVAO, hcVBO;
        glGenVertexArrays(1, &hcVAO);
        glBindVertexArray(hcVAO);
        glGenBuffers(1, &hcVBO);
        glBindBuffer(GL_ARRAY_BUFFER, hcVBO);
        glBufferData(GL_ARRAY_BUFFER, hcVerts.size() * sizeof(GLfloat), hcVerts.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, hcVerts.size() / 6);
        glDeleteBuffers(1, &hcVBO); glDeleteVertexArrays(1, &hcVAO);

        // --- Muzzle Flash (player) ---
        if (muzzleFlashActive) {
            float flashElapsed = (float)(currentTime - muzzleFlashStartTime);
            float flashAlpha = 1.0f - (flashElapsed / (float)MUZZLE_FLASH_DURATION); // Fade out
            flashAlpha = std::max(0.0f, std::min(1.0f, flashAlpha)); // Clamp between 0 and 1

            // Gun barrel position depends on facing
            float gunBarrelX = baseX + (playerFacingRight ? 0.11f : -0.11f);
            float gunBarrelY = baseY + 0.16f; // Middle of gun vertically

            // Draw bright flash at gun barrel
            std::vector<GLfloat> flashVerts;
            // Bright yellow-white flash that extends from barrel in facing direction
            float flashHeight = 0.06f * flashAlpha; // Vertical size of flash
            float flashWidth = 0.08f * flashAlpha * (playerFacingRight ? 1.0f : -1.0f); // Horizontal extension from barrel with sign
            float flashBrightness = 1.0f * flashAlpha; // Brightness fades from full to zero

            // Main flash (bright white-yellow) - horizontal extent depends on sign
            addQuad(flashVerts,
                gunBarrelX, gunBarrelY - flashHeight * 0.5f,
                gunBarrelX + flashWidth, gunBarrelY + flashHeight * 0.5f,
                flashBrightness, flashBrightness * 0.95f, flashBrightness * 0.4f); // Bright yellow-white

            // Secondary inner flash (brighter core) - only visible in first half of flash duration
            if (flashAlpha > 0.5f) {
                float coreAlpha = (flashAlpha - 0.5f) * 2.0f; // Scale from 0 to 1 over first half
                addQuad(flashVerts,
                    gunBarrelX, gunBarrelY - flashHeight * 0.3f * coreAlpha,
                    gunBarrelX + flashWidth * 0.7f * coreAlpha, gunBarrelY + flashHeight * 0.3f * coreAlpha,
                    1.0f, 1.0f, 0.85f); // Very bright white-yellow core
            }

            GLuint flashVAO, flashVBO;
            glGenVertexArrays(1, &flashVAO);
            glBindVertexArray(flashVAO);
            glGenBuffers(1, &flashVBO);
            glBindBuffer(GL_ARRAY_BUFFER, flashVBO);
            glBufferData(GL_ARRAY_BUFFER, flashVerts.size() * sizeof(GLfloat), flashVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, flashVerts.size() / 6);
            glDeleteBuffers(1, &flashVBO); glDeleteVertexArrays(1, &flashVAO);
        }

        // --- AI Cowboy + Horse ---
        // draw only if not fully gone; if falling, animate rotation and sink; if aiDead but falling, still draw
        if (!aiGone) {
            std::vector<GLfloat> aiVerts;
            float aiBaseX = aiCowboy.x;
            float aiBaseY = aiCowboy.y + aiFallY; // vertical offset while falling
            bool aiFacingRight = (aiCowboy.direction > 0.0f);
            drawCowboy(aiVerts, aiBaseX, aiBaseY, aiHatBob, aiFacingRight);

            // If falling, rotate vertices around pivot
            if (aiFalling) {
                // choose pivot near torso/head so it looks like tipping over
                float cx = aiBaseX;                 // pivot x
                float cy = aiBaseY + 0.12f;        // pivot y (around the upper body)
                float angleRad = aiFallRotation * (float)M_PI / 180.0f;
                float c = cos(angleRad), s = sin(angleRad);
                for (size_t i = 0; i + 1 < aiVerts.size(); i += 6) {
                    float x = aiVerts[i];
                    float y = aiVerts[i + 1];
                    float dx = x - cx;
                    float dy = y - cy;
                    float nx = cx + dx * c - dy * s;
                    float ny = cy + dx * s + dy * c;
                    aiVerts[i] = nx;
                    aiVerts[i + 1] = ny;
                }
            }

            GLuint aiVAO, aiVBO;
            glGenVertexArrays(1, &aiVAO);
            glBindVertexArray(aiVAO);
            glGenBuffers(1, &aiVBO);
            glBindBuffer(GL_ARRAY_BUFFER, aiVBO);
            glBufferData(GL_ARRAY_BUFFER, aiVerts.size() * sizeof(GLfloat), aiVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, aiVerts.size() / 6);
            glDeleteBuffers(1, &aiVBO); glDeleteVertexArrays(1, &aiVAO);
        }

        // --- AI Muzzle Flash ---
        // Only draw muzzle flash if AI not gone (and if currently active)
        if (!aiGone && aiCowboy.muzzleFlashActive) {
            float flashElapsed = (float)(currentTime - aiCowboy.muzzleFlashStartTime);
            float flashAlpha = 1.0f - (flashElapsed / (float)MUZZLE_FLASH_DURATION); // Fade out
            flashAlpha = std::max(0.0f, std::min(1.0f, flashAlpha)); // Clamp between 0 and 1

            // AI gun barrel position depends on facing
            float aiBaseX = aiCowboy.x;
            float aiBaseY = aiCowboy.y + aiFallY;
            bool aiFacingRight = (aiCowboy.direction > 0.0f);
            float aiGunBarrelX = aiBaseX + (aiFacingRight ? 0.11f : -0.11f);
            float aiGunBarrelY = aiBaseY + 0.16f; // Middle of gun vertically

            // Draw bright flash at gun barrel (extends in facing direction)
            std::vector<GLfloat> aiFlashVerts;
            float flashHeight = 0.06f * flashAlpha; // Vertical size of flash
            float flashWidth = 0.08f * flashAlpha * (aiFacingRight ? 1.0f : -1.0f); // signed
            float flashBrightness = 1.0f * flashAlpha; // Brightness fades from full to zero

            addQuad(aiFlashVerts,
                aiGunBarrelX, aiGunBarrelY - flashHeight * 0.5f,
                aiGunBarrelX + flashWidth, aiGunBarrelY + flashHeight * 0.5f,
                flashBrightness, flashBrightness * 0.95f, flashBrightness * 0.4f); // Bright yellow-white

            if (flashAlpha > 0.5f) {
                float coreAlpha = (flashAlpha - 0.5f) * 2.0f;
                addQuad(aiFlashVerts,
                    aiGunBarrelX, aiGunBarrelY - flashHeight * 0.3f * coreAlpha,
                    aiGunBarrelX + flashWidth * 0.7f * coreAlpha, aiGunBarrelY + flashHeight * 0.3f * coreAlpha,
                    1.0f, 1.0f, 0.85f);
            }

            GLuint aiFlashVAO, aiFlashVBO;
            glGenVertexArrays(1, &aiFlashVAO);
            glBindVertexArray(aiFlashVAO);
            glGenBuffers(1, &aiFlashVBO);
            glBindBuffer(GL_ARRAY_BUFFER, aiFlashVBO);
            glBufferData(GL_ARRAY_BUFFER, aiFlashVerts.size() * sizeof(GLfloat), aiFlashVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, aiFlashVerts.size() / 6);
            glDeleteBuffers(1, &aiFlashVBO); glDeleteVertexArrays(1, &aiFlashVAO);
        }

        // --- Bullets ---
        if (!bullets.empty()) {
            std::vector<GLfloat> bVerts;
            for (auto& b : bullets) {
                addQuad(bVerts, b.x, b.y, b.x + 0.02f, b.y + 0.01f, 1.0f, 1.0f, 0.0f);
            }
            GLuint bVAO, bVBO;
            glGenVertexArrays(1, &bVAO);
            glBindVertexArray(bVAO);
            glGenBuffers(1, &bVBO);
            glBindBuffer(GL_ARRAY_BUFFER, bVBO);
            glBufferData(GL_ARRAY_BUFFER, bVerts.size() * sizeof(GLfloat), bVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, bVerts.size() / 6);
            glDeleteBuffers(1, &bVBO); glDeleteVertexArrays(1, &bVAO);
        }

        // --- Lives Display (Top-Left Corner) (Player) ---
        std::vector<GLfloat> livesVerts;
        float livesStartX = -0.95f;
        float livesStartY = 0.85f;
        float lifeSquareSize = 0.05f;
        float lifeSpacing = 0.06f;

        for (int i = 0; i < 3; i++) {
            float xPos = livesStartX + (i * lifeSpacing);
            if (i < playerLives) {
                // Full life (red square with border)
                addQuad(livesVerts, xPos - 0.003f, livesStartY - 0.003f, xPos + lifeSquareSize + 0.003f, livesStartY + lifeSquareSize + 0.003f, 0.2f, 0.0f, 0.0f); // Border
                addQuad(livesVerts, xPos, livesStartY, xPos + lifeSquareSize, livesStartY + lifeSquareSize, 1.0f, 0.0f, 0.0f); // Red square
            }
            else {
                // Lost life (gray/empty square with border)
                addQuad(livesVerts, xPos - 0.003f, livesStartY - 0.003f, xPos + lifeSquareSize + 0.003f, livesStartY + lifeSquareSize + 0.003f, 0.15f, 0.15f, 0.15f); // Border
                addQuad(livesVerts, xPos, livesStartY, xPos + lifeSquareSize, livesStartY + lifeSquareSize, 0.25f, 0.25f, 0.25f); // Gray square
            }
        }

        GLuint livesVAO, livesVBO;
        glGenVertexArrays(1, &livesVAO);
        glBindVertexArray(livesVAO);
        glGenBuffers(1, &livesVBO);
        glBindBuffer(GL_ARRAY_BUFFER, livesVBO);
        glBufferData(GL_ARRAY_BUFFER, livesVerts.size() * sizeof(GLfloat), livesVerts.data(), GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, livesVerts.size() / 6);
        glDeleteBuffers(1, &livesVBO); glDeleteVertexArrays(1, &livesVAO);

        // --- AI Lives Display (Top-Right Corner) (green squares) ---
        std::vector<GLfloat> aiLivesVerts;
        float aiLivesStartX = 0.6f; // top-right area
        float aiLivesStartY = 0.85f;
        for (int i = 0; i < 3; i++) {
            float xPos = aiLivesStartX + (i * lifeSpacing);
            if (i < aiLives) {
                // Full AI life (green square with darker border)
                addQuad(aiLivesVerts, xPos - 0.003f, aiLivesStartY - 0.003f, xPos + lifeSquareSize + 0.003f, aiLivesStartY + lifeSquareSize + 0.003f, 0.0f, 0.35f, 0.0f); // Border (darker green)
                addQuad(aiLivesVerts, xPos, aiLivesStartY, xPos + lifeSquareSize, aiLivesStartY + lifeSquareSize, 0.0f, 1.0f, 0.0f); // Green square
            }
            else {
                // Lost AI life (gray/empty square with border)
                addQuad(aiLivesVerts, xPos - 0.003f, aiLivesStartY - 0.003f, xPos + lifeSquareSize + 0.003f, aiLivesStartY + lifeSquareSize + 0.003f, 0.15f, 0.15f, 0.15f); // Border
                addQuad(aiLivesVerts, xPos, aiLivesStartY, xPos + lifeSquareSize, aiLivesStartY + lifeSquareSize, 0.25f, 0.25f, 0.25f); // Gray square
            }
        }

        if (!aiLivesVerts.empty()) {
            GLuint aiLivesVAO, aiLivesVBO;
            glGenVertexArrays(1, &aiLivesVAO);
            glBindVertexArray(aiLivesVAO);
            glGenBuffers(1, &aiLivesVBO);
            glBindBuffer(GL_ARRAY_BUFFER, aiLivesVBO);
            glBufferData(GL_ARRAY_BUFFER, aiLivesVerts.size() * sizeof(GLfloat), aiLivesVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, aiLivesVerts.size() / 6);
            glDeleteBuffers(1, &aiLivesVBO); glDeleteVertexArrays(1, &aiLivesVAO);
        }

        // --- Ammo Bar ---
        if (showAmmoBar) {
            std::vector<GLfloat> ammoVerts;

            // Professional horizontal bar background with border
            float barWidth = 0.6f;
            float barHeight = 0.12f;
            float barX = -barWidth / 2.0f;
            float barY = 0.75f;

            // Outer border (darker)
            addQuad(ammoVerts, barX - 0.01f, barY - 0.01f, barX + barWidth + 0.01f, barY + barHeight + 0.01f, 0.1f, 0.1f, 0.1f);
            // Inner background (semi-transparent dark)
            addQuad(ammoVerts, barX, barY, barX + barWidth, barY + barHeight, 0.2f, 0.2f, 0.2f);

            // Label section
            addQuad(ammoVerts, barX, barY, barX + 0.15f, barY + barHeight, 0.15f, 0.15f, 0.15f);

            // Draw "AMMO" text
            drawText(ammoVerts, barX + 0.015f, barY + 0.03f, "AMMO", 0.04f, 0.9f, 0.9f, 0.9f);

            // Draw bullet indicators horizontally
            float bulletStartX = barX + 0.17f;
            float bulletY = barY + 0.035f;
            float bulletWidth = 0.045f;
            float bulletHeight = 0.05f;
            float bulletSpacing = 0.052f;

            for (int i = 0; i < 8; i++) {
                float xPos = bulletStartX + (i * bulletSpacing);
                if (i < bulletCount) {
                    // Filled bullet with gradient effect
                    addQuad(ammoVerts, xPos, bulletY, xPos + bulletWidth, bulletY + bulletHeight, 1.0f, 0.84f, 0.0f);
                    // Highlight
                    addQuad(ammoVerts, xPos + 0.005f, bulletY + bulletHeight - 0.015f, xPos + bulletWidth - 0.005f, bulletY + bulletHeight - 0.005f, 1.0f, 0.95f, 0.6f);
                }
                else {
                    // Empty bullet slot
                    addQuad(ammoVerts, xPos, bulletY, xPos + bulletWidth, bulletY + bulletHeight, 0.3f, 0.3f, 0.3f);
                    // Inner empty
                    addQuad(ammoVerts, xPos + 0.005f, bulletY + 0.005f, xPos + bulletWidth - 0.005f, bulletY + bulletHeight - 0.005f, 0.15f, 0.15f, 0.15f);
                }
            }

            GLuint ammoVAO, ammoVBO;
            glGenVertexArrays(1, &ammoVAO);
            glBindVertexArray(ammoVAO);
            glGenBuffers(1, &ammoVBO);
            glBindBuffer(GL_ARRAY_BUFFER, ammoVBO);
            glBufferData(GL_ARRAY_BUFFER, ammoVerts.size() * sizeof(GLfloat), ammoVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, ammoVerts.size() / 6);
            glDeleteBuffers(1, &ammoVBO); glDeleteVertexArrays(1, &ammoVAO);

            // Draw warning message if no bullets
            if (bulletCount == 0) {
                std::vector<GLfloat> msgVerts;
                float msgWidth = 0.8f;
                float msgHeight = 0.15f;
                float msgX = -msgWidth / 2.0f;
                float msgY = 0.55f;

                // Message box with border
                addQuad(msgVerts, msgX - 0.01f, msgY - 0.01f, msgX + msgWidth + 0.01f, msgY + msgHeight + 0.01f, 0.5f, 0.0f, 0.0f);
                addQuad(msgVerts, msgX, msgY, msgX + msgWidth, msgY + msgHeight, 0.8f, 0.1f, 0.1f);

                // Draw message text
                drawText(msgVerts, msgX + 0.05f, msgY + 0.06f, "NO BULLETS LEFT", 0.05f, 1.0f, 1.0f, 1.0f);
                drawText(msgVerts, msgX + 0.08f, msgY + 0.02f, "REFILL THE GUN", 0.04f, 1.0f, 0.9f, 0.0f);

                GLuint msgVAO, msgVBO;
                glGenVertexArrays(1, &msgVAO);
                glBindVertexArray(msgVAO);
                glGenBuffers(1, &msgVBO);
                glBindBuffer(GL_ARRAY_BUFFER, msgVBO);
                glBufferData(GL_ARRAY_BUFFER, msgVerts.size() * sizeof(GLfloat), msgVerts.data(), GL_DYNAMIC_DRAW);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
                glDrawArrays(GL_TRIANGLES, 0, msgVerts.size() / 6);
                glDeleteBuffers(1, &msgVBO); glDeleteVertexArrays(1, &msgVAO);
            }
        }

        // --- Victory Message when AI is defeated ---

        if (aiDead) {
            std::vector<GLfloat> victoryVerts;
            float msgWidth = 1.0f;
            float msgHeight = 0.25f;
            float msgX = -msgWidth / 2.0f;
            float msgY = 0.2f;

            // Game Won box with border (green theme)
            addQuad(victoryVerts, msgX - 0.02f, msgY - 0.02f,
                msgX + msgWidth + 0.02f, msgY + msgHeight + 0.0f,
                0.0f, 0.2f, 0.0f); // dark green border

            addQuad(victoryVerts, msgX, msgY,
                msgX + msgWidth, msgY + msgHeight,
                0.0f, 0.6f, 0.0f); // medium green background

            addQuad(victoryVerts, msgX + 0.01f, msgY + 0.01f,
                msgX + msgWidth - 0.01f, msgY + msgHeight - 0.01f,
                0.0f, 0.8f, 0.0f); // brighter green inner glow


            // Draw "GAME WON" text
            drawText(victoryVerts, msgX + 0.15f, msgY + 0.15f, "CONGRATULATIONS", 0.08f, 1.0f, 1.0f, 1.0f);
            drawText(victoryVerts, msgX + 0.22f, msgY + 0.05f, "YOU WON", 0.06f, 1.0f, 1.0f, 0.6f);


            GLuint victoryVAO, victoryVBO;
            glGenVertexArrays(1, &victoryVAO);
            glBindVertexArray(victoryVAO);
            glGenBuffers(1, &victoryVBO);
            glBindBuffer(GL_ARRAY_BUFFER, victoryVBO);
            glBufferData(GL_ARRAY_BUFFER, victoryVerts.size() * sizeof(GLfloat),
                victoryVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                6 * sizeof(GLfloat), (void*)0);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
            glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, victoryVerts.size() / 6);
            glDeleteBuffers(1, &victoryVBO);
            glDeleteVertexArrays(1, &victoryVAO);
        }

        // --- Game Over Message ---
        if (gameOver) {
            std::vector<GLfloat> gameOverVerts;
            float msgWidth = 1.0f;
            float msgHeight = 0.25f;
            float msgX = -msgWidth / 2.0f;
            float msgY = 0.2f;

            // Game over box with border
            addQuad(gameOverVerts, msgX - 0.02f, msgY - 0.02f, msgX + msgWidth + 0.02f, msgY + msgHeight + 0.02f, 0.0f, 0.0f, 0.0f);
            addQuad(gameOverVerts, msgX, msgY, msgX + msgWidth, msgY + msgHeight, 0.2f, 0.0f, 0.0f);
            addQuad(gameOverVerts, msgX + 0.01f, msgY + 0.01f, msgX + msgWidth - 0.01f, msgY + msgHeight - 0.01f, 0.4f, 0.0f, 0.0f);

            // Draw "GAME OVER" text
            drawText(gameOverVerts, msgX + 0.15f, msgY + 0.15f, "GAME OVER", 0.08f, 1.0f, 0.0f, 0.0f);
            drawText(gameOverVerts, msgX + 0.22f, msgY + 0.05f, "YOU LOST", 0.06f, 1.0f, 1.0f, 1.0f);

            GLuint gameOverVAO, gameOverVBO;
            glGenVertexArrays(1, &gameOverVAO);
            glBindVertexArray(gameOverVAO);
            glGenBuffers(1, &gameOverVBO);
            glBindBuffer(GL_ARRAY_BUFFER, gameOverVBO);
            glBufferData(GL_ARRAY_BUFFER, gameOverVerts.size() * sizeof(GLfloat), gameOverVerts.data(), GL_DYNAMIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0); glEnableVertexAttribArray(0);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
            glDrawArrays(GL_TRIANGLES, 0, gameOverVerts.size() / 6);
            glDeleteBuffers(1, &gameOverVBO); glDeleteVertexArrays(1, &gameOverVAO);
        }

        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}
