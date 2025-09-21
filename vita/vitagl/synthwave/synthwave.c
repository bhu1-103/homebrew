#include "synthwave.h"
#include <math.h>

const int circle_resolution = 100;
const int sun_radius = 10;
const int sun_distance_far = 20;

void draw_grid()
{
    float size = 20.0f;
    float step = 1.0f;
    glColor3f(1.0f,0.0f,1.0f);

    for(float i=-size; i<=size; i+=step)
    {
        glBegin(GL_LINES);
        glVertex3f(i, 0.0f, -size);
        glVertex3f(i, 0.0f,  size);

        glVertex3f(-size, 0.0f, i);
        glVertex3f( size, 0.0f, i);
        glEnd();
    }
}
