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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            (C->desktop == C->mon->curgrp->curdsk)
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)
#define CURLAYOUT(M)            (layouts[M->curgrp->curdsk->sellayout])

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNormal, SchemeCurrent, SchemePrevious, SchemeGroup,
	   SchemeCLabel, SchemeUrgent,
	   SchemeLayout, SchemeNmaster, SchemeMfact }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
	   NetWMFullscreen, NetActiveWindow, NetWMWindowType,
	   NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkGroup, ClkLayout, ClkLayoutParam, ClkDesktop, ClkClients,
	   ClkClientWin, ClkRootWin, ClkLast }; /* clicks */
enum { BarModeDesktop, BarModeClients, BarModeStatus };

typedef union {
	int i;
	float f;
	const void *v;
} Arg;

typedef struct {
	uint click;
	uint mask;
	uint button;
	void (*func)(const Arg *arg);
	const Arg arg;
} Button;

typedef struct Monitor Monitor;
typedef struct Desktop Desktop;
typedef struct Client Client;
struct Client {
	char name[256];
	float mina, maxa;
	int x, y, w, h;
	int oldx, oldy, oldw, oldh;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
	int bw, oldbw;
	int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
	int barx;
	Client *next;
	Client *snext;
	Monitor *mon;
	Desktop *desktop;
	Window win;
};

typedef struct Group Group;
struct Group {
	Group* next;
	Desktop *desktops, *curdsk, *prevdsk;

	int barx;
	uint urg;
	char label[16];
	int w_label;
};

typedef struct {
	uint mod;
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
	int isfloating;
	int monitor;
} Rule;

struct Desktop {
	Group *group;
	Desktop *next;

	int nmaster;
	float mfact;
	uint showbar;
	uint sellayout;

	int barx;
	uint occ;
	uint urg;
	char label[16];
	int w_label;
};

/* function declarations */
static void applyrules(Client *c);
static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void attach(Client *c);
static void attachstack(Client *c);
static void banish_pointer(const Arg *arg);
static void buttonpress(XEvent *e);
static void checkotherwm(void);
static void cleanup(void);
static void cleanupmon(Monitor *mon);
static void clientmessage(XEvent *e);
static void client_select(const Arg *arg);
static void client_stack(const Arg *arg);
static void client_traverse(const Arg *arg);
static void configure(Client *c);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static Monitor *createmon(void);
static void desktop_add(const Arg *arg);
static void desktop_adjacent(const Arg *arg);
static void desktop_move_client(const Arg *arg);
static void desktop_move_group(const Arg *arg);
static void desktop_remove(const Arg *arg);
static void desktop_select(const Arg *arg);
static void desktop_select_urg(const Arg *arg);
static void desktop_stack(const Arg *arg);
static void desktop_swap(const Arg *arg);
static void destroynotify(XEvent *e);
static void detach(Client *c);
static void detachstack(Client *c);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focus(Client *c);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static void group_add(const Arg *arg);
static void group_adjacent(const Arg *arg);
static void group_select(const Arg *arg);
static void group_stack(const Arg *arg);
static void group_swap(const Arg *arg);
static Atom getatomprop(Client *c, Atom prop);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, uint size);
static void grabbuttons(Client *c, int focused);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void monocle(Monitor *m);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
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
static void setbarmode(const Arg *arg);
static void setclientstate(Client *c, long state);
static void setfocus(Client *c);
static void setfullscreen(Client *c, int fullscreen);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void setup(void);
static void seturgent(Client *c, int urg);
static void showhide(Client *c);
static void spawn(const Arg *arg);
static void tagmon(const Arg *arg);
static void tile(Monitor *m);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
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
static char stext[500];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */
static int lrpad_2;
static int (*xerrorxlib)(Display *, XErrorEvent *);
static uint numlockmask = 0;
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

/* configuration, allows nested code to access above variables */
#include "config.h"

struct Monitor {
	Monitor *next;
	Window barwin;

	int num;
	int by;               /* bar geometry */
	int mx, my, mw, mh;   /* screen size */
	int wx, wy, ww, wh;   /* window area  */

	char ltsymbol[16];
	int topbar;
	uint barmode;

	Group *groups, *curgrp, *prevgrp;
	Client *clients, *stack, *sel;

	int x_group_ellipsis_l;
	int x_group_ellipsis_r;
	int x_urgent_list;
	int x_layout;
	int x_layout_param;
};

const char ellipsis_l[] = "<";
const char ellipsis_r[] = ">";
static int w_ellipsis_l, w_ellipsis_r;
static int w_clabels[LENGTH(clabels)];

void
_desktop_label_update(Group *g)
{
	Desktop *d = g->desktops;
	for (int i = 1; d; d = d->next, i++) {
		snprintf(d->label, sizeof(d->label), "%d", i);
		d->w_label = TEXTW(d->label);
	}
}

void
_group_label_update(Monitor *m)
{
	Group *g = m->groups;
	for (int i = 1; g; g = g->next, i++) {
		snprintf(g->label, sizeof(g->label), "F%d", i);
		g->w_label = TEXTW(g->label);
	}
}

void
_desktop_create(Group *g)
{
	Desktop *d = ecalloc(1, sizeof(Desktop));
	d->next = g->desktops;
	g->prevdsk = g->curdsk;
	g->desktops = g->curdsk = d;

	d->group = g;
	d->nmaster = NMASTER;
	d->mfact = MFACT;
	d->showbar = SHOWBAR;
	d->sellayout = 0;

	_desktop_label_update(g);
}

void
_group_create(Monitor *m)
{
	Group *g = ecalloc(1, sizeof(Group));
	g->next = m->groups;
	m->prevgrp = m->curgrp;
	m->groups = m->curgrp = g;

	_group_label_update(m);

	_desktop_create(g);
}

void
_desktop_detach(Desktop *d)
{
	Desktop **dpp;
	for (dpp = &d->group->desktops; *dpp && *dpp != d; dpp = &(*dpp)->next);
	*dpp = d->next;
}

void
_group_detach(Group *g)
{
	Group **gpp;
	for (gpp = &selmon->groups; *gpp && *gpp != g; gpp = &(*gpp)->next);
	*gpp = g->next;
}

void
_group_delete(Monitor *m, Group *g)
{
	if (!m || !g || !m->groups->next)
		return;

	for (Client *c = m->clients; c; c = c->next)
		if (c->desktop->group == g)
			return;

	if (g->desktops)
		free(g->desktops);

	_group_detach(g);
	free(g);
	_group_label_update(selmon);

	m->curgrp = m->prevgrp ? m->prevgrp : m->groups;
	m->prevgrp = NULL;

	focus(NULL);
	arrange(m);
}

void
_desktop_delete(Monitor *m, int idx)
{
	Group *g = m->curgrp;
	Desktop *cur = g->curdsk;
	Desktop *dest = g->prevdsk;

	if (idx) {
		if (idx > 0) {
			dest = g->desktops;
			for (int i = 1; dest && i != idx; dest = dest->next, i++);
		}

		if (!dest)
			return;

		for (Client *c = m->clients; c; c = c->next)
			if (c->desktop == cur)
				c->desktop = dest;
	} else if (g->desktops->next) {
		for (Client *c = m->clients; c; c = c->next)
			if (c->desktop == cur)
				return;
	} else {
		_group_delete(m, g);
		return;
	}

	_desktop_detach(cur);
	free(cur);

	g->curdsk = dest ? dest : g->desktops;
	g->prevdsk = NULL;

	_desktop_label_update(g);

	focus(NULL);
	arrange(selmon);
}

void
_desktop_move_client(Client *c, int to)
{
	if (!c)
		return;

	Group *g = selmon->curgrp;
	if (!to) {
		uint cnt = 0;
		for (Client *c2 = c->mon->clients; c2; c2 = c2->next)
			if (c2->desktop == c->desktop)
				cnt++;
		if (cnt < 2)
			return;

		_desktop_create(g);
		c->desktop = g->curdsk;
	} else {
		const uint absto = abs(to);
		Desktop *d = g->desktops;
		for (int i = 1; d && i != absto; d = d->next, i++);

		if (!d || d == c->desktop)
			return;

		c->desktop = d;
		if (to < 0) {
			g->prevdsk = g->curdsk;
			g->curdsk = d;
		}
	}

	focus(NULL);
	arrange(selmon);
}

void
_desktop_select(Desktop *d)
{
	if (!d)
		return;
	Group *g = d->group;
	if (d == g->curdsk)
		return;

	g->prevdsk = g->curdsk;
	g->curdsk = d;

	focus(NULL);
	arrange(selmon);
}

void
_group_select(Group *g)
{
	if (!g || g == selmon->curgrp)
		return;

	selmon->prevgrp = selmon->curgrp;
	selmon->curgrp = g;

	focus(NULL);
	arrange(selmon);
}

Desktop *
_desktop_tail(Monitor *m)
{
	Desktop *d = m->curgrp->curdsk;
	for (; d && d->next; d = d->next);
	return d;
}

Group *
_group_tail(Monitor *m)
{
	Group *g = selmon->curgrp;
	for (; g && g->next; g = g->next);
	return g;
}

void
_client_focus(Client *c)
{
	if (!c)
		return;

	Group *g = c->desktop->group;
	if (g != selmon->curgrp) {
		selmon->prevgrp = selmon->curgrp;
		selmon->curgrp = g;
	}
	if (c->desktop != g->curdsk) {
		g->prevdsk = g->curdsk;
		g->curdsk = c->desktop;
	}

	focus(c);
	arrange(selmon);
}

Client *
_client_search_d(Desktop *d)
{
	for (; d; d = d->next) {
		Client *c = selmon->clients;
		for (; c; c = c->next)
			if (c->desktop == d)
				return c;
	}

	return NULL;
}

Client *
_client_search_g(Group *g)
{
	Client *c = NULL;

	for (; g; g = g->next) {
		if ((c = _client_search_d(g->desktops)))
			return c;
	}

	return c;
}

Client *
_client_traverse_f(Monitor *m)
{
	if (!m)
		return NULL;

	Client *c;
	Desktop *d;
	Group *g;

	if (m->sel) {
		c = m->sel->next;
		d = m->sel->desktop;
		g = d->group;
	} else {
		c = m->clients;
		d = m->curgrp->curdsk;
		g = m->curgrp;
	}

	for (; c; c = c->next)
		if (c->desktop == d)
			return c;

	if ((c = _client_search_d(d->next)))
		return c;

	if ((c = _client_search_g(g->next)))
		return c;

	return _client_search_g(m->groups);
}

Client *
_client_traverse_b(Monitor *m)
{
	if (!m)
		return NULL;

	Client *c, *cand = NULL;
	Desktop *d;
	Group *g;

	const Client *curc = m->sel;
	const Desktop *curd = curc ? curc->desktop : m->curgrp->curdsk;

	for (c = m->clients; c && c != curc; c = c->next)
		if (c->desktop == curd)
			cand = c;
	if (cand)
		return cand;

	for (g = m->groups; g; g = g->next) {
		for (d = g->desktops; d; d = d->next) {
			if (d == curd) {
				if (cand)
					return cand;

				continue;
			}
			for (c = m->clients; c; c = c->next) {
				if (c->desktop == d)
					cand = c;
			}
		}
	}

	return cand;
}

/* function implementations */
void
applyrules(Client *c)
{
	const char *class, *instance;
	uint i;
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
			for (m = mons; m && m->num != r->monitor; m = m->next);
			if (m)
				c->mon = m;
		}
	}
	if (ch.res_class)
		XFree(ch.res_class);
	if (ch.res_name)
		XFree(ch.res_name);
	c->desktop = c->mon->curgrp->curdsk;
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
	if (RESIZEHINTS || c->isfloating || !CURLAYOUT(m).arrange) {
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
	strncpy(m->ltsymbol, CURLAYOUT(m).symbol, sizeof m->ltsymbol);
	if (CURLAYOUT(m).arrange)
		CURLAYOUT(m).arrange(m);
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
banish_pointer(const Arg *arg)
{
	if (!arg)
		return;

	XWarpPointer(dpy, None, root, 0, 0, 0, 0,
				 arg->i > 0 ? selmon->mw : 0, TOPBAR ? 0 : selmon->mh);
    XFlush(dpy);
}

void
buttonpress(XEvent *e)
{
	uint i, click;
	Arg arg = {0};
	Client *c;
	Group *g;
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
		if (ev->x < selmon->x_group_ellipsis_l) {
			if (ev->button == Button1)
				_group_select(selmon->groups);
			return;
		}
		for (g = selmon->groups; g; g = g->next) {
			if (ev->x < g->barx) {
				if (ev->button == Button1)
					_group_select(g);
				return;
			}
		}
		if (ev->x < selmon->x_group_ellipsis_r) {
			if (ev->button == Button1)
				_group_select(_group_tail(selmon));
			return;
		}

		if (ev->x < selmon->x_urgent_list) {
			if (ev->button == Button1)
				desktop_select_urg(&arg);
			return;
		} else if (ev->x < selmon->x_layout) {
			click = ClkLayout;
		} else if (ev->x < selmon->x_layout_param) {
			click = ClkLayoutParam;
		} else if (selmon->barmode == BarModeDesktop) {
			Desktop *d = selmon->curgrp->desktops;
			for (i = 1; d; d = d->next, i++)
				if (ev->x < d->barx)
					break;
			click = ClkDesktop;
			arg.i = ev->button == Button2 ? -i : i;
		} else if (selmon->barmode == BarModeClients) {
			switch (ev->button) {
			case Button1:
				for (c = selmon->clients; c; c = c->next)
					if (ev->x < c->barx) {
						focus(c);
						restack(selmon);
						break;
					}
				return;
			case Button3:
				for (c = selmon->clients; c; c = c->next)
					if (ev->x < c->barx) {
						focus(c);
						zoom(NULL);
						break;
					}
				return;
			}

			click = ClkClients;
		}
	} else if ((c = wintoclient(ev->window))) {
		focus(c);
		restack(selmon);
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
	for (i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
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
	Arg a = {.i = 0};
	Monitor *m;
	size_t i;

	desktop_select(&a);
	selmon->curgrp->curdsk->sellayout = 0;
	for (m = mons; m; m = m->next) {
		while (m->stack)
			unmanage(m->stack, 0);
		Group *g = m->groups;
		while (g) {
			Group *next = g->next;

			Desktop *d = g->desktops;
			while (d) {
				Desktop *next = d->next;
				free(d);
				d = next;
			}
			g->desktops = g->curdsk = g->prevdsk = NULL;

			free(g);
			g = next;
		}
		m->groups = m->curgrp = m->prevgrp = NULL;
	}
	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	while (mons)
		cleanupmon(mons);
	for (i = 0; i < CurLast; i++)
		drw_cur_free(drw, cursor[i]);
	for (i = 0; i < LENGTH(colors); i++)
		drw_scm_free(drw, scheme[i], 3);
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
client_select(const Arg *arg)
{
	if (!arg)
		return;

	const Desktop *dsk = selmon->curgrp->curdsk;
	Client *candidate = NULL;
	Client *c = selmon->clients;
	if (arg->i > 0) {
		int i = 1;
		for (; c; c = c->next)
			if (c->desktop == dsk) {
				if (i == arg->i) {
					candidate = c;
					break;
				}
				i++;
			}

	} else if (arg->i < 0) {
		for (; c; c = c->next)
			if (c->desktop == dsk)
				candidate = c;
	}

	if (candidate && candidate != selmon->sel) {
		focus(candidate);
		arrange(selmon);
	}
}

void
client_stack(const Arg *arg)
{
	Client *sel = selmon->sel;
	if (!arg || !sel)
		return;

	if (arg->i > 0) {
		if (sel == selmon->clients)
			return;

		detach(sel);
		sel->next = selmon->clients;
		selmon->clients = sel;
	} else {
		Client *tail = sel->next;
		for (; tail && tail->next; tail = tail->next);
		if (!tail)
			return;

		detach(sel);
		tail->next = sel;
		sel->next = NULL;
	}

	arrange(selmon);
}

void
client_traverse(const Arg *arg)
{
	if (!arg)
		return;

	Client *c;
	if (arg->i > 0) {
		c = _client_traverse_f(selmon);
	} else {
		c = _client_traverse_b(selmon);
	}

	_client_focus(c);
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
		else if (c->isfloating || !CURLAYOUT(selmon).arrange) {
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
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	m->topbar = TOPBAR;
	m->barmode = BarModeDesktop;

	_group_create(m);

	return m;
}

void
desktop_add(const Arg *arg)
{
	_desktop_create(selmon->curgrp);
	focus(NULL);
	arrange(selmon);
}

void
desktop_adjacent(const Arg *arg)
{
	Group *cg = selmon->curgrp;
	Desktop *cd = cg->curdsk;
	if (!arg || !cg->desktops->next)
		return;

	Desktop *d;
	if (arg->i > 0) {
		d = cd->next;
		if (!d)
			d = cg->desktops;
	} else {
		for (d = cg->desktops; d && d->next != cg->curdsk; d = d->next);
		if (!d)
			for (d = cd->next; d && d->next; d = d->next);
	}

	if (d && d != cd) {
		cg->prevdsk = cg->curdsk;
		cg->curdsk = d;

		focus(NULL);
		arrange(selmon);
	}
}

void
desktop_move_client(const Arg *arg)
{
	Client *c = selmon->sel;
	if (!arg || !c)
		return;

	_desktop_move_client(c, arg->i);
}

void
desktop_move_group(const Arg *arg)
{
	if (!arg)
		return;

	Group *cur = selmon->curgrp;
	Group *dest;
	if (arg->i > 0) {
		dest = selmon->groups;
		for (int i = 1; dest && i != arg->i; dest = dest->next, i++);
	} else if (arg->i < 0) {
		dest = _group_tail(selmon);
	} else {
		dest = selmon->prevgrp;
	}

	if (!dest || dest == cur)
		return;

	Desktop *d = cur->curdsk;
	_desktop_detach(d);
	d->next = dest->desktops;
	dest->desktops = d;
	d->group = dest;

	_desktop_label_update(dest);

	if (cur->desktops) {
		cur->curdsk = cur->prevdsk ? cur->prevdsk : cur->desktops;
		cur->prevdsk = NULL;

		_desktop_label_update(cur);
	} else {
		_group_delete(selmon, cur);
		return;
	}

	focus(NULL);
	arrange(selmon);
}

void
desktop_remove(const Arg *arg)
{
	if (!arg)
		return;
	_desktop_delete(selmon, arg->i);
}

void
desktop_select(const Arg *arg)
{
	if (!arg)
		return;

	Group *cg = selmon->curgrp;
	Desktop *d;
	if (arg->i > 0) {
		d = cg->desktops;
		for (int i = 1; d && i != arg->i; d = d->next, i++);
	} else if (arg->i < 0) {
		d = _desktop_tail(selmon);
	} else {
		d = cg->prevdsk;
	}

	_desktop_select(d);
}

void
desktop_select_urg(const Arg *arg)
{
	Client *c = selmon->clients;;
	for (; c; c = c->next)
		if (c->isurgent && c != selmon->sel)
			break;

	_client_focus(c);
}

void
desktop_stack(const Arg *arg)
{
	if (!arg)
		return;

	Group *cg = selmon->curgrp;
	Desktop *cd = cg->curdsk;
	if (arg->i > 0) {
		if (cd == cg->desktops)
			return;

		_desktop_detach(cd);
		cd->next = cg->desktops;
		cg->desktops = cd;
	} else {
		Desktop *tail = cd->next;
		for (; tail && tail->next; tail = tail->next);
		if (!tail)
			return;

		_desktop_detach(cd);
		tail->next = cd;
		cd->next = NULL;
	}

	_desktop_label_update(cg);
	drawbar(selmon);
}

void
desktop_swap(const Arg *arg)
{
	if (!arg)
		return;

	Group *cg = selmon->curgrp;
	Desktop *cd = cg->curdsk;
	if (arg->i > 0) {
		Desktop *swap = cd->next;
		if (!swap)
			return;

		_desktop_detach(cd);
		cd->next = swap->next;
		swap->next = cd;
	} else {
		Desktop *swap = cg->desktops;
		if (cd == swap)
			return;
		for (; swap && swap->next != cd; swap = swap->next);
		if (!swap)
			return;

		_desktop_detach(swap);
		swap->next = cd->next;
		cd->next = swap;
	}

	_desktop_label_update(cg);
	drawbar(selmon);
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

Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

void
drawbar(Monitor *m)
{
	const Group *cg = m->curgrp;
	const Desktop *cd = cg->curdsk;

	if (!cd->showbar)
		return;

	int x, w, i;
	int cg_idx, cd_idx, dsk_area_w;
	uint boxs = drw->fonts->h / 9;
	uint boxw = drw->fonts->h / 6 + 2;
	Group *g;
	Desktop *d;
	Client *c;
	char buf[16];

	for (i = 0, g = selmon->groups; g; g = g->next, i++) {
		g->barx = g->urg = 0;
		if (g == cg)
			cg_idx = i;
	}
	const int grp_cnt = i;

	for (i = 0, d = cg->desktops; d; d = d->next, i++) {
		d->barx = d->occ = d->urg = 0;
		if (d == cd)
			cd_idx = i;
	}
	const int dsk_cnt = i;

	for (c = m->clients; c; c = c->next) {
		c->barx = 0;
		d = c->desktop;
		d->occ++;
		if (c->isurgent)
			d->group->urg = d->urg = 1;
	}

	x = 0;

	{
		g = m->groups;
		int start = 0;
		int end = grp_cnt;
		if (grp_cnt > BAR_GROUP_MAX) {
			start = cg_idx - BAR_GROUP_MAX / 2;
			end = cg_idx + (BAR_GROUP_MAX - BAR_GROUP_MAX / 2);
			if (start < 0) {
				end += abs(start);
				start = 0;
			} else if (end > grp_cnt) {
				start -= end - grp_cnt;
			}
		}

		if (start) {
			for (i = 0; i < start; i++)
				g = g->next;

			drw_setscheme(drw, scheme[SchemeNormal]);
			drw_text(drw, x, 0, w_ellipsis_l, bh, lrpad_2, ellipsis_l, 0);

			x += w_ellipsis_l;
			m->x_group_ellipsis_l = x;
		} else {
			m->x_group_ellipsis_l = 0;
		}

		drw_setscheme(drw, scheme[SchemeGroup]);
		for (i = 0; g && i < BAR_GROUP_MAX; g = g->next, i++) {
			drw_text(drw, x, 0, g->w_label, bh, lrpad_2, g->label, g == cg);
			x += g->w_label;
			g->barx = x;
		}

		if (end < grp_cnt) {
			drw_setscheme(drw, scheme[SchemeNormal]);
			drw_text(drw, x, 0, w_ellipsis_r, bh, lrpad_2, ellipsis_r, 0);

			x += w_ellipsis_r;
			m->x_group_ellipsis_r = x;
		} else {
			m->x_group_ellipsis_r = 0;
		}
	}

	drw_setscheme(drw, scheme[SchemeUrgent]);
	for (Group *g = m->groups; g; g = g->next) {
		if (!g->urg)
			continue;
		w = g->w_label;
		drw_text(drw, x, 0, g->w_label, bh, lrpad_2, g->label, 1);
		x += g->w_label;
	}
	m->x_urgent_list = x;

	w = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeLayout]);
	drw_text(drw, x, 0, w, bh, lrpad_2, m->ltsymbol, 0);
	x += w;
	m->x_layout = x;

	snprintf(buf, sizeof(buf), "%d", cd->nmaster);
	w = TEXTW(buf);
	drw_setscheme(drw, scheme[SchemeNmaster]);
	drw_text(drw, x, 0, w, bh, lrpad_2, buf, 0);
	x += w;

	snprintf(buf, sizeof(buf), "%0.2f", cd->mfact);
	w = TEXTW(buf);
	drw_setscheme(drw, scheme[SchemeMfact]);
	drw_text(drw, x, 0, w, bh, lrpad_2, buf, 0);
	x += w;
	m->x_layout_param = x;

	dsk_area_w = m->mw - x;

	switch (m->barmode) {
	case BarModeDesktop:
		d = cg->desktops;
		if (dsk_cnt <= BAR_DESKTOP_MAX) {
			w = dsk_area_w / dsk_cnt;
		} else {
			int start = cd_idx - BAR_DESKTOP_MAX / 2;
			int end = cd_idx + (BAR_DESKTOP_MAX - BAR_DESKTOP_MAX / 2);
			if (start < 0) {
				end += abs(start);
				start = 0;
			} else if (end > dsk_cnt) {
				start -= end - dsk_cnt;
			}

			drw_setscheme(drw, scheme[SchemeNormal]);
			if (start) {
				for (i = 0; i < start; i++)
					d = d->next;

				drw_text(drw, x, 0, w_ellipsis_l, bh, lrpad_2, ellipsis_l, 0);
				x += w_ellipsis_l;
				dsk_area_w -= w_ellipsis_l;
			}
			if (end < dsk_cnt) {
				drw_text(drw, m->mw - w_ellipsis_r, 0, w_ellipsis_r, bh, lrpad_2, ellipsis_r, 0);
				dsk_area_w -= w_ellipsis_r;
			}

			w = dsk_area_w / BAR_DESKTOP_MAX;
		}

		for (i = 0; d && i < BAR_DESKTOP_MAX; d = d->next, i++) {
			for (c = m->stack; c && c->desktop != d; c = c->snext);

			int is_cur = d == cg->curdsk;
			int is_prev = d == cg->prevdsk;

			const int sch_idx =
				is_cur ? SchemeCurrent : is_prev ? SchemePrevious : SchemeNormal;
			drw_setscheme(drw, scheme[sch_idx]);
			drw_text(drw, x, 0, d->w_label, bh, lrpad_2, d->label, is_cur ? 1 : d->urg);
			if (d->occ)
				drw_rect(drw, x + boxs, boxs, boxw, boxw,
						 m == selmon && is_cur, is_cur ? 1 : d->urg);
			x += d->w_label;

			int w2 = w - d->w_label;
			if (c) {
				drw_text(drw, x, 0, w2, bh, lrpad_2, c->name, d->urg);
				if (c->isfloating)
					drw_rect(drw, x + boxs, boxs, boxw, boxw, c->isfixed, c->isurgent);
			} else {
				drw_rect(drw, x, 0, w2, bh, 1, !d->urg);
			}
			x += w2;

			d->barx = x;
		}
		break;
	case BarModeClients:
		d = cg->curdsk;
		if (!d->occ)
			break;

		w = dsk_area_w / d->occ;
		for (i = 0, c = m->clients; c; c = c->next) {
			if (c->desktop != d)
				continue;
			drw_setscheme(drw, scheme[c == m->sel ? SchemeCurrent : SchemeNormal]);

			int w2 = w;
			if (i < LENGTH(clabels)) {
				int w3 = w_clabels[i];
				drw_setscheme(drw, scheme[c == m->sel ? SchemeCurrent : SchemeCLabel]);
				drw_text(drw, x, 0, w3, bh, lrpad_2, clabels[i], c == m->sel ? 1 : c->isurgent);
				w2 -= w3;
				x += w3;
				i++;
			}

			drw_setscheme(drw, scheme[c == m->sel ? SchemeCurrent : SchemeNormal]);
			drw_text(drw, x, 0, w2, bh, lrpad_2, c->name, 0);
			if (c->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, c->isfixed, 0);

			x += w2;
			c->barx = x;
		}
		break;
	case BarModeStatus:
		w = dsk_area_w;
		drw_setscheme(drw, scheme[SchemeNormal]);
		drw_text(drw, x, 0, w, bh, lrpad_2, stext, 0);
		x += w;
		break;
	}

	if ((w = dsk_area_w - (x - m->x_layout_param)) > 0) {
		drw_setscheme(drw, scheme[SchemeNormal]);
		drw_rect(drw, x, 0, w, bh, 1, 1);
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
		XSetWindowBorder(dpy, c->win, scheme[SchemeCurrent][ColBorder].pixel);
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
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!arg || !mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!arg || !selmon->sel || (selmon->sel->isfullscreen && LOCKFULLSCREEN))
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
group_add(const Arg *arg)
{
	_group_create(selmon);
	focus(NULL);
	arrange(selmon);
}

void
group_adjacent(const Arg *arg)
{
	if (!arg || !selmon->groups->next)
		return;

	Group *g;
	if (arg->i > 0) {
		g = selmon->curgrp->next;
		if (!g)
			g = selmon->groups;
	} else {
		for (g = selmon->groups; g && g->next != selmon->curgrp; g = g->next);
		if (!g)
			for (g = selmon->curgrp->next; g && g->next; g = g->next);
	}

	selmon->prevgrp = selmon->curgrp;
	selmon->curgrp = g;

	focus(NULL);
	arrange(selmon);
}

void
group_stack(const Arg *arg)
{
	if (!arg)
		return;

	Group *g = selmon->curgrp;
	if (arg->i > 0) {
		if (g == selmon->groups)
			return;

		_group_detach(g);
		g->next = selmon->groups;
		selmon->groups = g;
	} else {
		Group *tail = g->next;
		for (; tail && tail->next; tail = tail->next);
		if (!tail)
			return;

		_group_detach(g);
		tail->next = g;
		g->next = NULL;
	}

	_group_label_update(selmon);
	drawbar(selmon);
}

void
group_swap(const Arg *arg)
{
	if (!arg)
		return;

	Group *g = selmon->curgrp;
	if (arg->i > 0) {
		Group *swap = g->next;
		if (!swap)
			return;

		_group_detach(g);
		g->next = swap->next;
		swap->next = g;
	} else {
		Group *swap = selmon->groups;
		if (g == swap)
			return;
		for (; swap && swap->next != g; swap = swap->next);
		if (!swap)
			return;

		_group_detach(swap);
		swap->next = g->next;
		g->next = swap;
	}

	_group_label_update(selmon);
	drawbar(selmon);
}

void
group_select(const Arg *arg)
{
	if (!arg)
		return;

	Group *g;
	if (arg->i > 0) {
		g = selmon->groups;
		for (int i = 1; g && i != arg->i; g = g->next, i++);
	} else if (arg->i < 0) {
		g = _group_tail(selmon);
	} else {
		g = selmon->prevgrp;
	}

	_group_select(g);
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
	uint dui;
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
gettextprop(Window w, Atom atom, char *text, uint size)
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
		uint i, j;
		uint modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
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
		uint i, j, k;
		uint modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
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
incnmaster(const Arg *arg)
{
	if (!arg)
		return;

	Desktop *d = selmon->curgrp->curdsk;
	d->nmaster = MAX((d->nmaster) + arg->i, 0);
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
	uint i;
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
	c->bw = BORDER_PX;

	wc.border_width = c->bw;
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
	XSetWindowBorder(dpy, w, scheme[SchemeNormal][ColBorder].pixel);
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
	c->desktop = c->mon->curgrp->curdsk;

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
monocle(Monitor *m)
{
	uint n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
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
			if ((ev.xmotion.time - lasttime) <= (1000 / REFRESH_RATE))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < SNAP_PX)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < SNAP_PX)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < SNAP_PX)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < SNAP_PX)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && CURLAYOUT(selmon).arrange
			&& (abs(nx - c->x) > SNAP_PX || abs(ny - c->y) > SNAP_PX))
				togglefloating(NULL);
			if (!CURLAYOUT(selmon).arrange || c->isfloating)
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
movestack(const Arg *arg)
{
	Client *c = NULL, *p = NULL, *pc = NULL, *i;

	if (!arg)
		return;

	if (arg->i > 0) {
		/* find the client after selmon->sel */
		for (c = selmon->sel->next; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
		if (!c)
			for (c = selmon->clients; c && (!ISVISIBLE(c) || c->isfloating); c = c->next);
	} else {
		/* find the client before selmon->sel */
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i) && !i->isfloating)
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i) && !i->isfloating)
					c = i;
	}
	/* find the client before selmon->sel and c */
	for (i = selmon->clients; i && (!p || !pc); i = i->next) {
		if (i->next == selmon->sel)
			p = i;
		if (i->next == c)
			pc = i;
	}

	/* swap c and selmon->sel selmon->clients in the selmon->clients list */
	if (c && c != selmon->sel) {
		Client *temp = selmon->sel->next==c ? selmon->sel : selmon->sel->next;
		selmon->sel->next = c->next == selmon->sel ? c : c->next;
		c->next = temp;

		if (p && p != c)
			p->next = c;
		if (pc && pc != selmon->sel)
			pc->next = selmon->sel;

		if (selmon->sel == selmon->clients)
			selmon->clients = c;
		else if (c == selmon->clients)
			selmon->clients = selmon->sel;

		arrange(selmon);
	}
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
			if ((ev.xmotion.time - lasttime) <= (1000 / REFRESH_RATE))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && CURLAYOUT(selmon).arrange
				&& (abs(nw - c->w) > SNAP_PX || abs(nh - c->h) > SNAP_PX))
					togglefloating(NULL);
			}
			if (!CURLAYOUT(selmon).arrange || c->isfloating)
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
	if (m->sel->isfloating || !CURLAYOUT(m).arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (CURLAYOUT(m).arrange) {
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
	/* main event loop */
	XSync(dpy, False);
	while (running && !XNextEvent(dpy, &ev))
		if (handler[ev.type])
			handler[ev.type](&ev); /* call handler */
}

void
scan(void)
{
	uint i, num;
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
	c->desktop = m->curgrp->curdsk;
	attach(c);
	attachstack(c);
	focus(NULL);
	arrange(NULL);
}

void
setbarmode(const Arg *arg)
{
	if (!arg || arg->i < 0 || arg->i > BarModeStatus)
		return;

	selmon->barmode = arg->i;
	drawbar(selmon);
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
	if (!arg)
		return;

	const uint i = arg->i;
	Desktop *d = selmon->curgrp->curdsk;
	if (i >= LENGTH(layouts) || i == d->sellayout)
		return;

	d->sellayout = i;
	strncpy(selmon->ltsymbol, layouts[i].symbol, sizeof selmon->ltsymbol);

	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !CURLAYOUT(selmon).arrange)
		return;

	Desktop *d = selmon->curgrp->curdsk;
	f = arg->f < 1.0 ? arg->f + d->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	d->mfact = f;
	arrange(selmon);
}

void
setup(void)
{
	int i;
	XSetWindowAttributes wa;
	Atom utf8string;
	struct sigaction sa;

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
	lrpad_2 = lrpad / 2;
	bh = drw->fonts->h + 2;

	w_ellipsis_l = TEXTW(ellipsis_l);
	w_ellipsis_r = TEXTW(ellipsis_r);

	for (int i = 0; i < LENGTH(clabels); i++) {
		w_clabels[i] = TEXTW(clabels[i]);
	}

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
		if ((!CURLAYOUT(c->mon).arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
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
tagmon(const Arg *arg)
{
	if (!arg || !selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

void
tile(Monitor *m)
{
	int i, n, h, mw, my, ty;
	Client *c;
	const Desktop *d = m->curgrp->curdsk;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	const int nm = d->nmaster;
	const float mf = d->mfact;

	if (n > nm)
		mw = nm ? m->ww * mf : 0;
	else
		mw = m->ww;
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < nm) {
			h = (m->wh - my) / (MIN(n, nm) - i);
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
			if (ty + HEIGHT(c) < m->wh)
				ty += HEIGHT(c);
		}
}

void
togglebar(const Arg *arg)
{
	selmon->curgrp->curdsk->showbar = !selmon->curgrp->curdsk->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	Client *sel = selmon->sel;

	if (!sel)
		return;
	if (sel->isfullscreen) /* no support for fullscreen windows */
		return;
	sel->isfloating = !sel->isfloating || sel->isfixed;
	if (sel->isfloating)
		resize(sel, sel->x, sel->y, sel->w, sel->h, 0);
	arrange(selmon);
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNormal][ColBorder].pixel);
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
	if (m->curgrp->curdsk->showbar) {
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
				c->mon = m;
				c->desktop = m->curgrp->curdsk;
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
	uint i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1ULL << i);
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
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
		strcpy(stext, "");

	if (selmon->barmode == BarModeStatus)
		drawbar(selmon);
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

	if (!CURLAYOUT(selmon).arrange || !c || c->isfloating)
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
