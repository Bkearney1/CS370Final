// CS370 - Fall 2018
// Final Project

#ifdef OSX
	#include <GLUT/glut.h>
#else
	#include <GL/glew.h>
	#include <GL/glut.h>
#endif
#include <SOIL/SOIL.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "lighting.h"
#include "materials.h"

// Shader file utility functions
#include "shaderutils.h"

#define DEG2RAD 0.01745
#define X 0
#define Y 1
#define Z 2



//TExture definitions
#define NO_TEXTURES 12//Always last texture+1
#define ENVIRONMENT 0
#define SPACE_CARPET 1
#define WALL_TEX 2
#define RKT 3
#define MODEL_S 4
#define SPACE_DOOR 5
#define OUTSIDE0 6
#define WOOD_TEXTURE 7
#define OUTSIDE_WINDOW  8
#define WINDOW_BLIND 9
#define HAL 10
#define CEILING 11



//for display list calls
#define WALLS 1 
#define FLOOR 2
#define CARPET 3 
#define ARTWORK0 4
#define ARTWORK1 5
#define SPACE_SCENE 6
#define DOOR 7
#define CHAIR 8
#define TABLE 9
#define WINDOW_PAINTING 10
#define CEILING_LIST 11
#define LOWER_CEILING_LIST 12

//shadow global variables 
GLfloat M_s[16];
GLfloat shadow_color[] = { 0.2f,0.2f,0.2f };
GLfloat light0_pos[] = { 0.0f, 5.0f, 0.0f, 1.0f };

//camera movement 
GLfloat fwdDisp = 0;
GLfloat sideDisp = 10;
GLfloat dx = 0.05;
GLfloat theta =85;

// Global camera vectors
GLfloat eye[3] = { 0.0f, 1.5f, 3.20f };
GLfloat at[3] = { 0.0f, 1.0f, -5.0f };
GLfloat up[3] = { 0.0f, 1.0f, 0.0f };


//animation dependacies
GLfloat fps = 60;
GLfloat door_dx = 0.02;
GLfloat blind_dx = 0.02;
GLfloat blind_disp = 0.0;
GLfloat door_disp = 0;
GLint time = 0;
GLint lasttime = 0;
bool move_door = false;
bool move_blinds = false;

// Global screen dimensions
GLint ww, hh;

//Texture mapping variables 
GLuint tex_ids[NO_TEXTURES];
char texture_files[13][20] = { "blank.bmp", "space_carpet.jpg","wall_texture.jpg","rocketfinal.png","models.jpg","space_door.png","outside_scene0.jpg","wood_tex.jpg","outside_window.jpg","window_blind.png","HAL.png","ceiling.png" };


//for rendering texcubes
GLfloat cube[][3] = { { -1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, -1.0f }, { 1.0f, -1.0f, 1.0f }, { -1.0f, -1.0f, 1.0f },
{ -1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, -1.0f }, { 1.0f, 1.0f, 1.0f }, { -1.0f, 1.0f, 1.0f } };
GLfloat cube_tex[][2] = { {0,0}, {0,1}, {1,1}, {1,0}};
// Shader files
GLchar* defaultVertexFile = "defaultvert.vs";
GLchar* defaultFragmentFile = "defaultfrag.fs";
GLchar* lightVertexFile = "lightvert.vs";
GLchar* lightFragmentFile = "lightfrag.fs";
GLchar* texVertexFile = "texturevert.vs";
GLchar* texFragmentFile = "texturefrag.fs";

// Shader objects
GLuint defaultShaderProg;
GLuint lightShaderProg;
GLuint textureShaderProg;
GLuint numLights_param;
GLint numLights = 1;
GLint texSampler;

void display();
void render_Scene(bool shadow);
void keyfunc(unsigned char key, int x, int y);

void idlefunc();
void reshape(int w, int h);
void create_lists();
bool load_textures();
void colorcube(int x);
void quad(GLfloat v1[], GLfloat v2[], GLfloat v3[], GLfloat v4[], GLfloat c[]);
static void mytimer(int v);
void texturecube();
void texquad(GLfloat v1[], GLfloat v2[], GLfloat v3[], GLfloat v4[], GLfloat t1[], GLfloat t2[], GLfloat t3[], GLfloat t4[]);
void create_mirror();
void render_mirror();

int main(int argc, char *argv[])
{
	// Initialize GLUT
	glutInit(&argc,argv);

	// Initialize the window with double buffering and RGB colors
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Set the window size to image size
	glutInitWindowSize(512,512);

	// Create window
	glutCreateWindow("Space Base");

#ifndef OSX
	// Initialize GLEW - MUST BE DONE AFTER CREATING GLUT WINDOW
	glewInit();
#endif
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	// Define callbacks
	glutDisplayFunc(display);
	glutKeyboardFunc(keyfunc);
	glutIdleFunc(idlefunc);
	glutReshapeFunc(reshape);
	glutTimerFunc(1000 / fps, mytimer, 1);

	create_lists();
	if (!load_textures())
	{
		exit(0);
	}

	// Set background color to white
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	
	//shadow matrix initialization	
	for (int i = 0; i < 16; i++) {
		M_s[i] = 0;
	}
	M_s[0] = M_s[5] = M_s[10] = 1;
	M_s[7] = -1 / light0_pos[1];
	// Load shader programs
	defaultShaderProg = load_shaders(defaultVertexFile,defaultFragmentFile);
	lightShaderProg = load_shaders(lightVertexFile, lightFragmentFile);
	textureShaderProg = load_shaders(texVertexFile, texFragmentFile);
	numLights_param = glGetUniformLocation(lightShaderProg, "numLights");
	texSampler = glGetUniformLocation(textureShaderProg, "texMap");
	glUseProgram(defaultShaderProg);

	// Begin event loop
	glutMainLoop();
	return 0;
}

// Display callback
void display()
{
	create_mirror();
	// Reset background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set projection matrix
	 //THIS LINE CHANGES EVERYTHING!!!!!!!!!!
	//WHEN ADJUSTING BACK TO FIRST PERSON FIX THIS S***!!!
	/*// Adjust viewing volume (orthographic)
	GLfloat xratio = 1.0f;
	GLfloat yratio = 1.0f;

	// If taller than wide adjust y
	if (ww <= hh)
	{
		yratio = (GLfloat)hh / (GLfloat)ww;
	}
	// If wider than tall adjust x
	else if (hh <= ww)
	{
		xratio = (GLfloat)ww / (GLfloat)hh;
	}*/
	//glOrtho(-3.0f*xratio, 3.0f*xratio, -3.0f*yratio, 3.0f*yratio, -3.0f, 3.0f);
	
	// Set modelview matrix

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum(-1.0f, 1.0f, -1.0f, 1.0f, 0.75f, 20.0f);


	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	
	gluLookAt(eye[X], eye[Y], eye[Z], at[X], at[Y], at[Z], up[X], up[Y], up[Z]);
	// Render scene
	render_Scene(false);
	render_Scene(true);

	render_mirror();

	// Flush buffer
	glFlush();

	// Swap buffers
	glutSwapBuffers();


}
//GLfloat theta = 0.1;
// Scene render function
void render_Scene(bool shadow)
{


	glUseProgram(lightShaderProg);
	glUniform1i(numLights_param, numLights);
	set_PointLight(GL_LIGHT0, &white_light, light0_pos);

	glPushMatrix();//WINDOW BLIND
//CANNOT PUT THIS IN SCENE GRAPH DUE TO THE WAY THE ANIMATION
//NEEDS TO MODIFY ONLY!!!!!! TWO VERTICIES 
	glBindTexture(GL_TEXTURE_2D, tex_ids[WINDOW_BLIND]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glTranslatef(-1, .75, -3.99);
	glScalef(2, 1.5, 1);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0 + blind_disp);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0 + blind_disp);
	glEnd();
	glPopMatrix();



	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_ids[WALL_TEX]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	for (int i = 0; i < 5; i++) {
		glCallList(WALLS);//for loop to build the walls
		glRotatef(90*i, 0, 1, 0);
	}
	glPopMatrix();


	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_ids[SPACE_CARPET]);//renders caroet
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glCallList(CARPET);
	glPopMatrix();


	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_ids[RKT]);//renders rocket artwork
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glCallList(ARTWORK0);
	glPopMatrix();


	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_ids[MODEL_S]);//renders model s artwork
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glCallList(ARTWORK1);
	glPopMatrix();


	glPushMatrix();
	glBindTexture(GL_TEXTURE_2D, tex_ids[SPACE_DOOR]);//door
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glTranslatef(0, door_disp, 0);
	glCallList(DOOR);
	glPopMatrix();



	glPushMatrix();//OUTSIDE SCENE LOOKING BACK AT EARTH 
	glBindTexture(GL_TEXTURE_2D, tex_ids[OUTSIDE0]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glCallList(SPACE_SCENE);
	glPopMatrix();


	glPushMatrix();//HAL 9000
	glBindTexture(GL_TEXTURE_2D, tex_ids[HAL]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, 1.0, 3.99);
	glScalef(-1,1,1);
	glBegin(GL_POLYGON);

	glTexCoord2f(0, 0);
	glVertex2f(0, 0);

	glTexCoord2f(0, 1);
	glVertex2f(0, .5);

	glTexCoord2f(1, 1);
	glVertex2f(.25, .5);

	glTexCoord2f(1, 0);
	glVertex2f(.25, 0);
	glEnd();

	glPopMatrix();

	

	glPushMatrix();	//""PAINTING"" OUTSIDE OF WINDOW
	glBindTexture(GL_TEXTURE_2D, tex_ids[OUTSIDE_WINDOW]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	glCallList(WINDOW_PAINTING);
	glPopMatrix();

	
	glPushMatrix();//renders table with shadow

	if (!shadow) {
		glBindTexture(GL_TEXTURE_2D, tex_ids[WOOD_TEXTURE]);
		glUniform1i(texSampler, 0);
		glUseProgram(textureShaderProg);
	}
	else {
	
		glTranslatef(0, .15, 0);
		glTranslatef(light0_pos[0], light0_pos[1], light0_pos[2]);
		glMultMatrixf(M_s);
		glTranslatef(-light0_pos[0], -light0_pos[1], -light0_pos[2]);
		glUseProgram(defaultShaderProg);
		glColor3fv(shadow_color);
	}

	glCallList(TABLE);
	glPopMatrix();



	glPushMatrix();//BEGIN ALL CHAIRS with shadow

	if (!shadow) {
		glBindTexture(GL_TEXTURE_2D, tex_ids[WOOD_TEXTURE]);
		glUseProgram(textureShaderProg);
		glUniform1i(texSampler, 0);

	}
	else {
	
		glTranslatef(0, .15, 0);
		glTranslatef(light0_pos[0], light0_pos[1], light0_pos[2]);
		glMultMatrixf(M_s);
		glTranslatef(-light0_pos[0], -light0_pos[1], -light0_pos[2]);
		glUseProgram(defaultShaderProg);
		glColor3fv(shadow_color);
	}
	

	glPushMatrix();
	glTranslatef(0, 0.55, 1);//chair 0
	glCallList(CHAIR);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(1, 0.55, 0);//chair 1 
	glRotatef(90, 0, 1, 0);
	glCallList(CHAIR);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-1, 0.55, 0);//chair 3
	glRotatef(-90, 0, 1, 0);
	glCallList(CHAIR);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0, 0.55, -1);//chair 0
	glRotatef(180, 0, 1, 0);
	glCallList(CHAIR);
	glPopMatrix();

    glPopMatrix();//END CHAIRS


	glPushMatrix();//CALLS BOTH SIDES OF SLANTED CEILING
	glUseProgram(textureShaderProg);
	glBindTexture(GL_TEXTURE_2D, tex_ids[CEILING]);
	glCallList(CEILING_LIST);
	glPopMatrix();





	glPushMatrix();
	glUseProgram(textureShaderProg);

	glBindTexture(GL_TEXTURE_2D, tex_ids[WALL_TEX]);

	glTranslatef(-4, 2, 4.01);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(0, 1);
	glVertex2f(0, 6);
	glTexCoord2f(1, 1);
	glVertex2f(8, 6);
	glTexCoord2f(1, 0);
	glVertex2f(8, 0);
	glEnd();
	glPopMatrix();



	glPushMatrix();

	glTranslatef(-4, 2, -4.01);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(0, 1);
	glVertex2f(0, 6);
	glTexCoord2f(1, 1);
	glVertex2f(8, 6);
	glTexCoord2f(1, 0);
	glVertex2f(8, 0);
	glEnd();
	glPopMatrix();


	
}



void create_mirror() {

// Reset background
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// TODO: Set projection matrix for flat "mirror" camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-5.0, 5.0, 0.0, 3.0, 0.0, 20.0);
	
	// TODO: Set modelview matrix positioning "mirror" camera
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0, 0.0, 3.5, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	// TODO: Render scene from mirror
	glPushMatrix();
	render_Scene(true);
	glPopMatrix();

	glFinish();

	// TODO: Copy scene to texture
	glBindTexture(GL_TEXTURE_2D, tex_ids[ENVIRONMENT]);
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 0, 0, 512, 512, 0);

}


void render_mirror() {//draws the mirror 
	glPushMatrix();//Mirror
	glBindTexture(GL_TEXTURE_2D, tex_ids[ENVIRONMENT]);
	glUseProgram(textureShaderProg);
	glUniform1i(texSampler, 0);
	
	glTranslatef(-1.5, .75, 3.99);
	glScalef(3, 1.5, 1);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0);
	glEnd();
	glPopMatrix();
}




// Keyboard callback
void keyfunc(unsigned char key, int x, int y)
{
	//glutKeyboardUpFunc

	if (key == 'w') {
		
		eye[X] = eye[X] + (at[X] - eye[X]) * dx;
		eye[Z] = eye[Z] + (at[Z] - eye[Z]) * dx;
		at[X] = eye[X] + cos(theta);
		at[Z] = eye[Z] + sin(theta);
		
	}
	else if (key == 's') {
		eye[X] = eye[X] - (at[X] - eye[X]) * dx;
		eye[Z] = eye[Z] - (at[Z] - eye[Z]) * dx;
		at[X] = eye[X] + cos(theta);
		at[Z] = eye[Z] + sin(theta);

		//s_key_down = true;
		
	}
	else if (key == 'a') {
		
		theta -= dx;
		at[X] = eye[X] + cos(theta);
		at[Z] = eye[Z] + sin(theta);
	}
	else if (key == 'd') {
		
		theta += dx;
		at[X] = eye[X] + cos(theta);
		at[Z] = eye[Z] + sin(theta);
	}
	else if (key == 'z') {
		at[Y] = at[Y] + dx;
	}
	else if (key == 'x') {
		at[Y] = at[Y] - dx;
	}
	else if (key == 'p') {
		move_door = !move_door;
	}
	else if (key == 'o') {
		move_blinds = !move_blinds;
	}
	// <esc> quits
	if (key == 27)
	{
		exit(0);
	}
	
	// Redraw screen
	glutPostRedisplay();
}

void create_lists()
{
	glNewList(WALLS, GL_COMPILE);//POLYGON FOR THE WALLS
		glPushMatrix();
		//glColor3f(0.70f, 0.45, 0.62);
		glBegin(GL_POLYGON);
		glTexCoord2f(0, 0);
			glVertex3f(-4,0,-4);
			glTexCoord2f(0, 1);
			glVertex3f(-4, 2.5, -4);
			glTexCoord2f(1, 1);
			glVertex3f(4, 2.5, -4);
			glTexCoord2f(1, 0);
			glVertex3f(4, 0, -4);
		glEnd();
		glPopMatrix();
	glEndList();

	glNewList(CARPET, GL_COMPILE);
	glPushMatrix();//CARPET
	glBegin(GL_POLYGON);//Carpet polygon
	glTexCoord2f(0, 0);
	glVertex3f(-4.0f, 0.1f, -4.0f);//BL
	glTexCoord2f(0, 1);
	glVertex3f(-4.0f, 0.1f, 4.0f);//TL
	glTexCoord2f(1, 1);
	glVertex3f(4.0f, 0.1f, 4.0f);//TR
	glTexCoord2f(1, 0);
	glVertex3f(4.0f, 0.1f, -4.0f);//BR=
	glEnd();
	glPopMatrix();
	glEndList();

	
	glNewList(ARTWORK0, GL_COMPILE);
	glPushMatrix();
	glTranslatef(-3.99, .85, -2.5);//artwork 0
	glRotatef(90, 0, 1, 0);
	glRotatef(90, 0, 0, 1);
	glBegin(GL_POLYGON);

	glTexCoord2f(0, 0);
	glVertex3f(-0.15, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3f(-.15, 2.15, 0);
	glTexCoord2f(1, 1);
	glVertex3f(1.15, 2.15, 0);
	glTexCoord2f(0, 1);
	glVertex3f(1.15, 0, 0);
	glEnd();

	glPopMatrix();
	glEndList();
	
	glNewList(ARTWORK1, GL_COMPILE);
	glPushMatrix();
	glTranslatef(-3.99, 0.85, 0.5);
	glRotatef(90, 0, 1, 0);
	glRotatef(90, 0, 0, 1);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex3f(-0.15, 0, 0);
	glTexCoord2f(1, 0);
	glVertex3f(-.15, 2.15, 0);
	glTexCoord2f(1, 1);
	glVertex3f(1.15, 2.15, 0);
	glTexCoord2f(0, 1);
	glVertex3f(1.15, 0, 0);
	
	glEnd();
	glPopMatrix();
	glEndList();


	glNewList(SPACE_SCENE, GL_COMPILE);
	glPushMatrix();
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, 0, 3.999);
	glColor3f(1, 0, 0);
	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);

	glTexCoord2f(0, 1);
	glVertex2f(0, 2.3);
	glTexCoord2f(1, 1);
	glVertex2f(2.5, 2.3);
	glTexCoord2f(1, 0);
	glVertex2f(2.5, 0);
	glEnd();
	glPopMatrix();
	glEndList();

	glNewList(DOOR, GL_COMPILE);
	glPushMatrix();
	
	glRotatef(90, 0, 1, 0);
	glTranslatef(0, 0, 3.99);

	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex2f(0, 0);

	glTexCoord2f(0, 1);
	glVertex2f(0, 2.3);
	glTexCoord2f(1, 1);
	glVertex2f(2.5, 2.3);
	glTexCoord2f(1, 0);
	glVertex2f(2.5, 0);
	glEnd();

	glPopMatrix();
	glEndList();


	glNewList(TABLE, GL_COMPILE);
	glPushMatrix();
	glPushMatrix();//tabletop
	glTranslatef(0, .85, 0);
	glScalef(.85, .05, .85);
	texturecube();
	glPopMatrix();
	glPushMatrix();//leg 0
	glTranslatef(.7, 0, .7);
	glScalef(.08, .854, .08);
	texturecube();
	glPopMatrix();
	glPushMatrix();//leg 1 
	glTranslatef(-.7, 0, .7);
	glScalef(.08, .854, .08);
	texturecube();
	glPopMatrix();
	glPushMatrix();//leg 2
	glTranslatef(-.7, 0, -.7);
	glScalef(.08, .854, .08);
	texturecube();
	glPopMatrix();
	glPushMatrix();//leg 3 
	glTranslatef(.7, 0, -.7);
	glScalef(.08, .854, .08);
	texturecube();
	glPopMatrix();
	glPopMatrix();
	glEndList();


	glNewList(WINDOW_PAINTING, GL_COMPILE);
	glPushMatrix();
	glTranslatef(-1, .75, -3.999);
	glScalef(2, 1.5, 1);
	glBegin(GL_POLYGON);

	glTexCoord2f(0, 0);
	glVertex2f(0, 0);
	glTexCoord2f(0, 1);
	glVertex2f(0, 1);
	glTexCoord2f(1, 1);
	glVertex2f(1, 1);
	glTexCoord2f(1, 0);
	glVertex2f(1, 0);
	glEnd();


	glPopMatrix();
	glEndList();


	glNewList(CHAIR, GL_COMPILE);
	glPushMatrix();

	glScalef(1.35, 1.35, 1.35);

	glPushMatrix();
	glScalef(.2, .01, .2);//chair seat
	texturecube();
	glPopMatrix();

	glPushMatrix();//chair back
	glRotatef(4, 1, 0, 0);
	glTranslatef(0, .2, .2);
	glRotatef(90, 1, 0, 0);
	glScalef(.2, .01, .2);
	texturecube();
	glPopMatrix();


	glPushMatrix();//chair leg 0
	glTranslatef(-.15, -.2, .15);
	glRotatef(90, 0, 1, 0);
	glScalef(.025, .2, .025);
	texturecube();
	glPopMatrix();

	glPushMatrix();//chair leg 1
	glTranslatef(-.15, -.2, -.15);
	glRotatef(90, 0, 1, 0);
	glScalef(.025, .2, .025);
	texturecube();
	glPopMatrix();

	glPushMatrix();//chair leg 2
	glTranslatef(.15, -.2, .15);
	glRotatef(90, 0, 1, 0);
	glScalef(.025, .2, .025);
	texturecube();
	glPopMatrix();

	glPushMatrix();//chair leg 3
	glTranslatef(.15, -.2, -.15);
	glRotatef(90, 0, 1, 0);
	glScalef(.025, .2, .025);
	texturecube();
	glPopMatrix();


	glPopMatrix();
	glEndList();

	glNewList(CEILING_LIST, GL_COMPILE);//BUILDS BOTH SIDES OF CEILING
	glPushMatrix();
	glTranslatef(-2.0, -1, 0);
	glRotatef(90, 0, 1, 0);
	glRotatef(60, 1, 0, 0);


	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex3f(-4, 0, -4);

	glTexCoord2f(0, 1);
	glVertex3f(-4, 5, -4);

	glTexCoord2f(1, 1);
	glVertex3f(4, 5, -4);

	glTexCoord2f(1, 0);
	glVertex3f(4, 0, -4);

	glEnd();
	glPopMatrix();


	glPushMatrix();//CEILING half 2
	glTranslatef(2, -1, 0);
	glRotatef(270, 0, 1, 0);
	glRotatef(60, 1, 0, 0);


	glBegin(GL_POLYGON);
	glTexCoord2f(0, 0);
	glVertex3f(-4, 0, -4);

	glTexCoord2f(0, 1);
	glVertex3f(-4, 5, -4);

	glTexCoord2f(1, 1);
	glVertex3f(4, 5, -4);

	glTexCoord2f(1, 0);
	glVertex3f(4, 0, -4);

	glEnd();
	glPopMatrix();

	glEndList();

	



}

// Routine to load textures using SOIL
bool load_textures()
{
	// Load environment map texture (NO MIPMAPPING)
	tex_ids[ENVIRONMENT] = SOIL_load_OGL_texture(texture_files[0], SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);

	// TODO: Set environment map properties if successfully loaded
	if (tex_ids[ENVIRONMENT] != 0)
	{
		// Set scaling filters (no mipmap)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Set wrapping modes (clamped)
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	}
	// Otherwise texture failed to load
	else
	{
		return false;
	}

	for (int i = 1; i < NO_TEXTURES; i++)
	{
		// TODO: Load images
		tex_ids[i] = SOIL_load_OGL_texture(texture_files[i], SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y);

		// Set texture properties if successfully loaded
		if (tex_ids[i] != 0)
		{
			// TODO: Set scaling filters
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

			// TODO: Set wrapping modes
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
		// Otherwise texture failed to load
		else
		{
			return false;
		}
	}
	return true;
}

static void mytimer(int v)
{

	glutPostRedisplay();
	glutTimerFunc(1000 / fps, mytimer, 1);
}


// Idle callback
void idlefunc()
{

	time = glutGet(GLUT_ELAPSED_TIME);


	if (time - lasttime > 1000.0f / fps)
	{
		if (move_door) {
			door_disp -= door_dx;
			if (door_disp <-2.2 || door_disp>0.01) {
				move_door = !move_door;
				door_dx *= -1;
			}
		}

		if (move_blinds) {
			blind_disp += blind_dx;
			if (blind_disp < 0.01 || blind_disp>.9) {
				move_blinds = !move_blinds;
				blind_dx *= -1;;
			}
		}

		lasttime = time;
		 glutPostRedisplay();
	}
	
	
}

// Reshape callback
void reshape(int w, int h)
{
	// Set new screen extents
	glViewport(0, 0, w, h);

	// Store new extents
	ww = w;
	hh = h;
}
// Routine to draw textured cube
void texturecube()
{
	// Top face
	texquad(cube[4], cube[7], cube[6], cube[5], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);

	// Bottom face
	texquad(cube[0], cube[1], cube[2], cube[3], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);

	// Left face
	texquad(cube[2], cube[6], cube[7], cube[3], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);

	// Right face
	texquad(cube[0], cube[4], cube[5], cube[1], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);

	// Front face
	texquad(cube[1], cube[5], cube[6], cube[2], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);

	// Back face
	texquad(cube[0], cube[3], cube[7], cube[4], cube_tex[0], cube_tex[1], cube_tex[2], cube_tex[3]);
}

// Routine to draw quadrilateral face
void texquad(GLfloat v1[], GLfloat v2[], GLfloat v3[], GLfloat v4[], GLfloat t1[], GLfloat t2[], GLfloat t3[], GLfloat t4[])
{
	// Draw face 
	glBegin(GL_POLYGON);


	glTexCoord2fv(t1);
	glVertex3fv(v1);


	glTexCoord2fv(t2);
	glVertex3fv(v2);


	glTexCoord2fv(t3);
	glVertex3fv(v3);

	glTexCoord2fv(t4);
	glVertex3fv(v4);

	glEnd();
}