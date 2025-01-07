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
		glLineWidth(2.0f);
		gluLookAt(povX,povY,3.0f+povZ,
				  0.0f,0.0f,0.0f,
				  0.0f,1.0f,0.0f
		);
	
		draw_grid();
		
		vglSwapBuffers(GL_FALSE);
	}

	vglEnd();
}
