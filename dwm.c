/* See LICENSE file for copyright and license details.
 *
 * dynamic window manager is designed like any other X client as well. It is
 * driven through handling X events. In contrast to other X clients, a window
 * manager selects for SubstructureRedirectMask on the root window, to receive
 * events about window (dis-)appearance. Only one X connection at a time is
 * allowed to select for this event mask.
 *
 * The event handlers of dwm are organized in an array which is accessed
 * whenever a new event has been fetched. This allows event dispatching
 * in O(1) time.
 *
 * Each child of the root window is called a client, except windows which have
 * set the override_redirect flag. Clients are organized in a linked client
 * list on each monitor, the focus history is remembered through a stack list
 * on each monitor. Each client contains a bit array to indicate the tags of a
 * client.
 *
 * Keys and tagging rules are organized as arrays and defined in config.h.
 *
 * To understand everything else, start reading main().
 */
#include <errno.h>
#include <locale.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>

#include "drw.h"
#include "util.h"
#include "statustext.h"

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define WORKSPACE(M)            (&(M->workspaces[M->ws_idx]))
#define ISVISIBLE(WS,C)         (C->tags & WS->tags)
#define ISVISIBLE_V(WS,C,V)     (C->tags & (WS->viewtag[V]))
#define ISVISIBLE_F(WS,C)       ((C->tags & WS->tags) && (C->isfloating || C->isfullscreen))
#define ISVISIBLE_WS(C)         (ISVISIBLE(WORKSPACE(C->mon), C))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1ULL << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define TEXTW_(X)               (drw_fontset_getwidth(drw, (X)))

#define WS_VIEWS 2

typedef unsigned int uint;
typedef uint64_t tag_t;

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { /* color schemes */
  SchemeNorm,
  SchemeSel1,
  SchemeSel2,
  SchemeWS,
  SchemeValue1,
  SchemeValue2,
  SchemeValue3,
  SchemeValue4,
  SchemeClientId,
  SchemeOverflow,
};
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
       NetWMFullscreen, NetActiveWindow, NetWMWindowType,
       NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
       ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

typedef union {
	int i;
	tag_t ui;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int click;
	unsigned int mask;
	unsigned int button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Client Client;
struct Client {
  char name[256];
  float mina, maxa;
  int x, y, w, h;
  int oldx, oldy, oldw, oldh;
  int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
  int bw, oldbw;
  tag_t tags;
  int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen, ismaximized;
  int origx, origy, origw, origh;
  Client *next;
  Client *snext;
  Monitor *mon;
  Window win;
};

typedef struct {
	unsigned int mod;
	KeySym keysym;
	void (*func)(const Arg *);
	const Arg arg;
} Key;

typedef struct {
	const char *symbol;
	void (*arrange)(Monitor *);
} Layout;

typedef struct {
	const char *class;
	const char *instance;
	const char *title;
	tag_t tags;
	int isfloating;
	int monitor;
} Rule;

typedef struct {
  tag_t tags;
  tag_t viewtag[WS_VIEWS];
  int spawn_view;
  int spawn_floating;
  const Layout *layout;
  int v1;
  int v2;
  float vf;
} Workspace;

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void buttonpress(XEvent *e);
static void centerwindow(const Arg *arg);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clearviews(const Arg *arg);
static void clientmessage(XEvent *e);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focuscycle(const Arg *arg);
static void focuscycle_view(const Arg *arg);
static void focuscycle_floating(const Arg *arg);
static void focus_1st_1(const Arg *arg);
static void focus_1st_2(const Arg *arg);
void focus_1st_visible(tag_t tags);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incv1(const Arg *arg);
static void incv2(const Arg *arg);
static void incvf(const Arg *arg);
static void juststack(Monitor *m);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void maximize(const Arg *arg);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
void moveclient(Client *, int x, int y, int w, int c);
static void moveclient_x(const Arg *arg);
static void moveclient_y(const Arg *arg);
static void moveclient_w(const Arg *arg);
static void moveclient_h(const Arg *arg);
static void movemouse(const Arg *arg);
static void movepointer(const Arg *arg);
static void movestack(const Arg *arg);
static Client *nexttiled(Client *c, int view);
static void pop(Client *c);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static Monitor *recttomon(int x, int y, int w, int h);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void restack(Monitor *m);
static void run(void);
static void scan(void);
static int sendevent(Client *c, Atom proto);
static void sendmon(Client *c, Monitor *m);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setview_1(const Arg *arg);
static void setview_2(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void snapandcenter_x(const Arg *arg);
static void snapandcenter_y(const Arg *arg);
static void spawn(const Arg *arg);
static void swapspawnview(const Arg *arg);
static void swapviews(const Arg *arg);
static void switchworkspace(const Arg *arg);
static void tag(const Arg *arg);
static void tile_vv(Monitor *m);
static void tile_vh(Monitor *m);
static void tile_hv(Monitor *m);
static void tile_hh(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void togglespawnfloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void unfocus(Client *c, int setfocus);
static void unmanage(Client *c, int destroyed);
static void unmapnotify(XEvent *e);
static void updatebarpos(Monitor *m);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatesizehints(Client *c);
static void updatestatus(void);
static void updatetitle(Client *c);
static void updatewindowtype(Client *c);
static void updatewmhints(Client *c);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);

/* variables */
static const char broken[] = "broken";
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
	[ButtonPress] = buttonpress,
	[ClientMessage] = clientmessage,
	[ConfigureRequest] = configurerequest,
	[ConfigureNotify] = configurenotify,
	[DestroyNotify] = destroynotify,
	[EnterNotify] = enternotify,
	[Expose] = expose,
	[FocusIn] = focusin,
	[KeyPress] = keypress,
	[MappingNotify] = mappingnotify,
	[MapRequest] = maprequest,
	[MotionNotify] = motionnotify,
	[PropertyNotify] = propertynotify,
	[UnmapNotify] = unmapnotify
};
static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
static Monitor *mons, *selmon;
static Window root, wmcheckwin;
static struct timespec ts_last_drawbar;

/* configuration, allows nested code to access above variables */
#include "config.h"

struct Monitor {
  char ltsymbol[16];
  uint8_t ws_idx;
  uint8_t last_ws_idx;
  Workspace workspaces[LENGTH(workspacez)];
  int num;
  int by;               /* bar geometry */
  int mx, my, mw, mh;   /* screen size */
  int wx, wy, ww, wh;   /* window area  */
  int showbar;
  int topbar;
  Client *clients;
  Client *sel;
  Client *stack;
  Monitor *next;
  Window barwin;
  int pointer_oldx, pointer_oldy;
};

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) >= sizeof(tag_t) * 8 ? -1 : 1]; };

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	unsigned int i;
	const Rule *r;
	Monitor *m;
	XClassHint ch = { NULL, NULL };

	/* rule matching */
	c->isfloating = 0;
	XGetClassHint(dpy, c->win, &ch);
	class    = ch.res_class ? ch.res_class : broken;
	instance = ch.res_name  ? ch.res_name  : broken;

	for (i = 0; i < LENGTH(rules); i++) {
		r = &rules[i];
		if ((!r->title || strstr(c->name, r->title))
		&& (!r->class || strstr(class, r->class))
		&& (!r->instance || strstr(instance, r->instance)))
		{
			c->isfloating = r->isfloating;
			c->tags |= r->tags;
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	const Workspace *ws = WORKSPACE(c->mon);
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	if (!(c->tags & TAGMASK)) {
		tag_t t = ws->viewtag[ws->spawn_view];
		c->tags = t ? t : 1;
	}
	if (!c->isfloating)
		c->isfloating = ws->spawn_floating;
}

int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !WORKSPACE(c->mon)->layout->arrange) {
		if (!c->hintsvalid)
			updatesizehints(c);
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

void
arrange(Monitor *m)
{
	if (m)
		showhide(m->stack);
	else for (m = mons; m; m = m->next)
		showhide(m->stack);
	if (m) {
		arrangemon(m);
		restack(m);
	} else for (m = mons; m; m = m->next)
		arrangemon(m);
}

void
arrangemon(Monitor *m)
{
  Workspace *ws = WORKSPACE(m);
  const Layout *l = ws->layout;
  strncpy(m->ltsymbol, l->symbol, sizeof m->ltsymbol);

  if (l->arrange) l->arrange(m);
}

void
attach(Client *c)
{
	c->next = c->mon->clients;
	c->mon->clients = c;
}

void
attachstack(Client *c)
{
	c->snext = c->mon->stack;
	c->mon->stack = c;
}

void
buttonpress(XEvent *e)
{
	unsigned int i, click;
	Arg arg = {0};
	Client *c;
	Monitor *m;
	XButtonPressedEvent *ev = &e->xbutton;

	click = ClkRootWin;
	/* focus monitor if necessary */
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	if (ev->window == selmon->barwin) {
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}

void
centerwindow(const Arg *arg)
{
  Client *c = selmon->sel;
  if (c && c->isfloating) {
    uint maxw = selmon->ww - 2 * c->bw;
    uint maxh = selmon->wh - 2 * c->bw;
    uint w = MIN(c->w, maxw);
    uint h = MIN(c->h, maxh);
    uint x = selmon->wx + (maxw - w) / 2;
    uint y = selmon->wy + (maxh - h) / 2;

    moveclient(c, x, y, w, h);
  }
}

void
checkotherwm(void)
{
	xerrorxlib = XSetErrorHandler(xerrorstart);
	/* this causes an error if some other window manager is running */
	XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
	XSync(dpy, False);
	XSetErrorHandler(xerror);
	XSync(dpy, False);
}

void
cleanup(void)
{
	Layout foo = { "", NULL };
	Monitor *m;
	size_t i;

	for (int i = 0; i < LENGTH(workspacez); i++) {
		for (int j = 0; j < WS_VIEWS; j++)
			selmon->workspaces[i].viewtag[j] = 0;
		selmon->workspaces[i].tags = 0;
	}
	for (m = mons; m; m = m->next) {
		WORKSPACE(m)->layout = &foo;
		while (m->stack)
			unmanage(m->stack, 0);
	}
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		free(scheme[i]);
	free(scheme);
	XDestroyWindow(dpy, wmcheckwin);
	drw_free(drw);
	XSync(dpy, False);
	XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

void
clearviews(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  for (int i = 0; i < WS_VIEWS; i++)
    ws->viewtag[i] = 0;
  ws->tags = 0;
  ws->spawn_view = 0;
  unfocus(selmon->sel, 0);
  arrange(selmon);
}

void
clientmessage(XEvent *e)
{
	XClientMessageEvent *cme = &e->xclient;
	Client *c = wintoclient(cme->window);

	if (!c)
		return;
	if (cme->message_type == netatom[NetWMState]) {
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			setfullscreen(c, (cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	} else if (cme->message_type == netatom[NetActiveWindow]) {
		if (c != selmon->sel && !c->isurgent)
			seturgent(c, 1);
	}
}

void
configure(Client *c)
{
	XConfigureEvent ce;

	ce.type = ConfigureNotify;
	ce.display = dpy;
	ce.event = c->win;
	ce.window = c->win;
	ce.x = c->x;
	ce.y = c->y;
	ce.width = c->w;
	ce.height = c->h;
	ce.border_width = c->bw;
	ce.above = None;
	ce.override_redirect = False;
	XSendEvent(dpy, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void
configurenotify(XEvent *e)
{
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified */
	if (ev->window == root) {
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			}
			focus(NULL);
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !WORKSPACE(selmon)->layout->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
				configure(c);
			if (ISVISIBLE_WS(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
			configure(c);
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
  Monitor *m;

  m = ecalloc(1, sizeof(Monitor));
  m->ws_idx = 0;
  m->last_ws_idx = 0;
  for (int i = 0; i < LENGTH(workspacez); i++) {
    Workspace *ws = &m->workspaces[i];
    ws->layout = &layouts[0];
    for (int j = 0; j < WS_VIEWS; j++)
      ws->viewtag[j] = 0;
    ws->tags = 0;
    ws->v1 = v1_init;
    ws->v2 = v2_init;
    ws->vf = vf_init;
    ws->spawn_view = 0;
    ws->spawn_floating = 0;
  }
  m->showbar = showbar;
  m->topbar = topbar;
  strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
  return m;
}

void
destroynotify(XEvent *e)
{
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;

	if ((c = wintoclient(ev->window)))
		unmanage(c, 1);
}

void
detach(Client *c)
{
	Client **tc;

	for (tc = &c->mon->clients; *tc && *tc != c; tc = &(*tc)->next);
	*tc = c->next;
}

void
detachstack(Client *c)
{
	Client **tc, *t;

	for (tc = &c->mon->stack; *tc && *tc != c; tc = &(*tc)->snext);
	*tc = c->snext;

	if (c == c->mon->sel) {
		for (t = c->mon->stack; t && !ISVISIBLE(WORKSPACE(t->mon), t); t = t->snext);
		c->mon->sel = t;
	}
}

void
drawbar(Monitor *m)
{
  int x = 0, w = 0, tw = 0;
  int boxs = drw->fonts->h / 9;
  int boxw = drw->fonts->h / 6 + 2;
  tag_t occ = 0, urg = 0;
  Client *c;

  if (!m->showbar)
    return;

  /* draw status first so it can be overdrawn by tags later */
  if (m == selmon) { /* status is only drawn on selected monitor */
    render_statustext();

    char buf[sizeof(statustext)];
    char *p1 = statustext, *p2 = buf;
    while (*p1 != '\0') {
      if (*p1 == '\1') {
        p1++;
        if (*p1 == '\0') break;
        p1++;
      } else {
        *p2++ = *p1++;
      }
    }
    *p2 = '\0';

    const uint status_lrpad = 4;
    tw = TEXTW_(buf) + (status_lrpad * 2);

    drw_setscheme(drw, scheme[SchemeNorm]);
    drw_rect(drw, m->ww - tw, 0, tw, bh, 1, 1);
    x += status_lrpad;

    p1 = statustext;
    p2 = buf;
    while (1) {
      if (*p1 == '\0' || *p1 == '\1') {
        if (p2 != buf) {
          *p2 = '\0';
          drw_text(drw, m->ww - tw + x, 0, tw - x, bh, 0, buf, 0);
          x += TEXTW_(buf);
          p2 = buf;
        }

        if (*p1 == '\0') break;
        p1++;
        if (*p1 == '\0') break;

        size_t scheme_idx = MAX((size_t) *p1, 0x20) - 0x20;
        if (scheme_idx >= LENGTH(colors)) {
	  scheme_idx = SchemeNorm;
        }
	drw_setscheme(drw, scheme[scheme_idx]);

        p1++;
      } else {
        *p2++ = *p1++;
      }
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_last_drawbar);
  }

  const Workspace *ws = WORKSPACE(m);
  const uint x_limit = m->ww - tw;

  int cnt_all = 0, c_idx = 0, cnt_1 = 0, cnt_2 = 0;
  for (c = m->clients; c; c = c->next) {
    cnt_all++;
    if (ISVISIBLE_V(ws, c, 0)) cnt_1++;
    if (ISVISIBLE_V(ws, c, 1)) cnt_2++;
    if (c == m->sel) c_idx = cnt_all;

    occ |= c->tags;
    if (c->isurgent) urg |= c->tags;
  }

  x = 0;

  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s", workspacez[m->ws_idx]);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeWS]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 1);

    if (ws->spawn_floating)
      drw_rect(drw, x + boxs, boxs, boxw, boxw, 1, 1);

    x += w;
  }

  {
    char buf[50];
    snprintf(buf, sizeof(buf), "%d / %d / %d", cnt_1, cnt_2, cnt_all);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeNorm]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
    x += w;
  }

  {
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", ws->v1);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeValue1]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
    x += w;

    snprintf(buf, sizeof(buf), "%d", ws->v2);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeValue2]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
    x += w;

    snprintf(buf, sizeof(buf), "%.2f", ws->vf);
    w = TEXTW_(buf) + lrpad / 2;
    drw_setscheme(drw, scheme[SchemeValue3]);
    drw_text(drw, x, 0, w, bh, 0, buf, 0);
    x += w;

    w = TEXTW_(m->ltsymbol) + lrpad / 2;
    drw_setscheme(drw, scheme[SchemeValue4]);
    drw_text(drw, x, 0, w, bh, 0, m->ltsymbol, 0);
    x += w;
  }

  const char overflow[] = "...";
  const int ow = TEXTW(overflow) + 1;
  const uint ox = x_limit - ow;

  for (tag_t bit = 1, i = 0; i < LENGTH(tags) && x < x_limit; i++, bit <<= 1) {
    int view1_on = (bit & ws->viewtag[0]) > 0;
    int view2_on = (bit & ws->viewtag[1]) > 0;
    int has_client = (bit & occ) > 0;
    int is_urgent = (bit & urg) > 0;
    uint scheme_idx = view1_on ? SchemeSel1 : view2_on ? SchemeSel2 : SchemeNorm;
    if (view1_on || view2_on || has_client) {
      w = TEXTW(tags[i]);
      if (x + w > x_limit) w = x_limit - x;

      drw_setscheme(drw, scheme[scheme_idx]);
      drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], is_urgent);
      if (has_client) {
	drw_rect(drw, x + boxs, boxs, boxw, boxw,
		 m == selmon && m->sel && bit & m->sel->tags,
		 is_urgent);
      }

      if (bit & ws->viewtag[ws->spawn_view]) {
        uint ix = x + w - boxw - 1;
        uint iy = 1;
        for (int i = boxw; i > 0; i--, ix++, iy++) {
	  drw_rect(drw, ix, iy, i, 1, 1, 0);
        }
      }

      if (selmon->sel && bit & selmon->sel->tags) {
       uint ix = x + w - 2;
        uint iy = bh - boxw;
        for (int i = 1; i <= boxw; i++, ix--, iy++) {
	  drw_rect(drw, ix, iy, i, 1, 1, 0);
        }
      }

      x += w;
    }
  }

  w = x_limit - x;
  if (w == 0) {
    drw_setscheme(drw, scheme[SchemeOverflow]);
    drw_text(drw, ox + 1, 0, ow - 1, bh, lrpad / 2, overflow, 1);
  } else if (cnt_1 + cnt_2 == 0) {
    drw_setscheme(drw, scheme[SchemeNorm]);
    drw_rect(drw, x, 0, w, bh, 1, 1);
  } else if ((c = m->sel)) {
    char buf[20];
    snprintf(buf, sizeof buf, "%d", c_idx);
    uint iw = TEXTW(buf) + 10;
    uint cw = MIN(MAX(TEXTW(c->name), BAR_CLIENT_MIN_WIDTH), w - iw);

    if (iw + cw <= w) {
      drw_setscheme(drw, scheme[SchemeClientId]);
      drw_text(drw, x, 0, iw, bh, lrpad / 2 + 5, buf, 0);
      x += iw;

      size_t si = c->tags & ws->viewtag[0] ? SchemeSel1 : SchemeSel2;
      drw_setscheme(drw, scheme[si]);
      drw_text(drw, x, 0, cw, bh, lrpad / 2, c->name, 1);
      if (c->isfloating)
	drw_rect(drw, x + boxs, boxs, boxw, boxw, 1, 1);

      x += cw;
    } else {
      drw_setscheme(drw, scheme[SchemeOverflow]);
      drw_text(drw, ox + 1, 0, ow - 1, bh, lrpad / 2, overflow, 1);
    }

    if (x < x_limit) {
      drw_setscheme(drw, scheme[SchemeNorm]);
      drw_rect(drw, x, 0, x_limit - x, bh, 1, 1);
    }
  }

  drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

void
enternotify(XEvent *e)
{
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;

	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) && ev->window != root)
		return;
	c = wintoclient(ev->window);
	m = c ? c->mon : wintomon(ev->window);
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	} else if (!c || c == selmon->sel)
		return;
	focus(c);
}

void
expose(XEvent *e)
{
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

	if (ev->count == 0 && (m = wintomon(ev->window)))
		drawbar(m);
}

void
focus(Client *c)
{
	const Workspace *ws = WORKSPACE(selmon);
	if (!c || !ISVISIBLE_WS(c)) {
		for (c = selmon->stack; c && !ISVISIBLE(ws, c); c = c->snext);
	}
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			seturgent(c, 0);
		detachstack(c);
		attachstack(c);
		grabbuttons(c, 1);
		size_t si = c->tags & ws->viewtag[0] ? SchemeSel1 : SchemeSel2;
		XSetWindowBorder(dpy, c->win, scheme[si][ColBorder].pixel);
		setfocus(c);
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void
focusin(XEvent *e)
{
	XFocusChangeEvent *ev = &e->xfocus;

	if (selmon->sel && ev->window != selmon->sel->win)
		setfocus(selmon->sel);
}

void
focuscycle(const Arg *arg)
{
  Client *c = NULL, *i;

  if (!selmon->sel) {
    focus(NULL);
    restack(selmon);
    return;
  } else if (selmon->sel->isfullscreen && lockfullscreen) {
    return;
  }

  const Workspace *ws = WORKSPACE(selmon);
  const size_t idx = ISVISIBLE_V(ws, selmon->sel, 0) ? 0 : 1;
  if (arg->i > 0) {
    for (c = selmon->sel->next; c && !ISVISIBLE_V(ws, c, idx); c = c->next);
    if (!c)
      for (c = selmon->clients; c && !ISVISIBLE_V(ws, c, idx ^ 1); c = c->next);
    if (!c)
      for (c = selmon->clients; c && !ISVISIBLE_V(ws, c, idx); c = c->next);
  } else {
    for (i = selmon->clients; i != selmon->sel; i = i->next)
      if (ISVISIBLE_V(ws, i, idx))
	c = i;
    if (!c)
      for (i = selmon->clients; i; i = i->next)
	if (ISVISIBLE_V(ws, i, idx ^ 1))
	  c = i;
    if (!c)
      for (i = selmon->sel->next; i; i = i->next)
	if (ISVISIBLE_V(ws, i, idx))
	  c = i;
  }

  if (c && c != selmon->sel) {
    focus(c);
    restack(selmon);
  }
}

void
focuscycle_floating(const Arg *arg)
{
  Client *c = NULL, *i;
  const Workspace *ws = WORKSPACE(selmon);

  if (!selmon->sel)
    for (c = selmon->clients; c && !ISVISIBLE_F(ws, c); c = c->next);
  else if (selmon->sel->isfullscreen && lockfullscreen)
    return;
  else {
    if (arg->i > 0) {
      for (c = selmon->sel->next; c && !ISVISIBLE_F(ws, c); c = c->next);
      if (!c)
	for (c = selmon->clients; c && !ISVISIBLE_F(ws, c); c = c->next);
    } else {
      for (i = selmon->clients; i != selmon->sel; i = i->next)
	if (ISVISIBLE_F(ws, i))
	  c = i;
      if (!c)
	for (i = i->next; i; i = i->next)
	  if (ISVISIBLE_F(ws, i))
	    c = i;
    }
  }

  if (c && c != selmon->sel) {
    focus(c);
    restack(selmon);
  }
}

void
focuscycle_view(const Arg *arg)
{
  Client *c = NULL, *i;

  if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
    return;

  const Workspace *ws = WORKSPACE(selmon);
  const size_t idx = ISVISIBLE_V(ws, selmon->sel, 0) ? 0 : 1;

  if (arg->i > 0) {
    for (c = selmon->sel->next; c && !ISVISIBLE_V(ws, c, idx); c = c->next);
    if (!c)
      for (c = selmon->clients; c && !ISVISIBLE_V(ws, c, idx); c = c->next);
  } else {
    for (i = selmon->clients; i != selmon->sel; i = i->next)
      if (ISVISIBLE_V(ws, i, idx))
	c = i;
    if (!c)
      for (i = i->next; i; i = i->next)
	if (ISVISIBLE_V(ws, i, idx))
	  c = i;
  }

  if (c && c != selmon->sel) {
    focus(c);
    restack(selmon);
  }
}

void
focus_1st_1(const Arg *arg)
{
  Client *c;
  const Workspace *ws = WORKSPACE(selmon);

  for (c = selmon->clients; c && !ISVISIBLE_V(ws, c, 0); c = c->next);
  if (c && c != selmon->sel) {
    focus(c);
    restack(selmon);
  }
}

void
focus_1st_2(const Arg *arg)
{
  Client *c;
  const Workspace *ws = WORKSPACE(selmon);

  for (c = selmon->clients; c && !ISVISIBLE_V(ws, c, 1); c = c->next);
  if (c && c != selmon->sel) {
    focus(c);
    restack(selmon);
  }
}

void
focus_1st_visible(tag_t tags)
{
  Client *c, *tiled_candidate = NULL;
  for (c = selmon->stack; c; c = c->snext) {
    if (c->tags & tags) {
      if (!tiled_candidate) tiled_candidate = c;
      if (c->isfloating) {
	focus(c);
	return;
      }
    }
  }

  focus(tiled_candidate);
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;
	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;
	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j, k;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		int start, end, skip;
		KeySym *syms;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		XDisplayKeycodes(dpy, &start, &end);
		syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
		if (!syms)
			return;
		for (k = start; k <= end; k++)
			for (i = 0; i < LENGTH(keys); i++)
				/* skip modifier codes, we do that ourselves */
				if (keys[i].keysym == syms[(k - start) * skip])
					for (j = 0; j < LENGTH(modifiers); j++)
						XGrabKey(dpy, k,
							 keys[i].mod | modifiers[j],
							 root, True,
							 GrabModeAsync, GrabModeAsync);
		XFree(syms);
	}
}

void
incv1(const Arg *arg)
{
  if (selmon->sel && selmon->sel->isfloating) return;

  Workspace *ws = WORKSPACE(selmon);
  ws->v1 = MAX(ws->v1 + arg->i, 0);
  arrange(selmon);
}

void
incv2(const Arg *arg)
{
  if (selmon->sel && selmon->sel->isfloating) return;

  Workspace *ws = WORKSPACE(selmon);
  ws->v2 = MAX(ws->v2 + arg->i, 0);
  arrange(selmon);
}


void
incvf(const Arg *arg)
{
  if (selmon->sel && selmon->sel->isfloating) return;

  Workspace *ws = WORKSPACE(selmon);
  float f = arg->f + ws->vf;
  if (f < 0.05 || f > 0.95) return;

  ws->vf = f;
  arrange(selmon);
}

#ifdef XINERAMA
static int
isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info)
{
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

void
juststack(Monitor *m)
{
  Client *c;
  const Workspace *ws = WORKSPACE(selmon);

  if (ws->v1 || ws->v2) {
    for (c = nexttiled(m->clients, -1); c; c = nexttiled(c->next, -1)) {
      uint maxw = selmon->ww - 2 * c->bw;
      uint maxh = selmon->wh - 2 * c->bw;
      uint w = MIN(c->w, maxw);
      uint h = MIN(c->h, maxh);
      uint x = m->wx + (maxw - w) / 2;
      uint y = m->wy + (maxh - h) / 2;

      resize(c, x, y, w, h, 0);
    }
  } else {
    for (c = nexttiled(m->clients, -1); c; c = nexttiled(c->next, -1)) {
      resize(c, m->wx, m->wy,
	     MIN(c->w, m->ww - 2 * c->bw), MIN(c->h, m->wh - 2 * c->bw),
	     0);
    }
  }
}

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym;
	XKeyEvent *ev;

	ev = &e->xkey;
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
	for (i = 0; i < LENGTH(keys); i++)
		if (keysym == keys[i].keysym
		&& CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
		&& keys[i].func)
			keys[i].func(&(keys[i].arg));
}

void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!sendevent(selmon->sel, wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
}

void
manage(Window w, XWindowAttributes *wa)
{
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = ecalloc(1, sizeof(Client));
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

	updatetitle(c);
	if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	} else {
		c->mon = selmon;
		applyrules(c);
	}

	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
	c->bw = borderpx;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
	configure(c); /* propagates border_width, if size doesn't change */
	updatewindowtype(c);
	updatesizehints(c);
	updatewmhints(c);
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
		XRaiseWindow(dpy, c->win);
	attach(c);
	attachstack(c);
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
	setclientstate(c, NormalState);
	if (c->mon == selmon)
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
	arrange(c->mon);
	XMapWindow(dpy, c->win);
	focus(NULL);
}

void
mappingnotify(XEvent *e)
{
	XMappingEvent *ev = &e->xmapping;

	XRefreshKeyboardMapping(ev);
	if (ev->request == MappingKeyboard)
		grabkeys();
}

void
maprequest(XEvent *e)
{
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
	if (!wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
maximize(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  switch (c->ismaximized) {
  case 1:
    resize(c, selmon->wx, selmon->wy, selmon->mw - 2 * c->bw, selmon->mh - 2 * c->bw, 0);
    c->ismaximized = 2;
    break;
  case 2:
    resize(c, c->origx, c->origy, c->origw, c->origh, 0);
    c->ismaximized = 0;
    break;
  default:
    c->origx = c->x;
    c->origy = c->y;
    c->origw = c->w;
    c->origh = c->h;
    resize(c, selmon->wx, selmon->wy, selmon->ww - 2 * c->bw, selmon->wh - 2 * c->bw, 0);
    c->ismaximized = 1;
  }

}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;
	const Workspace *ws = WORKSPACE(m);

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(ws, c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients, -1); c; c = nexttiled(c->next, -1))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

void
motionnotify(XEvent *e)
{
	static Monitor *mon = NULL;
	Monitor *m;
	XMotionEvent *ev = &e->xmotion;

	if (ev->window != root)
		return;
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
moveclient(Client *c, int x, int y, int w, int h)
{
  resize(c, x, y, w, h, 0);
  XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, w / 2, h / 2);
}

void
moveclient_x(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int x;
  if (arg->f < 0.0) {
    x = MAX(c->x + ((int) selmon->ww * MAX(arg->f, -1.0)), selmon->wx);
  } else {
    x = MIN(c->x + ((int) selmon->ww * MIN(arg->f, 1.0)), selmon->ww - c->w - 2 * c->bw);
  }

  moveclient(c, x, c->y, c->w, c->h);
}

void
moveclient_y(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int y;
  if (arg->f < 0.0) {
    y = MAX(c->y + ((int) selmon->wh * MAX(arg->f, -1.0)), selmon->wy);
  } else {
    y = MIN(c->y + ((int) selmon->wh * MIN(arg->f, 1.0)), selmon->wh - c->h - 2 * c->bw);
  }

  moveclient(c, c->x, y, c->w, c->h);
}

void
moveclient_w(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int x = c->x;
  int w;
  if (arg->f < 0.0) {
    w = MAX(c->w + ((int) selmon->ww * MAX(arg->f, -1.0)), 100);
  } else {
    w = MIN(c->w + ((int) selmon->ww * MIN(arg->f, 1.0)), selmon->ww - 2 * c->bw);
    int diff = (x + w) - (selmon->ww - 2 * c->bw);
    if (diff > 0) x -= diff;
  }

  moveclient(c, x, c->y, w, c->h);
}

void
moveclient_h(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int y = c->y;
  int h;
  if (arg->f < 0.0) {
    h = MAX(c->h + ((int) selmon->wh * MAX(arg->f, -1.0)), 100);
  } else {
    h = MIN(c->h + ((int) selmon->wh * MIN(arg->f, 1.0)), selmon->wh - 2 * c->bw);
    int diff = (y + h) - (selmon->wh - 2 * c->bw);
    if (diff > 0) y -= diff;
  }

  moveclient(c, c->x, y, c->w, h);
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && WORKSPACE(selmon)->layout->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!WORKSPACE(selmon)->layout->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
movepointer(const Arg *arg)
{
  Client *c = selmon->sel;

  int x, y;
  if (arg->i > 0) {
    if (!c) return;

    x = c->x + (c->w / 2);
    y = c->y + (c->h / 2);
  } else {
    x = selmon->mw / 2;
    y = selmon->mh - 1;
  }

  Window r, w;
  int rx, ry, wx, wy;
  unsigned int mask;

  if (XQueryPointer(dpy, root, &r, &w, &rx, &ry, &wx, &wy, &mask)
      == False)
    return;

  if (x == rx && y == ry) {
    x = selmon->pointer_oldx;
    y = selmon->pointer_oldy;
  } else {
    selmon->pointer_oldx = rx;
    selmon->pointer_oldy = ry;
  }
  XWarpPointer(dpy, None, root, 0, 0, 0, 0, x, y);
}

void
movestack(const Arg *arg)
{
  if (!selmon->sel || selmon->sel->isfloating) return;

  Client *c = NULL, *p = NULL, *pc = NULL, *i;
  const Workspace *ws = WORKSPACE(selmon);
  const size_t idx = ISVISIBLE_V(ws, selmon->sel, 0) ? 0 : 1;

  if(arg->i > 0) {
    /* find the client after selmon->sel */
    for(c = selmon->sel->next;
	c && (!ISVISIBLE_V(ws, c, idx) || c->isfloating); c = c->next);
    if(!c)
      for(c = selmon->clients;
	  c && (!ISVISIBLE_V(ws, c, idx) || c->isfloating); c = c->next);
  }
  else {
    /* find the client before selmon->sel */
    for(i = selmon->clients; i != selmon->sel; i = i->next)
      if(ISVISIBLE_V(ws, i, idx) && !i->isfloating)
	c = i;
    if(!c)
      for(; i; i = i->next)
	if(ISVISIBLE_V(ws, i, idx) && !i->isfloating)
	  c = i;
  }

  if(!c || c == selmon->sel) return;

  /* find the client before selmon->sel and c */
  for(i = selmon->clients; i && (!p || !pc); i = i->next) {
    if(i->next == selmon->sel)
      p = i;
    if(i->next == c)
      pc = i;
  }

  /* swap c and selmon->sel selmon->clients in the selmon->clients list */
  Client *temp = selmon->sel->next == c ? selmon->sel : selmon->sel->next;
  selmon->sel->next = c->next == selmon->sel ? c : c->next;
  c->next = temp;

  if(p && p != c)
    p->next = c;
  if(pc && pc != selmon->sel)
    pc->next = selmon->sel;

  if(selmon->sel == selmon->clients)
    selmon->clients = c;
  else if(c == selmon->clients)
    selmon->clients = selmon->sel;

  arrange(selmon);
}

Client *
nexttiled(Client *c, int view)
{
  if (c) {
    const Workspace *ws = WORKSPACE(c->mon);
    if (view < 0 || view >= WS_VIEWS)
      for (; c && (c->isfloating || !ISVISIBLE(ws, c)); c = c->next);
    else
      for (; c && (c->isfloating || !ISVISIBLE_V(ws, c, view)); c = c->next);
  }
  return c;
}

void
pop(Client *c)
{
	detach(c);
	attach(c);
	focus(c);
	arrange(c->mon);
}

void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			updatewmhints(c);
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
			updatetitle(c);
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
			updatewindowtype(c);
	}
}

void
quit(const Arg *arg)
{
	running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
	configure(c);
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && WORKSPACE(selmon)->layout->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!WORKSPACE(selmon)->layout->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;
	const Workspace *ws = WORKSPACE(m);

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !ws->layout->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (WORKSPACE(m)->layout->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(ws, c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void
run(void)
{
  XEvent ev;
  int x11_fd;
  fd_set in_fds;
  struct timespec timer;
  ts_last_drawbar.tv_sec = ts_last_drawbar.tv_nsec = 0;

  x11_fd = ConnectionNumber(dpy);
  /* main event loop */
  XSync(dpy, False);
  while (running) {
    FD_ZERO(&in_fds);
    FD_SET(x11_fd, &in_fds);

    if (selmon->showbar) {
      clock_gettime(CLOCK_MONOTONIC, &timer);

      timer.tv_sec -= ts_last_drawbar.tv_sec;
      timer.tv_nsec -= ts_last_drawbar.tv_nsec;
      if (timer.tv_nsec < 0) {
	timer.tv_sec--;
	timer.tv_nsec += 1.0e9;
      }
      if (timer.tv_sec > 0) {
	drawbar(selmon);

	timer.tv_sec = 1;
	timer.tv_nsec = 0;
      } else {
	timer.tv_nsec = 1.0e9 - timer.tv_nsec;
      }
    }

    int num_ready_fds = pselect(x11_fd + 1, &in_fds, NULL, NULL,
				selmon->showbar ? &timer : NULL, NULL);
    if (num_ready_fds < 0) {
      printf("An error occured!\n");
      break;
    } else if (num_ready_fds > 0) {
      while(XPending(dpy))
	if (!XNextEvent(dpy, &ev))
	  if (handler[ev.type])
	    handler[ev.type](&ev);
    }
  }
}

void
scan(void)
{
	unsigned int i, num;
	Window d1, d2, *wins = NULL;
	XWindowAttributes wa;

	if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
		for (i = 0; i < num; i++) {
			if (!XGetWindowAttributes(dpy, wins[i], &wa)
			|| wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
				continue;
			if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
				manage(wins[i], &wa);
		}
		for (i = 0; i < num; i++) { /* now the transients */
			if (!XGetWindowAttributes(dpy, wins[i], &wa))
				continue;
			if (XGetTransientForHint(dpy, wins[i], &d1)
			&& (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
				manage(wins[i], &wa);
		}
		if (wins)
			XFree(wins);
	}
}

void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	detach(c);
	detachstack(c);
	c->mon = m;
	c->tags = WORKSPACE(m)->tags;
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setclientstate(Client *c, long state)
{
	long data[] = { state, None };

	XChangeProperty(dpy, c->win, wmatom[WMState], wmatom[WMState], 32,
		PropModeReplace, (unsigned char *)data, 2);
}

int
sendevent(Client *c, Atom proto)
{
	int n;
	Atom *protocols;
	int exists = 0;
	XEvent ev;

	if (XGetWMProtocols(dpy, c->win, &protocols, &n)) {
		while (!exists && n--)
			exists = protocols[n] == proto;
		XFree(protocols);
	}
	if (exists) {
		ev.type = ClientMessage;
		ev.xclient.window = c->win;
		ev.xclient.message_type = wmatom[WMProtocols];
		ev.xclient.format = 32;
		ev.xclient.data.l[0] = proto;
		ev.xclient.data.l[1] = CurrentTime;
		XSendEvent(dpy, c->win, False, NoEventMask, &ev);
	}
	return exists;
}

void
setfocus(Client *c)
{
	if (!c->neverfocus) {
		XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
		XChangeProperty(dpy, root, netatom[NetActiveWindow],
			XA_WINDOW, 32, PropModeReplace,
			(unsigned char *) &(c->win), 1);
	}
	sendevent(c, wmatom[WMTakeFocus]);
}

void
setfullscreen(Client *c, int fullscreen)
{
	if (fullscreen && !c->isfullscreen) {
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
		c->isfullscreen = 1;
		c->oldstate = c->isfloating;
		c->oldbw = c->bw;
		c->bw = 0;
		c->isfloating = 1;
		resizeclient(c, c->mon->mx, c->mon->my, c->mon->mw, c->mon->mh);
		XRaiseWindow(dpy, c->win);
	} else if (!fullscreen && c->isfullscreen){
		XChangeProperty(dpy, c->win, netatom[NetWMState], XA_ATOM, 32,
			PropModeReplace, (unsigned char*)0, 0);
		c->isfullscreen = 0;
		c->isfloating = c->oldstate;
		c->bw = c->oldbw;
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		resizeclient(c, c->x, c->y, c->w, c->h);
		arrange(c->mon);
	}
}

void
setlayout(const Arg *arg)
{
  if (!arg) return;

  size_t i = arg->ui;
  if (i >= LENGTH(layouts)) return;

  Workspace *ws = WORKSPACE(selmon);
  ws->layout = &layouts[i];
  strncpy(selmon->ltsymbol, ws->layout->symbol, sizeof selmon->ltsymbol);

  if (selmon->sel)
    arrange(selmon);
  else
    drawbar(selmon);
}

void
setviews(size_t view, uint tag_idx)
{
  if (view >= WS_VIEWS) return;
  if (tag_idx < 0 || tag_idx >= LENGTH(tags)) return;

  Workspace *ws = WORKSPACE(selmon);
  tag_t tag = 1ULL << tag_idx;
  size_t other = view ^ 1;
  if (tag == ws->viewtag[view]) {
    ws->viewtag[view] = 0;
    ws->spawn_view = other;
  } else {
    if (tag == ws->viewtag[other]) {
      ws->viewtag[other] = ws->viewtag[view];
    }
    ws->viewtag[view] = tag;
    ws->spawn_view = view;
  }

  ws->tags = ws->viewtag[view] | ws->viewtag[other];
  focus_1st_visible(tag);
  arrange(selmon);
}

void
setview_1(const Arg *arg)
{
  setviews(0, arg->i);
}

void
setview_2(const Arg *arg)
{
  setviews(1, arg->i);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

	status_dirpath = getenv(ENV_STATUS_DIR);

	/* do not transform children into zombies when they terminate */
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGCHLD, &sa, NULL);

	/* clean up any zombies (inherited from .xinitrc etc) immediately */
	while (waitpid(-1, NULL, WNOHANG) > 0);

	/* init screen */
	screen = DefaultScreen(dpy);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	root = RootWindow(dpy, screen);
	drw = drw_create(dpy, screen, root, sw, sh);
	if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
		die("no fonts could be loaded.");
	lrpad = drw->fonts->h;
	bh = drw->fonts->h + 2;
	updategeom();
	/* init atoms */
	utf8string = XInternAtom(dpy, "UTF8_STRING", False);
	wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
	wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
	wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
	wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
	netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
	netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
	netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
	netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
	netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
	netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
	netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
	netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	/* init cursors */
	cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
	cursor[CurResize] = drw_cur_create(drw, XC_sizing);
	cursor[CurMove] = drw_cur_create(drw, XC_fleur);
	/* init appearance */
	scheme = ecalloc(LENGTH(colors), sizeof(Clr *));
	for (i = 0; i < LENGTH(colors); i++)
		scheme[i] = drw_scm_create(drw, colors[i], 3);
	/* init bars */
	updatebars();
	updatestatus();
	/* supporting window for NetWMCheck */
	wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
		PropModeReplace, (unsigned char *) "dwm", 3);
	XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
		PropModeReplace, (unsigned char *) &wmcheckwin, 1);
	/* EWMH support per view */
	XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
		PropModeReplace, (unsigned char *) netatom, NetLast);
	XDeleteProperty(dpy, root, netatom[NetClientList]);
	/* select events */
	wa.cursor = cursor[CurNormal]->cursor;
	wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
		|ButtonPressMask|PointerMotionMask|EnterWindowMask
		|LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
	XSelectInput(dpy, root, wa.event_mask);
	grabkeys();
	focus(NULL);
}

void
seturgent(Client *c, int urg)
{
	XWMHints *wmh;

	c->isurgent = urg;
	if (!(wmh = XGetWMHints(dpy, c->win)))
		return;
	wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
	XSetWMHints(dpy, c->win, wmh);
	XFree(wmh);
}

void
showhide(Client *c)
{
	if (!c)
		return;
	const Workspace *ws = WORKSPACE(c->mon);
	if (ISVISIBLE(ws, c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!ws->layout->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

void
snapandcenter_x(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int x;
  if (arg->i < 0) {
    x = selmon->wx;
  } else {
    x = selmon->ww - c->w - 2 * c->bw;
  }
  int y = (selmon->wh - c->h) / 2;

  moveclient(c, x, y, c->w, c->h);
}

void
snapandcenter_y(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || !c->isfloating) return;

  int x = (selmon->ww - c->w) / 2;
  int y;
  if (arg->i < 0) {
    y = selmon->wy;
  } else {
    y = selmon->wh - c->h - 2 * c->bw;
  }

  moveclient(c, x, y, c->w, c->h);
}

void
spawn(const Arg *arg)
{
	const Workspace *ws = WORKSPACE(selmon);
	if (!ws->viewtag[ws->spawn_view]) return;

	struct sigaction sa;

	if (arg->v == dmenucmd)
		dmenumon[0] = '0' + selmon->num;
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();

		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = SIG_DFL;
		sigaction(SIGCHLD, &sa, NULL);

		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

void
swapspawnview(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  if (!ws->viewtag[0] || !ws->viewtag[1])
    return;
  ws->spawn_view ^= 1;
  drawbar(selmon);
}

void
swapviews(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  if (!ws->tags) return;

  tag_t temp = ws->viewtag[0];
  ws->viewtag[0] = ws->viewtag[1];
  ws->viewtag[1] = temp;
  ws->spawn_view ^= 1;
  arrange(selmon);
}

void
switchworkspace(const Arg *arg)
{
  if (!arg) return;
  int i =
    (arg->i >= 0 && arg->i != selmon->ws_idx) ? arg->i
    : selmon->last_ws_idx;

  if (i >= LENGTH(workspacez) || i == selmon->ws_idx)
    return;

  selmon->last_ws_idx = selmon->ws_idx;
  selmon->ws_idx = i;
  Workspace *ws = WORKSPACE(selmon);

  focus_1st_visible(ws->tags);
  strncpy(selmon->ltsymbol, ws->layout->symbol, sizeof selmon->ltsymbol);
  arrange(selmon);
}

void
tag(const Arg *arg)
{
  if (!selmon->sel) return;

  Workspace *ws = WORKSPACE(selmon);
  tag_t arg_tag = arg->ui == 0 ? ws->tags : arg->ui & TAGMASK;
  if (!arg_tag) return;

  selmon->sel->tags = arg_tag;

  if (!(arg_tag & ws->tags)) {
    focus_1st_visible(ws->tags);
  } else {
    size_t si = arg_tag & ws->viewtag[0] ? SchemeSel1 : SchemeSel2;
    XSetWindowBorder(dpy, selmon->sel->win, scheme[si][ColBorder].pixel);
  }
  arrange(selmon);
}

void
tile(Monitor *m, int vert1, int vert2)
{
  uint n, m_cnt = 0, s_cnt = 0;
  Client *c;
  Workspace *ws = WORKSPACE(m);

  const tag_t tag1 = ws->viewtag[0];
  const tag_t tag2 = ws->viewtag[1];
  for (n = 0, c = nexttiled(m->clients, -1); c; c = nexttiled(c->next, -1), n++) {
    if (c->tags & tag1) m_cnt++;
    else if (c->tags & tag2) s_cnt++;
  }
  if (!n) return;

  c = nexttiled(m->clients, -1);

  const uint w1 =
    tag1 && tag2 ? m->ww * ws->vf
    : tag1 ? m->ww : 0;
  const uint w2 = m->ww - w1;
  const uint x1 = m->wx;
  const uint x2 = m->wx + w1;

  const uint div1 = ws->v1 ? MIN(m_cnt, ws->v1) : m_cnt;
  const uint div2 = ws->v2 ? MIN(s_cnt, ws->v2) : s_cnt;
  const uint lim1 = div1 - 1;
  const uint lim2 = div2 - 1;

  const uint each1 = div1 ? (vert1 ? m->wh : w1) / div1 : 0;
  const uint each2 = div2 ? (vert2 ? m->wh : w2) / div2 : 0;
  const uint r1 = vert1 ? m->wh - each1 * div1 : w1 - each1 * div1;
  const uint r2 = vert2 ? m->wh - each2 * div2 : w2 - each2 * div2;

  for (int i1 = 0, i2 = 0; n; n--, c = nexttiled(c->next, -1)) {
    if (c->tags & tag1) {
      if (vert1)
	resize(c, x1, m->wy + MIN(i1, lim1) * each1, w1 - (2*c->bw),
	       each1 + (i1 < lim1 ? 0 : r1) - (2*c->bw), 0);
      else
	resize(c, x1 + MIN(i1, lim1) * each1, m->wy,
	       each1 + (i1 < lim1 ? 0 : r1) - (2*c->bw),
	       m->wh - (2*c->bw), 0);
      i1++;
    } else {
      if (vert2)
	resize(c, x2, m->wy + MIN(i2, lim2) * each2, w2 - (2*c->bw),
	       each2 + (i2 < lim2 ? 0 : r2) - (2*c->bw), 0);
      else
	resize(c, x2 + MIN(i2, lim2) * each2, m->wy,
	       each2 + (i2 < lim2 ? 0 : r2) - (2*c->bw),
	       m->wh - (2*c->bw), 0);
      i2++;
    }
  }
}

void
tile_vv(Monitor *m)
{
  tile(m, 1, 1);
}

void
tile_vh(Monitor *m)
{
  tile(m, 1, 0);
}

void
tile_hv(Monitor *m)
{
  tile(m, 0, 1);
}

void
tile_hh(Monitor *m)
{
  tile(m, 0, 0);
}

void
togglebar(const Arg *arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
togglespawnfloating(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  ws->spawn_floating ^= 1;
  drawbar(selmon);
}

void
toggletag(const Arg *arg)
{
  tag_t newtags;
  if (!selmon->sel) return;

  newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
  if (newtags) {
    selmon->sel->tags = newtags;

    Workspace *ws = WORKSPACE(selmon);
    if (!(newtags & ws->tags)) {
      focus_1st_visible(ws->tags);
      arrange(selmon);
    } else {
      drawbar(selmon);
    }
  }
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}

void
unmanage(Client *c, int destroyed)
{
	Monitor *m = c->mon;
	XWindowChanges wc;

	detach(c);
	detachstack(c);
	if (!destroyed) {
		wc.border_width = c->oldbw;
		XGrabServer(dpy); /* avoid race conditions */
		XSetErrorHandler(xerrordummy);
		XSelectInput(dpy, c->win, NoEventMask);
		XConfigureWindow(dpy, c->win, CWBorderWidth, &wc); /* restore border */
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		setclientstate(c, WithdrawnState);
		XSync(dpy, False);
		XSetErrorHandler(xerror);
		XUngrabServer(dpy);
	}
	free(c);
	focus(NULL);
	updateclientlist();
	arrange(m);
}

void
unmapnotify(XEvent *e)
{
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = wintoclient(ev->window))) {
		if (ev->send_event)
			setclientstate(c, WithdrawnState);
		else
			unmanage(c, 0);
	}
}

void
updatebars(void)
{
	Monitor *m;
	XSetWindowAttributes wa = {
		.override_redirect = True,
		.background_pixmap = ParentRelative,
		.event_mask = ButtonPressMask|ExposureMask
	};
	XClassHint ch = {"dwm", "dwm"};
	for (m = mons; m; m = m->next) {
		if (m->barwin)
			continue;
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
		XMapRaised(dpy, m->barwin);
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateclientlist(void)
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

int
updategeom(void)
{
	int dirty = 0;

#ifdef XINERAMA
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++);
		/* only consider unique geometries as separate screens */
		unique = ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);
		nn = j;

		/* new monitors if nn > n */
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next);
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				detachstack(c);
				c->mon = mons;
				attach(c);
				attachstack(c);
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}

void
updatesizehints(Client *c)
{
	long msize;
	XSizeHints size;

	if (!XGetWMNormalHints(dpy, c->win, &size, &msize))
		/* size is uninitialized, ensure that size.flags aren't used */
		size.flags = PSize;
	if (size.flags & PBaseSize) {
		c->basew = size.base_width;
		c->baseh = size.base_height;
	} else if (size.flags & PMinSize) {
		c->basew = size.min_width;
		c->baseh = size.min_height;
	} else
		c->basew = c->baseh = 0;
	if (size.flags & PResizeInc) {
		c->incw = size.width_inc;
		c->inch = size.height_inc;
	} else
		c->incw = c->inch = 0;
	if (size.flags & PMaxSize) {
		c->maxw = size.max_width;
		c->maxh = size.max_height;
	} else
		c->maxw = c->maxh = 0;
	if (size.flags & PMinSize) {
		c->minw = size.min_width;
		c->minh = size.min_height;
	} else if (size.flags & PBaseSize) {
		c->minw = size.base_width;
		c->minh = size.base_height;
	} else
		c->minw = c->minh = 0;
	if (size.flags & PAspect) {
		c->mina = (float)size.min_aspect.y / size.min_aspect.x;
		c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
	} else
		c->maxa = c->mina = 0.0;
	c->isfixed = (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
	c->hintsvalid = 1;
}

void
updatestatus(void)
{
}

void
updatetitle(Client *c)
{
	if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name))
		gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
	if (c->name[0] == '\0') /* hack to mark broken clients */
		strcpy(c->name, broken);
}

void
updatewindowtype(Client *c)
{
	Atom state = getatomprop(c, netatom[NetWMState]);
	Atom wtype = getatomprop(c, netatom[NetWMWindowType]);

	if (state == netatom[NetWMFullscreen])
		setfullscreen(c, 1);
	if (wtype == netatom[NetWMWindowTypeDialog])
		c->isfloating = 1;
}

void
updatewmhints(Client *c)
{
	XWMHints *wmh;

	if ((wmh = XGetWMHints(dpy, c->win))) {
		if (c == selmon->sel && wmh->flags & XUrgencyHint) {
			wmh->flags &= ~XUrgencyHint;
			XSetWMHints(dpy, c->win, wmh);
		} else
			c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
		if (wmh->flags & InputHint)
			c->neverfocus = !wmh->input;
		else
			c->neverfocus = 0;
		XFree(wmh);
	}
}

Client *
wintoclient(Window w)
{
	Client *c;
	Monitor *m;

	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			if (c->win == w)
				return c;
	return NULL;
}

Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int
xerror(Display *dpy, XErrorEvent *ee)
{
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
		return 0;
	fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
		ee->request_code, ee->error_code);
	return xerrorxlib(dpy, ee); /* may call exit */
}

int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}

/* Startup Error handler to check if another window manager
 * is already running. */
int
xerrorstart(Display *dpy, XErrorEvent *ee)
{
	die("dwm: another window manager is already running");
	return -1;
}

void
zoom(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || c->isfloating) return;

  const Workspace *ws = WORKSPACE(selmon);
  const size_t idx = ISVISIBLE_V(ws, c, 0) ? 0 : 1;

  if (!ws->layout->arrange)
    return;
  if (c == nexttiled(selmon->clients, idx) && !(c = nexttiled(c->next, idx)))
    return;
  pop(c);
}

int
main(int argc, char *argv[])
{
	if (argc == 2 && !strcmp("-v", argv[1]))
		die("dwm-"VERSION);
	else if (argc != 1)
		die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fputs("warning: no locale support\n", stderr);
	if (!(dpy = XOpenDisplay(NULL)))
		die("dwm: cannot open display");
	checkotherwm();
	setup();
#ifdef __OpenBSD__
	if (pledge("stdio rpath proc exec", NULL) == -1)
		die("pledge");
#endif /* __OpenBSD__ */
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
