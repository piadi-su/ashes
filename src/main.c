/*
 * ash - simple X11/i3 status bar
 *
 * A small status bar implemented with Xlib and Xft.
 * Uses i3 IPC for workspace information and periodically
 * updates system status (time, RAM, volume, network).
 *
 * The program runs a single event loop using select(2)
 * over X11 connection and i3 IPC socket.
 *
 * Built directly on X11 (Xlib + Xft) without GUI toolkits.
 * 
 */


#include <X11/X.h>
#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <stdlib.h>

//my files
#include "bar.h"
#include "config.h"



volatile sig_atomic_t running = 1;

void handle_sigint(int sig);


int main(void)
{
	XEvent ev;
    BarState s = {0};

    // configure signal handler 
    signal(SIGINT, handle_sigint); 


	// dpy connection
    Display *dpy = XOpenDisplay(NULL);
    if(dpy == NULL)
    {
        fprintf(stderr, "can not open the disply!\n");
        return 1;
    }

	// get the display like DP-2 ecc
	int screen = DefaultScreen(dpy);

	//create root window/desktop
	Window root = RootWindow(dpy, screen);

	// for dock pos
	XSetWindowAttributes attrs = {0};

	//get the width of the screen
	int width = DisplayWidth(dpy, screen);

	//get bottom position
	int screen_height = DisplayHeight(dpy, screen);
    int y_pos = (BOTTOM) ? (screen_height - BAR_HEIGHT) : 0;

    //create the window
    Window win = XCreateWindow(
            dpy, root, 0, y_pos, width, BAR_HEIGHT, 0,
            CopyFromParent, InputOutput, CopyFromParent,
            CWOverrideRedirect, &attrs
            );

	//make it not in tyle mode reserv the pixel size 
	set_dock_properties(dpy, win, width);

	// say what kind of input the bar can recive
	XSelectInput(dpy, win, ExposureMask | KeyPressMask);

	//inizialize the font 
	init_font(dpy, win, screen);

	//set in wait the window and make it visible 
	XMapWindow(dpy, win);

	//create the thing to draw stuff on bar
	GC gc = XCreateGC(dpy, win, 0, NULL);

	// Caricamento dati iniziale prima di entrare nel ciclo
	update_datetime(&s);
	update_volume(&s);
	update_ram(&s);
	update_ipv4(&s);

	// selector config
	int x11_fd = ConnectionNumber(dpy);
	fd_set in_fds;
	struct timeval tv;

	//bar cycle
	while (running)
	{
		FD_ZERO(&in_fds);
		FD_SET(x11_fd, &in_fds);

		tv.tv_sec = 1;
		tv.tv_usec = 0;

		int redraw = 0;


		// X11 event handler
		while (XPending(dpy))
		{
			XNextEvent(dpy, &ev);
			if (ev.type == Expose && ev.xexpose.count == 0)
				redraw = 1;
		}


		// timer handler / background refresh
		if (redraw) {
			update_datetime(&s);
			update_volume(&s);
			update_ram(&s);
			update_ipv4(&s);
			redraw = 1; 
		}

		if (redraw) {
			draw_bar(dpy, win, gc, &s);
		}
	}
	
	cleanup(dpy, win, gc);
	return 0;
}


void 
handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}


