// miniwm.c
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_H 32
#define CLOSE_W 40

typedef struct Client {
	Window body;
	Window frame;
	int x, y, w, h;
	struct Client* next;
	char title[100];
} Client;

Display* dpy;
Window root;
Client* clients = NULL;

Client* find_by_frame(Window w) {
	for (Client* c = clients; c; c = c->next) {
		if (c->frame == w) return c;
	}
	return NULL;
}

Client* find_by_body(Window w) {
	for (Client* c = clients; c; c = c->next) {
		if (c->body == w) return c;
	}
	return NULL;
}

void draw_titlebar(Client* c) {
	GC gc = XCreateGC(dpy, c->frame, 0, NULL);

	XSetForeground(dpy, gc, 0x202020);
	XFillRectangle(dpy, c->frame, gc, 0, 0, c->w, TITLE_H);

	XSetForeground(dpy, gc, 0xffffff);
	// XDrawString(dpy, c->frame, gc, 10, 21, "MiniWM Window", 13);
	XDrawString(dpy, c->frame, gc, 10, 21, c->title, strlen(c->title));

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
	if (find_by_body(w)) return;

	XWindowAttributes attr;
	if (!XGetWindowAttributes(dpy, w, &attr)) return;
	if (attr.override_redirect) return;

	char title[100] = "MiniWM Window";
	printf("\n");
	XClassHint hint;
	if (XGetClassHint(dpy, w, &hint)) {
		printf("res_name  = %s\n", hint.res_name ? hint.res_name : "(null)");
		printf("res_class = %s\n", hint.res_class ? hint.res_class : "(null)");

		if (hint.res_name) {
			strcpy(title, hint.res_name);
			XFree(hint.res_name);
		}
		if (hint.res_class) {
			XFree(hint.res_class);
		}
	} else {
		printf("window has no WM_CLASS\n");
	}
	printf("x %d y %d w %d h %d\n", attr.x, attr.y, attr.width, attr.height);
	printf("border-width %d\n", attr.border_width);
	printf("depth %d\n", attr.depth);
	printf("\n");

	Client* c = calloc(1, sizeof(Client));
	c->body = w;
	c->x = attr.x;
	c->y = attr.y;
	c->w = attr.width;
	c->h = attr.height;

	strcpy(c->title, title);

	c->frame = XCreateSimpleWindow(dpy, root, c->x, c->y, c->w, c->h + TITLE_H,
								   1, 0x444444, 0x111111);

	XSelectInput(dpy, c->frame,
				 ExposureMask | ButtonPressMask | ButtonReleaseMask |
					 PointerMotionMask | StructureNotifyMask);
	XSelectInput(dpy, w, StructureNotifyMask | PropertyChangeMask);

	XAddToSaveSet(dpy, w);

	// 把应用窗口塞进 frame，向下偏移 TITLE_H
	XReparentWindow(dpy, w, c->frame, 0, TITLE_H);

	XMapWindow(dpy, c->frame);
	XMapWindow(dpy, w);

	c->next = clients;
	clients = c;

	draw_titlebar(c);
}

void remove_client(Client* target) {
	Client** p = &clients;

	while (*p) {
		if (*p == target) {
			*p = target->next;
			free(target);
			return;
		}

		p = &(*p)->next;
	}
}

void handle_unmap_notify(XUnmapEvent* e) {
	Client* c = find_by_body(e->window);

	if (c) {
		printf("XEvent: UnmapNotify: body = %s\n", c->title);
		// XReparentWindow(dpy, c->body, root, c->x, c->y);
		// XRemoveFromSaveSet(dpy, c->body);
		return;
	}

	c = find_by_frame(e->window);

	if (c) {
		printf("XEvent: UnmapNotify: frame = %s\n", c->title);
		return;
	}
}

void handle_destroy_notify(XDestroyWindowEvent* e) {
	Client* c = find_by_body(e->window);

	if (c) {
		printf("XEvent: DestroyNotify: body = %s\n", c->title);
		XDestroyWindow(dpy, c->frame);
		remove_client(c);
		return;
	}

	c = find_by_frame(e->window);

	if (c) {
		printf("XEvent: DestroyNotify: frame = %s\n", c->title);
		return;
	}
}

int moving = 0;
Client* moving_client = NULL;
int drag_start_x, drag_start_y;
int win_start_x, win_start_y;

void handle_button_press(XButtonEvent* e) {
	printf("XEvent: ButtonPress\n");
	Client* c = find_by_frame(e->window);
	if (!c) return;

	if (e->y < TITLE_H) {
		// 点击关闭按钮
		if (e->x >= c->w - CLOSE_W) {
			send_delete_window(c->body);
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

void handle_motion_notify(XMotionEvent* e) {
	printf("XEvent: MotionNotify\n");
	if (!moving || !moving_client) return;

	int dx = e->x_root - drag_start_x;
	int dy = e->y_root - drag_start_y;

	moving_client->x = win_start_x + dx;
	moving_client->y = win_start_y + dy;

	XMoveWindow(dpy, moving_client->frame, moving_client->x, moving_client->y);
}

void handle_button_release(XButtonEvent* e) {
	printf("XEvent: ButtonRelease\n");
	moving = 0;
	moving_client = NULL;
}

void handle_expose(XExposeEvent* e) {
	printf("XEvent: Expose\n");
	Client* c = find_by_frame(e->window);
	if (c) draw_titlebar(c);
}

void handle_configure_request(XConfigureRequestEvent* e) {
	printf("XEvent: ConfigureRequest\n");
	Client* c = find_by_body(e->window);

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
		XMoveResizeWindow(dpy, c->body, 0, TITLE_H, c->w, c->h);
		draw_titlebar(c);
	}
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
			case KeyPress:
				printf("XEvent: KeyPress\n");
				break;

			case KeyRelease:
				printf("XEvent: KeyRelease\n");
				break;

			case ButtonPress:
				handle_button_press(&ev.xbutton);
				break;

			case ButtonRelease:
				handle_button_release(&ev.xbutton);
				break;

			case MotionNotify:
				handle_motion_notify(&ev.xmotion);
				break;

			case EnterNotify:
				printf("XEvent: EnterNotify\n");
				break;

			case LeaveNotify:
				printf("XEvent: LeaveNotify\n");
				break;

			case FocusIn:
				printf("XEvent: FocusIn\n");
				break;

			case FocusOut:
				printf("XEvent: FocusOut\n");
				break;

			case KeymapNotify:
				printf("XEvent: KeymapNotify\n");
				break;

			case Expose:
				handle_expose(&ev.xexpose);
				break;

			case GraphicsExpose:
				printf("XEvent: GraphicsExpose\n");
				break;

			case NoExpose:
				printf("XEvent: NoExpose\n");
				break;

			case VisibilityNotify:
				printf("XEvent: VisibilityNotify\n");
				break;

			case CreateNotify:
				printf("XEvent: CreateNotify\n");
				break;

			case DestroyNotify:
				handle_destroy_notify(&ev.xdestroywindow);
				break;

			case UnmapNotify:
				handle_unmap_notify(&ev.xunmap);
				break;

			case MapNotify:
				printf("XEvent: MapNotify\n");
				break;

			case MapRequest:
				printf("XEvent: MapRequest\n");
				frame_window(ev.xmaprequest.window);
				break;

			case ReparentNotify:
				printf("XEvent: ReparentNotify\n");
				break;

			case ConfigureNotify:
				printf("XEvent: ConfigureNotify\n");
				break;

			case ConfigureRequest:
				handle_configure_request(&ev.xconfigurerequest);
				break;

			case GravityNotify:
				printf("XEvent: GravityNotify\n");
				break;

			case ResizeRequest:
				printf("XEvent: ResizeRequest\n");
				break;

			case CirculateNotify:
				printf("XEvent: CirculateNotify\n");
				break;

			case CirculateRequest:
				printf("XEvent: CirculateRequest\n");
				break;

			case PropertyNotify:
				printf("XEvent: PropertyNotify\n");
				break;

			case SelectionClear:
				printf("XEvent: SelectionClear\n");
				break;

			case SelectionRequest:
				printf("XEvent: SelectionRequest\n");
				break;

			case SelectionNotify:
				printf("XEvent: SelectionNotify\n");
				break;

			case ColormapNotify:
				printf("XEvent: ColormapNotify\n");
				break;

			case ClientMessage:
				printf("XEvent: ClientMessage\n");
				break;

			case MappingNotify:
				printf("XEvent: MappingNotify\n");
				break;

			case GenericEvent:
				printf("XEvent: GenericEvent\n");
				break;

			case LASTEvent:
				printf("XEvent: LASTEvent\n");
				break;
		}
	}

	XCloseDisplay(dpy);
	return 0;
}
