#include <GL/glut.h>    /*??? Linux ? Windows*/
float a = 0;
void reshape(int w, int h)
{
        glViewport(0, 0, w, h);
       
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, 0, h, -1000, 1000);
       
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
}
 
void display()
{
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
		   
        glTranslatef(400, 300, 500);
        glRotatef(a, 1, 0.5, 0);
        
        glBegin(GL_QUADS);
            glColor3f(1.0, 0.0, 0.0);
            glVertex3f(100, 100, 100);
            glVertex3f(-100, 100, 100);
            glVertex3f(-100, -100, 100);
            glVertex3f(100, -100, 100);
            glColor3f(0.0, 1.0, 1.0);
            glVertex3f(100, 100, -100);
            glVertex3f(-100, 100, -100);
            glVertex3f(-100, -100, -100);
            glVertex3f(100, -100, -100);
            glColor3f(0.0, 1.0, 0.0);
            glVertex3f(100, 100, -100);
            glVertex3f(-100, 100, -100);
            glVertex3f(-100, 100, 100);
            glVertex3f(100, 100, 100);
            glColor3f(1.0, 0.0, 1.0);
            glVertex3f(100, -100, -100);
            glVertex3f(-100, -100, -100);
            glVertex3f(-100, -100, 100);
            glVertex3f(100, -100, 100);
            glColor3f(0.0, 0.0, 1.0);
            glVertex3f(100, 100, -100);
            glVertex3f(100, -100, -100);
            glVertex3f(100, -100, 100);
            glVertex3f(100, 100, 100);
            glColor3f(1.0, 1.0, 0.0);
            glVertex3f(-100, 100, -100);
            glVertex3f(-100, -100, -100);
            glVertex3f(-100, -100, 100);
            glVertex3f(-100, 100, 100);
        glEnd();
       
        glutSwapBuffers();
        a += 0.5;
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

