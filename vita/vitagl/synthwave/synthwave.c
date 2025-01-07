#include "synthwave.h"
#include <math.h>

const int circle_resolution = 100;
const int sun_radius = 10;
const int sun_distance_far = 20;

void draw_grid()
{
	glLineWidth(0.05f);
	for(float i=0;i<5.0;i+=0.1)
	{
		for(float j=0;j<5.0;j+=0.1)
		{
			glBegin(GL_LINES); //vertical lines
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(i,0,0);
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(i,1,0);
			glEnd();

			glBegin(GL_LINES); //horizontal lines
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(0,j,j/2);
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(1,j,j/2);
			glEnd();

		}
	}
}
