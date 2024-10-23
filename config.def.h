/* See LICENSE file for copyright and license details. */

#define BAR_CLIENT_MIN_WIDTH 200
#define ENV_STATUS_DIR   "STATUS_DIR"
#define STATUS_MAX_FILE  10
#define STATUS_SEP_LEN   sizeof(status_separator) - 1
static const char status_separator[] = " / ";

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 16;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 0;        /* 0 means bottom bar */
static const char *fonts[]          = { "sans-serif:size=7" };
static const char dmenufont[]       = "sans-serif:size=11";
static const char col_cyan1[]       = "#00bbff";
static const char col_cyan2[]       = "#0077cc";
static const char col_yellow[]      = "#ffff00";
static const char col_green[]       = "#afff00";
static const char col_aqua[]        = "#00dddd";
static const char col_red1[]        = "#f32f7c";
static const char col_red2[]        = "#ff0087";
static const char col_blue[]        = "#4992ff";
static const char col_gray[]        = "#888888";
static const char col_fg1[]         = "#dddddd";
static const char col_fg2[]         = "#eeeeee";
static const char col_fg3[]         = "#ffffff";
static const char col_bg1[]         = "#000000";
static const char col_bg2[]         = "#191919";
static const char col_bg3[]         = "#2a2a2a";
static const char col_bg4[]         = "#3f3f3f";
static const char col_bdr1[]        = "#242424";
static const char col_bdr2[]        = "#0090ff";
static const char col_bdr3[]        = "#c01770";


static const char *colors[][3]      = {
  /*                   fg            bg         border   */
  [SchemeNorm]     = { col_fg1,      col_bg2,   col_bdr1 },
  [SchemeSel1]     = { col_cyan1,    col_bg1,   col_bdr2 },
  [SchemeSel2]     = { col_red1,     col_bg1,   col_bdr3 },
  [SchemeWS]       = { col_green,    col_bg1,   col_bdr1 },
  [SchemeValue1]   = { col_blue,     col_bg3,   col_bdr1 },
  [SchemeValue2]   = { col_red1,     col_bg3,   col_bdr1 },
  [SchemeValue3]   = { col_green,    col_bg3,   col_bdr1 },
  [SchemeValue4]   = { col_yellow,   col_bg3,   col_bdr1 },
  [SchemeClientId] = { col_fg2,      col_bg3,   col_bdr1 },
  [SchemeOverflow] = { col_fg3,      col_bg4,   col_bdr1 },
                     { col_yellow,   col_bg2,   col_bdr1 },
                     { col_green,    col_bg2,   col_bdr1 },
                     { col_red2,     col_bg2,   col_bdr1 },
                     { col_aqua,     col_bg2,   col_bdr1 },
};

static const char *workspacez[] = {
  "~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
};

/* tagging */
static const char *tags[] = {
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\\",
  "A", "S", "D", "F", "G", ";", "'",
  "Z", "X", "C", "V", "B", "N", "M", "<", ">", "/",
};

static const Rule rules[] = {
  /* xprop(1):
   *	WM_CLASS(STRING) = instance, class
   *	WM_NAME(STRING) = title
   */
  /* class      instance    title       tags      isfloating   monitor */
  { "Gimp",     NULL,       NULL,       0,        1,           -1 },
};

/* layout(s) */
static const float vf_init   = 0.55; /* float variable [0.05..0.95] */
static const uint v1_init    = 0;    /* interger variable 1 */
static const uint v2_init    = 0;    /* interger variable 2 */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
  /* symbol   arrange function */
  { "[]=",    tile },
  { "<>",     juststack },
  { "[M]",    monocle },
};

/* key definitions */
#define AltMask Mod1Mask
#define ASCMask AltMask|ShiftMask|ControlMask
#define MODKEY Mod4Mask
#define WSKEYS(KEY,TAG) \
  { MODKEY,                       KEY, switchworkspace, {.i = TAG} }

#define TAGKEYS(KEY,TAG) \
  { MODKEY,                       KEY, setview_1,       {.i = TAG} }, \
  { MODKEY|ControlMask,           KEY, setview_2,       {.i = TAG} }, \
  { MODKEY|ShiftMask,             KEY, tag,             {.ui = 1ULL << TAG} }, \
  { MODKEY|AltMask,               KEY, toggletag,       {.ui = 1ULL << TAG} }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
  "mydmenu",
  "-m",   dmenumon,
  "-fn",  dmenufont,
  "-nb",  col_bg2,
  "-nf",  col_fg1,
  "-sb",  col_cyan2,
  "-sf",  col_fg2,
  NULL };
static const char *termcmd[]  = { "st", NULL };

static Key keys[] = {
  /* modifier                     key              function         argument */
  { MODKEY|ASCMask,               XK_BackSpace,    quit,            {0} },
  { MODKEY|AltMask,               XK_Delete,       killclient,      {0} },
  { MODKEY,                       XK_Return,       spawn,           {.v = dmenucmd } },
  { MODKEY|ShiftMask,             XK_Return,       spawn,           {.v = termcmd } },
  { MODKEY|ControlMask,           XK_Return,       togglespawnfloating, {0} },
  { MODKEY|AltMask,               XK_Return,       swapspawnview,   {0} },
  { MODKEY,                       XK_Left,         setlayout,       {.ui = 0 } },
  { MODKEY,                       XK_Right,        setlayout,       {.ui = 1 } },
  { MODKEY,                       XK_Up,           setlayout,       {.ui = 2 } },
  { MODKEY,                       XK_Down,         togglebar,       {0} },
  { MODKEY,                       XK_Tab,          switchworkspace, {.i = -1 } },
  { MODKEY|ControlMask,           XK_Tab,          clearviews,      {.ui = 0 } },
  { MODKEY|AltMask,               XK_Tab,          focuscycle_floating, {.i = +1 } },
  { MODKEY,                       XK_h,            incvf,           {.f = -0.05 } },
  { MODKEY,                       XK_j,            focuscycle,      {.i = +1 } },
  { MODKEY,                       XK_k,            focuscycle,      {.i = -1 } },
  { MODKEY,                       XK_l,            incvf,           {.f = +0.05 } },
  { MODKEY|AltMask,               XK_h,            focus_1st_1,     {0} },
  { MODKEY|AltMask,               XK_j,            focuscycle_view, {.i = +1 } },
  { MODKEY|AltMask,               XK_k,            focuscycle_view, {.i = -1 } },
  { MODKEY|AltMask,               XK_l,            focus_1st_2,     {0} },
  { MODKEY|ShiftMask,             XK_h,            incvf,           {.f = -0.01 } },
  { MODKEY|ShiftMask,             XK_j,            movestack,       {.i = +1 } },
  { MODKEY|ShiftMask,             XK_k,            movestack,       {.i = -1 } },
  { MODKEY|ShiftMask,             XK_l,            incvf,           {.f = +0.01 } },
  { MODKEY|ShiftMask,             XK_h,            moveclient_x,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_j,            moveclient_y,    {.f = +0.05 } },
  { MODKEY|ShiftMask,             XK_k,            moveclient_y,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_l,            moveclient_x,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_h,            incv1,           {.i = -1 } },
  { MODKEY|ControlMask,           XK_j,            incv1,           {.i = +1 } },
  { MODKEY|ControlMask,           XK_k,            incv2,           {.i = -1 } },
  { MODKEY|ControlMask,           XK_l,            incv2,           {.i = +1 } },
  { MODKEY|ControlMask,           XK_h,            moveclient_w,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_j,            moveclient_h,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_k,            moveclient_h,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_l,            moveclient_w,    {.f = +0.05 } },
  { MODKEY|AltMask|ControlMask,   XK_j,            movepointer,     {.i = +1 } },
  { MODKEY|AltMask|ControlMask,   XK_k,            movepointer,     {.i = -1 } },
  { MODKEY|AltMask|ShiftMask,     XK_h,            snapandcenter_x, {.i = -1 } },
  { MODKEY|AltMask|ShiftMask,     XK_j,            snapandcenter_y, {.i = +1 } },
  { MODKEY|AltMask|ShiftMask,     XK_k,            snapandcenter_y, {.i = -1 } },
  { MODKEY|AltMask|ShiftMask,     XK_l,            snapandcenter_x, {.i = +1 } },
  { MODKEY,                       XK_space,        zoom,            {0} },
  { MODKEY,                       XK_space,        maximize,        {0} },
  { MODKEY|ControlMask,           XK_space,        centerwindow,    {0} },
  { MODKEY|ShiftMask,             XK_space,        togglefloating,  {0} },
  { MODKEY|ShiftMask,             XK_space,        centerwindow,    {0} },
  TAGKEYS(                        XK_q,                              0),
  TAGKEYS(                        XK_w,                              1),
  TAGKEYS(                        XK_e,                              2),
  TAGKEYS(                        XK_r,                              3),
  TAGKEYS(                        XK_t,                              4),
  TAGKEYS(                        XK_y,                              5),
  TAGKEYS(                        XK_u,                              6),
  TAGKEYS(                        XK_i,                              7),
  TAGKEYS(                        XK_o,                              8),
  TAGKEYS(                        XK_p,                              9),
  TAGKEYS(                        XK_bracketleft,                   10),
  TAGKEYS(                        XK_bracketright,                  11),
  TAGKEYS(                        XK_backslash,                     12),
  TAGKEYS(                        XK_a,                             13),
  TAGKEYS(                        XK_s,                             14),
  TAGKEYS(                        XK_d,                             15),
  TAGKEYS(                        XK_f,                             16),
  TAGKEYS(                        XK_g,                             17),
  TAGKEYS(                        XK_semicolon,                     18),
  TAGKEYS(                        XK_apostrophe,                    19),
  TAGKEYS(                        XK_z,                             20),
  TAGKEYS(                        XK_x,                             21),
  TAGKEYS(                        XK_c,                             22),
  TAGKEYS(                        XK_v,                             23),
  TAGKEYS(                        XK_b,                             24),
  TAGKEYS(                        XK_n,                             25),
  TAGKEYS(                        XK_m,                             26),
  TAGKEYS(                        XK_comma,                         27),
  TAGKEYS(                        XK_period,                        28),
  TAGKEYS(                        XK_slash,                         29),
  WSKEYS(                         XK_grave,                          0),
  WSKEYS(                         XK_1,                              1),
  WSKEYS(                         XK_2,                              2),
  WSKEYS(                         XK_3,                              3),
  WSKEYS(                         XK_4,                              4),
  WSKEYS(                         XK_5,                              5),
  WSKEYS(                         XK_6,                              6),
  WSKEYS(                         XK_7,                              7),
  WSKEYS(                         XK_8,                              8),
  WSKEYS(                         XK_9,                              9),
  WSKEYS(                         XK_0,                             10),
  WSKEYS(                         XK_minus,                         11),
  WSKEYS(                         XK_equal,                         12),
  WSKEYS(                         XK_F1,                            13),
  WSKEYS(                         XK_F2,                            14),
  WSKEYS(                         XK_F3,                            15),
  WSKEYS(                         XK_F4,                            16),
  WSKEYS(                         XK_F5,                            17),
  WSKEYS(                         XK_F6,                            18),
  WSKEYS(                         XK_F7,                            19),
  WSKEYS(                         XK_F8,                            20),
  WSKEYS(                         XK_F9,                            21),
  WSKEYS(                         XK_F10,                           22),
  WSKEYS(                         XK_F11,                           23),
  WSKEYS(                         XK_F12,                           24),
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                  event mask      button          function        argument */
  { ClkClientWin,           MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,           MODKEY,         Button3,        resizemouse,    {0} },
};
