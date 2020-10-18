#include <GL/glut.h>    /*??? Linux ? Windows*/
float a = 0;
void reshape(int w, int h)
{
        glViewport(0, 0, w, h);
       
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0, w, 0, h);
       
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
}
 
void display()
{
        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();
        
        glTranslatef(400, 300, 0);
        glRotatef(a, 0, 0, 1);
        
        glBegin(GL_QUADS);
            glColor3f(1.0, 1.0, 1.0);
            glVertex2i(100, 100);
            glColor3f(0.0, 0.0, 1.0);
            glVertex2i(100, -100);
            glColor3f(0.0, 1.0, 0.0);
            glVertex2i(-100, -100);
            glColor3f(1.0, 0.0, 0.0);
            glVertex2i(-100, 100);
        glEnd();
       
        glutSwapBuffers();
        a += 0.1;
}

void timf(int value) // Timer function 
{ 
	    glutPostRedisplay(); // Redraw windows 
        glutTimerFunc(10, timf, 0); // Setup next timer 
}

 
int main (int argc, char * argv[])
{
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA);
       
        glutInitWindowSize(800, 600);
        glutCreateWindow("OpenGL lesson 1");
       
        glutReshapeFunc(reshape);
        glutDisplayFunc(display);
        
        glutTimerFunc(40, timf, 0); 
       
        glutMainLoop();
       
        return 0;
}

