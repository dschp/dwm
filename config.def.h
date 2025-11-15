/* See LICENSE file for copyright and license details. */

/* appearance */
static const uint borderpx          = 1;  /* border pixel of windows */
static const uint snap              = 16; /* snap pixel */
static const uint showbar           = 1;  /* 0 means no bar */
static const uint topbar            = 1;  /* 0 means bottom bar */
static const char *fonts[]          = { "sans-serif:size=11" };
static const char dmenufont[]       = "sans-serif:size=15";

static const char col_white1[]      = "#ffffff";
static const char col_white2[]      = "#eeeeee";
static const char col_black1[]      = "#000000";
static const char col_black2[]      = "#191919";
static const char col_bdr0[]        = "#242424";
static const char col_bdr1[]        = "#0090ff";
static const char col_red[]         = "#f32f7c";
static const char col_green[]       = "#afff00";
static const char col_blue[]        = "#00bbff";
static const char col_yellow[]      = "#ffff00";
static const char col_current[]     = "#00bbff";
static const char col_previous[]    = "#be2df1";
static const char col_dmenu_selbg[] = "#0077cc";

static const char *colors[][3]      = {
	/*                   fg            bg            border   */
	[SchemeNorm]     = { col_white2,   col_black2,   col_bdr0 },
	[SchemeCurrent]  = { col_current,  col_black1,   col_bdr1 },
	[SchemePrevious] = { col_previous, col_black2,   col_bdr1 },
	[SchemeLayout]   = { col_green,    col_black2,   col_bdr0 },
	[SchemeNmaster]  = { col_red,      col_black2,   col_bdr0 },
	[SchemeMfact]    = { col_yellow,   col_black2,   col_bdr0 },
};

/* tagging */
static const char *tags[] = {
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
	"A", "S", "D", "F", "G",
	"Z", "X", "C", "V", "B", "N", "M",
};

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	{ "Dummy",     NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const float mfact         = 0.50; /* factor of master area size [0.05..0.95] */
static const uint nmaster        = 1;    /* number of clients in master area */
static const uint resizehints    = 0;    /* 1 means respect size hints in tiled resizals */
static const uint lockfullscreen = 1;    /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "[M]",      monocle },
	{ "><>",      NULL },    /* no layout function means floating behavior */
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.i = TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.i = TAG} }, \
	{ MODKEY|ControlMask,           KEY,      tagnview,       {.i = TAG} }, \
	{ MODKEY|Mod1Mask,              KEY,      toggletag,      {.i = TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
	"mydmenu",
	"-m",   dmenumon,
	"-fn",  dmenufont,
	"-nb",  col_black2,
	"-nf",  col_white1,
	"-sb",  col_dmenu_selbg,
	"-sf",  col_white2,
	NULL,
};
static const char *termcmd[]  = { "st", NULL };
static const char *pavucmd[]  = { "pavucontrol", NULL };

static const Key keys[] = {
	/* modifier                     key        function        argument */
	{ MODKEY|ControlMask|Mod1Mask, XK_Escape,  quit,           {0} },
	{ MODKEY,               XK_semicolon,      spawn,          {.v = dmenucmd } },
	{ MODKEY,              XK_apostrophe,      spawn,          {.v = dmenucmd } },
	{ MODKEY,                   XK_grave,      spawn,          {.v = pavucmd } },
	{ MODKEY|ShiftMask,        XK_Return,      spawn,          {.v = termcmd } },
	{ MODKEY,                  XK_Return,      zoom,           {0} },
	{ MODKEY,                     XK_Tab,      view,           {.i = -1} },
	{ MODKEY|Mod1Mask,      XK_BackSpace,      killclient,     {0} },
	{ MODKEY,                   XK_space,      togglefloating, {0} },
	{ MODKEY,               XK_backslash,      togglebar,      {0} },
	{ MODKEY,                   XK_minus,      incnmaster,     {.i = -1 } },
	{ MODKEY,                   XK_equal,      incnmaster,     {.i = +1 } },
	{ MODKEY,             XK_bracketleft,      focusstack,     {.i = -1 } },
	{ MODKEY,            XK_bracketright,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY|ControlMask,           XK_j,      focusmon,       {.i = +1 } },
	{ MODKEY|ControlMask,           XK_k,      focusmon,       {.i = -1 } },
	{ MODKEY|Mod1Mask,              XK_j,      tagmon,         {.i = +1 } },
	{ MODKEY|Mod1Mask,              XK_k,      tagmon,         {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ControlMask,           XK_h,      setmfact,       {.f = -0.01} },
	{ MODKEY|ControlMask,           XK_l,      setmfact,       {.f = +0.01} },
	{ MODKEY,                   XK_comma,      setlayout,      {.ui = 0} },
	{ MODKEY,                  XK_period,      setlayout,      {.ui = 1} },
	{ MODKEY,                   XK_slash,      setlayout,      {.ui = 2} },
	{ MODKEY|ShiftMask,         XK_comma,      setbarmode,     {.ui = BarModeDefault} },
	{ MODKEY|ShiftMask,        XK_period,      setbarmode,     {.ui = BarModeOccurrence} },
	{ MODKEY|ShiftMask,         XK_slash,      setbarmode,     {.ui = BarModeCurrent} },
	TAGKEYS(                        XK_1,                      0)
	TAGKEYS(                        XK_2,                      1)
	TAGKEYS(                        XK_3,                      2)
	TAGKEYS(                        XK_4,                      3)
	TAGKEYS(                        XK_5,                      4)
	TAGKEYS(                        XK_6,                      5)
	TAGKEYS(                        XK_7,                      6)
	TAGKEYS(                        XK_8,                      7)
	TAGKEYS(                        XK_9,                      8)
	TAGKEYS(                        XK_0,                      9)
	TAGKEYS(                        XK_q,                      10)
	TAGKEYS(                        XK_w,                      11)
	TAGKEYS(                        XK_e,                      12)
	TAGKEYS(                        XK_r,                      13)
	TAGKEYS(                        XK_t,                      14)
	TAGKEYS(                        XK_y,                      15)
	TAGKEYS(                        XK_u,                      16)
	TAGKEYS(                        XK_i,                      17)
	TAGKEYS(                        XK_o,                      18)
	TAGKEYS(                        XK_p,                      19)
	TAGKEYS(                        XK_a,                      20)
	TAGKEYS(                        XK_s,                      21)
	TAGKEYS(                        XK_d,                      22)
	TAGKEYS(                        XK_f,                      23)
	TAGKEYS(                        XK_g,                      24)
	TAGKEYS(                        XK_z,                      25)
	TAGKEYS(                        XK_x,                      26)
	TAGKEYS(                        XK_c,                      27)
	TAGKEYS(                        XK_v,                      28)
	TAGKEYS(                        XK_b,                      29)
	TAGKEYS(                        XK_n,                      30)
	TAGKEYS(                        XK_m,                      31)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLtSymbol,          0,              Button1,        setlayout,      {.v = &layouts[0]} },
	{ ClkLtSymbol,          0,              Button2,        setlayout,      {.v = &layouts[1]} },
	{ ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
	{ ClkWinTitle,          0,              Button1,        focusstack,     {.i = +1} },
	{ ClkWinTitle,          0,              Button2,        zoom,           {0} },
	{ ClkWinTitle,          0,              Button3,        focusstack,     {.i = -1} },
	{ ClkWinTitle,          0,              Button4,        focusstack,     {.i = -1} },
	{ ClkWinTitle,          0,              Button5,        focusstack,     {.i = +1} },
	{ ClkStatusText,        0,              Button1,        incnmaster,     {.i = +1 } },
	{ ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkStatusText,        0,              Button3,        incnmaster,     {.i = -1 } },
	{ ClkStatusText,        0,              Button4,        setmfact,       {.f = +0.01} },
	{ ClkStatusText,        0,              Button5,        setmfact,       {.f = -0.01} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
	{ ClkTagBar,            0,              Button1,        view,           {0} },
	{ ClkTagBar,            0,              Button2,        tag,            {0} },
	{ ClkTagBar,            0,              Button3,        toggletag,      {0} },
};
