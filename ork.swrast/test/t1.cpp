
#include "render_graph.h"
#include <unistd.h>
#include <ork/timer.h>
//#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <caca.h>

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

		/*glutInit(&argc, argv);
		glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
		glutInitWindowSize(width, height);
		glutInitWindowPosition(300, 200);
		glutCreateWindow("OrkRender");
		glutDisplayFunc(Redraw);
		glutReshapeFunc(Resize);
		glutIdleFunc(Update);*/

		t.Start();

  	}

  	void run_loop()
  	{
  		 //glutMainLoop();
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
	glutPostRedisplay();
}

void Redraw()
{
	gwin->mRG.Compute(*gwin->mopg);
	//static int iframe = 0;
	//printf( "frame<%d>\n", iframe );
	//iframe++;

	glClearColor(1.0f,0.0f,0.0f,1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-2.0, 2.0, -2.0, 2.0, -2.0, 500.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	Blit(gwin->mRG);
	    
	glutSwapBuffers();

	float ft = timer.SecsSinceStart();
	printf( "FPS<%f>\n", 1.0f/ft );
	timer.Start();

}
//////////////////////////////////////////////////
//////////////////////////////////////////////////
//////////////////////////////////////////////////

int main(int argc, char** argv)
{	
    auto cv = caca_create_canvas(80, 24);
    if(cv == NULL)
    {
        printf("Failed to create canvas\n");
        return 1;
    }

    auto dp = caca_create_display(cv);
    if(dp == NULL)
    {
        printf("Failed to create display\n");
        return 1;
    }

	int dispW = caca_get_canvas_width(cv);
	int dispH = caca_get_canvas_height(cv);

    //auto sprite = caca_create_canvas(0, 0);
    //caca_set_color_ansi(sprite, CACA_LIGHTRED, CACA_BLACK);
    //caca_set_color_argb(sprite,rand()&0xffff,rand()&0xffff);
    //caca_import_canvas_from_memory(sprite, pig, strlen(pig), "text");
    //int spriteW = caca_get_canvas_width(sprite);
    //int spriteH = caca_get_canvas_height(sprite);
    //caca_set_canvas_handle(sprite, spriteW/2,spriteH/2);

    //caca_set_color_ansi(cv, CACA_WHITE, CACA_BLUE);
    //caca_put_str(cv, 0, 0, "Centered sprite");


	bool done = false;
	int lx = 0, ly = 0;
	const int KBUFDIM=256;
	uint32_t buffer[KBUFDIM*KBUFDIM];
	int frame = 0;

	gwin = new window(argc,argv);
	gwin->mRG.Resize(KBUFDIM,KBUFDIM);

    auto dither = caca_create_dither( 32, KBUFDIM, KBUFDIM, 4 * KBUFDIM,
                                 	  0x00ff0000, 0x0000ff00, 0x000000ff, 0x0);

    caca_set_dither_antialias(dither,"default");
    caca_set_dither_color(dither,"full16");
    //caca_set_dither_color(dither,"fullgray");
    caca_set_dither_charset(dither,"shades");
    //caca_set_dither_charset(dither,"blocks");
    caca_set_dither_brightness(dither,2.0);

	gwin->mRG.ComputeAsync(*gwin->mopg);
	while(false==done)
	{	
	    /////////////////////////////////////////
		// render
	    /////////////////////////////////////////

		gwin->mopg->drain();
	
	    caca_dither_bitmap(cv, 0, 0, dispW, dispH, dither, gwin->mRG.GetPixels());

		gwin->mRG.ComputeAsync(*gwin->mopg);

	    /////////////////////////////////////////

		caca_set_color_ansi(cv, CACA_LIGHTGRAY, CACA_BLACK);
	    caca_put_str(cv, 3, 20, "This is bold    This is blink    This is italics    This is underline");
	    caca_set_attr(cv, CACA_BOLD);
	    caca_put_str(cv, 3 + 8, 20, "bold");
	    caca_set_attr(cv, CACA_BLINK);
	    caca_put_str(cv, 3 + 24, 20, "blink");
	    caca_set_attr(cv, CACA_ITALICS);
	    caca_put_str(cv, 3 + 41, 20, "italics");
	    caca_set_attr(cv, CACA_UNDERLINE);
	    caca_put_str(cv, 3 + 60, 20, "underline");
	    caca_set_attr(cv, 0);

		//int x = rand()%dispW;
		//int y = rand()%dispH;
	    //caca_set_color_argb(sprite,rand()&0xffff,rand()&0xffff);
	    //caca_import_canvas_from_memory(sprite, pig, strlen(pig), "text");
		//caca_blit(cv,x,y,sprite, NULL);
		//caca_draw_line(cv, lx, lx, x, y, ' ');

	
	    /////////////////////////////////////////
    	// swap
	    /////////////////////////////////////////

	    caca_refresh_display(dp);
 		
	    /////////////////////////////////////////
	    // input
	    /////////////////////////////////////////

 		caca_event_t ev;
 		int ret = caca_get_event(dp, CACA_EVENT_ANY, &ev, 0);
		if(ret)
		{
			if(caca_get_event_type(&ev) & CACA_EVENT_KEY_PRESS)
            {
                int key = caca_get_event_key_ch(&ev);
                if(key == 'q')
                	done = true;
            }
        }

	    /////////////////////////////////////////
	    // advance frame
	    /////////////////////////////////////////

 		//lx = x, ly = y;
 		frame++;
            
   	}

    caca_free_dither(dither);
    caca_free_display(dp);
    //caca_free_canvas(sprite);
    caca_free_canvas(cv);

	return 0;
}
