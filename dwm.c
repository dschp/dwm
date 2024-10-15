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
#include <math.h>
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
#define ISVISIBLE(C)            ((C->tags & WORKSPACE(C->mon)->tags))
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TAGMASK                 ((1ULL << LENGTH(tags)) - 1)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define TEXTW_(X)               (drw_fontset_getwidth(drw, (X)))

typedef unsigned int uint;

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { /* color schemes */
  SchemeNorm,
  SchemeSel,
  SchemeLayout,
  SchemeValue1,
  SchemeValue2,
  SchemeValue3,
  SchemeTagged,
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
	uint64_t ui;
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
  uint64_t tags;
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
	uint64_t tags;
	int isfloating;
	int monitor;
} Rule;

typedef struct {
  uint64_t own_tag;
  uint64_t tags;
  const Layout *layout;
  float mfact;
  int nmaster;
  uint64_t last_toggled_tags;
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
static void focusstack(const Arg *arg);
void focus_1st_visible(uint64_t tags);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void grid(Monitor *m);
static void gridnmaster(Monitor *m);
void gridtile(Monitor *m, int right);
static void gridtileleft(Monitor *m);
static void gridtileright(Monitor *m);
Client* grid_resize(Monitor *m, Client *c, uint cnt, uint x, uint y, uint w, uint h);
static void incnmaster(const Arg *arg);
void justfocus(Client *c);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void maximize(const Arg *arg);
static void motionnotify(XEvent *e);
void moveclient(Client *, int x, int y, int w, int c);
static void moveclient_x(const Arg *arg);
static void moveclient_y(const Arg *arg);
static void moveclient_w(const Arg *arg);
static void moveclient_h(const Arg *arg);
static void movemouse(const Arg *arg);
static void movepointer(const Arg *arg);
static void movestack(const Arg *arg);
static Client *nexttiled(Client *c);
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
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void snapandcenter_x(const Arg *arg);
static void snapandcenter_y(const Arg *arg);
static void spawn(const Arg *arg);
static void stackcenter(Monitor *m);
static void switchworkspace(const Arg *arg);
static void tag(const Arg *arg);
void tile(Monitor *m, int right);
static void tileleft(Monitor *m);
static void tileright(Monitor *m);
static void tilelimitleft(Monitor *m);
static void tilelimitright(Monitor *m);
static void tilelimit2left(Monitor *m);
static void tilelimit2right(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggleview(const Arg *arg);
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
static void viewclients(const Arg *arg);
static Client *wintoclient(Window w);
static Monitor *wintomon(Window w);
static int xerror(Display *dpy, XErrorEvent *ee);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static int xerrorstart(Display *dpy, XErrorEvent *ee);
static void xyzero(Monitor *m);
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
  Workspace workspaces[LENGTH(tags)];
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
struct NumTags { char limitexceeded[LENGTH(tags) > 63 ? -1 : 1]; };

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
	if (!(c->tags & TAGMASK))
		c->tags = ws->own_tag;
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
  const Layout *l = WORKSPACE(m)->layout;
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

	for (int i = 0; i < LENGTH(tags); i++)
		selmon->workspaces[i].tags = ~0;
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
			if (ISVISIBLE(c))
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
	for (int i = 0; i < LENGTH(tags); i++) {
	  Workspace *ws = &m->workspaces[i];
	  ws->layout = &layouts[0];
	  ws->own_tag = ws->tags = 1ULL << i;
	  ws->mfact = mfact;
	  ws->nmaster = nmaster;
	  ws->last_toggled_tags = 0;
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
		for (t = c->mon->stack; t && !ISVISIBLE(t); t = t->snext);
		c->mon->sel = t;
	}
}

void
drawbar(Monitor *m)
{
  int x = 0, w = 0, tw = 0;
  int boxs = drw->fonts->h / 9;
  int boxw = drw->fonts->h / 6 + 2;
  uint64_t occ = 0, urg = 0;
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
  }

  int cnt_all = 0, cnt_vis = 0;
  for (c = m->clients; c; c = c->next) {
    cnt_all++;
    if (ISVISIBLE(c)) cnt_vis++;

    occ |= c->tags;
    if (c->isurgent) urg |= c->tags;
  }

  const Workspace *ws = WORKSPACE(m);
  const uint x_limit = m->ww - tw;
  x = 0;

  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s", tags[m->ws_idx]);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeLayout]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 1);
    x += w;
  }

  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d / %d", cnt_vis, cnt_all);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeNorm]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
    x += w;
  }

  {
    char buf[32];

    snprintf(buf, sizeof(buf), "%d", ws->nmaster);
    w = TEXTW(buf);
    drw_setscheme(drw, scheme[SchemeValue1]);
    drw_text(drw, x, 0, w, bh, lrpad / 2, buf, 0);
    x += w;

    snprintf(buf, sizeof(buf), "%.2f", ws->mfact);
    w = TEXTW_(buf) + lrpad / 2;
    drw_setscheme(drw, scheme[SchemeValue2]);
    drw_text(drw, x, 0, w, bh, 0, buf, 0);
    x += w;

    w = TEXTW_(m->ltsymbol) + lrpad / 2;
    drw_setscheme(drw, scheme[SchemeValue3]);
    drw_text(drw, x, 0, w, bh, 0, m->ltsymbol, 0);
    x += w;
  }

  const char overflow[] = "...";
  const int ow = TEXTW(overflow) + 1;
  const uint ox = x_limit - ow;

  for (uint64_t bit = 1, i = 0; i < LENGTH(tags) && x < x_limit; i++, bit <<= 1) {
    uint64_t selected = bit & WORKSPACE(m)->tags;
    uint64_t has_client = bit & occ;
    uint64_t is_urgent = bit & urg;
    if (selected || has_client) {
      w = TEXTW(tags[i]);
      if (x + w > x_limit) w = x_limit - x;

      drw_setscheme(drw, scheme[selected ? SchemeSel : SchemeNorm]);
      drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], is_urgent);
      if (has_client) {
	drw_rect(drw, x + boxs, boxs, boxw, boxw,
		 m == selmon && m->sel && bit & m->sel->tags,
		 is_urgent);
      }

      x += w;
    }
  }

  w = x_limit - x;
  if (w == 0) {
    drw_setscheme(drw, scheme[SchemeOverflow]);
    drw_text(drw, ox + 1, 0, ow - 1, bh, lrpad / 2, overflow, 1);
  } else if (cnt_vis == 0) {
    drw_setscheme(drw, scheme[SchemeNorm]);
    drw_rect(drw, x, 0, w, bh, 1, 1);
  } else  {
    uint end_x = x_limit;
    uint each_w = w / cnt_vis;
    uint drawable_cnt = cnt_vis;
    if (each_w < BAR_CLIENT_MIN_WIDTH) {
      drw_setscheme(drw, scheme[SchemeNorm]);
      drw_rect(drw, ox, 0, 1, bh, 1, 1);
      drw_setscheme(drw, scheme[SchemeOverflow]);
      drw_text(drw, ox + 1, 0, ow - 1, bh, lrpad / 2, overflow, 1);

      end_x -= ow;
      w -= ow;
      drawable_cnt = MAX(w / BAR_CLIENT_MIN_WIDTH, 1);
      each_w = w / drawable_cnt;
    }

    for (c = m->clients; drawable_cnt > 0; c = c->next) {
      if (!ISVISIBLE(c)) continue;

      drw_setscheme(drw, scheme[c == m->sel ? SchemeSel : SchemeNorm]);

      char buf[2 * sizeof(((Client){0}).name)];
      const char *fmt = "[%s] %s";
      size_t tag_i = log2(c->tags & -(c->tags));
      const char *tname = tags[tag_i];

      if (strcmp(tname, "[") == 0 || strcmp(tname, "]") == 0) {
	fmt = "<%s> %s";
      }
      snprintf(buf, sizeof(buf), fmt, tname, c->name);

      uint client_w = each_w;
      if (cnt_vis == 1) {
	client_w = MIN(MAX(TEXTW(buf), BAR_CLIENT_MAX_WIDTH), w);
      } else if (drawable_cnt == 1) {
	client_w = end_x - x;
      }

      drw_text(drw, x, 0, client_w, bh, lrpad / 2, buf, c == m->sel);
      if (c->isfloating)
	drw_rect(drw, x + boxs, boxs, boxw, boxw,
		 c == m->sel || c->isfixed, c == m->sel);

      x += client_w;
      drawable_cnt--;
    }

    if (x < end_x) {
      drw_setscheme(drw, scheme[SchemeNorm]);
      drw_rect(drw, x, 0, end_x - x, bh, 1, 1);
    }
  }

  drw_map(drw, m->barwin, 0, 0, m->ww, bh);
  clock_gettime(CLOCK_MONOTONIC, &ts_last_drawbar);
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
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
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
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
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
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}


void
focus_1st_visible(uint64_t tags)
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
  if (tiled_candidate) {
    focus(tiled_candidate);
  }
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
grid(Monitor *m) {
  uint n;
  Client *c;

  for(n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
  if (!n) return;

  grid_resize(m, nexttiled(m->clients), n, 0, 0, m->ww, m->wh);
}

void
gridnmaster(Monitor *m)
{
  uint n = 0;
  Client *c;

  const Workspace *ws = WORKSPACE(m);
  const uint grid_cnt = MAX(ws->nmaster, 1);
  const uint cell_cnt = grid_cnt * grid_cnt;
  const uint gw = m->ww / grid_cnt;
  const uint gh = m->wh / grid_cnt;

  for (c = nexttiled(m->clients); c; c = nexttiled(c->next), n++) {
    uint a = n % cell_cnt;
    uint row = a / grid_cnt;
    uint col = a % grid_cnt;

    resize(c, col * gw, row * gh, gw - 2 * c->bw, gh - 2 * c->bw, 0);
  }

  const int behinds = n - cell_cnt;
  if (n > 0 && behinds > 0)
    snprintf(m->ltsymbol, sizeof m->ltsymbol, "|%d|/%d", cell_cnt, behinds);
  else
    snprintf(m->ltsymbol, sizeof m->ltsymbol, "|%d|", cell_cnt);
}

void
gridtile(Monitor *m, int right)
{
  uint n, mw, m_cnt;
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
  if (!n) return;

  const Workspace *ws = WORKSPACE(m);
  if (!ws->nmaster) {
    mw = m_cnt = 0;
  } else if (n > ws->nmaster) {
    mw = m->ww * ws->mfact;
    m_cnt = ws->nmaster;
  } else {
    mw = m->ww;
    m_cnt = n;
  }

  c = nexttiled(m->clients);
  if (ws->nmaster) {
    if (n > ws->nmaster) {
      mw = m->ww * ws->mfact;
      m_cnt = ws->nmaster;
    } else {
      mw = m->ww;
      m_cnt = n;
    }
    c = grid_resize(m, c, m_cnt, right ? 0 : m->ww - mw, 0, mw, m->wh);
  } else {
    mw = m_cnt = 0;
  }

  if (n > m_cnt) {
    grid_resize(m, c, n - m_cnt, right ? mw : 0, 0, m->ww - mw, m->wh);
  }
}

void
gridtileleft(Monitor *m)
{
  gridtile(m, 0);
}

void
gridtileright(Monitor *m)
{
  gridtile(m, 1);
}

Client*
grid_resize(Monitor *m, Client *c, uint cnt, uint x, uint y, uint w, uint h)
{
  if (!c) return c;

  const uint g = ceil(sqrt(cnt));
  const uint q = cnt / g;
  const uint r = cnt % g;
  const uint e = g - r;

  const uint g_fixed = (w > h ? w : h) / g;

  uint col_i = 0, row_i = 0;
  uint cel_cnt = q, g_inflatable = (w > h ? h : w) / q;
  for (uint i = 0; c && i < cnt; c = nexttiled(c->next), i++, row_i++) {
    if (row_i == cel_cnt) {
      row_i = 0;
      col_i++;
      if (col_i >= e) {
	cel_cnt = q + 1;
	g_inflatable = (w > h ? h : w) / cel_cnt;
      }
    }

    uint cx, cy;
    if (w > h) {
      cx = col_i * g_fixed;
      cy = row_i * g_inflatable;
    } else {
      cx = row_i * g_inflatable;
      cy = col_i * g_fixed;
    }

    resize(c, cx + x, cy + y,
	   (w > h ? g_fixed : g_inflatable) - 2 * c->bw,
	   (w > h ? g_inflatable : g_fixed) - 2 * c->bw, 0);
  }

  return c;
}

void
incnmaster(const Arg *arg)
{
	if (selmon->sel && selmon->sel->isfloating) return;

	Workspace *ws = WORKSPACE(selmon);
	ws->nmaster = MAX(ws->nmaster + arg->i, 0);
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

  if(arg->i > 0) {
    /* find the client after selmon->sel */
    for(c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
    if(!c)
      for(c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
  }
  else {
    /* find the client before selmon->sel */
    for(i = selmon->clients; i != selmon->sel; i = i->next)
      if(ISVISIBLE(i) && !i->isfloating)
	c = i;
    if(!c)
      for(; i; i = i->next)
	if(ISVISIBLE(i) && !i->isfloating)
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
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
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

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !WORKSPACE(m)->layout->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (WORKSPACE(m)->layout->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
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

    int num_ready_fds = pselect(x11_fd + 1, &in_fds, NULL, NULL, &timer, NULL);
    if (num_ready_fds < 0) {
      printf("An error occured!\n");
      break;
    } else if (num_ready_fds > 0) {
      while(XPending(dpy)) {
	if (!XNextEvent(dpy, &ev))
	  if (handler[ev.type])
	    handler[ev.type](&ev);
      }
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
  if (i >= sizeof(layouts)) return;

  Workspace *ws = WORKSPACE(selmon);
  ws->layout = &layouts[i];
  strncpy(selmon->ltsymbol, ws->layout->symbol, sizeof selmon->ltsymbol);

  if (selmon->sel)
    arrange(selmon);
  else
    drawbar(selmon);
}

void
setmfact(const Arg *arg)
{
	float f;
	if (selmon->sel && selmon->sel->isfloating) return;

	if (!arg || !WORKSPACE(selmon)->layout->arrange)
		return;
	Workspace *ws = WORKSPACE(selmon);
	f = arg->f < 1.0 ? arg->f + ws->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	ws->mfact = f;
	arrange(selmon);
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
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!WORKSPACE(c->mon)->layout->arrange || c->isfloating) && !c->isfullscreen)
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
stackcenter(Monitor *m)
{
  Client *c;
  for (c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
    uint maxw = selmon->ww - 2 * c->bw;
    uint maxh = selmon->wh - 2 * c->bw;
    uint w = MIN(c->w, maxw);
    uint h = MIN(c->h, maxh);
    uint x = m->wx + (maxw - w) / 2;
    uint y = m->wy + (maxh - h) / 2;

    resize(c, x, y, w, h, 0);
  }
}

void
switchworkspace(const Arg *arg)
{
  if (!arg) return;
  int i =
    (arg->i >= 0 && arg->i != selmon->ws_idx) ? arg->i
    : selmon->last_ws_idx;

  if (i >= LENGTH(tags) || i == selmon->ws_idx)
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

  uint64_t arg_tag = arg->ui & TAGMASK;
  if (!arg_tag) return;

  selmon->sel->tags = arg_tag;
  focus(NULL);
  arrange(selmon);
}

void
tile(Monitor *m, int right)
{
  uint i, n, h, mw, my, ty;
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
  if (!n) return;

  const Workspace *ws = WORKSPACE(m);
  if (n > ws->nmaster)
    mw = ws->nmaster ? m->ww * ws->mfact : 0;
  else
    mw = m->ww;
  for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
    if (i < ws->nmaster) {
      h = (m->wh - my) / (MIN(n, ws->nmaster) - i);
      if (right)
	resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
      else
	resize(c, m->wx + m->ww - mw, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
      if (my + HEIGHT(c) < m->wh)
	my += HEIGHT(c);
    } else {
      h = (m->wh - ty) / (n - i);
      if (right)
	resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
      else
	resize(c, m->wx, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
      if (ty + HEIGHT(c) < m->wh)
	ty += HEIGHT(c);
    }
}

void
tileleft(Monitor *m)
{
  tile(m, 0);
}

void
tileright(Monitor *m)
{
  tile(m, 1);
}

void
tilelimit(Monitor *m, int right)
{
  uint i, n, mw;
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
  if (!n) return;

  const Workspace *ws = WORKSPACE(m);
  const char *def = right ? "[]-" : "-[]";
  if (n == 1) {
    c = nexttiled(m->clients);
    resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
    snprintf(m->ltsymbol, sizeof m->ltsymbol, def);
  } else {
    mw = m->ww * ws->mfact;
    c = nexttiled(m->clients);
    resize(c, m->wx + (right ? 0 : m->ww - mw), m->wy, mw - 2 * c->bw, m->wh - 2 * c->bw, 0);
    n--;

    const uint tile_cnt = MIN(n, MAX(ws->nmaster, 1));
    const uint cw = m->mw - mw - 2 * c->bw;
    const uint each_h = m->wh / tile_cnt;
    const uint limit = tile_cnt - 1;
    for (i = 0; i < n; i++) {
      c = nexttiled(c->next);
      resize(c, m->wx + (right ? mw : 0), m->wy + MIN(i, limit) * each_h, cw, each_h - 2 * c->bw, 0);
    }

    if (n > tile_cnt)
      snprintf(m->ltsymbol, sizeof m->ltsymbol, right ? "[]-/%d" : "-/%d[]", n - tile_cnt);
    else
      snprintf(m->ltsymbol, sizeof m->ltsymbol, def);
  }
}

void
tilelimitleft(Monitor *m)
{
  tilelimit(m, 0);
}

void
tilelimitright(Monitor *m)
{
  tilelimit(m, 1);
}

void
tilelimit2(Monitor *m, int right)
{
  uint n;
  Client *c;

  for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
  if (!n) return;

  const Workspace *ws = WORKSPACE(m);
  const char *def = right ? "[]%%" : "%%[]";
  if (n == 1) {
    c = nexttiled(m->clients);
    resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
    snprintf(m->ltsymbol, sizeof m->ltsymbol, def);
  } else {
    uint mw = m->ww * ws->mfact;
    c = nexttiled(m->clients);
    resize(c, m->wx + (right ? 0 : m->ww - mw), m->wy, mw - 2 * c->bw, m->wh - 2 * c->bw, 0);
    n--;

    const uint cw = m->mw - mw - 2 * c->bw;
    uint each_h = m->wh / n;
    if (each_h > TILE_LIMIT_MIN_HEIGHT) {
      for (uint i = 0; i < n; i++) {
	c = nexttiled(c->next);
	resize(c, m->wx + (right ? mw : 0), m->wy + i * each_h, cw, each_h - 2 * c->bw, 0);
      }
      snprintf(m->ltsymbol, sizeof m->ltsymbol, def);
    } else {
      const uint tile_cnt  = MAX(m->wh / TILE_LIMIT_MIN_HEIGHT, 1);
      each_h = m->wh / tile_cnt;
      const uint limit = tile_cnt - 1;
      for (uint i = 0; i < n; i++) {
	c = nexttiled(c->next);
	resize(c, m->wx + (right ? mw : 0), m->wy + MIN(i, limit) * each_h, cw, each_h - 2 * c->bw, 0);
      }
      snprintf(m->ltsymbol, sizeof m->ltsymbol, right ? "[]%%%d" : "%%%d[]", n - tile_cnt);
    }
  }
}

void
tilelimit2left(Monitor *m)
{
  tilelimit2(m, 0);
}

void
tilelimit2right(Monitor *m)
{
  tilelimit2(m, 1);
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
toggleview(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  uint64_t arg_tag =
    arg->ui == 0 ? ws->last_toggled_tags
    : arg->ui != ws->own_tag ? arg->ui & TAGMASK
    : ws->tags == ws->own_tag ? ws->last_toggled_tags
    : ws->tags ^ ws->own_tag;
  if (!arg_tag) return;

  uint64_t newtags = ws->tags ^ arg_tag;
  if (!newtags) return;

  ws->tags = newtags;
  ws->last_toggled_tags = arg_tag;
  uint64_t added = newtags & arg_tag;

  if (arg->i < 0 || !added) {
    if (selmon->sel && !ISVISIBLE(selmon->sel)) {
      focus_1st_visible(newtags);
    }
  } else {
    focus_1st_visible(added);
  }

  arrange(selmon);
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

void
viewclients(const Arg *arg)
{
  Workspace *ws = WORKSPACE(selmon);
  uint64_t newtags = ws->own_tag;
  for (Client *c = selmon->clients; c; c = c->next) {
    newtags |= c->tags;
  }
  if (!newtags) return;

  if (ws->tags == newtags) return;

  ws->last_toggled_tags = ws->tags ^ newtags;
  ws->tags = newtags;

  focus_1st_visible(newtags);
  arrange(selmon);
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
xyzero(Monitor *m)
{
  Client *c;
  for (c = nexttiled(m->clients); c; c = nexttiled(c->next)) {
    resize(c, m->wx, m->wy,
	   MIN(c->w, m->ww - 2 * c->bw), MIN(c->h, m->wh - 2 * c->bw),
	   0);
  }
}

void
zoom(const Arg *arg)
{
  Client *c = selmon->sel;
  if (!c || c->isfloating) return;

  if (!WORKSPACE(selmon)->layout->arrange)
    return;
  if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
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
