#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrender.h>
#include <fontconfig/fontconfig.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>

#include "bar.h"
#include "config.h"

XftFont *xft_font = NULL;
XftColor xft_color;

static XftDraw *monitor_draws[4] = {NULL, NULL, NULL, NULL};

void 
init_multi_draws(Display *dpy, Window wins[], int count, int screen)
{
    for (int i = 0; i < count && i < 4; i++) {
        monitor_draws[i] = XftDrawCreate(
            dpy,
            wins[i],
            DefaultVisual(dpy, screen),
            DefaultColormap(dpy, screen)
        );
    }
}

void 
free_multi_draws(int count)
{
    for (int i = 0; i < count && i < 4; i++) {
        if (monitor_draws[i]) {
            XftDrawDestroy(monitor_draws[i]);
            monitor_draws[i] = NULL;
        }
    }
}

void 
init_font(Display *dpy, Window win, int screen)
{
    (void)win; // Non più necessario qui per XftDraw singolo
    xft_font = XftFontOpenName(dpy, screen, BAR_FONT);

    unsigned r = (TEXT_COLOR >> 16) & 0xFF;
    unsigned g = (TEXT_COLOR >> 8)  & 0xFF;
    unsigned b = (TEXT_COLOR)       & 0xFF;

    XRenderColor xrcolor = {
        .red   = r * 257,
        .green = g * 257,
        .blue  = b * 257,
        .alpha = 0xffff
    };

    XftColorAllocValue(
        dpy,
        DefaultVisual(dpy, screen),
        DefaultColormap(dpy, screen),
        &xrcolor,
        &xft_color
    );
}

void 
cleanup(Display *dpy, Window win, GC gc)
{
    int screen = DefaultScreen(dpy);

    XftColorFree(
        dpy,
        DefaultVisual(dpy, screen),
        DefaultColormap(dpy, screen),
        &xft_color
    );

    if (xft_font)
        XftFontClose(dpy, xft_font);

    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}

void 
set_dock_properties(Display *dpy, Window win, int width)
{
    Atom type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    Atom dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);

    XChangeProperty(
        dpy, win,
        type,
        XA_ATOM,
        32,
        PropModeReplace,
        (unsigned char *)&dock,
        1
    );

    unsigned long strut[12] = {0};

    if (BOTTOM) {
        strut[3] = BAR_HEIGHT; // bottom
        strut[10] = 0;
        strut[11] = width;
    } else {
        strut[2] = BAR_HEIGHT; // top
        strut[8] = 0;
        strut[9] = width;
    }

    Atom strut_atom = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    XChangeProperty(
        dpy, win,
        strut_atom,
        XA_CARDINAL,
        32,
        PropModeReplace,
        (unsigned char *)strut,
        12
    );
}



void 
draw_bar_on_monitor(Display *dpy, Window win, GC gc, BarState *s, int monitor_idx)
{
    if (monitor_idx >= 4 || !monitor_draws[monitor_idx] || !dpy) return;

    char local_workspace_str[128] = "";

    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);

    Atom prop = XInternAtom(dpy, "_ASHWM_WORKSPACES", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_to_read = NULL;

    int start_ws = monitor_idx * WORKSPACES_PER_MONITOR;
    int end_ws = start_ws + WORKSPACES_PER_MONITOR;

    if (XGetWindowProperty(dpy, root, prop, 0, 128, False, XA_STRING,
                           &actual_type, &actual_format, &nitems, &bytes_after,
                           &prop_to_read) == Success && prop_to_read) 
    {
        char *prop_copy = strdup((char *)prop_to_read);
        if (prop_copy) {
            char *token = strtok(prop_copy, " ");
            int local_ws_idx = 1; 

            while (token != NULL) {
                int num;
                char status;
                if (sscanf(token, "%d:%c", &num, &status) == 2) {
                    if (num >= start_ws && num < end_ws) {
                        char tmp[32];
                        if (status == 'A') {
                            snprintf(tmp, sizeof(tmp), "%s%d%s ", ACTIVE_WS_L_BRACKET, local_ws_idx, ACTIVE_WS_R_BRACKET);
                        } else if (status == 'O') {
                            snprintf(tmp, sizeof(tmp), "%d ", local_ws_idx);
                        } else {
                            tmp[0] = '\0'; // Nascondi se vuoto
                        }
                        strncat(local_workspace_str, tmp, sizeof(local_workspace_str) - strlen(local_workspace_str) - 1);
                        local_ws_idx++;
                    }
                }
                token = strtok(NULL, " ");
            }
            free(prop_copy);
        }
        XFree(prop_to_read);
    } else {
        snprintf(local_workspace_str, sizeof(local_workspace_str), "(1) ");
    }

    size_t len_ws = strlen(local_workspace_str);
    if (len_ws > 0 && local_workspace_str[len_ws - 1] == ' ') {
        local_workspace_str[len_ws - 1] = '\0';
    }

    BarLayout l;
    build_layout(s, &l);
    
    snprintf(l.left, sizeof(l.left), "%s", local_workspace_str);

    XSetWindowBackground(dpy, win, BACKGROUND_COLOR);
    XSetForeground(dpy, gc, TEXT_COLOR);
    XClearWindow(dpy, win);

    XWindowAttributes wa;
    XGetWindowAttributes(dpy, win, &wa);

    int bar_width = wa.width;
    int bar_height = wa.height;

    int ascent = xft_font->ascent;
    int descent = xft_font->descent;

    int text_y = (bar_height - (ascent + descent)) / 2 + ascent;
    int padding = 10;

    XftDrawStringUtf8(
        monitor_draws[monitor_idx],
        &xft_color,
        xft_font,
        padding,
        text_y,
        (FcChar8 *)l.left,
        strlen(l.left)
    );

    XGlyphInfo ext;
    XftTextExtentsUtf8(dpy, xft_font, (FcChar8 *)l.right, strlen(l.right), &ext);

    int right_x = bar_width - ext.xOff - padding;

    XftDrawStringUtf8(
        monitor_draws[monitor_idx],
        &xft_color,
        xft_font,
        right_x,
        text_y,
        (FcChar8 *)l.right,
        strlen(l.right)
    );

    XFlush(dpy);
}

void 
build_layout(BarState *s, BarLayout *l)
{
    snprintf(l->left, sizeof(l->left), "%s", s->workspace);

    snprintf(l->right, sizeof(l->right),
             "Vol:%s%s%s%sRAM %s%s%s",
             s->volume[0] ? s->volume : "?",
             BAR_SPACER,
             s->ipv4[0] ? s->ipv4 : "?",
             BAR_SPACER,
             s->ram[0] ? s->ram : "?",
             BAR_SPACER,
             s->datetime[0] ? s->datetime : "?");
}


void 
update_datetime(BarState *s)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(s->datetime, sizeof(s->datetime), "%a/%d  %H:%M", tm_info);
}

void 
update_volume(BarState *s)
{
    FILE *f = popen("pactl get-sink-volume @DEFAULT_SINK@", "r");
    if (!f) return;

    char line[256];
    if (fgets(line, sizeof(line), f)) {
        int vol = 0;
        sscanf(line, "Volume: %*[^/]/ %d%%", &vol);
        snprintf(s->volume, sizeof(s->volume), "%d%%", vol);
    }
    pclose(f);
}

void 
update_ram(BarState *s)
{
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return;

    float total = 0, free_ram = 0;
    char line[128];

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "MemTotal: %f kB", &total) == 1) continue;
        if (sscanf(line, "MemAvailable: %f kB", &free_ram) == 1) continue;
    }
    fclose(f);

    float used = total - free_ram;
    snprintf(s->ram, sizeof(s->ram), "%.1fGi/%.1fGi", used / 1024 / 1000, total / 1024 / 1000);
}

void 
update_ipv4(BarState *s) 
{
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
    
    if (getifaddrs(&ifaddr) == -1) {
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, host, NI_MAXHOST);
            if (strcmp(host, "127.0.0.1") != 0) {
                strcpy(s->ipv4, host);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
}

