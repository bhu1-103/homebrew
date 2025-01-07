#include "synthwave.h"
#include <math.h>

const int circle_resolution = 100;
const int sun_radius = 10;
const int sun_distance_far = 20;

void draw_grid()
{
	glLineWidth(0.2f);
	for(float i=-2.0;i<2.0;i+=0.2)
	{
		for(float j=-2.0;j<2.0;j+=0.2)
		{
			glBegin(GL_LINES); //vertical lines
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(i,0,0);
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(i,1,0);
			glEnd();

			glBegin(GL_LINES); //horizontal lines
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(0,j,0);
				glColor3f(1.0f,0.0f,1.0f);
				glVertex3f(1,j,0);
			glEnd();

		}
	}
}
