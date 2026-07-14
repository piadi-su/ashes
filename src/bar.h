#ifndef XWIN_H
#define XWIN_H

#include <X11/Xlib.h>
#include <X11/Xft/Xft.h>
#include <fontconfig/fontconfig.h>
#include <stdint.h>

#define I3_IPC_MAGIC "i3-ipc"
#define I3_IPC_MESSAGE_TYPE_GET_WORKSPACES 1
#define I3_IPC_MESSAGE_TYPE_SUBSCRIBE 2

//struct that keeps the bar state
typedef struct {
    char workspace[128];
    char volume[64];
    char ipv4[64];
    char ram[64];
    char datetime[128];
} BarState;

//bar layout
typedef struct {
    char left[256];
    char center[256];
    char right[512];
} BarLayout;


//bar setup

void init_font(Display *dpy, Window win, int screen);

void cleanup(Display *dpy, Window win, GC gc);

void set_dock_properties(Display *dpy, Window win, int width);

void draw_bar(Display *dpy, Window win, GC gc, BarState *s);

void build_layout(BarState *s, BarLayout *l);


//bar modules
void update_workspace(BarState *s);
void update_volume(BarState *s);
void update_ipv4(BarState *s);
void update_ram(BarState *s);
void update_datetime(BarState *s);

int connect_i3_ipc(void);
void send_i3_message(int sock, uint32_t type, const char *payload);
void i3_subscribe(int sock);
void update_workspaces(int query_sock, BarState *s);
int read_full(int fd, void *buf, size_t n);
 

//font rendering
extern XftDraw *xft_draw;
extern XftFont *xft_font;
extern XftColor xft_color;





#endif

