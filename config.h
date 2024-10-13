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
static const char col_bg3[]         = "#2a2a2a";
static const char col_bdr1[]        = "#0090ff";
static const char col_bdr2[]        = "#242424";
static const char col_cyan1[]       = "#00bbff";
static const char col_cyan2[]       = "#0077cc";
static const char col_yellow[]      = "#ffff00";
static const char col_green[]       = "#afff00";
static const char col_red[]         = "#ff0087";
static const char col_aqua[]        = "#00dddd";
static const char col_gray[]        = "#888888";
static const char col_nmaster[]     = "#4992ff";
static const char col_mfactor[]     = "#f32f7c";


static const char *colors[][3]      = {
  /*                   fg            bg         border       status */
  [SchemeNorm]     = { col_fg1,      col_bg2,   col_bdr2 }, /* 0x20 */
  [SchemeSel]      = { col_cyan1,    col_bg1,   col_bdr1 }, /* 0x21 */
  [SchemeLayout]   = { col_green,    col_bg1,   col_bdr1 }, /* 0x22 */
  [SchemeValue1]   = { col_nmaster,  col_bg3,   col_bdr2 }, /* 0x23 */
  [SchemeValue2]   = { col_mfactor,  col_bg3,   col_bdr2 }, /* 0x24 */
  [SchemeValue3]   = { col_yellow,   col_bg3,   col_bdr2 }, /* 0x25 */
  [SchemeTagged]   = { col_gray,     col_bg1,   col_bdr2 }, /* 0x26 */
  [SchemeOverflow] = { col_fg1,      col_bg2,   col_bdr2 }, /* 0x27 */
                     { col_yellow,   col_bg2,   col_bdr2 }, /* 0x28 */
                     { col_green,    col_bg2,   col_bdr2 }, /* 0x29 */
                     { col_red,      col_bg2,   col_bdr2 }, /* 0x2A */
                     { col_aqua,     col_bg2,   col_bdr2 }, /* 0x2B */
};

/* tagging */
static const char *tags[] = {
  "~",
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\\",
  "A", "S", "D", "F", "G", ";", "'",
  "Z", "X", "C", "B", "N", "M", "V", "<", ">", "/",
};

/* tagging */
static const char *memorytags[] = {
  "Esc",
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
};

static const Rule rules[] = {
  /* xprop(1):
   *	WM_CLASS(STRING) = instance, class
   *	WM_NAME(STRING) = title
   */
  /* class      instance    title       tag index     isfloating   monitor */
  { "Gimp",     NULL,       NULL,       -1,            1,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
  /* symbol     arrange function */
  { "[]=",      tileright },
  { "=[]",      tileleft },
  { "[M]",      monocle },
  { "#",        grid },
  { "+#",       gridtileright },
  { "#+",       gridtileleft },
};

/* key definitions */
#define AltMask Mod1Mask
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
  { MODKEY,                       KEY, toggleview,     {.ui = 1ULL << TAG} }, \
  { MODKEY|ControlMask,           KEY, view,           {.ui = 1ULL << TAG} }, \
  { MODKEY|ShiftMask,             KEY, tag,            {.ui = 1ULL << TAG} }

#define VIEWMEMORYKEYS(KEY,TAG) \
  { MODKEY,                       KEY, recallviews,    {.i = TAG} }, \
  { MODKEY|ShiftMask,             KEY, memorizeviews,  {.i = TAG} }

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
  { MODKEY,                       XK_Left,         setlayout,       {.v = &layouts[0]} },
  { MODKEY,                       XK_Right,        setlayout,       {.v = &layouts[1]} },
  { MODKEY,                       XK_Up,           setlayout,       {.v = &layouts[2]} },
  { MODKEY,                       XK_Down,         setlayout,       {.v = &layouts[3]} },
  { MODKEY|ShiftMask,             XK_Left,         setlayout,       {.v = &layouts[4]} },
  { MODKEY|ShiftMask,             XK_Right,        setlayout,       {.v = &layouts[5]} },
  { MODKEY,                       XK_Tab,          toggleview,      {.ui = 0 } },
  { MODKEY|ShiftMask,             XK_Tab,          viewclients,     {0} },
  { MODKEY|AltMask,               XK_Tab,          togglebar,       {0} },
  { MODKEY,                       XK_h,            setmfact,        {.f = -0.05 } },
  { MODKEY,                       XK_j,            focusstack,      {.i = +1 } },
  { MODKEY,                       XK_k,            focusstack,      {.i = -1 } },
  { MODKEY,                       XK_l,            setmfact,        {.f = +0.05 } },
  { MODKEY|AltMask,               XK_h,            snapandcenter_x, {.i = -1 } },
  { MODKEY|AltMask,               XK_j,            snapandcenter_y, {.i = +1 } },
  { MODKEY|AltMask,               XK_k,            snapandcenter_y, {.i = -1 } },
  { MODKEY|AltMask,               XK_l,            snapandcenter_x, {.i = +1 } },
  { MODKEY|ShiftMask,             XK_h,            incnmaster,      {.i = -1 } },
  { MODKEY|ShiftMask,             XK_h,            moveclient_x,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_j,            movestack,       {.i = +1 } },
  { MODKEY|ShiftMask,             XK_j,            moveclient_y,    {.f = +0.05 } },
  { MODKEY|ShiftMask,             XK_k,            movestack,       {.i = -1 } },
  { MODKEY|ShiftMask,             XK_k,            moveclient_y,    {.f = -0.05 } },
  { MODKEY|ShiftMask,             XK_l,            incnmaster,      {.i = +1 } },
  { MODKEY|ShiftMask,             XK_l,            moveclient_x,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_h,            moveclient_w,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_j,            moveclient_h,    {.f = +0.05 } },
  { MODKEY|ControlMask,           XK_k,            moveclient_h,    {.f = -0.05 } },
  { MODKEY|ControlMask,           XK_l,            moveclient_w,    {.f = +0.05 } },
  { MODKEY|AltMask|ShiftMask,     XK_h,            moveclient_w,    {.f = +1.0 } },
  { MODKEY|AltMask|ShiftMask,     XK_j,            movepointer,     {.i = +1 } },
  { MODKEY|AltMask|ShiftMask,     XK_k,            movepointer,     {.i = -1 } },
  { MODKEY|AltMask|ShiftMask,     XK_l,            moveclient_h,    {.f = +1.0 } },
  { MODKEY,                       XK_space,        zoom,            {0} },
  { MODKEY,                       XK_space,        maximize,        {0} },
  { MODKEY|ControlMask,           XK_space,        centerwindow,    {0} },
  { MODKEY|ShiftMask,             XK_space,        togglefloating,  {0} },
  { MODKEY|ShiftMask,             XK_space,        centerwindow,    {0} },
  TAGKEYS(                        XK_grave,                          0),
  TAGKEYS(                        XK_q,                              1),
  TAGKEYS(                        XK_w,                              2),
  TAGKEYS(                        XK_e,                              3),
  TAGKEYS(                        XK_r,                              4),
  TAGKEYS(                        XK_t,                              5),
  TAGKEYS(                        XK_y,                              6),
  TAGKEYS(                        XK_u,                              7),
  TAGKEYS(                        XK_i,                              8),
  TAGKEYS(                        XK_o,                              9),
  TAGKEYS(                        XK_p,                             10),
  TAGKEYS(                        XK_bracketleft,                   11),
  TAGKEYS(                        XK_bracketright,                  12),
  TAGKEYS(                        XK_backslash,                     13),
  TAGKEYS(                        XK_a,                             14),
  TAGKEYS(                        XK_s,                             15),
  TAGKEYS(                        XK_d,                             16),
  TAGKEYS(                        XK_f,                             17),
  TAGKEYS(                        XK_g,                             18),
  TAGKEYS(                        XK_semicolon,                     19),
  TAGKEYS(                        XK_apostrophe,                    20),
  TAGKEYS(                        XK_z,                             21),
  TAGKEYS(                        XK_x,                             22),
  TAGKEYS(                        XK_c,                             23),
  TAGKEYS(                        XK_v,                             24),
  TAGKEYS(                        XK_b,                             25),
  TAGKEYS(                        XK_n,                             26),
  TAGKEYS(                        XK_m,                             27),
  TAGKEYS(                        XK_comma,                         28),
  TAGKEYS(                        XK_period,                        29),
  TAGKEYS(                        XK_slash,                         30),
  VIEWMEMORYKEYS(                 XK_1,                              0),
  VIEWMEMORYKEYS(                 XK_2,                              1),
  VIEWMEMORYKEYS(                 XK_3,                              2),
  VIEWMEMORYKEYS(                 XK_4,                              3),
  VIEWMEMORYKEYS(                 XK_5,                              4),
  VIEWMEMORYKEYS(                 XK_6,                              5),
  VIEWMEMORYKEYS(                 XK_7,                              6),
  VIEWMEMORYKEYS(                 XK_8,                              7),
  VIEWMEMORYKEYS(                 XK_9,                              8),
  VIEWMEMORYKEYS(                 XK_0,                              9),
  VIEWMEMORYKEYS(                 XK_minus,                         10),
  VIEWMEMORYKEYS(                 XK_equal,                         11),
  VIEWMEMORYKEYS(                 XK_u,                             12),
  VIEWMEMORYKEYS(                 XK_i,                             13),
  VIEWMEMORYKEYS(                 XK_g,                             14),
  VIEWMEMORYKEYS(                 XK_b,                             15),
  VIEWMEMORYKEYS(                 XK_n,                             16),
  VIEWMEMORYKEYS(                 XK_m,                             17),
  VIEWMEMORYKEYS(                 XK_Escape,                        18),
  VIEWMEMORYKEYS(                 XK_F1,                            19),
  VIEWMEMORYKEYS(                 XK_F2,                            20),
  VIEWMEMORYKEYS(                 XK_F3,                            21),
  VIEWMEMORYKEYS(                 XK_F4,                            22),
  VIEWMEMORYKEYS(                 XK_F5,                            23),
  VIEWMEMORYKEYS(                 XK_F6,                            24),
  VIEWMEMORYKEYS(                 XK_F7,                            25),
  VIEWMEMORYKEYS(                 XK_F8,                            26),
  VIEWMEMORYKEYS(                 XK_F9,                            27),
  VIEWMEMORYKEYS(                 XK_F10,                           28),
  VIEWMEMORYKEYS(                 XK_F11,                           29),
  VIEWMEMORYKEYS(                 XK_F12,                           30),
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                  event mask      button          function        argument */
  { ClkClientWin,           MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,           MODKEY,         Button3,        resizemouse,    {0} },
};
