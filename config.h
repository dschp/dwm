/* See LICENSE file for copyright and license details. */

#define BAR_CLIENT_MAX_WIDTH 250
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
static const char col_fg1[]         = "#dddddd";
static const char col_fg2[]         = "#eeeeee";
static const char col_bg1[]         = "#000000";
static const char col_bg2[]         = "#191919";
static const char col_bdr1[]        = "#2652ac";
static const char col_bdr2[]        = "#242424";
static const char col_cyan1[]       = "#00bbff";
static const char col_cyan2[]       = "#0077cc";
static const char col_yellow[]      = "#ffff00";
static const char col_green[]       = "#afff00";
static const char col_red[]         = "#ff0087";
static const char col_aqua[]        = "#00dddd";

static const char *colors[][3]      = {
  /*                 fg          bg        border   */
  [SchemeNorm]   = { col_fg1,    col_bg2,  col_bdr2 },
  [SchemeSel]    = { col_cyan1,  col_bg1,  col_bdr1 },
  [SchemeSpawn]  = { col_green,  col_bg1,  col_bdr1 },
                   { col_yellow, col_bg2,  col_bdr2 },
                   { col_green,  col_bg2,  col_bdr2 },
                   { col_red,    col_bg2,  col_bdr2 },
                   { col_aqua,   col_bg2,  col_bdr2 },
};

/* tagging */
static const char *tags[] = {
  "~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\\",
  "A", "S", "D", "F", "G", ";", "'",
  "Z", "X", "C", "V", "B", "N", "M", "<", ">", "/",
};

static const Rule rules[] = {
  /* xprop(1):
   *	WM_CLASS(STRING) = instance, class
   *	WM_NAME(STRING) = title
   */
  /* class      instance    title       tag index     isfloating   monitor */
  { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
  /* symbol     arrange function */
  { "",         NULL },
};

/* key definitions */
#define AltMask Mod1Mask
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
  { MODKEY,                       KEY, toggleview,     {.i  = TAG} }, \
  { MODKEY|ControlMask,           KEY, view,           {.i  = TAG} }, \
  { MODKEY|ShiftMask,             KEY, tag,            {.ui = TAG} }, \
  { MODKEY|AltMask,               KEY, tagandview,     {.ui = TAG} },

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
  { MODKEY|AltMask|ShiftMask,     XK_Escape,       quit,            {0} },
  { MODKEY|AltMask,               XK_Delete,       killclient,      {0} },
  { MODKEY,                       XK_Return,       spawn,           {.v = dmenucmd } },
  { MODKEY|ShiftMask,             XK_Return,       spawn,           {.v = termcmd } },
  { MODKEY,                       XK_Tab,          toggleview,      {.i = -1} },
  { MODKEY|ControlMask,           XK_Tab,          view,            {.i = -1} },
  { MODKEY|ShiftMask,             XK_Tab,          viewclients,     {0} },
  { MODKEY|AltMask,               XK_Tab,          togglebar,       {0} },
  { MODKEY|AltMask|ShiftMask,     XK_h,            moveclient_w,    {.f = +1.0 } },
  { MODKEY,                       XK_j,            focusstack,      {.i = +1 } },
  { MODKEY,                       XK_k,            focusstack,      {.i = -1 } },
  { MODKEY|AltMask|ShiftMask,     XK_l,            moveclient_h,    {.f = +1.0 } },
  { MODKEY|AltMask,               XK_h,            snapandcenter_x, {.i = -1} },
  { MODKEY|AltMask,               XK_j,            snapandcenter_y, {.i = +1} },
  { MODKEY|AltMask,               XK_k,            snapandcenter_y, {.i = -1} },
  { MODKEY|AltMask,               XK_l,            snapandcenter_x, {.i = +1} },
  { MODKEY|ShiftMask,             XK_h,            moveclient_x,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_j,            moveclient_y,    {.f = +0.05 } },
  { MODKEY|ShiftMask,             XK_k,            moveclient_y,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_l,            moveclient_x,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_h,            moveclient_w,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_j,            moveclient_h,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_k,            moveclient_h,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_l,            moveclient_w,    {.f = +0.05 } },
  { MODKEY,                       XK_space,        maximize,        {0} },
  { MODKEY|ControlMask,           XK_space,        centerwindow,    {0} },
  TAGKEYS(                        XK_grave,                         0)
  TAGKEYS(                        XK_1,                             1)
  TAGKEYS(                        XK_2,                             2)
  TAGKEYS(                        XK_3,                             3)
  TAGKEYS(                        XK_4,                             4)
  TAGKEYS(                        XK_5,                             5)
  TAGKEYS(                        XK_6,                             6)
  TAGKEYS(                        XK_7,                             7)
  TAGKEYS(                        XK_8,                             8)
  TAGKEYS(                        XK_9,                             9)
  TAGKEYS(                        XK_0,                             10)
  TAGKEYS(                        XK_minus,                         11)
  TAGKEYS(                        XK_equal,                         12)
  TAGKEYS(                        XK_q,                             13)
  TAGKEYS(                        XK_w,                             14)
  TAGKEYS(                        XK_e,                             15)
  TAGKEYS(                        XK_r,                             16)
  TAGKEYS(                        XK_t,                             17)
  TAGKEYS(                        XK_y,                             18)
  TAGKEYS(                        XK_u,                             19)
  TAGKEYS(                        XK_i,                             20)
  TAGKEYS(                        XK_o,                             21)
  TAGKEYS(                        XK_p,                             22)
  TAGKEYS(                        XK_bracketleft,                   23)
  TAGKEYS(                        XK_bracketright,                  24)
  TAGKEYS(                        XK_backslash,                     25)
  TAGKEYS(                        XK_a,                             26)
  TAGKEYS(                        XK_s,                             27)
  TAGKEYS(                        XK_d,                             28)
  TAGKEYS(                        XK_f,                             29)
  TAGKEYS(                        XK_g,                             30)
  TAGKEYS(                        XK_semicolon,                     31)
  TAGKEYS(                        XK_apostrophe,                    32)
  TAGKEYS(                        XK_z,                             33)
  TAGKEYS(                        XK_x,                             34)
  TAGKEYS(                        XK_c,                             35)
  TAGKEYS(                        XK_v,                             36)
  TAGKEYS(                        XK_b,                             37)
  TAGKEYS(                        XK_n,                             38)
  TAGKEYS(                        XK_m,                             39)
  TAGKEYS(                        XK_comma,                         40)
  TAGKEYS(                        XK_period,                        41)
  TAGKEYS(                        XK_slash,                         42)
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                  event mask      button          function        argument */
  { ClkClientWin,           MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,           MODKEY,         Button3,        resizemouse,    {0} },
};
