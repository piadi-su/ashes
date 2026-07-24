/*
 * ashes, is a fork of ash a status bar for i3-wm
 * but adapted to be used for ash-wm by reading is ipc
 *
 * ash repo: https://github.com/piadi-su/ash.git
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h> 
#include <signal.h>

//my files
#include "bar.h"
#include "config.h"


volatile sig_atomic_t running = 1;

void handle_sigint(int sig);

int main(void)
{
    XEvent ev;
    BarState s = {0};

    signal(SIGINT, handle_sigint);

    Display *dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
        fprintf(stderr, "can not open the display!\n");
        return 1;
	}

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    XSetWindowAttributes attrs = {0};

    XSelectInput(dpy, root, PropertyChangeMask);

    int monitors_count = 1;
    XineramaScreenInfo *info = NULL;

    if (XineramaIsActive(dpy)) {
        int n;
        info = XineramaQueryScreens(dpy, &n);
        if (info && n > 0) {
            monitors_count = n;
        }
    }

    if (monitors_count > MAX_MONITORS) {
        monitors_count = MAX_MONITORS;
    }

    Window wins[MAX_MONITORS];

    for (int i = 0; i < monitors_count; i++) {
        int m_x = 0, m_y = 0, m_w = 0, m_h = 0;

        if (info) {
            m_x = info[i].x_org;
            m_y = info[i].y_org;
            m_w = info[i].width;
            m_h = info[i].height;
        } else {
            m_x = 0;
            m_y = 0;
            m_w = DisplayWidth(dpy, screen);
            m_h = DisplayHeight(dpy, screen);
        }

		int bar_w = m_w - (BAR_MARGIN_X * 2);
		int bar_x = m_x + BAR_MARGIN_X;

		int y_pos = (BOTTOM) 
			? (m_y + m_h - BAR_HEIGHT - BAR_MARGIN_Y) 
			: (m_y + BAR_MARGIN_Y);

		wins[i] = XCreateWindow(
				dpy, root, bar_x, y_pos, bar_w, BAR_HEIGHT, 0,
				CopyFromParent, InputOutput, CopyFromParent,
				CWOverrideRedirect, &attrs
				);

		set_dock_properties(dpy, wins[i], bar_w);

        XSelectInput(dpy, wins[i], ExposureMask | KeyPressMask);

        XMapWindow(dpy, wins[i]);
    }

    if (info) {
        XFree(info);
    }

    init_font(dpy, wins[0], screen);

    GC gc = XCreateGC(dpy, wins[0], 0, NULL);
    init_multi_draws(dpy, wins, monitors_count, screen);

    update_datetime(&s);
    update_volume(&s);
    update_ram(&s);
    update_ipv4(&s);

	int x11_fd = ConnectionNumber(dpy);
    fd_set in_fds;
    struct timeval tv;
    
    int update_counter = 0;

    int redraw = 1;

    while (running)
    {
        FD_ZERO(&in_fds);
        FD_SET(x11_fd, &in_fds);

        tv.tv_sec = 0;
        tv.tv_usec = 80000; 

        int activity = select(x11_fd + 1, &in_fds, NULL, NULL, &tv);
        
        if (activity < 0) continue;

        // X11 Event Handler 
        while (XPending(dpy))
        {
            XNextEvent(dpy, &ev);
            
            if (ev.type == Expose && ev.xexpose.count == 0) {
                redraw = 1;
            }

            if (ev.type == PropertyNotify && ev.xproperty.window == root) {
                Atom prop = XInternAtom(dpy, "_ASHWM_WORKSPACES", False);
                if (ev.xproperty.atom == prop) {
                    redraw = 1; 
                }
            }
        }

        if (activity == 0) {
            update_counter += 80; 
            
            if (update_counter % 480 == 0) {
                update_datetime(&s);
                redraw = 1;
            }
            
            if (update_counter >= 2000) {
                update_volume(&s);
                update_ram(&s);
                update_ipv4(&s);
                update_counter = 0;
                redraw = 1;
            }
        }

        if (redraw) {
            for (int i = 0; i < monitors_count; i++) {
                draw_bar_on_monitor(dpy, wins[i], gc, &s, i);
            }
            redraw = 0; 
        }
    }
    
    free_multi_draws(monitors_count);
    for (int i = 1; i < monitors_count; i++) {
        XDestroyWindow(dpy, wins[i]);
    }
    cleanup(dpy, wins[0], gc);

    return 0;
}

void handle_sigint(int sig)
{
    (void)sig;
    running = 0;
}
