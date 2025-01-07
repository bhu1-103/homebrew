#include "synthwave.h"
#include <math.h>

const int circle_resolution = 100;
const int sun_radius = 10;
const int sun_distance_far = 20;

void draw_grid()
{
	glColor3f(1.0f,0.0f,1.0f);
	for(float i=-2.0;i<2.0;i+=0.2)
	{
		glBegin(GL_LINES); //vertical lines
			glVertex3f(i,-2,0);
			glVertex3f(i,+2,0);
		glEnd();

		glBegin(GL_LINES); //horizontal lines
			glVertex3f(-2,i,0);
			glVertex3f(+2,i,0);
		glEnd();

	}
}
