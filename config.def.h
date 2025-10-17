/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 16;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "sans-serif:size=11" };
static const char dmenufont[]       = "sans-serif:size=15";

static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";

static const char col_sel1[]        = "#00bbff";
static const char col_sel2[]        = "#f32f7c";
static const char col_sel3[]        = "#afff00";
static const char col_sel4[]        = "#ffcc35";
static const char col_sel5[]        = "#be2df1";
static const char col_col6[]        = "#00f2ae";
static const char col_white1[]      = "#ffffff";
static const char col_white2[]      = "#eeeeee";
static const char col_gray[]        = "#888888";
static const char col_black1[]      = "#000000";
static const char col_black2[]      = "#191919";
static const char col_black3[]      = "#2a2a2a";
static const char col_red[]         = "#f32f7c";
static const char col_green[]       = "#afff00";
static const char col_blue[]        = "#00bbff";
static const char col_yellow[]      = "#ffff00";
static const char col_bdr0[]        = "#242424";
static const char col_bdr1[]        = "#0090ff";
static const char col_bdr2[]        = "#c01770";
static const char col_bdr3[]        = "#7fb900";
static const char col_bdr4[]        = "#f2b500";
static const char col_bdr5[]        = "#be2df1";
static const char col_bdr6[]        = "#00f2ae";
static const char col_dmenu_selbg[] = "#0077cc";

static const char *colors[][3]      = {
	/*                  fg            bg            border   */
	[SchemeNorm]    = { col_white2,   col_black2,   col_bdr0 },
	[SchemeSel]     = { col_sel1,     col_black1,   col_bdr1 },
	[SchemeLayout]  = { col_green,    col_black2,   col_bdr0 },
	[SchemeNmaster] = { col_red,      col_black2,   col_bdr0 },
	[SchemeMfact]   = { col_yellow,   col_black2,   col_bdr0 },
};

/* tagging */
static const char *tags[] = {
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"Q", "W", "E", "R", "A", "S", "D", "F", "Z", "X", "C", "V",
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
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 0;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

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
	{ MODKEY,                    XK_semicolon, spawn,          {.v = dmenucmd } },
	{ MODKEY,                   XK_apostrophe, spawn,          {.v = dmenucmd } },
	{ MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
	{ MODKEY,                       XK_p,      spawn,          {.v = pavucmd } },
	{ MODKEY,                       XK_b,      togglebar,      {0} },
	{ MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
	{ MODKEY,                 XK_bracketleft,  focusstack,     {.i = -1 } },
	{ MODKEY,                 XK_bracketright, focusstack,     {.i = +1 } },
	{ MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
	{ MODKEY,                       XK_u,      incnmaster,     {.i = -1 } },
	{ MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
	{ MODKEY|ControlMask,           XK_h,      setmfact,       {.f = -0.01} },
	{ MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
	{ MODKEY|ControlMask,           XK_l,      setmfact,       {.f = +0.01} },
	{ MODKEY,                       XK_Return, zoom,           {0} },
	{ MODKEY,                       XK_Tab,    view,           {0} },
	{ MODKEY,                       XK_g,      setbarmode,     {.ui = BarModeDefault} },
	{ MODKEY,                       XK_o,      setbarmode,     {.ui = BarModeOccurrence} },
	{ MODKEY,                       XK_n,      setbarmode,     {.ui = BarModeCurrent} },
	{ MODKEY|Mod1Mask,           XK_BackSpace, killclient,     {0} },
	{ MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
	{ MODKEY,                       XK_y,      setlayout,      {.v = &layouts[1]} },
	{ MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
	{ MODKEY,                       XK_space,  setlayout,      {0} },
	{ MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
	{ MODKEY,                       XK_F13,    focusmon,       {.i = -1 } },
	{ MODKEY,                       XK_F14,    focusmon,       {.i = +1 } },
	{ MODKEY|ShiftMask,             XK_F13,    tagmon,         {.i = -1 } },
	{ MODKEY|ShiftMask,             XK_F14,    tagmon,         {.i = +1 } },
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
	TAGKEYS(                        XK_a,                      14)
	TAGKEYS(                        XK_s,                      15)
	TAGKEYS(                        XK_d,                      16)
	TAGKEYS(                        XK_f,                      17)
	TAGKEYS(                        XK_z,                      18)
	TAGKEYS(                        XK_x,                      19)
	TAGKEYS(                        XK_c,                      20)
	TAGKEYS(                        XK_v,                      21)
	{ MODKEY|ControlMask|Mod1Mask,  XK_Escape,    quit,        {0} },
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
	{ ClkTagBar,            0,              Button3,        toggleview,     {0} },
	{ ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
	{ ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};
