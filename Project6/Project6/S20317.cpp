#include <glut.h>
#include <math.h>
#include <SOIL2.h>
#include <stdio.h>
#include <vector>
#include <random>
#include <chrono>

#define M_PI 3.14159265358979323846

GLuint poleTexture;
GLuint grassTexture;

// ---------------- Scene constants ----------------
const float RADIUS = 10.0f;      // hangar arch radius (before scaling)
const float LENGTH = 48.0f;      // hangar length (before scaling)
const float BASE_Y = -3.0f;

const int   SEG_ARC = 36;
const int   SEG_LEN = 40;

const int   TERRAIN_SIZE = 1600;     // bigger world so the apron doesn't fill it all
const int   TERRAIN_GRID_RES = 240;

const float TERRAIN_MIN_HEIGHT = 0.0f;

// ---- Apron / road layout (all in world units) ----
const float APRON_W = 900.0f;   // width (x)
const float APRON_H = 620.0f;   // depth (z)
const float APRON_Y = BASE_Y + TERRAIN_MIN_HEIGHT + 0.05f;
const float APRON_EDGE = 6.0f;    // white edge line width

// Road comes from negative X into the apron on its left side
const float ROAD_W = 120.0f;
const float ROAD_LEN = 800.0f;
const float ROAD_Y = APRON_Y;
const float ROAD_Z = -APRON_H * 0.30f;  // align roughly with left entrance in refs
const float ROAD_X0 = -APRON_W * 0.5f - ROAD_LEN; // start far left
const float ROAD_X1 = -APRON_W * 0.5f;             // meets apron

// Hangar placement (on the right half of the apron, like in the pictures)
const float HANGAR_X = +APRON_W * 0.28f;
const float HANGAR_Z = +APRON_H * 0.18f;
const float HANGAR_S = 5.0f;   // scale factor for your shell

// ---------------- Colors ----------------
float concreteColor[] = { 0.56f, 0.56f, 0.56f }; // apron slab
float roadColor[] = { 0.48f, 0.48f, 0.48f }; // slightly darker asphalt
float edgeColor[] = { 0.92f, 0.92f, 0.92f }; // white paint / curb line

float wallColor[] = { 0.70f, 0.70f, 0.70f };
float roofColor[] = { 0.80f, 0.80f, 0.80f };
float groundTint[] = { 0.22f, 0.55f, 0.16f }; // tint multiplier over grass tex
float doorColor[] = { 0.50f, 0.50f, 0.50f };

// New (for trees & mountains)
float treeTrunkColor[] = { 0.45f, 0.30f, 0.08f };
float treeCanopyColor[] = { 0.20f, 0.50f, 0.20f };
float mountainColor[] = { 0.40f, 0.40f, 0.40f };

// ---------------- Camera ----------------
float angle = 0.0f;
float camDistance = 1150.0f;
float camHeight = 480.0f;

// ---------------- Helpers ----------------
inline bool inRect(float x, float z, float cx, float cz, float w, float h, float margin = 0.0f) {
    return (x >= cx - w * 0.5f - margin) && (x <= cx + w * 0.5f + margin) &&
        (z >= cz - h * 0.5f - margin) && (z <= cz + h * 0.5f + margin);
}

void archPoint(float t, float& x, float& y) {
    x = RADIUS * cosf(t);
    y = RADIUS * sinf(t);
}

void loadTexture() {
    poleTexture = SOIL_load_OGL_texture(
        "wall.jpg",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );
    grassTexture = SOIL_load_OGL_texture(
        "grass.jpg",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );

    if (!poleTexture || !grassTexture) {
        printf("Texture load failed: %s\n", SOIL_last_result());
    }

    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, poleTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// ---------------- GL init ----------------
void init() {
    glEnable(GL_DEPTH_TEST);
    loadTexture();
    glClearColor(0.68f, 0.78f, 0.90f, 1.0f); // soft sky

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 200.0f, 300.0f, 120.0f, 1.0f };
    GLfloat lightColor[] = { 1.0f, 1.0f, 0.95f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glShadeModel(GL_SMOOTH);
}

// ---------------- Terrain (grassy hills) ----------------
void drawTerrain() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glColor3fv(groundTint);

    const float d = (float)TERRAIN_SIZE / TERRAIN_GRID_RES;
    const float offset = TERRAIN_SIZE * 0.5f;

    // small safety margin so no grass pokes through the apron edges
    const float A_MARGIN = 2.0f * APRON_EDGE + 6.0f;
    const float R_MARGIN = 4.0f;

    for (int i = 0; i < TERRAIN_GRID_RES; ++i) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= TERRAIN_GRID_RES; ++j) {
            float x1 = i * d - offset;
            float z1 = j * d - offset;

            float x2 = (i + 1) * d - offset;
            float z2 = j * d - offset;

            // Instead of skipping terrain, flatten it under apron/road
            bool insideApron1 = inRect(x1, z1, 0.0f, 0.0f, APRON_W, APRON_H, A_MARGIN);
            bool insideRoad1 = inRect(x1, z1, (ROAD_X0 + ROAD_X1) * 0.5f, ROAD_Z,
                (ROAD_X1 - ROAD_X0), ROAD_W, R_MARGIN);

            bool insideApron2 = inRect(x2, z2, 0.0f, 0.0f, APRON_W, APRON_H, A_MARGIN);
            bool insideRoad2 = inRect(x2, z2, (ROAD_X0 + ROAD_X1) * 0.5f, ROAD_Z,
                (ROAD_X1 - ROAD_X0), ROAD_W, R_MARGIN);

            // Heights
            float y1 = 6.0f * sinf(x1 * 0.0045f) + 5.0f * cosf(z1 * 0.0050f);
            y1 += 2.2f * sinf((x1 + z1) * 0.0032f);
            if (y1 < TERRAIN_MIN_HEIGHT) y1 = TERRAIN_MIN_HEIGHT;
            if (insideApron1 || insideRoad1) y1 = TERRAIN_MIN_HEIGHT;

            float y2 = 6.0f * sinf(x2 * 0.0045f) + 5.0f * cosf(z2 * 0.0050f);
            y2 += 2.2f * sinf((x2 + z2) * 0.0032f);
            if (y2 < TERRAIN_MIN_HEIGHT) y2 = TERRAIN_MIN_HEIGHT;
            if (insideApron2 || insideRoad2) y2 = TERRAIN_MIN_HEIGHT;

            // repeat texture a bit
            float u1 = x1 * 0.0025f;  float v1 = z1 * 0.0025f;
            float u2 = x2 * 0.0025f;  float v2 = z2 * 0.0025f;

            // simple upward normal (we’re not doing per-vertex normal calc here)
            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(u1, v1); glVertex3f(x1, BASE_Y + y1, z1);

            glNormal3f(0.0f, 1.0f, 0.0f);
            glTexCoord2f(u2, v2); glVertex3f(x2, BASE_Y + y2, z2);
        }
        glEnd();
    }

    glDisable(GL_TEXTURE_2D);
}

// ---------------- Concrete apron + road + edges ----------------
static void drawRectQuad(float cx, float cy, float cz, float w, float h) {
    float hx = w * 0.5f, hz = h * 0.5f;
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(cx - hx, cy, cz - hz);
    glVertex3f(cx + hx, cy, cz - hz);
    glVertex3f(cx + hx, cy, cz + hz);
    glVertex3f(cx - hx, cy, cz + hz);
    glEnd();
}

void drawApronAndRoad() {
    glDisable(GL_TEXTURE_2D);

    // Push apron/road slightly CLOSER in depth to beat the (coplanar) terrain
    glEnable(GL_POLYGON_OFFSET_FILL);
    // Negative values pull geometry toward the camera in depth
    glPolygonOffset(-2.0f, -4.0f);

    // --- Apron slab ---
    glColor3fv(concreteColor);
    drawRectQuad(0.0f, APRON_Y, 0.0f, APRON_W, APRON_H);

    // --- Entry road slab ---
    glColor3fv(roadColor);
    float rcx = (ROAD_X0 + ROAD_X1) * 0.5f;
    drawRectQuad(rcx, APRON_Y, ROAD_Z, (ROAD_X1 - ROAD_X0), ROAD_W);

    // Done biasing filled polys
    glDisable(GL_POLYGON_OFFSET_FILL);

    // --- White edge lines (lifted a tiny epsilon to avoid z-fight with apron) ---
    glColor3fv(edgeColor);
    const float edgeLift = 0.02f; // small vertical lift

    // left / right / near / far edges
    drawRectQuad(-APRON_W * 0.5f - APRON_EDGE * 0.5f, APRON_Y + edgeLift, 0.0f,
        APRON_EDGE, APRON_H + 2.0f * APRON_EDGE);
    drawRectQuad(+APRON_W * 0.5f + APRON_EDGE * 0.5f, APRON_Y + edgeLift, 0.0f,
        APRON_EDGE, APRON_H + 2.0f * APRON_EDGE);
    drawRectQuad(0.0f, APRON_Y + edgeLift, -APRON_H * 0.5f - APRON_EDGE * 0.5f,
        APRON_W + 2.0f * APRON_EDGE, APRON_EDGE);
    drawRectQuad(0.0f, APRON_Y + edgeLift, +APRON_H * 0.5f + APRON_EDGE * 0.5f,
        APRON_W + 2.0f * APRON_EDGE, APRON_EDGE);

    glEnable(GL_TEXTURE_2D);
}

// ---------------- Trees & Mountains (NEW) ----------------

// Simple tree: cylinder trunk + cone canopy
void drawTree(float x, float z, float scale = 1.0f) {
    glPushMatrix();
    glTranslatef(x, APRON_Y, z);
    glScalef(scale, scale, scale);

    glDisable(GL_TEXTURE_2D);

    // Trunk
    glColor3fv(treeTrunkColor);
    GLUquadric* quad = gluNewQuadric();
    gluCylinder(quad, 0.9, 0.9, 8.5, 10, 1);
    gluDeleteQuadric(quad);

    // Canopy
    glTranslatef(0.0f, 8.5f, 0.0f);
    glColor3fv(treeCanopyColor);
    glutSolidCone(5.5, 10.0, 12, 2);

    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

// Scatter trees outside the apron + road exclusion zone
void drawTrees() {
    // Exclusion bounds for apron+road with some margin
    const float MARGIN = 40.0f;

    std::mt19937 rng(12345); // deterministic
    std::uniform_real_distribution<float> dist(-TERRAIN_SIZE * 0.45f, TERRAIN_SIZE * 0.45f);
    std::uniform_real_distribution<float> sdist(0.8f, 1.4f);

    for (int i = 0; i < 350; ++i) {
        float x = dist(rng);
        float z = dist(rng);

        bool nearApron = inRect(x, z, 0.0f, 0.0f, APRON_W, APRON_H, MARGIN);
        bool nearRoad = inRect(x, z, (ROAD_X0 + ROAD_X1) * 0.5f, ROAD_Z,
            (ROAD_X1 - ROAD_X0), ROAD_W, MARGIN);
        if (nearApron || nearRoad) continue;

        drawTree(x, z, sdist(rng));
    }
}

// Simple mountain “peaks” (cones) arranged around the base
struct MountainPeak { float x, z, height, base_radius; };

void drawMountains() {
    glDisable(GL_TEXTURE_2D);
    glColor3fv(mountainColor);

    // Place a ring of peaks around the apron perimeter
    std::vector<MountainPeak> peaks;

    const float ringR = fmaxf(APRON_W, APRON_H) * 0.9f + 450.0f; // distance from center
    const int   count = 18;

    for (int i = 0; i < count; ++i) {
        float t = (float)i / count * 2.0f * (float)M_PI;
        MountainPeak p;
        p.x = cosf(t) * ringR;
        p.z = sinf(t) * ringR;
        p.base_radius = 70.0f + 25.0f * sinf(t * 3.0f);
        p.height = 160.0f + 40.0f * cosf(t * 2.0f);
        peaks.push_back(p);
    }

    for (const auto& pk : peaks) {
        glPushMatrix();
        glTranslatef(pk.x, APRON_Y, pk.z);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidCone(pk.base_radius, pk.height, 20, 3);
        glPopMatrix();
    }

    glEnable(GL_TEXTURE_2D);
}

// ---------------- Hangar (your shell + end walls) ----------------
void drawShell() {
    glEnable(GL_TEXTURE_2D);
    glColor3fv(roofColor);
    glBindTexture(GL_TEXTURE_2D, poleTexture);

    float z0 = -LENGTH * 0.5f;
    float dT = (float)M_PI / SEG_ARC;
    float dZ = LENGTH / SEG_LEN;

    for (int i = 0; i < SEG_ARC; ++i) {
        float t1 = i * dT, t2 = (i + 1) * dT;
        float x1, y1, x2, y2;
        archPoint(t1, x1, y1);
        archPoint(t2, x2, y2);

        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= SEG_LEN; ++j) {
            float z = z0 + j * dZ;

            float u1 = (float)i / SEG_ARC;
            float u2 = (float)(i + 1) / SEG_ARC;
            float v = (float)j / SEG_LEN;

            glNormal3f(cosf(t2), sinf(t2), 0);
            glTexCoord2f(u2, v);
            glVertex3f(x2, y2, z);

            glNormal3f(cosf(t1), sinf(t1), 0);
            glTexCoord2f(u1, v);
            glVertex3f(x1, y1, z);
        }
        glEnd();
    }
    glDisable(GL_TEXTURE_2D);
}

void drawEndWall(float zPos, bool withDoor) {
    glColor3fv(wallColor);
    float dT = (float)M_PI / SEG_ARC;
    for (int i = 0; i < SEG_ARC; ++i) {
        float t1 = i * dT, t2 = (i + 1) * dT;
        float x1, y1, x2, y2;
        archPoint(t1, x1, y1);
        archPoint(t2, x2, y2);

        glBegin(GL_QUADS);
        glNormal3f(0, 0, (zPos > 0) ? 1.0f : -1.0f);
        glVertex3f(x1, 0.0f, zPos);
        glVertex3f(x1, y1, zPos);
        glVertex3f(x2, y2, zPos);
        glVertex3f(x2, 0.0f, zPos);
        glEnd();
    }

    if (withDoor) {
        glColor3fv(doorColor);
        glBegin(GL_QUADS);
        glNormal3f(0, 0, (zPos > 0) ? 1.0f : -1.0f);
        glVertex3f(-4.0f, 0.0f, zPos + 0.01f);
        glVertex3f(4.0f, 0.0f, zPos + 0.01f);
        glVertex3f(4.0f, 7.0f, zPos + 0.01f);
        glVertex3f(-4.0f, 7.0f, zPos + 0.01f);
        glEnd();
    }
}

void drawHangarOnApron() {
    glPushMatrix();
    glTranslatef(HANGAR_X, APRON_Y, HANGAR_Z);
    glScalef(HANGAR_S, HANGAR_S, HANGAR_S);
    drawShell();
    drawEndWall(LENGTH * 0.5f, true);
    drawEndWall(-LENGTH * 0.5f, false);
    glPopMatrix();
}

// ---------------- Display ----------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float cx = camDistance * sinf(angle * (float)M_PI / 180.0f);
    float cz = camDistance * cosf(angle * (float)M_PI / 180.0f);
    gluLookAt(cx, camHeight, cz, 0.0f, APRON_Y + 5.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    drawTerrain();        // grassy base (flattened under apron/road)
    drawApronAndRoad();   // slabs on top (with polygon offset)
    drawHangarOnApron();  // hangar sitting on apron

    // New: ring the base with mountains + scatter trees (outside exclusion)
    //drawMountains();
    //drawTrees();

    glutSwapBuffers();
}

// ---------------- Reshape ----------------
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 1.0, 4000.0);
    glMatrixMode(GL_MODELVIEW);
}

// ---------------- Timer (optional auto-rotate) ----------------
void update(int value) {
    angle += 0.12f;
    if (angle > 360) angle -= 360;
    glutPostRedisplay();
    glutTimerFunc(25, update, 0);
}

// ---------------- Keyboard ----------------
void keyboard(unsigned char key, int, int) {
    switch (key) {
    case 'a': case 'A': angle -= 5.0f; break;
    case 'd': case 'D': angle += 5.0f; break;
    case 'w': case 'W': camDistance -= 20.0f; break;
    case 's': case 'S': camDistance += 20.0f; break;
    case 'q': case 'Q': camHeight += 10.0f; break;
    case 'e': case 'E': camHeight -= 10.0f; break;
    }
    glutPostRedisplay();
}

// ---------------- Main ----------------
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 800);
    glutCreateWindow("Military Base Apron - OpenGL/GLUT");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    //glutTimerFunc(25, update, 0); // enable for slow orbit

    glutMainLoop();
    return 0;
}
