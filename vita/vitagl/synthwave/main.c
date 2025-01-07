#include <vitaGL.h>
#include <vitasdk.h>
#include "synthwave.h"

float povX=0.0f;
float povY=0.0f;
float povZ=0.0f;

int main(){
	
	vglInit(0x800000);
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glEnable(GL_DEPTH_TEST);
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, 960.0f / 544.0f, 0.1f, 50.0f);
	glMatrixMode(GL_MODELVIEW);

	for (;;) {
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);
		povX += (pad.lx / 255.0f) * 0.1f; //lx -> left stick x
		povY += (pad.ly / 255.0f) * 0.1f; //ly -> left stick y
		if (pad.buttons & SCE_CTRL_TRIANGLE){povZ += 0.1f;}
		if (pad.buttons & SCE_CTRL_CROSS)   {povZ -= 0.1f;}

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();
		gluLookAt(povX,povY,3.0f+povZ,
				  0.0f,0.0f,0.0f,
				  0.0f,1.0f,0.0f
		);
	
		draw_grid();
		
		vglSwapBuffers(GL_FALSE);
	}

	vglEnd();
}
