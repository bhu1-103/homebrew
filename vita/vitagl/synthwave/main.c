#include <vitaGL.h>
#include <vitasdk.h>
#include <math.h>
#include "synthwave.h"

float camX = 0.0f, camY = 0.0f, camZ = 3.0f;
float yaw = -90.0f;
float pitch = 0.0f;
float moveSpeed = 0.05f;
float lookSpeed = 1.0f;

float radians(float deg) {
    return deg * M_PI / 180.0f;
}

void getFrontVector(float* frontX, float* frontY, float* frontZ) {
    *frontX = cosf(radians(yaw)) * cosf(radians(pitch));
    *frontY = sinf(radians(pitch));
    *frontZ = sinf(radians(yaw)) * cosf(radians(pitch));
}

int main() {
    vglInit(0x800000);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 960.0f / 544.0f, 0.1f, 50.0f);
    glMatrixMode(GL_MODELVIEW);

    for (;;) {
        sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
        SceCtrlData pad;
        sceCtrlPeekBufferPositive(0, &pad, 1);

        float rx = pad.rx - 128.0f;
        float ry = pad.ry - 128.0f;

        yaw   += rx * lookSpeed / 128.0f;
        pitch += -ry * lookSpeed / 128.0f;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        float frontX, frontY, frontZ;
        getFrontVector(&frontX, &frontY, &frontZ);

        float deadzone = 16.0f;
        float lx = pad.lx - 128.0f;
        float ly = pad.ly - 128.0f;

        if (fabs(lx) < deadzone) lx == 0.0f;
        if (fabs(ly) < deadzone) ly == 0.0f;

        camX += frontX * (-ly / 128.0f) * moveSpeed;
        camY = 1.0f;//+= frontY * (ly / 128.0f) * moveSpeed;
        camZ += frontZ * (-ly / 128.0f) * moveSpeed;

        float rightX = -frontZ;
        float rightZ = frontX;
        camX += rightX * (lx / 128.0f) * moveSpeed;
        camZ += rightZ * (lx / 128.0f) * moveSpeed;

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        gluLookAt(
            camX, camY, camZ,
            camX + frontX, camY + frontY, camZ + frontZ,
            0.0f, 1.0f, 0.0f
        );

        draw_grid();

        vglSwapBuffers(GL_FALSE);

        sceKernelDelayThread(16666);
    }

    vglEnd();
}
