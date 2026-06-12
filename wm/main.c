// miniwm.c
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

#define TITLE_H 32
#define CLOSE_W 40

typedef struct Client {
    Window client;
    Window frame;
    int x, y, w, h;
    struct Client *next;
} Client;

Display *dpy;
Window root;
Client *clients = NULL;

Client *find_by_frame(Window w) {
    for (Client *c = clients; c; c = c->next) {
        if (c->frame == w) return c;
    }
    return NULL;
}

Client *find_by_client(Window w) {
    for (Client *c = clients; c; c = c->next) {
        if (c->client == w) return c;
    }
    return NULL;
}

void draw_titlebar(Client *c) {
    GC gc = XCreateGC(dpy, c->frame, 0, NULL);

    XSetForeground(dpy, gc, 0x202020);
    XFillRectangle(dpy, c->frame, gc, 0, 0, c->w, TITLE_H);

    XSetForeground(dpy, gc, 0xffffff);
    XDrawString(dpy, c->frame, gc, 10, 21, "MiniWM Window", 13);

    // close button
    XSetForeground(dpy, gc, 0x992222);
    XFillRectangle(dpy, c->frame, gc, c->w - CLOSE_W, 0, CLOSE_W, TITLE_H);

    XSetForeground(dpy, gc, 0xffffff);
    XDrawString(dpy, c->frame, gc, c->w - 25, 21, "X", 1);

    XFreeGC(dpy, gc);
}

void send_delete_window(Window w) {
    Atom wm_protocols = XInternAtom(dpy, "WM_PROTOCOLS", False);
    Atom wm_delete = XInternAtom(dpy, "WM_DELETE_WINDOW", False);

    XEvent ev = {0};
    ev.xclient.type = ClientMessage;
    ev.xclient.window = w;
    ev.xclient.message_type = wm_protocols;
    ev.xclient.format = 32;
    ev.xclient.data.l[0] = wm_delete;
    ev.xclient.data.l[1] = CurrentTime;

    XSendEvent(dpy, w, False, NoEventMask, &ev);
}

void frame_window(Window w) {
    if (find_by_client(w)) return;

    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, w, &attr)) return;
    if (attr.override_redirect) return;

    Client *c = calloc(1, sizeof(Client));
    c->client = w;
    c->x = attr.x;
    c->y = attr.y;
    c->w = attr.width;
    c->h = attr.height;

    c->frame = XCreateSimpleWindow(
        dpy,
        root,
        c->x,
        c->y,
        c->w,
        c->h + TITLE_H,
        1,
        0x444444,
        0x111111
    );

    XSelectInput(
        dpy,
        c->frame,
        ExposureMask |
        ButtonPressMask |
        ButtonReleaseMask |
        PointerMotionMask |
        StructureNotifyMask
    );

    XAddToSaveSet(dpy, w);

    // 把应用窗口塞进 frame，向下偏移 TITLE_H
    XReparentWindow(dpy, w, c->frame, 0, TITLE_H);

    XMapWindow(dpy, c->frame);
    XMapWindow(dpy, w);

    c->next = clients;
    clients = c;

    draw_titlebar(c);
}

int moving = 0;
Client *moving_client = NULL;
int drag_start_x, drag_start_y;
int win_start_x, win_start_y;

void handle_button_press(XButtonEvent *e) {
    Client *c = find_by_frame(e->window);
    if (!c) return;

    if (e->y < TITLE_H) {
        // 点击关闭按钮
        if (e->x >= c->w - CLOSE_W) {
            send_delete_window(c->client);
            return;
        }

        // 标题栏拖动
        moving = 1;
        moving_client = c;
        drag_start_x = e->x_root;
        drag_start_y = e->y_root;
        win_start_x = c->x;
        win_start_y = c->y;
    }
}

void handle_motion(XMotionEvent *e) {
    if (!moving || !moving_client) return;

    int dx = e->x_root - drag_start_x;
    int dy = e->y_root - drag_start_y;

    moving_client->x = win_start_x + dx;
    moving_client->y = win_start_y + dy;

    XMoveWindow(dpy, moving_client->frame, moving_client->x, moving_client->y);
}

void handle_button_release(XButtonEvent *e) {
    moving = 0;
    moving_client = NULL;
}

int main() {
    dpy = XOpenDisplay(NULL);
    if (!dpy) {
        fprintf(stderr, "Cannot open display\n");
        return 1;
    }

    root = DefaultRootWindow(dpy);

    // 成为 WM：如果已经有其他 WM，这里会触发 BadAccess
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(dpy, False);

    printf("MiniWM started\n");

    for (;;) {
        XEvent ev;
        XNextEvent(dpy, &ev);

        switch (ev.type) {
            case MapRequest:
                frame_window(ev.xmaprequest.window);
                break;

            case Expose: {
                Client *c = find_by_frame(ev.xexpose.window);
                if (c) draw_titlebar(c);
                break;
            }

            case ButtonPress:
                handle_button_press(&ev.xbutton);
                break;

            case MotionNotify:
                handle_motion(&ev.xmotion);
                break;

            case ButtonRelease:
                handle_button_release(&ev.xbutton);
                break;

            case ConfigureRequest: {
                XConfigureRequestEvent *e = &ev.xconfigurerequest;
                Client *c = find_by_client(e->window);

                if (!c) {
                    XWindowChanges changes;
                    changes.x = e->x;
                    changes.y = e->y;
                    changes.width = e->width;
                    changes.height = e->height;
                    changes.border_width = e->border_width;
                    changes.sibling = e->above;
                    changes.stack_mode = e->detail;

                    XConfigureWindow(dpy, e->window, e->value_mask, &changes);
                } else {
                    if (e->value_mask & CWX) c->x = e->x;
                    if (e->value_mask & CWY) c->y = e->y;
                    if (e->value_mask & CWWidth) c->w = e->width;
                    if (e->value_mask & CWHeight) c->h = e->height;

                    XMoveResizeWindow(dpy, c->frame, c->x, c->y, c->w, c->h + TITLE_H);
                    XMoveResizeWindow(dpy, c->client, 0, TITLE_H, c->w, c->h);
                    draw_titlebar(c);
                }

                break;
            }
        }
    }

    XCloseDisplay(dpy);
    return 0;
}
