
#include "render_graph.h"
#include <unistd.h>
#include <ork/timer.h>
//#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#ifdef __APPLE__
#include <GLUT/glut.h>          /* Open GL Util    APPLE */
#else
#include <GL/glut.h>            /* Open GL Util    OpenGL*/
#endif

#include <ork/timer.h>

using namespace ork;

void Redraw();
void Resize(int w, int h);
void Update();

int width = 1280;
int height = 720;

Timer timer;

struct window
{
	window(int argc,char** argv)
		: omq(64)
	{
		mopg = omq.CreateOpGroup("l0");

		//glutInit(&argc, argv);
		//glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
		//glutInitWindowSize(width, height);
		//glutInitWindowPosition(300, 200);
		//glutCreateWindow("OrkRender");
		//glutDisplayFunc(Redraw);
		//glutReshapeFunc(Resize);
		//glutIdleFunc(Update);

		t.Start();

  	}

  	void run_loop()
  	{
	while(1)	Redraw();
  	//	 glutMainLoop();
  	}

	~window()
	{
	}



	OpMultiQ omq;
	OpGroup* mopg;
	render_graph mRG;
	Timer t;
};

window* gwin = nullptr;

void Blit(render_graph& rg)
{
	const uint32_t* src_pixels = rg.GetPixels();
	glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, src_pixels );
}

void Resize(int w, int h)
{
	width = w;
	height = h;
	gwin->mRG.Resize(width,height);
}

void Update()
{
	//glutPostRedisplay();
}

void Redraw()
{
	gwin->mRG.Compute(*gwin->mopg);
	//static int iframe = 0;
	//printf( "frame<%d>\n", iframe );
	//iframe++;

	//glClearColor(1.0f,0.0f,0.0f,1.0f);
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//	glEnable(GL_DEPTH_TEST);
//	glMatrixMode(GL_PROJECTION);
//	glLoadIdentity();
//	glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 500.0);
//	glMatrixMode(GL_MODELVIEW);
//	glLoadIdentity();
//
//	Blit(gwin->mRG);
	    
//	glutSwapBuffers();

	float ft = timer.SecsSinceStart();
	printf( "FPS<%f>\n", 1.0f/ft );
	timer.Start();

}
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////

int main(int argc, char** argv)
{	
	gwin = new window(argc,argv);
	gwin->mRG.Resize(width,height);
	gwin->run_loop();
	return 0;
}
