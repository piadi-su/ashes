#ifndef XWIN_H
#define XWIN_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#include <stdint.h>

// bar status
typedef struct {
    char workspace[128];
    char volume[64];
    char ipv4[64];
    char ram[64];
    char datetime[128];
} BarState;

// Layout 
typedef struct {
    char left[256];
    char center[256];
    char right[512];
} BarLayout;

void init_font(Display *dpy, Window win, int screen);
void cleanup(Display *dpy, Window win, GC gc);
void set_dock_properties(Display *dpy, Window win, int width);
void build_layout(BarState *s, BarLayout *l);

void init_multi_draws(Display *dpy, Window wins[], int count, int screen);
void draw_bar_on_monitor(Display *dpy, Window win, GC gc, BarState *s, int monitor_idx);
void free_multi_draws(int count);

void update_workspace(BarState *s, int monitor_idx);
void update_volume(BarState *s);
void update_ipv4(BarState *s);
void update_ram(BarState *s);
void update_datetime(BarState *s);

// Font 
extern XftFont *xft_font;
extern XftColor xft_color;

#endif
