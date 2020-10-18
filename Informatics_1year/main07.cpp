#include <GL/glut.h>    
#include <iostream>    
#include <math.h>    
#include <time.h>    
#define N 10
#define CELL 20
#define PN 1000
using namespace std;
float angle = 0.0f, lx = 0, lz = -1.0, x = 0, z = 5, dangle = 0, dmove = 0;
int fv = 1, xOrigin = -1;
float color[N][N][3];
float snowangle[N][N];
char map[N][N] = {	{'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w'}, 
			{'w', ' ', 'w', ' ', ' ', 'b', ' ', 'r', ' ', 'w'},
			{'w', 'b', 'w', ' ', 'w', ' ', ' ', 'w', 'g', 'w'},
			{'w', ' ', 'w', ' ', 'w', 'w', 'w', 'w', ' ', 'w'},
			{'w', ' ', 'w', 'r', ' ', ' ', ' ', 'w', ' ', 'w'},
			{'w', ' ', 'w', ' ', ' ', ' ', ' ', 'w', 'g', 'w'}, 
			{'w', 'g', 'w', ' ', 'g', ' ', ' ', ' ', ' ', 'w'},
			{'w', ' ', 'w', ' ', 'w', 'w', 'w', 'w', 'w', 'w'},
			{'w', ' ', 'b', ' ', ' ', ' ', ' ', ' ', ' ', 'w'},
			{'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w'},
		 };
int n_empty=0, n_snowman=0;
GLfloat LightAmbient[]= { 0.5f, 0.5f, 0.5f, 1.0f }; // ???????? ???????? ????? ( ????? )
GLfloat LightDiffuse[]= { 1.0f, 1.0f, 1.0f, 1.0f }; // ???????? ?????????? ????? ( ????? )
GLfloat LightPosition[]= { 0.0f, 0.0f, 2.0f, 1.0f };     // ??????? ????? ( ????? )

struct particle 
{
	float x, y, z;	
	float vx, vy, vz;
	float color[3];
	float lifetime;
} set[PN];
float set_c[3] = {0};
 
void p_init(particle *s, float x, float y, float z, float r, float g, float b)
{
	set_c[0] = x; set_c[1] = y; set_c[2] = z;
	for (int i=0; i<PN; i++)
	{
		s[i].x = s[i].y = s[i].z = 0;
		s[i].vx = 1.0*(rand()%100/100.0 - 0.5);
		s[i].vy = 1.0*(rand()%100/100.0 - 0.5);
		s[i].vz = 1.0*(rand()%100/100.0 - 0.5);
		s[i].color[0] = r*(rand()%100)/100.0;
		s[i].color[1] = g*(rand()%100)/100.0;
		s[i].color[2] = b*(rand()%100)/100.0;
		s[i].lifetime = 1.0;
	}
}

void p_step(particle *s)
{
	for (int i=0; i<PN; i++)
	{
		s[i].x += s[i].vx;
		s[i].y += s[i].vy;
		s[i].z += s[i].vz;
		s[i].lifetime -= 0.01;
	}
}

void p_draw(particle *s)
{	
	glTranslatef(set_c[0], set_c[1], set_c[2]);	
	for (int i=0; i<PN; i++)
	{
		if (s[i].lifetime < 0) 
		{
			if (n_snowman <=0)
				exit(0);
			continue;
		}
		glPushMatrix();
		glTranslatef(s[i].x, s[i].y, s[i].z);	
		glColor3f(s[i].color[0], s[i].color[1], s[i].color[2]);	
		glRotatef(s[i].lifetime*360, 0.0f, 1.0f, 0.0f);
		glRotatef(s[i].lifetime*360, 1.0f, 0.0f, 0.0f);
		glBegin(GL_TRIANGLES);
			glVertex3f(-s[i].lifetime*0.05, 0, 0);
			glVertex3f(0, s[i].lifetime*0.05, 0);
			glVertex3f(s[i].lifetime*0.05, 0, 0);
		glEnd();	
		glPopMatrix();
	}
}
void drawSnowman(int i, int j)
{
	glTranslatef(0, 0.5*(sin(snowangle[i][j])+1), 0);
	snowangle[i][j] += 0.5 + 0.25*(rand()%100)/100.0;
	glColor3f(1, 1, 1);
	glTranslatef(0, 0.75f, 0);
	if (map[i][j] == 'r')
		glColor3f(1, 0, 0);
	else if (map[i][j] == 'g')
		glColor3f(0, 1, 0);	
	else if (map[i][j] == 'b')
		glColor3f(0, 0, 1);	
	else return;
	glutSolidSphere(0.75f, 20, 20);
	glTranslatef(0, 1, 0);	
	glutSolidSphere(0.25f, 20, 20);
	glPushMatrix();
	glColor3f(1, 0, 0);
	glTranslatef(0.05f, 0.1f, 0.18f);	
	glutSolidSphere(0.05f, 10, 10);
	glTranslatef(-0.1f, 0, 0);	
	glutSolidSphere(0.05f, 20, 20);
	glPopMatrix();
	glColor3f(1, 0.5f, 0.5f);
	glRotatef(0, 1.0f, 0.0f, 0.0f);
	glutSolidCone(0.08f,0.5f,10,2);
}
 
void drawMap()
{	
	glColor3f(0.75, 1, 0.5);
        glBegin(GL_QUADS);
        glVertex3f(-N*CELL/2, 0, -N*CELL/2);
        glVertex3f(-N*CELL/2, 0,  N*CELL/2);
        glVertex3f( N*CELL/2, 0,  N*CELL/2);
        glVertex3f( N*CELL/2, 0, -N*CELL/2);
        glEnd();
	for (int i=-N/2; i<N/2; i++)
		for (int j=-N/2; j<N/2; j++)
			if (map[i+N/2][j+N/2] == 'w')
			{
				glColor3f(color[i+N/2][j+N/2][0]+0.1, color[i+N/2][j+N/2][1]+0.1, color[i+N/2][j+N/2][2]+0.1);	
				glBegin(GL_QUADS);
				glVertex3f( i*CELL+0.1, 0, (j+1)*CELL-0.1);
				glVertex3f( i*CELL+0.1, 0,  j*CELL+0.1);
				glVertex3f( i*CELL+0.1, CELL,  j*CELL+0.1);
				glVertex3f( i*CELL+0.1, CELL,  (j+1)*CELL-0.1);

				glVertex3f( i*CELL+0.1, 0, j*CELL+0.1);
				glVertex3f( (i+1)*CELL-0.1, 0,  j*CELL+0.1);
				glVertex3f( (i+1)*CELL-0.1, CELL,  j*CELL+0.1);
				glVertex3f( i*CELL+0.1, CELL,  j*CELL+0.1);

				glVertex3f( (i+1)*CELL-0.1, 0, j*CELL+0.1);
				glVertex3f( (i+1)*CELL-0.1, 0,  (j+1)*CELL-0.1);
				glVertex3f( (i+1)*CELL-0.1, CELL,  (j+1)*CELL-0.1);
				glVertex3f( (i+1)*CELL-0.1, CELL,  j*CELL+0.1);

				glVertex3f( (i+1)*CELL-0.1, 0, (j+1)*CELL-0.1);
				glVertex3f( i*CELL+0.1, 0,  (j+1)*CELL-0.1);
				glVertex3f( i*CELL+0.1, CELL,  (j+1)*CELL-0.1);
				glVertex3f( (i+1)*CELL-0.1, CELL,  (j+1)*CELL-0.1);
				glEnd();				
			}
}

void reshape(int w, int h)
{
	float ratio = w/(1.0*h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
	glViewport(0, 0, w, h);
	gluPerspective(45, ratio, 0.1, 1000);
       
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
}
 
void comPos(float dm)
{
	int 	i = (x + dm * lx * 0.1)/CELL + N/2,
		j = (z + dm * lz * 0.1)/CELL + N/2;
	if (map[i][j] == 'w')
		return;
	if (map[i][j] == 'g')
	{
		map[i][j] = ' ';
		p_init(set, (i-N/2+0.5)*CELL, 0.5, (j-N/2+0.5)*CELL, 0, 1, 0);
		n_snowman--;
	}
	if (map[i][j] == 'r')
	{
		map[i][j] = ' ';
		p_init(set, (i-N/2+0.5)*CELL, 0.5, (j-N/2+0.5)*CELL, 1, 0, 0);
		n_snowman--;
	}
	if (map[i][j] == 'b')
	{
		map[i][j] = ' ';
		p_init(set, (i-N/2+0.5)*CELL, 0.5, (j-N/2+0.5)*CELL, 0, 0, 1);
		n_snowman--;
	}
	x += dm * lx * 0.1;
	z += dm * lz * 0.1;
}

void comDir(float da)
{
	angle += da;
	lx = sin(angle);
	lz = -cos(angle);
}

void pressKey(int key, int xx, int yy)
{
	switch (key)
	{
		case GLUT_KEY_LEFT: dangle = -0.05; break;
		case GLUT_KEY_RIGHT: dangle = 0.05; break;
		case GLUT_KEY_UP: dmove = 12.5; break;
		case GLUT_KEY_DOWN: dmove = -12.5; break;
	}
}

void releaseKey(int key, int xx, int yy)
{
	switch (key)
	{
		case GLUT_KEY_LEFT: 
		case GLUT_KEY_RIGHT: dangle = 0; break;
		case GLUT_KEY_UP: 
		case GLUT_KEY_DOWN: dmove = 0; break;
	}
}

void display()
{
	if (dmove)
		comPos(dmove);
	if (dangle)
		comDir(dangle);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();
       	gluLookAt( x, 1.0f, z,
		   x+lx, 1.0f, z+lz,
		   0.0f, 1.0f,0.0f);
	
        glColor3f(0.9, 0.9, 0.9);

	drawMap();
	for (int i=-N/2; i<N/2; i++)
		for (int j=-N/2; j<N/2; j++)
		{
			glPushMatrix();
			glTranslatef((i+0.5)*CELL, 0, (j+0.5)*CELL);
			if (map[i+N/2][j+N/2] == 'r' || map[i+N/2][j+N/2] == 'g' || map[i+N/2][j+N/2] == 'b')	
				drawSnowman(i+N/2, j+N/2);
			glPopMatrix();
		}
	p_draw(set);
	p_step(set);
        glutSwapBuffers();
}
 
void key_n(unsigned char c, int x, int y)
{
	if (c == 27)
		exit(0);
}

void key_s(int c, int x, int y)
{
	int mod = glutGetModifiers();
	float fraction = 0.1;
	switch (c){
		case GLUT_KEY_LEFT:
			angle-= 0.01;
			lx = sin(angle);
			lz = -cos(angle);
			break;
		case GLUT_KEY_RIGHT:
			angle+= 0.01;
			lx = sin(angle);
			lz = -cos(angle);
			break;
		case GLUT_KEY_UP:
			x += lx*fraction;
			z += lz*fraction;
			break;
		case GLUT_KEY_DOWN:
			x -= lx*fraction;
			z -= lz*fraction;
			break;
	}
}

void mouseMove(int x, int y) { 	
 
         if (xOrigin >= 0) {
 
		dangle = (x - xOrigin) * 0.001f;
 
		lx = sin(angle + dangle);
		lz = -cos(angle + dangle);
	}
}
 
void mouseButton(int button, int state, int x, int y) {
 
	if (button == GLUT_LEFT_BUTTON) {
 
		if (state == GLUT_UP) {
			angle += dangle;
			xOrigin = -1;
		}
		else  {
			xOrigin = x;
		}
	}
}

int main (int argc, char * argv[])
{
	srand(time(NULL));
        glutInit(&argc, argv);
        glutInitDisplayMode(GLUT_DEPTH| GLUT_DOUBLE|GLUT_RGBA);
       
        glutInitWindowSize(800, 600);
        glutCreateWindow("OpenGL lesson 1");
       
        glutReshapeFunc(reshape);
        glutDisplayFunc(display);
	glutIdleFunc(display);
	
	glutKeyboardFunc(key_n);
	glutSpecialFunc(pressKey);

	glutMouseFunc(mouseButton);
	glutMotionFunc(mouseMove);

	glutIgnoreKeyRepeat(1);
	glutSpecialUpFunc(releaseKey);
	glEnable(GL_DEPTH_TEST);
        for (int i=0; i<N; i++)
		for (int j=0; j<N; j++)
		{
			for (int k=0; k<3; k++)
				color[i][j][k] = (rand()%1000)/1000.0;
			snowangle[i][j] = (rand()%360)/360.0;
		}
	for (int i=0; i<N; i++)
		for (int j=0; j<N; j++)
		{
			if (map[i][j] != 'w')
				n_empty++;
			if (map[i][j] == 'r' || map[i][j] == 'g' || map[i][j] == 'b')
				n_snowman++;
		}
		glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
		glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
		glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	    glEnable(GL_LIGHT1); 
        glutMainLoop();
       
        return 0;
}

