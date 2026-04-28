/*
 * 3D Maze Wireframe - C / OpenGL + GLFW
 * 
 * Build:
 *   gcc -o maze maze.c -lglfw -lGL -lGLEW -lm
 *
 * Controls:
 *   WASD  - Move
 *   Mouse - Look around
 *   ESC   - Quit
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/* ── Configuration ── */
#define MAZE_SIZE   10
#define CELL_SIZE   4.0f
#define WALL_HEIGHT 3.0f
#define MOVE_SPEED  8.0f
#define MOUSE_SENS  0.002f
#define PLAYER_H    1.6f
#define COLLISION_R 0.4f

/* ── Maze Layout: 1 = wall, 0 = path ── */
static const int layout[MAZE_SIZE][MAZE_SIZE] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1},
};

/* ── Player State ── */
static float playerX, playerY, playerZ;
static float yaw = 0.0f;
static float pitch = 0.0f;
static double lastMouseX, lastMouseY;
static int firstMouse = 1;

/* ── Input State ── */
static int keyW = 0, keyA = 0, keyS = 0, keyD = 0;

/* ── Collision Detection ── */
static int collides(float x, float z) {
    for (int gz = 0; gz < MAZE_SIZE; gz++) {
        for (int gx = 0; gx < MAZE_SIZE; gx++) {
            if (layout[gz][gx] != 1) continue;
            float wx = gx * CELL_SIZE;
            float wz = gz * CELL_SIZE;
            float half = CELL_SIZE / 2.0f + COLLISION_R;
            if (fabsf(x - wx) < half && fabsf(z - wz) < half) {
                return 1;
            }
        }
    }
    return 0;
}

/* ── Draw a Wireframe Box ── */
static void draw_wire_box(float cx, float cy, float cz, float sx, float sy, float sz) {
    float hx = sx / 2.0f, hy = sy / 2.0f, hz = sz / 2.0f;
    float x0 = cx - hx, x1 = cx + hx;
    float y0 = cy - hy, y1 = cy + hy;
    float z0 = cz - hz, z1 = cz + hz;

    /* 8 vertices of a box */
    float v[8][3] = {
        {x0,y0,z0}, {x1,y0,z0}, {x1,y1,z0}, {x0,y1,z0},
        {x0,y0,z1}, {x1,y0,z1}, {x1,y1,z1}, {x0,y1,z1}
    };

    /* 12 edges */
    int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, /* front */
        {4,5},{5,6},{6,7},{7,4}, /* back  */
        {0,4},{1,5},{2,6},{3,7}  /* sides */
    };

    glBegin(GL_LINES);
    for (int i = 0; i < 12; i++) {
        glVertex3fv(v[edges[i][0]]);
        glVertex3fv(v[edges[i][1]]);
    }
    glEnd();
}

/* ── Draw the Floor Grid ── */
static void draw_floor(void) {
    float total = MAZE_SIZE * CELL_SIZE;
    glColor3f(0.15f, 0.15f, 0.15f);
    glBegin(GL_LINES);
    for (float i = 0; i <= total; i += CELL_SIZE) {
        /* X lines */
        glVertex3f(0, 0, i - CELL_SIZE / 2.0f);
        glVertex3f(total - CELL_SIZE, 0, i - CELL_SIZE / 2.0f);
        /* Z lines */
        glVertex3f(i - CELL_SIZE / 2.0f, 0, 0);
        glVertex3f(i - CELL_SIZE / 2.0f, 0, total - CELL_SIZE);
    }
    glEnd();
}

/* ── Callbacks ── */
static void key_callback(GLFWwindow *w, int key, int scancode, int action, int mods) {
    (void)scancode; (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(w, GLFW_TRUE);

    int state = (action != GLFW_RELEASE);
    if (key == GLFW_KEY_W) keyW = state;
    if (key == GLFW_KEY_A) keyA = state;
    if (key == GLFW_KEY_S) keyS = state;
    if (key == GLFW_KEY_D) keyD = state;
}

static void cursor_callback(GLFWwindow *w, double xpos, double ypos) {
    (void)w;
    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = 0;
        return;
    }
    float dx = (float)(xpos - lastMouseX);
    float dy = (float)(ypos - lastMouseY);
    lastMouseX = xpos;
    lastMouseY = ypos;

    yaw   -= dx * MOUSE_SENS;
    pitch -= dy * MOUSE_SENS;

    /* Clamp pitch */
    if (pitch >  1.5f) pitch =  1.5f;
    if (pitch < -1.5f) pitch = -1.5f;
}

static void resize_callback(GLFWwindow *w, int width, int height) {
    (void)w;
    if (height == 0) height = 1;
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    float aspect = (float)width / (float)height;
    float fov = 75.0f * (3.14159265f / 180.0f);
    float near = 0.1f, far = 100.0f;
    float top = near * tanf(fov / 2.0f);
    float right = top * aspect;
    glFrustum(-right, right, -top, top, near, far);

    glMatrixMode(GL_MODELVIEW);
}

/* ── Main ── */
int main(void) {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return 1;
    }

    GLFWwindow *window = glfwCreateWindow(1280, 720, "3D Maze Wireframe", NULL, NULL);
    if (!window) {
        fprintf(stderr, "Failed to create window\n");
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glewInit();

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetFramebufferSizeCallback(window, resize_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.02f, 0.02f, 0.02f, 1.0f);

    /* Trigger initial resize */
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    resize_callback(window, w, h);

    /* Start position: maze cell (1,1) */
    playerX = 1.0f * CELL_SIZE;
    playerY = PLAYER_H;
    playerZ = 1.0f * CELL_SIZE;

    double lastTime = glfwGetTime();

    /* ── Game Loop ── */
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;

        /* ── Movement ── */
        float forwardX = -sinf(yaw);
        float forwardZ = -cosf(yaw);
        float rightX   =  cosf(yaw);
        float rightZ   = -sinf(yaw);

        float dx = 0.0f, dz = 0.0f;
        if (keyW) { dx += forwardX; dz += forwardZ; }
        if (keyS) { dx -= forwardX; dz -= forwardZ; }
        if (keyD) { dx += rightX;   dz += rightZ;   }
        if (keyA) { dx -= rightX;   dz -= rightZ;   }

        /* Normalize diagonal movement */
        float len = sqrtf(dx * dx + dz * dz);
        if (len > 0.0f) {
            dx = (dx / len) * MOVE_SPEED * dt;
            dz = (dz / len) * MOVE_SPEED * dt;
        }

        /* Axis-separated collision (slide along walls) */
        float newX = playerX + dx;
        float newZ = playerZ + dz;
        if (!collides(newX, playerZ)) playerX = newX;
        if (!collides(playerX, newZ)) playerZ = newZ;

        /* ── Render ── */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        /* Camera look direction */
        float lookX = playerX + forwardX * cosf(pitch);
        float lookY = playerY + sinf(pitch);
        float lookZ = playerZ + forwardZ * cosf(pitch);

        /* gluLookAt equivalent (manual) */
        float fx = lookX - playerX, fy = lookY - playerY, fz = lookZ - playerZ;
        float flen = sqrtf(fx*fx + fy*fy + fz*fz);
        fx /= flen; fy /= flen; fz /= flen;

        float sx = fy * 0 - fz * 0, sy = fz * 0 - fx * 0, sz = fx * 0 - fy * 0;
        /* Use proper up vector cross */
        sx = fy * 0.0f - fz * 0.0f; /* This needs proper cross product */

        /* Simpler: use gluLookAt-style with manual matrix */
        {
            float up[3] = {0, 1, 0};
            float s[3], u[3];
            /* s = f x up */
            s[0] = fx * up[2] - fz * up[1]; /* fy*upz - fz*upy but up=(0,1,0) */
            s[0] = fy * up[2] - fz * up[1];
            s[1] = fz * up[0] - fx * up[2];
            s[2] = fx * up[1] - fy * up[0];
            float slen = sqrtf(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
            if (slen > 0) { s[0]/=slen; s[1]/=slen; s[2]/=slen; }

            /* u = s x f */
            u[0] = s[1]*fz - s[2]*fy;
            u[1] = s[2]*fx - s[0]*fz;
            u[2] = s[0]*fy - s[1]*fx;

            float m[16] = {
                s[0], u[0], -fx, 0,
                s[1], u[1], -fy, 0,
                s[2], u[2], -fz, 0,
                0,    0,     0,  1
            };
            glMultMatrixf(m);
            glTranslatef(-playerX, -playerY, -playerZ);
        }

        /* Draw floor */
        draw_floor();

        /* Draw maze walls */
        glColor3f(0.0f, 1.0f, 0.0f);
        for (int z = 0; z < MAZE_SIZE; z++) {
            for (int x = 0; x < MAZE_SIZE; x++) {
                if (layout[z][x] == 1) {
                    draw_wire_box(
                        x * CELL_SIZE, WALL_HEIGHT / 2.0f, z * CELL_SIZE,
                        CELL_SIZE, WALL_HEIGHT, CELL_SIZE
                    );
                }
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
