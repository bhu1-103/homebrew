#include <vitaGL.h>
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
	glLoadIdentity();
	gluLookAt(povX,povY,3.0f+povZ,
			  0.0f,0.0f,0.0f,
			  0.0f,1.0f,0.0f
	);
	
	for (;;) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_grid();
		
		vglSwapBuffers(GL_FALSE);
	}

	vglEnd();
}
