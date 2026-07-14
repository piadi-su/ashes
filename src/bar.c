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
XftDraw *xft_draw = NULL;
XftColor xft_color;


//reder the font 
void 
init_font(Display *dpy, Window win, int screen)
{
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

    xft_draw = XftDrawCreate(
        dpy,
        win,
        DefaultVisual(dpy, screen),
        DefaultColormap(dpy, screen)
    );
}

//free everything
void 
cleanup(Display *dpy, Window win, GC gc)
{
    int screen = DefaultScreen(dpy);

	// free the font color
    XftColorFree(
        dpy,
        DefaultVisual(dpy, screen),
        DefaultColormap(dpy, screen),
        &xft_color
    );

	//close the font
    if (xft_font)
        XftFontClose(dpy, xft_font);

    // destroy xft
    if (xft_draw)
        XftDrawDestroy(xft_draw);

    XFreeGC(dpy, gc);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
}


// set all the dock posiotion propieties
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

	if(BOTTOM)
	{
		strut[3] = BAR_HEIGHT;        // top
		strut[10] = 0;         // start x
		strut[11] = width;     // end x
	}

	else{
		strut[2] = BAR_HEIGHT;        // top
		strut[8] = 0;         // start x
		strut[9] = width;     // end x
	}

    Atom strut_atom =
        XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);

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




//make the bar  
void 
draw_bar(Display *dpy, Window win, GC gc, BarState *s)
{
    BarLayout l;
    build_layout(s, &l);

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

    // ================= LEFT =================
    XftDrawStringUtf8(
        xft_draw,
        &xft_color,
        xft_font,
        padding,
        text_y,
        (FcChar8 *)l.left,
        strlen(l.left)
    );

    // ================= RIGHT =================
    XGlyphInfo ext;
    XftTextExtentsUtf8(
        dpy,
        xft_font,
        (FcChar8 *)l.right,
        strlen(l.right),
        &ext
    );

    int right_x = bar_width - ext.xOff - padding;
	// int right_x = bar_width - ext.width - padding;

    XftDrawStringUtf8(
        xft_draw,
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
    // LEFT
    snprintf(l->left, sizeof(l->left),
             "%s", s->workspace);

    // RIGHT 
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



/*============ bar modules =============*/



void 
update_datetime(BarState *s)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    strftime(s->datetime, sizeof(s->datetime),
             "%a/%d  %H:%M", tm_info);
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

    float total = 0, free_ram = 0;
    char line[128];

    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "MemTotal: %f kB", &total) == 1) continue;
        if (sscanf(line, "MemAvailable: %f kB", &free_ram) == 1) continue;
    }

    fclose(f);

    float used = total - free_ram;

    snprintf(s->ram, sizeof(s->ram),
             "%.1fGi/%.1fGi", used / 1024 /1000, total / 1024 /1000);
}


void 
update_ipv4(BarState *s) {
    struct ifaddrs *ifaddr, *ifa;
    char host[NI_MAXHOST];
	
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            void *addr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            inet_ntop(AF_INET, addr, host, NI_MAXHOST);
            if (strcmp(host, "127.0.0.1") != 0) { // ignora localhost
				strcpy(s->ipv4, host);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);
}



//worksapces


int 
connect_i3_ipc(void) 
{
    char *path = getenv("I3SOCK");
    if (!path) path = "/run/user/1000/i3/ipc-socket"; // Fallback standard

    int sock = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

int 
read_full(int fd, void *buf, size_t n) 
{
    size_t off = 0;
    while (off < n) {
        ssize_t r = read(fd, (char*)buf + off, n - off);
        if (r <= 0) return -1;
        off += r;
    }
    return 0;
}

void 
send_i3_message(int sock, uint32_t type, const char *payload) 
{
    uint32_t len = payload ? strlen(payload) : 0;

    write(sock, I3_IPC_MAGIC, 6);
    write(sock, &len, 4);
    write(sock, &type, 4);

    if (len > 0)
        write(sock, payload, len);
}

void 
i3_subscribe(int sock) 
{
    const char *subscribe_json = "[\"workspace\"]";
    send_i3_message(sock, I3_IPC_MESSAGE_TYPE_SUBSCRIBE, subscribe_json);
}

void 
update_workspaces(int query_sock, BarState *s) 
{
    send_i3_message(query_sock, I3_IPC_MESSAGE_TYPE_GET_WORKSPACES, NULL);

    char magic[6];
    uint32_t len, type;

    if (read_full(query_sock, magic, 6) < 0) return;
    if (read_full(query_sock, &len, 4) < 0) return;
    if (read_full(query_sock, &type, 4) < 0) return;

    char *json = malloc(len + 1);
    if (!json) return;

    if (read_full(query_sock, json, len) < 0) {
        free(json);
        return;
    }
    json[len] = '\0';

	//strct for saving json
    struct ws_info {
        int num;
        char name[64];
        int focused;
    } ws_list[32];
    int ws_count = 0;

    char *p = json;
    while ((p = strstr(p, "\"name\":\"")) != NULL && ws_count < 32) {
        p += 8;
        char *end = strchr(p, '"');
        if (!end) break;

        // extract workspace name
        char name[64] = {0};
        size_t nlen = end - p;
        if (nlen >= sizeof(name)) nlen = sizeof(name) - 1;
        memcpy(name, p, nlen);

		//fucussed extract
        char *focused_ptr = strstr(end, "\"focused\":");
        int focused = 0;
        if (focused_ptr && (focused_ptr - end < 150)) {
            if (strncmp(focused_ptr + 10, "true", 4) == 0) {
                focused = 1;
            }
        }

		//get the real workspace number
        int num = atoi(name); 

		// save in the temporary list
        ws_list[ws_count].num = num;
        ws_list[ws_count].focused = focused;
        strcpy(ws_list[ws_count].name, name);
        ws_count++;

        p = end;
    }
    free(json);

    // bubble sort
    for (int i = 0; i < ws_count - 1; i++) {
        for (int j = 0; j < ws_count - i - 1; j++) {
            if (ws_list[j].num > ws_list[j + 1].num) {
                struct ws_info temp = ws_list[j];
                ws_list[j] = ws_list[j + 1];
                ws_list[j + 1] = temp;
            }
        }
    }

    // making the string
    s->workspace[0] = '\0';
    for (int i = 0; i < ws_count; i++) {
        char tmp[128];
        if (ws_list[i].focused)
            snprintf(tmp, sizeof(tmp), "%s%s%s ",ACTIVE_WS_L_BRACKET,ws_list[i].name,ACTIVE_WS_R_BRACKET);
        else
            snprintf(tmp, sizeof(tmp), "%s ", ws_list[i].name);

        strncat(s->workspace, tmp, sizeof(s->workspace) - strlen(s->workspace) - 1);
    }

    // removing left spaces
    size_t len_ws = strlen(s->workspace);
    if (len_ws > 0 && s->workspace[len_ws - 1] == ' ')
        s->workspace[len_ws - 1] = '\0';
}

