#include <GL/glut.h>    /*??? Linux ? Windows*/
#include <math.h>
float angle = 0;
void reshape(int w, int h)
{
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glViewport(0, 0, w, h);
        float ratio = w/(1.0*h);
	    gluPerspective(45, ratio, 1, 1000);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
}

void draw_cube (float x, float y, float z, float a)
{		   
        glTranslatef(x, y, z);
        glRotatef(angle, 1, 0.5, 0);
        
        glBegin(GL_QUADS);
            glColor3f(1.0, 0.0, 0.0);
            glVertex3f(a, a, a);
            glVertex3f(-a, a, a);
            glVertex3f(-a, -a, a);
            glVertex3f(a, -a, a);
            glColor3f(0.0, 1.0, 1.0);
            glVertex3f(a, a, -a);
            glVertex3f(-a, a, -a);
            glVertex3f(-a, -a, -a);
            glVertex3f(a, -a, -a);
            glColor3f(0.0, 1.0, 0.0);
            glVertex3f(a, a, -a);
            glVertex3f(-a, a, -a);
            glVertex3f(-a, a, a);
            glVertex3f(a, a, a);
            glColor3f(1.0, 0.0, 1.0);
            glVertex3f(a, -a, -a);
            glVertex3f(-a, -a, -a);
            glVertex3f(-a, -a, a);
            glVertex3f(a, -a, a);
            glColor3f(0.0, 0.0, 1.0);
            glVertex3f(a, a, -a);
            glVertex3f(a, -a, -a);
            glVertex3f(a, -a, a);
            glVertex3f(a, a, a);
            glColor3f(1.0, 1.0, 0.0);
            glVertex3f(-a, a, -a);
            glVertex3f(-a, -a, -a);
            glVertex3f(-a, -a, a);
            glVertex3f(-a, a, a);
        glEnd();
}
 
void display()
{
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
        
        for (int i=-10; i<10; i++)
        {
            glPushMatrix();
            draw_cube (i*20, 50*sin(3.14*i/10.0), -500, 10);        
            glPopMatrix();
        }
       
        glutSwapBuffers();
        angle += 0.5;
}

void timf(int value) // Timer function 
{ 
	    glutPostRedisplay(); // Redraw windows 
        glutTimerFunc(10, timf, 0); // Setup next timer 
}

 
int main (int argc, char * argv[])
{
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DEPTH| GLUT_DOUBLE|GLUT_RGBA);
       
        glutInitWindowSize(800, 600);
        glutCreateWindow("OpenGL lesson 1");
	    glEnable(GL_DEPTH_TEST);
       	glClearDepth(1.0f);						
       
        glutReshapeFunc(reshape);
        glutDisplayFunc(display);
        
        glutTimerFunc(40, timf, 0); 
       
        glutMainLoop();
       
        return 0;
}

