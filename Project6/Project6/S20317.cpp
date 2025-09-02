

#include <glut.h>
#include <math.h>
#include <SOIL2.h>
#include <stdio.h>


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GLuint poleTexture;
GLuint grassTexture;

// ---------------- Scene constants ----------------
const float RADIUS = 10.0f;      // hangar arch radius (before scaling)
const float LENGTH = 48.0f;      // hangar length (before scaling)
const float BASE_Y = -3.0f;

const int   SEG_ARC = 36;
const int   SEG_LEN = 40;

// Make the world big so terrain feels “infinite-ish”
const int   TERRAIN_SIZE = 10000;        // wide world
const int   TERRAIN_GRID_RES = 600;      // higher res so it still looks smooth

const float TERRAIN_MIN_HEIGHT = 0.0f;

// ---- Apron / road layout (all in world units) ----
const float APRON_W = 900.0f;   // width (x)
const float APRON_H = 620.0f;   // depth (z)
const float APRON_Y = BASE_Y + TERRAIN_MIN_HEIGHT + 0.05f;
const float APRON_EDGE = 6.0f;  // white edge line width

// Road comes from negative X into the apron on its left side
const float ROAD_W = 120.0f;
const float ROAD_LEN = 800.0f;
const float ROAD_Y = APRON_Y;
const float ROAD_Z = -APRON_H * 0.30f;  // align roughly with left entrance
const float ROAD_X0 = -APRON_W * 0.5f - ROAD_LEN; // start far left (more negative)
const float ROAD_X1 = -APRON_W * 0.5f;            // meets apron

// Hangar placement (on the right half of the apron)
const float HANGAR_X = +APRON_W * 0.28f;
const float HANGAR_Z = +APRON_H * 0.18f;
const float HANGAR_S = 5.0f;   // scale factor

// ---------------- Colors ----------------
float concreteColor[] = { 0.56f, 0.56f, 0.56f }; // apron slab
float roadColor[] = { 0.48f, 0.48f, 0.48f }; // asphalt
float edgeColor[] = { 0.92f, 0.92f, 0.92f }; // paint

float wallColor[] = { 0.70f, 0.70f, 0.70f };
float roofColor[] = { 0.80f, 0.80f, 0.80f };
float groundTint[] = { 0.22f, 0.55f, 0.16f }; // tint multiplier over grass tex
float doorColor[] = { 0.50f, 0.50f, 0.50f };
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
        "wall.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y
    );
    grassTexture = SOIL_load_OGL_texture(
        "grass.jpg", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
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
    glEnable(GL_NORMALIZE); 

    // Global ambient
    GLfloat globalAmb[] = { 0.18f, 0.18f, 0.20f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);

    // Key light (sun)
    glEnable(GL_LIGHT0);
    GLfloat sunPos[] = { 200.0f, 300.0f, 120.0f, 1.0f };
    GLfloat sunDiff[] = { 1.00f, 1.00f, 0.95f, 1.0f };
    GLfloat sunSpec[] = { 1.00f, 1.00f, 1.00f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, sunPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, sunDiff);
    glLightfv(GL_LIGHT0, GL_SPECULAR, sunSpec);

    // Sky fill (directional)
    glEnable(GL_LIGHT1);
    GLfloat skyDir[] = { -0.2f, -1.0f, 0.1f, 0.0f }; // w=0 -> directional
    GLfloat skyDiff[] = { 0.25f, 0.32f, 0.45f, 1.0f };
    GLfloat skySpec[] = { 0.00f, 0.00f, 0.00f, 1.0f };
    glLightfv(GL_LIGHT1, GL_POSITION, skyDir);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, skyDiff);
    glLightfv(GL_LIGHT1, GL_SPECULAR, skySpec);

    
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    glShadeModel(GL_SMOOTH);
}

// ---------------- Isolated MESAS ----------------
struct MesaHill { float x, z, baseR, topR, height; };

const MesaHill HILLS[] = {
    { +1600.0f, +1400.0f, 520.0f, 0.35f * 520.0f, 260.0f },
    { +2200.0f, -1300.0f, 680.0f, 0.35f * 680.0f, 340.0f },
    { -1700.0f, +1800.0f, 600.0f, 0.35f * 600.0f, 300.0f },
    { -2400.0f, -1600.0f, 520.0f, 0.35f * 520.0f, 240.0f },
    {    200.0f, +2300.0f, 750.0f, 0.35f * 750.0f, 360.0f },
};
const int HILL_COUNT = sizeof(HILLS) / sizeof(HILLS[0]);

static inline float clamp01(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }
static inline float smoothstep(float a, float b, float x) {
    float t = clamp01((x - a) / (b - a));
    return t * t * (3.0f - 2.0f * t);
}

static inline float mesaHeight(float dx, float dz, float baseR, float topR, float peak) {
    float d = sqrtf(dx * dx + dz * dz);
    if (d >= baseR) return 0.0f;

    float ang = atan2f(dz, dx);
    float rimJitter = 1.0f + 0.06f * sinf(6.0f * ang + 0.7f)
        + 0.04f * cosf(11.0f * ang + 1.3f);
    float topRV = topR * rimJitter;
    float baseRV = baseR * (1.0f + 0.03f * sinf(5.0f * ang));

    if (d <= topRV) return peak; 

    float s = smoothstep(topRV, baseRV, d); 
    float radialNoise = 0.08f * sinf(18.0f * d / baseR + 2.1f);
    float fall = powf(1.0f - s, 2.2f) * (1.0f + radialNoise);
    return clamp01(fall) * peak;
}

// ---------------- Terrain (grassy base + mesas) ----------------
void drawTerrain() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, grassTexture);
    glColor3fv(groundTint);

    const float d = (float)TERRAIN_SIZE / TERRAIN_GRID_RES;
    const float offset = TERRAIN_SIZE * 0.5f;

    const float A_MARGIN = 2.0f * APRON_EDGE + 6.0f; // apron safe margin
    const float R_MARGIN = 4.0f;

    for (int i = 0; i < TERRAIN_GRID_RES; ++i) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int j = 0; j <= TERRAIN_GRID_RES; ++j) {
            float x1 = i * d - offset;
            float z1 = j * d - offset;

            float x2 = (i + 1) * d - offset;
            float z2 = j * d - offset;

            bool insideApron1 = inRect(x1, z1, 0.0f, 0.0f, APRON_W, APRON_H, A_MARGIN);
            bool insideRoad1 = inRect(x1, z1, (ROAD_X0 + ROAD_X1) * 0.5f, ROAD_Z,
                (ROAD_X1 - ROAD_X0), ROAD_W, R_MARGIN);

            bool insideApron2 = inRect(x2, z2, 0.0f, 0.0f, APRON_W, APRON_H, A_MARGIN);
            bool insideRoad2 = inRect(x2, z2, (ROAD_X0 + ROAD_X1) * 0.5f, ROAD_Z,
                (ROAD_X1 - ROAD_X0), ROAD_W, R_MARGIN);

            // Base undulations
            float y1 = 6.0f * sinf(x1 * 0.0045f) + 5.0f * cosf(z1 * 0.0050f);
            y1 += 2.2f * sinf((x1 + z1) * 0.0032f);

            float y2 = 6.0f * sinf(x2 * 0.0045f) + 5.0f * cosf(z2 * 0.0050f);
            y2 += 2.2f * sinf((x2 + z2) * 0.0032f);

            // Mesas
            for (int k = 0; k < HILL_COUNT; ++k) {
                y1 += mesaHeight(x1 - HILLS[k].x, z1 - HILLS[k].z,
                    HILLS[k].baseR, HILLS[k].topR, HILLS[k].height);
                y2 += mesaHeight(x2 - HILLS[k].x, z2 - HILLS[k].z,
                    HILLS[k].baseR, HILLS[k].topR, HILLS[k].height);
            }

            // Flatten for apron/road
            if (insideApron1 || insideRoad1) y1 = TERRAIN_MIN_HEIGHT;
            if (insideApron2 || insideRoad2) y2 = TERRAIN_MIN_HEIGHT;

            if (y1 < TERRAIN_MIN_HEIGHT) y1 = TERRAIN_MIN_HEIGHT;
            if (y2 < TERRAIN_MIN_HEIGHT) y2 = TERRAIN_MIN_HEIGHT;

            float u1 = x1 * 0.0025f, v1 = z1 * 0.0025f;
            float u2 = x2 * 0.0025f, v2 = z2 * 0.0025f;

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

    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-2.0f, -4.0f);

    // Apron slab
    glColor3fv(concreteColor);
    drawRectQuad(0.0f, APRON_Y, 0.0f, APRON_W, APRON_H);

    // Road slab
    glColor3fv(roadColor);
    float rcx = (ROAD_X0 + ROAD_X1) * 0.5f;
    drawRectQuad(rcx, APRON_Y, ROAD_Z, (ROAD_X1 - ROAD_X0), ROAD_W);

    glDisable(GL_POLYGON_OFFSET_FILL);

    // Edge paint (slightly lifted)
    glColor3fv(edgeColor);
    const float edgeLift = 0.02f;
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

// ---------------- Hangar (shell + end walls) ----------------
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
            glTexCoord2f(u2, v); glVertex3f(x2, y2, z);

            glNormal3f(cosf(t1), sinf(t1), 0);
            glTexCoord2f(u1, v); glVertex3f(x1, y1, z);
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
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(-2.0f, -2.0f);

        glColor3fv(doorColor);
        glBegin(GL_QUADS);
        glNormal3f(0, 0, (zPos > 0) ? 1.0f : -1.0f);
        glVertex3f(-4.0f, 0.0f, zPos);
        glVertex3f(4.0f, 0.0f, zPos);
        glVertex3f(4.0f, 7.0f, zPos);
        glVertex3f(-4.0f, 7.0f, zPos);
        glEnd();

        glDisable(GL_POLYGON_OFFSET_FILL);
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

// ================== MRAP VEHICLE (with materials & headlights) ==================
namespace MRAP {

    static inline void C(float r, float g, float b) { glColor3f(r, g, b); }
    static float g_wheelSpin = 0.0f; // degrees

    // Material helpers
    static void setSpec(float r, float g, float b, float shininess) {
        GLfloat spec[4] = { r, g, b, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, spec);
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }
    static void setEmission(float r, float g, float b) {
        GLfloat emi[4] = { r, g, b, 1.0f };
        glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emi);
    }
    static void clearEmission() { setEmission(0, 0, 0); }

    static void solidCylinder(float r0, float r1, float h, int slices = 18, int stacks = 1) {
        GLUquadric* q = gluNewQuadric();
        gluCylinder(q, r0, r1, h, slices, stacks);
        glPushMatrix(); gluDisk(q, 0.0, r0, slices, 1); glTranslatef(0, 0, h); gluDisk(q, 0.0, r1, slices, 1); glPopMatrix();
        gluDeleteQuadric(q);
    }
    static void box(float sx, float sy, float sz) { glPushMatrix(); glScalef(sx, sy, sz); glutSolidCube(1.0f); glPopMatrix(); }

    static void slitWindow(float w = 0.85f, float h = 0.42f) {
        C(0.18f, 0.22f, 0.26f);
        glBegin(GL_QUADS); glNormal3f(0, 0, 1);
        glVertex3f(-w * 0.5f, -h * 0.5f, 0); glVertex3f(w * 0.5f, -h * 0.5f, 0);
        glVertex3f(w * 0.5f, h * 0.5f, 0); glVertex3f(-w * 0.5f, h * 0.5f, 0);
        glEnd();
        C(0.06f, 0.06f, 0.06f); glLineWidth(2.f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-w * 0.5f, -h * 0.5f, 0); glVertex3f(w * 0.5f, -h * 0.5f, 0);
        glVertex3f(w * 0.5f, h * 0.5f, 0); glVertex3f(-w * 0.5f, h * 0.5f, 0);
        glEnd();
    }

    static void grenadeLauncher(float tiltDeg = 28.f) {
        glPushMatrix(); glRotatef(-tiltDeg, 1, 0, 0); C(0.16f, 0.16f, 0.17f);
        solidCylinder(0.11f, 0.11f, 0.9f, 12, 1);
        glTranslatef(0, 0, 0.9f); GLUquadric* q = gluNewQuadric(); gluDisk(q, 0, 0.11f, 12, 1); gluDeleteQuadric(q);
        glPopMatrix();
    }

    static void mirrorUnit() {
        C(0.08f, 0.08f, 0.08f);
        glPushMatrix(); solidCylinder(0.03f, 0.03f, 0.5f, 10, 1); glTranslatef(0, 0, 0.5f); box(0.35f, 0.45f, 0.08f); glPopMatrix();
    }

    static void wheel(float R = 0.9f, float W = 0.7f) {
        glPushMatrix();
        glRotatef(180, 1.0, 0, 0);       // cylinder axis -> Z
        glRotatef(g_wheelSpin, 0, 0, 1); // rolling
        setSpec(0.05f, 0.05f, 0.05f, 8.0f); // rubbery low spec
        C(0.06f, 0.06f, 0.06f);
        solidCylinder(R, R, W, 24, 1);

        for (int ring = 0; ring < 2; ++ring) {
            for (int i = 0; i < 18; ++i) {
                glPushMatrix();
                glRotatef(i * (360.0f / 18), 0, 0, 1);
                glTranslatef(R - 0.06f, 0, W * (0.25f + 0.5f * ring));
                glScalef(0.14f, 0.36f, 0.18f);
                glutSolidCube(1.0f);
                glPopMatrix();
            }
        }

        // Rim face
        setSpec(0.35f, 0.35f, 0.35f, 48.0f);
        C(0.18f, 0.18f, 0.18f);
        GLUquadric* q = gluNewQuadric();
        glPushMatrix(); glTranslatef(0, 0, 0.02f);      gluDisk(q, 0.0, R * 0.62f, 24, 1); glPopMatrix();
        glPushMatrix(); glTranslatef(0, 0, W - 0.02f);  gluDisk(q, 0.0, R * 0.62f, 24, 1); glPopMatrix();
        gluDeleteQuadric(q);
        glPopMatrix();
    }

    static void winch() {
        setSpec(0.35f, 0.35f, 0.35f, 48.0f);
        C(0.12f, 0.12f, 0.12f);
        glPushMatrix(); box(0.9f, 0.30f, 0.40f); glTranslatef(0, -0.05f, 0.35f); solidCylinder(0.08f, 0.08f, 0.9f, 12, 1); glPopMatrix();
    }

    // Configure/attach headlights as spotlights in vehicle local space
    static void setupHeadlights(bool on = true) {
        if (!on) { glDisable(GL_LIGHT2); glDisable(GL_LIGHT3); return; }

        glEnable(GL_LIGHT2);
        glEnable(GL_LIGHT3);

        // Warm headlight color
        GLfloat amb[] = { 0.00f, 0.00f, 0.00f, 1.0f };
        GLfloat diff[] = { 1.00f, 0.95f, 0.85f, 1.0f };
        GLfloat spec[] = { 1.00f, 1.00f, 1.00f, 1.0f };

        // Positions at the two bulbs (model units)
        GLfloat posR[] = { 3.2f, 1.2f, +1.1f, 1.0f };
        GLfloat posL[] = { 3.2f, 1.2f, -1.1f, 1.0f };

        // Point mostly forward (+X) with a small downward tilt
        GLfloat dirF[] = { 1.0f, -0.08f, 0.0f, };

        // Right
        glLightfv(GL_LIGHT2, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT2, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT2, GL_SPECULAR, spec);
        glLightfv(GL_LIGHT2, GL_POSITION, posR);
        glLightfv(GL_LIGHT2, GL_SPOT_DIRECTION, dirF);
        glLightf(GL_LIGHT2, GL_SPOT_CUTOFF, 20.0f);
        glLightf(GL_LIGHT2, GL_SPOT_EXPONENT, 8.0f);
        glLightf(GL_LIGHT2, GL_CONSTANT_ATTENUATION, 0.6f);
        glLightf(GL_LIGHT2, GL_LINEAR_ATTENUATION, 0.020f);
        glLightf(GL_LIGHT2, GL_QUADRATIC_ATTENUATION, 0.0010f);

        // Left
        glLightfv(GL_LIGHT3, GL_AMBIENT, amb);
        glLightfv(GL_LIGHT3, GL_DIFFUSE, diff);
        glLightfv(GL_LIGHT3, GL_SPECULAR, spec);
        glLightfv(GL_LIGHT3, GL_POSITION, posL);
        glLightfv(GL_LIGHT3, GL_SPOT_DIRECTION, dirF);
        glLightf(GL_LIGHT3, GL_SPOT_CUTOFF, 20.0f);
        glLightf(GL_LIGHT3, GL_SPOT_EXPONENT, 8.0f);
        glLightf(GL_LIGHT3, GL_CONSTANT_ATTENUATION, 0.6f);
        glLightf(GL_LIGHT3, GL_LINEAR_ATTENUATION, 0.020f);
        glLightf(GL_LIGHT3, GL_QUADRATIC_ATTENUATION, 0.0010f);
    }

    static void drawVehicle() {
        const float G = 1.0f; // ground clearance
        const float H0[3] = { 0.12f,0.13f,0.14f };
        const float H1[3] = { 0.16f,0.17f,0.19f };
        const float MET[3] = { 0.20f,0.21f,0.23f };

        // Hull & armor – moderate specular
        setSpec(0.25f, 0.25f, 0.25f, 32.0f);
        C(H0[0], H0[1], H0[2]); glPushMatrix(); glTranslatef(0, G + 0.9f, 0); box(7.2f, 1.6f, 2.9f); glPopMatrix();
        C(H1[0], H1[1], H1[2]); glPushMatrix(); glTranslatef(2.4f, G + 1.15f, 0); box(1.6f, 0.7f, 3.2f); glPopMatrix();
        C(H1[0], H1[1], H1[2]); glPushMatrix(); glTranslatef(-2.4f, G + 1.15f, 0); box(1.6f, 0.7f, 3.2f); glPopMatrix();

        C(H1[0], H1[1], H1[2]); glPushMatrix(); glTranslatef(-0.4f, G + 2.0f, 0); box(5.0f, 1.0f, 2.6f); glPopMatrix();

        // Sloped windshield block
        C(H0[0] * 1.05f, H0[1] * 1.05f, H0[2] * 1.05f);
        glPushMatrix(); glTranslatef(1.5f, G + 2.05f, 0); glRotatef(-20, 0, 0, 1); box(1.9f, 0.55f, 2.5f); glPopMatrix();

        // Hood plates
        C(H0[0], H0[1], H0[2]); glPushMatrix(); glTranslatef(2.7f, G + 1.55f, 0); box(1.3f, 0.35f, 2.5f); glPopMatrix();

        // Front bumper + winch (more metallic)
        setSpec(0.45f, 0.45f, 0.45f, 64.0f);
        C(MET[0], MET[1], MET[2]); glPushMatrix(); glTranslatef(3.7f, G + 1.0f, 0); box(0.9f, 0.7f, 2.8f); glPopMatrix();
        glPushMatrix(); glTranslatef(3.4f, G + 0.95f, 0); winch(); glPopMatrix();

        // Headlight bulbs with emission (small glow)
        glPushMatrix();
        setSpec(0.1f, 0.1f, 0.1f, 8.0f);
        glTranslatef(3.2f, G + 1.2f, 1.1f); setEmission(0.9f, 0.85f, 0.6f);  glutSolidSphere(0.18, 12, 10);
        glTranslatef(0, 0, -2.2f);         setEmission(0.9f, 0.85f, 0.6f);  glutSolidSphere(0.18, 12, 10);
        clearEmission();
        glPopMatrix();

        // Mirrors
        setSpec(0.2f, 0.2f, 0.2f, 24.0f);
        glPushMatrix(); glTranslatef(1.0f, G + 1.9f, 1.6f); mirrorUnit(); glPopMatrix();
        glPushMatrix(); glTranslatef(1.0f, G + 1.9f, -1.6f); mirrorUnit(); glPopMatrix();

        // Side door slab (left)
        C(H0[0] * 0.95f, H0[1] * 0.95f, H0[2] * 0.95f);
        glPushMatrix(); glTranslatef(-0.8f, G + 1.6f, 1.33f); box(1.35f, 1.15f, 0.06f); glPopMatrix();

        // Small side windows (4 per side)
        setSpec(0.05f, 0.05f, 0.08f, 12.0f);
        for (int s = -1; s <= 1; s += 2) {
            float z = s * 1.33f;
            for (int i = 0; i < 4; ++i) {
                glPushMatrix();
                glTranslatef(1.2f - i * 1.0f, G + 2.15f, z + 0.02f);
                if (s < 0) glRotatef(180, 0, 1, 0);
                slitWindow(0.7f, 0.42f);
                glPopMatrix();
            }
        }

        // Roof hatch/turret
        setSpec(0.25f, 0.25f, 0.25f, 32.0f);
        C(H0[0] * 1.1f, H0[1] * 1.1f, H0[2] * 1.1f);
        glPushMatrix(); glTranslatef(0.0f, G + 2.65f, 0.0f); box(1.4f, 0.7f, 1.2f); glPopMatrix();

        // Roof grenade launchers
        glPushMatrix(); glTranslatef(-0.2f, G + 2.55f, 0.6f);
        for (int i = 0; i < 3; ++i) { glPushMatrix(); glTranslatef(i * 0.38f, 0, 0); grenadeLauncher(30); glPopMatrix(); }
        glPopMatrix();

        // Rear doors panel
        C(H0[0], H0[1], H0[2]); glPushMatrix(); glTranslatef(-3.6f, G + 1.6f, 0); box(0.5f, 1.6f, 2.2f); glPopMatrix();

        // Wheels (RHS slightly tucked in)
        const float ax = 2.35f, az = 1.25f, insetR = 0.15f;
        glPushMatrix(); glTranslatef(ax, G, 2 - insetR); wheel(); glPopMatrix();
        glPushMatrix(); glTranslatef(ax, G, -az); wheel(); glPopMatrix();
        glPushMatrix(); glTranslatef(-ax, G, 2 - insetR); wheel(); glPopMatrix();
        glPushMatrix(); glTranslatef(-ax, G, -az); wheel(); glPopMatrix();

        // Mud flap
        setSpec(0.05f, 0.05f, 0.05f, 8.0f);
        C(0.08f, 0.08f, 0.08f); glPushMatrix(); glTranslatef(-3.5f, G + 0.6f, 0); box(0.1f, 0.3f, 0.9f); glPopMatrix();
    }

    // place MRAP at apron height with yaw+scale and update headlights
    static void drawAt(float x, float z, float yawDeg = 0.0f, float scale = 14.0f) {
        glPushMatrix();
        glTranslatef(x, APRON_Y, z);
        glRotatef(yawDeg, 0, 1, 0);
        glScalef(scale, scale, scale);

        // update headlight spotlights in vehicle space
        setupHeadlights(true);

        drawVehicle();
        glPopMatrix();
    }

} // namespace MRAP

// ================== MRAP motion state (reverse out, then drive in) ==================
float g_mrapX;                 // animated X along the road
const float g_mrapZ = ROAD_Z;    // road center
const float g_mrapYaw = 0.0f;      // face +X
const float g_mrapScale = 14.0f;

bool  g_reversePhase = true;       // true = reversing (toward ROAD_X0)
float g_mrapSpeed = 140.0f;     // world units per second
int   g_lastAnimMs = 0;

const float ROAD_START_X = ROAD_X1 - 20.0f;  // near apron edge
const float ROAD_END_X = ROAD_X0 + 20.0f;  // far left end

// ---------------- Display ----------------
void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    float cx = camDistance * sinf(angle * (float)M_PI / 180.0f);
    float cz = camDistance * cosf(angle * (float)M_PI / 180.0f);
    gluLookAt(cx, camHeight, cz, 0.0f, APRON_Y + 5.0f, 0.0f, 0.0f, 1.0f, 0.0f);

    drawTerrain();        // grassy base (with mesa mountains)
    drawApronAndRoad();   // slabs on top (with polygon offset)
    drawHangarOnApron();  // hangar sitting on apron

    // Animated MRAP
    MRAP::drawAt(g_mrapX, g_mrapZ, g_mrapYaw, g_mrapScale);

    glutSwapBuffers();
}

// ---------------- Reshape ----------------
void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, (float)w / (float)h, 1.0, 8000.0); // extended far plane
    glMatrixMode(GL_MODELVIEW);
}

// ---------------- Vehicle animation timer ----------------
void driveTick(int) {
    int ms = glutGet(GLUT_ELAPSED_TIME);
    if (!g_lastAnimMs) g_lastAnimMs = ms;
    float dt = (ms - g_lastAnimMs) / 1000.0f;
    g_lastAnimMs = ms;

    // Move along X: reverse = toward negative; forward = toward start
    float dir = g_reversePhase ? -1.0f : +1.0f;
    g_mrapX += dir * g_mrapSpeed * dt;

    // Wheel spin from linear velocity (circumference = 2*pi*Rworld)
    const float wheelR_world = 0.9f * g_mrapScale; // 0.9 is wheel radius in model units
    const float circumference = 2.0f * (float)M_PI * wheelR_world;
    float rotDegPerSec = (g_mrapSpeed / circumference) * 360.0f;
    MRAP::g_wheelSpin += dir * rotDegPerSec * dt;
    if (MRAP::g_wheelSpin > 360.0f) MRAP::g_wheelSpin -= 360.0f;
    if (MRAP::g_wheelSpin < 0.0f) MRAP::g_wheelSpin += 360.0f;

    // Turnaround logic
    if (g_reversePhase && g_mrapX <= ROAD_END_X) {
        g_mrapX = ROAD_END_X;
        g_reversePhase = false; // drive forward
    }
    else if (!g_reversePhase && g_mrapX >= ROAD_START_X) {
        g_mrapX = ROAD_START_X;
        g_reversePhase = true;  // reverse again
    }

    glutPostRedisplay();
    glutTimerFunc(16, driveTick, 0); // ~60 FPS
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
    glutCreateWindow("3D Military Base");

    init();

    // Initialize MRAP start position and animation clock
    g_mrapX = ROAD_START_X;   // start near apron edge, on road center
    g_lastAnimMs = glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);

   
    glutTimerFunc(16, driveTick, 0);

    glutMainLoop();
    return 0;
}
