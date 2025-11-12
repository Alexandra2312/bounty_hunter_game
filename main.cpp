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

struct Bullet { float x, y, speed; };
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

// --- Gun/Ammo System ---
int bulletCount = 8;
bool showAmmoBar = false;
double ammoBarShowTime = 0.0;
const double AMMO_BAR_DURATION = 2.0; // Show for 2 seconds

// --- Second Cowboy (AI) ---
struct AICowboy {
    float x, y;
    float speed;
    float direction; // 1 = right, -1 = left
    float minX, maxX;
} aiCowboy = { -0.5f, -0.35f, 0.3f, 1.0f, -0.8f, 0.8f };

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
        else {
            // Show ammo bar when attempting to shoot
            showAmmoBar = true;
            ammoBarShowTime = glfwGetTime();

            // Only shoot if we have bullets
            if (bulletCount > 0) {
                bullets.push_back({ cowboyX + 0.15f, cowboyY - 0.05f, 0.8f });
                bulletCount--;
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

void drawCowboy(std::vector<GLfloat>& verts, float baseX, float baseY) {
    // Horse body
    addQuad(verts, baseX - 0.2f, baseY - 0.1f, baseX + 0.2f, baseY + 0.1f, 0.4f, 0.2f, 0.0f);
    // Horse legs
    addQuad(verts, baseX - 0.18f, baseY - 0.1f, baseX - 0.15f, baseY - 0.25f, 0.35f, 0.2f, 0.0f);
    addQuad(verts, baseX + 0.15f, baseY - 0.1f, baseX + 0.18f, baseY - 0.25f, 0.35f, 0.2f, 0.0f);
    // Horse head
    verts.insert(verts.end(), {
        baseX + 0.2f,baseY + 0.1f,0.0f,0.4f,0.2f,0.0f,
        baseX + 0.3f,baseY + 0.1f,0.0f,0.4f,0.2f,0.0f,
        baseX + 0.3f,baseY + 0.2f,0.0f,0.4f,0.2f,0.0f
        });
    // Cowboy body
    addQuad(verts, baseX - 0.04f, baseY + 0.1f, baseX + 0.04f, baseY + 0.22f, 0.1f, 0.1f, 0.1f);
    // Cowboy head
    addQuad(verts, baseX - 0.03f, baseY + 0.22f, baseX + 0.03f, baseY + 0.28f, 0.9f, 0.8f, 0.6f);
    // Cowboy hat
    verts.insert(verts.end(), {
        baseX - 0.05f,baseY + 0.28f,0.0f,0.3f,0.1f,0.0f,
        baseX + 0.05f,baseY + 0.28f,0.0f,0.3f,0.1f,0.0f,
        baseX,baseY + 0.32f,0.0f,0.3f,0.1f,0.0f
        });
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

        // === Movement: WASD and Arrow keys ===
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
            cowboyY += 0.4f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
            cowboyY -= 0.4f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
            cowboyX -= 0.4f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
            cowboyX += 0.4f * deltaTime;

        // === Jumping ===
        if (!isJumping && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            isJumping = true;
            jumpStartTime = currentTime;
        }
        float jumpY = 0.0f;
        if (isJumping) {
            float t = (float)(currentTime - jumpStartTime);
            if (t < 1.0f)
                jumpY = 0.25f * sin(t * M_PI);
            else
                isJumping = false;
        }

        for (auto& b : bullets) b.x += b.speed * deltaTime;
        bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet& b) {return b.x > 1.2f;}), bullets.end());

        // Hide ammo bar after duration
        if (showAmmoBar && (currentTime - ammoBarShowTime) > AMMO_BAR_DURATION) {
            showAmmoBar = false;
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
        for (int i = 0;i < segs;i++) {
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
        for (int i = 0;i < segCount;i++) {
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

        // --- Player Cowboy + Horse ---
        std::vector<GLfloat> hcVerts;
        float baseX = cowboyX;
        float baseY = cowboyY + jumpY - 0.2f;
        drawCowboy(hcVerts, baseX, baseY);

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

        // --- AI Cowboy + Horse ---
        std::vector<GLfloat> aiVerts;
        drawCowboy(aiVerts, aiCowboy.x, aiCowboy.y);

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

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
    glDeleteProgram(programID);
    glfwTerminate();
    return 0;
}