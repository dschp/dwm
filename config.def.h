/* See LICENSE file for copyright and license details. */

/* constants */
#define MFACT             0.50  /* factor of master area size [0.05..0.95] */
#define NMASTER           1     /* number of clients in master area */
#define RESIZEHINTS       0     /* 1 means respect size hints in tiled resizals */
#define LOCKFULLSCREEN    1     /* 1 will force focus on the fullscreen window */
#define REFRESH_RATE      120    /* refresh rate (per second) for client move/resize */
#define BORDER_PX         1     /* border pixel of windows */
#define SNAP_PX           16    /* snap pixel */
#define SHOWBAR           1     /* 0 means no bar */
#define TOPBAR            1     /* 0 means bottom bar */
#define BAR_FOLDER_MAX    5
#define BAR_WS_MAX        9

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
static const char col_purple[]      = "#be2df1";
static const char col_cyan[]        = "#005577";
static const char col_orange[]      = "#ffcc35";
static const char col_emerald[]     = "#00f2ae";
static const char col_dmenu_selbg[] = "#0077cc";

static const char *colors[][3]      = {
	/*                   fg            bg            border   */
	[SchemeNormal]   = { col_white2,   col_black2,   col_bdr0 },
	[SchemeCurrent]  = { col_blue,     col_black1,   col_bdr1 },
	[SchemePrevious] = { col_purple,   col_black2,   col_bdr1 },
	[SchemeFolder]   = { col_emerald,  col_black2,   col_bdr1 },
	[SchemeCLabel]   = { col_white1,   col_black3,   col_bdr1 },
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
	/* class      instance    title      isfloating    monitor */
	{ "Dummy",    NULL,       NULL,      1,            -1 },
};

/* layouts */
static const Layout layouts[] = {
	/* symbol     arrange function */
	{ "[]=",      tile },    /* first entry is default */
	{ "[M]",      monocle },
	{ "><>",      NULL },    /* no layout function means floating behavior */
};

/* client labels */
static const char *clabels[] = {
	"Z", "X", "C", "V", "B", "N", "M",
};

/* key definitions */
#define MODKEY Mod4Mask
#define WSKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,      ws_select,      {.i =  IDX} }, \
	{ MODKEY|ShiftMask,           KEY,      ws_move_client, {.i =  IDX} }, \
	{ MODKEY|Mod1Mask,            KEY,      ws_move_client, {.i = -IDX} }, \
	{ MODKEY|ShiftMask|Mod1Mask,  KEY,      ws_remove,      {.i =  IDX} },
#define TAGKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,      folder_select,  {.i =  IDX} }, \
	{ MODKEY|ShiftMask,           KEY,      ws_move_folder, {.i =  IDX} },
#define CLIENTKEYS(KEY,IDX) \
	{ MODKEY,                     KEY,      client_select,  {.i =  IDX} },

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
	/* modifier                     key        function         argument */
	{ MODKEY,                       XK_p,      spawn,           {.v = dmenucmd} },
	{ MODKEY|ShiftMask,             XK_p,      spawn,           {.v = pavucmd} },
	{ MODKEY|ShiftMask,        XK_Return,      spawn,           {.v = termcmd} },
	{ MODKEY,                  XK_Return,      zoom,            {0} },
	{ MODKEY,                   XK_space,      togglefloating,  {0} },
	{ MODKEY,                   XK_comma,      togglebar,       {0} },
	{ MODKEY,                  XK_period,      ws_select_urg,   {0} },
	{ MODKEY,                     XK_Tab,      ws_select,       {.i =  0} },
	{ MODKEY|Mod1Mask,            XK_Tab,      folder_select,   {.i =  0} },
	{ MODKEY,                   XK_slash,      ws_select,       {.i = -1} },
	{ MODKEY|Mod1Mask,          XK_slash,      folder_select,   {.i = -1} },
	{ MODKEY|ShiftMask,         XK_slash,      client_select,   {.i = -1} },
	{ MODKEY,             XK_bracketleft,      client_traverse, {.i = -1} },
	{ MODKEY,            XK_bracketright,      client_traverse, {.i = +1} },
	{ MODKEY,                       XK_q,      setbarmode,      {.i = BarModeWorkspace} },
	{ MODKEY,                       XK_w,      setbarmode,      {.i = BarModeClients} },
	{ MODKEY,                       XK_e,      setbarmode,      {.i = BarModeStatus} },
	{ MODKEY,                       XK_t,      setlayout,       {.i = 0} },
	{ MODKEY,                       XK_f,      setlayout,       {.i = 1} },
	{ MODKEY,                       XK_g,      setlayout,       {.i = 2} },
	{ MODKEY,                       XK_a,      ws_add,          {0} },
	{ MODKEY|ShiftMask,             XK_a,      ws_move_client,  {.i =  0} },
	{ MODKEY|Mod1Mask,              XK_a,      folder_add,      {0} },
	{ MODKEY,                       XK_r,      ws_remove,       {.i =  0} },
	{ MODKEY|ShiftMask,             XK_r,      ws_remove,       {.i = -1} },
	{ MODKEY,                       XK_s,      banish_pointer,  {.i = -1} },
	{ MODKEY,                       XK_d,      banish_pointer,  {.i = +1} },
	{ MODKEY,                       XK_y,      folder_adjacent, {.i = -1} },
	{ MODKEY|ShiftMask,             XK_y,      folder_swap,     {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_y,      folder_stack,    {.i = +1} },
	{ MODKEY,                       XK_o,      folder_adjacent, {.i = +1} },
	{ MODKEY|ShiftMask,             XK_o,      folder_swap,     {.i = +1} },
	{ MODKEY|Mod1Mask,              XK_o,      folder_stack,    {.i = -1} },
	{ MODKEY,                       XK_u,      ws_adjacent,     {.i = -1} },
	{ MODKEY|ShiftMask,             XK_u,      ws_swap,         {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_u,      ws_stack,        {.i = +1} },
	{ MODKEY,                       XK_i,      ws_adjacent,     {.i = +1} },
	{ MODKEY|ShiftMask,             XK_i,      ws_swap,         {.i = +1} },
	{ MODKEY|Mod1Mask,              XK_i,      ws_stack,        {.i = -1} },
	{ MODKEY,                       XK_j,      focusstack,      {.i = +1} },
	{ MODKEY,                       XK_k,      focusstack,      {.i = -1} },
	{ MODKEY|ShiftMask,             XK_j,      movestack,       {.i = +1} },
	{ MODKEY|ShiftMask,             XK_k,      movestack,       {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_j,      client_stack,    {.i = -1} },
	{ MODKEY|Mod1Mask,              XK_k,      client_stack,    {.i = +1} },
	{ MODKEY|ControlMask,           XK_j,      incnmaster,      {.i = -1} },
	{ MODKEY|ControlMask,           XK_k,      incnmaster,      {.i = +1} },
	{ MODKEY,                       XK_h,      setmfact,        {.f = -0.05} },
	{ MODKEY,                       XK_l,      setmfact,        {.f = +0.05} },
	{ MODKEY|ControlMask,           XK_h,      setmfact,        {.f = -0.01} },
	{ MODKEY|ControlMask,           XK_l,      setmfact,        {.f = +0.01} },
	{ MODKEY,                   XK_minus,      focusmon,        {.i = -1} },
	{ MODKEY,                   XK_equal,      focusmon,        {.i = +1} },
	{ MODKEY|ShiftMask,         XK_minus,      tagmon,          {.i = +1} },
	{ MODKEY|ShiftMask,         XK_equal,      tagmon,          {.i = -1} },
	WSKEYS(                         XK_1,                       1)
	WSKEYS(                         XK_2,                       2)
	WSKEYS(                         XK_3,                       3)
	WSKEYS(                         XK_4,                       4)
	WSKEYS(                         XK_5,                       5)
	WSKEYS(                         XK_6,                       6)
	WSKEYS(                         XK_7,                       7)
	WSKEYS(                         XK_8,                       8)
	WSKEYS(                         XK_9,                       9)
	WSKEYS(                         XK_0,                      10)
	TAGKEYS(                       XK_F1,                       1)
	TAGKEYS(                       XK_F2,                       2)
	TAGKEYS(                       XK_F3,                       3)
	TAGKEYS(                       XK_F4,                       4)
	TAGKEYS(                       XK_F5,                       5)
	TAGKEYS(                       XK_F6,                       6)
	TAGKEYS(                       XK_F7,                       7)
	TAGKEYS(                       XK_F8,                       8)
	TAGKEYS(                       XK_F9,                       9)
	TAGKEYS(                      XK_F10,                      10)
	TAGKEYS(                      XK_F11,                      11)
	TAGKEYS(                      XK_F12,                      12)
	CLIENTKEYS(                     XK_z,                       1)
	CLIENTKEYS(                     XK_x,                       2)
	CLIENTKEYS(                     XK_c,                       3)
	CLIENTKEYS(                     XK_v,                       4)
	CLIENTKEYS(                     XK_b,                       5)
	CLIENTKEYS(                     XK_n,                       6)
	CLIENTKEYS(                     XK_m,                       7)
	{ MODKEY|Mod1Mask,                        XK_Escape,  killclient,  {0} },
	{ MODKEY|ShiftMask|ControlMask|Mod1Mask,  XK_Escape,  quit,        {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
	/* click                event mask      button          function        argument */
	{ ClkLayout,            0,              Button1,        setlayout,      {.i = 0} },
	{ ClkLayout,            0,              Button2,        setlayout,      {.i = 2} },
	{ ClkLayout,            0,              Button3,        setlayout,      {.i = 1} },
	{ ClkLayoutParam,       0,              Button1,        incnmaster,     {.i = +1 } },
	{ ClkLayoutParam,       0,              Button2,        spawn,          {.v = termcmd } },
	{ ClkLayoutParam,       0,              Button3,        incnmaster,     {.i = -1 } },
	{ ClkLayoutParam,       0,              Button4,        setmfact,       {.f = +0.01} },
	{ ClkLayoutParam,       0,              Button5,        setmfact,       {.f = -0.01} },
	{ ClkWorkspace,         0,              Button1,        ws_select,      {0} },
	{ ClkWorkspace,         0,              Button2,        ws_move_client, {0} },
	{ ClkWorkspace,         0,              Button3,        ws_move_client, {0} },
	{ ClkWorkspace,         0,              Button4,        ws_adjacent,    {.i = -1} },
	{ ClkWorkspace,         0,              Button5,        ws_adjacent,    {.i = +1} },
	{ ClkClients,           0,              Button4,        focusstack,     {.i = -1} },
	{ ClkClients,           0,              Button5,        focusstack,     {.i = +1} },
	{ ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
	{ ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
	{ ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
};
