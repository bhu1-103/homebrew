#include <vitaGL.h>
#include <vitasdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 1000
#define MAX_INDICES 1000
float povX=0.0f;
float povY=0.0f;
float povZ=0.0f;

float vertices[MAX_VERTICES * 3];
unsigned short indices[MAX_INDICES * 3];

int numVertices = 0;
int numFaces = 0;

int loadOBJ(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open OBJ file: %s\n", filename);
        return -1;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == 'v' && line[1] == ' ') {
            float x, y, z;
            sscanf(line, "v %f %f %f", &x, &y, &z);
            vertices[numVertices * 3] = x;
            vertices[numVertices * 3 + 1] = y;
            vertices[numVertices * 3 + 2] = z;
            numVertices++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            unsigned short a, b, c;
            sscanf(line, "f %hu %hu %hu", &a, &b, &c);
            indices[numFaces * 3] = a - 1;
            indices[numFaces * 3 + 1] = b - 1;
            indices[numFaces * 3 + 2] = c - 1;
            numFaces++;
        }
    }

    fclose(file);
    return 0;
}

int main() {
    vglInit(0x800000);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glEnable(GL_DEPTH_TEST);

    if (loadOBJ("ux0:data/obj_loader/model.obj") != 0) {
        printf("Failed to load model!\n");
        return -1;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 960.0f / 544.0f, 0.1f, 50.0f);
    glMatrixMode(GL_MODELVIEW);

    for (;;) {
		sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		if(!(pad.lx<136 && pad.lx>120))
		{
			povX += ((pad.lx-128.0f) / 128.0f) * 0.1f;
		}
		if(!(pad.ly<136 && pad.ly>120))
		{
			povY += ((pad.ly-128.0f) / 128.0f) * 0.1f;
		}
		if (pad.buttons & SCE_CTRL_TRIANGLE){povZ += 0.1f;}
		if (pad.buttons & SCE_CTRL_CROSS)   {povZ -= 0.1f;}

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glLoadIdentity();
        gluLookAt(povX, povY, 3.0f+povZ,
				  0.0f, 0.0f, 0.0f,
				  0.0f, 1.0f, 0.0f);

        glEnableClientState(GL_VERTEX_ARRAY);

        glVertexPointer(3, GL_FLOAT, 0, vertices);
        
        glDrawElements(GL_TRIANGLES, numFaces * 3, GL_UNSIGNED_SHORT, indices);
        
        vglSwapBuffers(GL_FALSE);
    }

    vglEnd();
    return 0;
}
