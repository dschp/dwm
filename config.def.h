/* See LICENSE file for copyright and license details. */

/* constants */
#define RESIZEHINTS       0     /* 1 means respect size hints in tiled resizals */
#define LOCKFULLSCREEN    1     /* 1 will force focus on the fullscreen window */
#define REFRESH_RATE      120    /* refresh rate (per second) for client move/resize */
#define BORDER_PX         1     /* border pixel of windows */
#define SNAP_PX           16    /* snap pixel */
#define TOPBAR            1     /* 0 means bottom bar */
#define BAR_CLASS_MAX     5
#define BAR_DESKTOP_MAX   9
#define BAR_TAG_MAX       10
#define BAR_CLIENT_MAX    6
#define BAR_CLIENT_WIDTH  150
#define BAR_URGENT_MAX    3
#define BAR_URGENT_WIDTH  100

/* appearance */
static const char *fonts[]          = { "sans-serif:size=11" };
static const char dmenufont[]       = "sans-serif:size=15";

static const char col_white1[]      = "#ffffff";
static const char col_white2[]      = "#eeeeee";
static const char col_black1[]      = "#000000";
static const char col_black2[]      = "#191919";
static const char col_black3[]      = "#2a2a2a";
static const char col_bdr0[]        = "#242424";
static const char col_bdr1[]        = "#0090ff";
static const char col_red[]         = "#f32f7c";
static const char col_green[]       = "#afff00";
static const char col_blue[]        = "#00bbff";
static const char col_yellow[]      = "#ffff00";
static const char col_emerald[]     = "#00f2ae";
static const char col_dmenu_selbg[] = "#0077cc";

static const char *colors[][3]      = {
	/*                   fg            bg            border   */
	[SchemeNormal]   = { col_white2,   col_black2,   col_bdr0 },
	[SchemeClass]    = { col_emerald,  col_black2,   col_bdr1 },
	[SchemeDesktop]  = { col_blue,     col_black1,   col_bdr1 },
	[SchemeTag]      = { col_red,      col_black2,   col_bdr1 },
	[SchemeClntLbl]  = { col_white1,   col_black3,   col_bdr1 },
	[SchemeUrgent]   = { col_white2,   col_black2,   col_bdr1 },
	[SchemeLayout]   = { col_green,    col_black2,   col_bdr0 },
	[SchemeNmaster]  = { col_red,      col_black2,   col_bdr0 },
	[SchemeMfact]    = { col_yellow,   col_black2,   col_bdr0 },
};

static const Rule rules[] = {
	/* xprop(1):
	 *	WM_CLASS(STRING) = instance, class
	 *	WM_NAME(STRING) = title
	 */
	/* class      instance    title       tags mask     isfloating   monitor */
	0
	/*
	{ "Gimp",     NULL,       NULL,       0,            1,           -1 },
	{ "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
	*/
};

static const ClassRule crules[] = {
	/* class                        rename       nmaster  mfact  showbar  lt_idx */
	{ "st-256color",                "st",        {     1,   0.5,       1,      0} },
	{ "firefox",                    "Firefox",   {     1,   0.5,       1,      1} },
	{ "Brave-browser",              "Brave",     {     1,   0.5,       1,      1} },
	{ "install4j-jclient-Launcher", "Java",      {     1,   0.5,       1,      1} },
};

/* preallocated layout params                  nmaster  mfact  showbar  lt_idx */
const static LayoutParams default_lt_params  = {     1,   0.5,       1,      0 };
static LayoutParams fallback_lt_params       = {     1,   0.5,       1,      0 };
static LayoutParams urgent_lt_params         = {     1,   0.5,       1,      1 };

/* layouts */
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "[M]",      monocle },
	{ "><>",      NULL },    /* no layout function means floating behavior */
	{ "=[]",      tile },
};

/* client labels */
static const char *clabels[] = {
	"Z", "X", "C", "V", "B", "N", "M",
};

/* tag labels */
static const char *tags[] = {
	"Q", "W", "E", "R", "T",
	"A", "S", "D", "F", "G",
};

/* key definitions */
#define MODKEY Mod4Mask
#define CLASSKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,   class_select,         {.i =  IDX} },
#define DESKTOPKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,   desktop_select,       {.i =  IDX} }, \
	{ MODKEY|ShiftMask,           KEY,   desktop_move_client,  {.i =  IDX} }, \
	{ MODKEY|Mod1Mask,            KEY,   desktop_move_client,  {.i = -IDX} }, \
	{ MODKEY|ShiftMask|Mod1Mask,  KEY,   desktop_merge,        {.i =  IDX} },
#define TAGKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,   tag_select,           {.t = TAG_UNIT << IDX} }, \
	{ MODKEY|ShiftMask,           KEY,   tag_set,              {.t = TAG_UNIT << IDX} }, \
	{ MODKEY|Mod1Mask,            KEY,   tag_toggle_c,         {.t = TAG_UNIT << IDX} }, \
	{ MODKEY|ControlMask,         KEY,   tag_toggle_m,         {.t = TAG_UNIT << IDX} },
#define CLIENTKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,   client_select,        {.i =  IDX} },

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
	/* modifier                     key        function              argument */
	{ MODKEY|ShiftMask,        XK_Escape,      quit,                 {0} },
	{ MODKEY|Mod1Mask,         XK_Escape,      killclient,           {0} },
	{ MODKEY,                       XK_p,      spawn,                {.v = dmenucmd} },
	{ MODKEY|ShiftMask,             XK_p,      spawn,                {.v = pavucmd} },
	{ MODKEY|ShiftMask,        XK_Return,      spawn,                {.v = termcmd} },
	{ MODKEY,                  XK_Return,      zoom,                 {0} },
	{ MODKEY,                   XK_space,      togglefloating,       {0} },
	{ MODKEY,                   XK_comma,      togglebar,            {0} },
	{ MODKEY,                     XK_Tab,      dwim_select,          {.i =  0} },
	{ MODKEY,                   XK_slash,      client_select,        {.i = -1} },
	{ MODKEY,                  XK_period,      client_select_urg,    {0} },
	{ MODKEY,                   XK_grave,      banish_pointer,       {.i = -1} },
	{ MODKEY|ShiftMask,         XK_grave,      banish_pointer,       {.i =  1} },
	{ MODKEY,                    XK_Left,      setlayout,            {.i =  0} },
	{ MODKEY,                      XK_Up,      setlayout,            {.i =  1} },
	{ MODKEY,                    XK_Down,      setlayout,            {.i =  2} },
	{ MODKEY,                   XK_Right,      setlayout,            {.i =  3} },
	{ MODKEY|ShiftMask,          XK_Left,      setbarmode,           {.i = BarModeSelClient} },
	{ MODKEY|ShiftMask,            XK_Up,      setbarmode,           {.i = BarModeClients} },
	{ MODKEY|ShiftMask,          XK_Down,      setbarmode,           {.i = BarModeStatus} },
	{ MODKEY|ShiftMask,         XK_Right,      setbarmode,           {.i = BarModeSelClient} },
	{ MODKEY,                   XK_equal,      desktop_add,          {0} },
	{ MODKEY|ShiftMask,         XK_equal,      desktop_move_client,  {.i =  0} },
	{ MODKEY,                   XK_minus,      desktop_remove,       {.i =  1} },
	{ MODKEY|ShiftMask,         XK_minus,      desktop_remove,       {.i = -1} },
	{ MODKEY|Mod1Mask,          XK_minus,      desktop_merge,        {.i = -1} },
	{ MODKEY,             XK_bracketleft,      tag_insert,           {.i = -1} },
	{ MODKEY,            XK_bracketright,      tag_insert,           {.i =  1} },
	{ MODKEY|Mod1Mask,    XK_bracketleft,      tag_remove,           {.i = -1} },
	{ MODKEY|Mod1Mask,   XK_bracketright,      tag_remove,           {.i =  1} },
	{ MODKEY,                       XK_y,      dwim_select,          {.i =  1} },
	{ MODKEY|ShiftMask,             XK_y,      dwim_stack,           {.i =  1} },
	{ MODKEY,                       XK_u,      dwim_adjacent,        {.i = -1} },
	{ MODKEY|ShiftMask,             XK_u,      dwim_swap,            {.i = -1} },
	{ MODKEY,                       XK_i,      dwim_adjacent,        {.i =  1} },
	{ MODKEY|ShiftMask,             XK_i,      dwim_swap,            {.i =  1} },
	{ MODKEY,                       XK_o,      dwim_select,          {.i = -1} },
	{ MODKEY|ShiftMask,             XK_o,      dwim_stack,           {.i = -1} },
	{ MODKEY,                       XK_j,      focusstack,           {.i =  1} },
	{ MODKEY,                       XK_k,      focusstack,           {.i = -1} },
	{ MODKEY|ShiftMask,             XK_j,      movestack,            {.i =  1} },
	{ MODKEY|ShiftMask,             XK_k,      movestack,            {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_j,      client_stack,         {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_k,      client_stack,         {.i =  1} },
	{ MODKEY|ControlMask,           XK_j,      incnmaster,           {.i = -1} },
	{ MODKEY|ControlMask,           XK_k,      incnmaster,           {.i =  1} },
	{ MODKEY,                       XK_h,      setmfact,             {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,             {.f =  0.05} },
	{ MODKEY|ControlMask,           XK_h,      setmfact,             {.f = -0.01} },
	{ MODKEY|ControlMask,           XK_l,      setmfact,             {.f =  0.01} },
	{ MODKEY,                    XK_Home,      focusmon,             {.i = -1} },
	{ MODKEY,                     XK_End,      focusmon,             {.i =  1} },
	{ MODKEY|ShiftMask,          XK_Home,      tagmon,               {.i =  1} },
	{ MODKEY|ShiftMask,           XK_End,      tagmon,               {.i = -1} },
	DESKTOPKEYS(                    XK_1,                            1)
	DESKTOPKEYS(                    XK_2,                            2)
	DESKTOPKEYS(                    XK_3,                            3)
	DESKTOPKEYS(                    XK_4,                            4)
	DESKTOPKEYS(                    XK_5,                            5)
	DESKTOPKEYS(                    XK_6,                            6)
	DESKTOPKEYS(                    XK_7,                            7)
	DESKTOPKEYS(                    XK_8,                            8)
	DESKTOPKEYS(                    XK_9,                            9)
	DESKTOPKEYS(                    XK_0,                           10)
	CLASSKEYS(                     XK_F1,                            1)
	CLASSKEYS(                     XK_F2,                            2)
	CLASSKEYS(                     XK_F3,                            3)
	CLASSKEYS(                     XK_F4,                            4)
	CLASSKEYS(                     XK_F5,                            5)
	CLASSKEYS(                     XK_F6,                            6)
	CLASSKEYS(                     XK_F7,                            7)
	CLASSKEYS(                     XK_F8,                            8)
	CLASSKEYS(                     XK_F9,                            9)
	CLASSKEYS(                    XK_F10,                           10)
	CLASSKEYS(                    XK_F11,                           11)
	CLASSKEYS(                    XK_F12,                           12)
	TAGKEYS(                        XK_q,                            0)
	TAGKEYS(                        XK_w,                            1)
	TAGKEYS(                        XK_e,                            2)
	TAGKEYS(                        XK_r,                            3)
	TAGKEYS(                        XK_t,                            4)
	TAGKEYS(                        XK_a,                            5)
	TAGKEYS(                        XK_s,                            6)
	TAGKEYS(                        XK_d,                            7)
	TAGKEYS(                        XK_f,                            8)
	TAGKEYS(                        XK_g,                            9)
	CLIENTKEYS(                     XK_z,                            1)
	CLIENTKEYS(                     XK_x,                            2)
	CLIENTKEYS(                     XK_c,                            3)
	CLIENTKEYS(                     XK_v,                            4)
	CLIENTKEYS(                     XK_b,                            5)
	CLIENTKEYS(                     XK_n,                            6)
	CLIENTKEYS(                     XK_m,                            7)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click             event mask   button      function              argument */
	{ ClkLayout,         0,           Button1,    setlayout,            {.i = 0} },
	{ ClkLayout,         0,           Button2,    setlayout,            {.i = 2} },
	{ ClkLayout,         0,           Button3,    setlayout,            {.i = 1} },
	{ ClkLayoutParam,    0,           Button1,    incnmaster,           {.i =  1 } },
	{ ClkLayoutParam,    0,           Button2,    spawn,                {.v = termcmd } },
	{ ClkLayoutParam,    0,           Button3,    incnmaster,           {.i = -1 } },
	{ ClkLayoutParam,    0,           Button4,    setmfact,             {.f =  0.01} },
	{ ClkLayoutParam,    0,           Button5,    setmfact,             {.f = -0.01} },
	{ ClkDesktop,        0,           Button1,    desktop_select,       {0} },
	{ ClkDesktop,        0,           Button2,    desktop_move_client,  {0} },
	{ ClkDesktop,        0,           Button3,    desktop_move_client,  {0} },
	{ ClkDesktop,        0,           Button4,    desktop_adjacent,     {.i = -1} },
	{ ClkDesktop,        0,           Button5,    desktop_adjacent,     {.i =  1} },
	{ ClkClients,        0,           Button4,    focusstack,           {.i = -1} },
	{ ClkClients,        0,           Button5,    focusstack,           {.i =  1} },
	{ ClkClientWin,      MODKEY,      Button1,    movemouse,            {0} },
	{ ClkClientWin,      MODKEY,      Button2,    togglefloating,       {0} },
	{ ClkClientWin,      MODKEY,      Button3,    resizemouse,          {0} },
};
