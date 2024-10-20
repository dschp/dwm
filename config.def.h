/* See LICENSE file for copyright and license details. */

#define BAR_CLIENT_MIN_WIDTH 200
#define ENV_STATUS_DIR   "STATUS_DIR"
#define STATUS_MAX_FILE  10
#define STATUS_SEP_LEN   sizeof(status_separator) - 1
static const char status_separator[] = " / ";

#define TILE_LIMIT_MIN_HEIGHT 200

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 16;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 0;        /* 0 means bottom bar */
static const char *fonts[]          = { "sans-serif:size=7" };
static const char dmenufont[]       = "sans-serif:size=11";
static const char col_fg1[]         = "#dddddd";
static const char col_fg2[]         = "#eeeeee";
static const char col_fg3[]         = "#ffffff";
static const char col_bg1[]         = "#000000";
static const char col_bg2[]         = "#191919";
static const char col_bg3[]         = "#2a2a2a";
static const char col_bg4[]         = "#3f3f3f";
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
  /*                   fg            bg         border    status code */
  [SchemeNorm]     = { col_fg1,      col_bg2,   col_bdr2 }, /* 0x20 */
  [SchemeSel]      = { col_cyan1,    col_bg1,   col_bdr1 }, /* 0x21 */
  [SchemeLayout]   = { col_green,    col_bg1,   col_bdr1 }, /* 0x22 */
  [SchemeValue1]   = { col_nmaster,  col_bg3,   col_bdr2 }, /* 0x23 */
  [SchemeValue2]   = { col_mfactor,  col_bg3,   col_bdr2 }, /* 0x24 */
  [SchemeValue3]   = { col_green,    col_bg3,   col_bdr2 }, /* 0x25 */
  [SchemeValue4]   = { col_yellow,   col_bg3,   col_bdr2 }, /* 0x25 */
  [SchemeTagged]   = { col_cyan1,   col_gray,   col_bdr2 }, /* 0x26 */
  [SchemeSelIdx]   = { col_fg2,      col_bg3,   col_bdr2 }, /* 0x27 */
  [SchemeOverflow] = { col_fg3,      col_bg4,   col_bdr2 }, /* 0x27 */
                     { col_yellow,   col_bg2,   col_bdr2 }, /* 0x28 */
                     { col_green,    col_bg2,   col_bdr2 }, /* 0x29 */
                     { col_red,      col_bg2,   col_bdr2 }, /* 0x2A */
                     { col_aqua,     col_bg2,   col_bdr2 }, /* 0x2B */
};

/* tagging */
static const char *tags[] = {
  "~",
  "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=",
  "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "\\",
  "A", "S", "D", "F", "G", ";", "'",
  "Z", "X", "C", "V", "B", "N", "M", "<", ">", "/",
  "Esc",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
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
static const uint v1_init     = 1;    /* interger variable 1 */
static const uint v2_init     = 1;    /* interger variable 2 */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
  /* symbol   arrange function */
  // === 0 - 3
  { "[]=",    tileleft },
  { "=[]",    tileright },
  { "[M]",    monocle },
  { "|G|",    gridv1v2 },
  // === 4 - 7
  { "[]-",    tilev1v2left },
  { "-[]",    tilev1v2right },
  { "(0,0)",  xyzero },
  { "><",     stackcenter },
  // === 8 - 11
  { "[]%",    tilelimitright },
  { "%[]",    tilelimitleft },
};

/* key definitions */
#define AltMask Mod1Mask
#define ASCMask AltMask|ShiftMask|ControlMask
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG) \
  { MODKEY,                       KEY, toggleview,      {.ui = 1ULL << TAG} }, \
  { MODKEY|ShiftMask,             KEY, tag,             {.ui = 1ULL << TAG} }, \
  { MODKEY|AltMask,               KEY, toggletag,       {.ui = 1ULL << TAG} }, \
  { MODKEY|ControlMask,           KEY, switchworkspace, {.i = TAG} }

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
  { MODKEY|ControlMask,           XK_Return,       togglespawnfloating,  {0} },
  { MODKEY|AltMask,               XK_Return,       toggleexclusiveview,  {0} },
  { MODKEY,                       XK_Home,         togglebar,       {0} },
  { MODKEY,                       XK_Left,         setlayout,       {.ui = 0 } },
  { MODKEY,                       XK_Right,        setlayout,       {.ui = 1 } },
  { MODKEY,                       XK_Up,           setlayout,       {.ui = 2 } },
  { MODKEY,                       XK_Down,         setlayout,       {.ui = 3 } },
  { MODKEY|ShiftMask,             XK_Left,         setlayout,       {.ui = 4 } },
  { MODKEY|ShiftMask,             XK_Right,        setlayout,       {.ui = 5 } },
  { MODKEY|ShiftMask,             XK_Up,           setlayout,       {.ui = 6 } },
  { MODKEY|ShiftMask,             XK_Down,         setlayout,       {.ui = 7 } },
  { MODKEY|AltMask,               XK_Left,         setlayout,       {.ui = 8 } },
  { MODKEY|AltMask,               XK_Right,        setlayout,       {.ui = 9 } },
  { MODKEY|AltMask,               XK_Up,           setlayout,       {.ui = 10 } },
  { MODKEY|AltMask,               XK_Down,         setlayout,       {.ui = 11 } },
  { MODKEY,                       XK_Tab,          toggleview,      {.ui = 0 } },
  { MODKEY|AltMask,               XK_Tab,          tag,             {.ui = 0 } },
  { MODKEY|ShiftMask,             XK_Tab,          viewclients,     {0} },
  { MODKEY|ControlMask,           XK_Tab,          switchworkspace, {.i = -1 } },
  { MODKEY,                       XK_h,            incvf,           {.f = -0.05 } },
  { MODKEY,                       XK_j,            focusstack,      {.i = +1 } },
  { MODKEY,                       XK_k,            focusstack,      {.i = -1 } },
  { MODKEY,                       XK_l,            incvf,           {.f = +0.05 } },
  { MODKEY|AltMask,               XK_h,            focus_1st_master,{0} },
  { MODKEY|AltMask,               XK_j,            focuscycle,      {.i = +1 } },
  { MODKEY|AltMask,               XK_k,            focuscycle,      {.i = -1 } },
  { MODKEY|AltMask,               XK_l,            focus_1st_stack, {0} },
  { MODKEY|AltMask,               XK_h,            snapandcenter_x, {.i = -1 } },
  { MODKEY|AltMask,               XK_j,            snapandcenter_y, {.i = +1 } },
  { MODKEY|AltMask,               XK_k,            snapandcenter_y, {.i = -1 } },
  { MODKEY|AltMask,               XK_l,            snapandcenter_x, {.i = +1 } },
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
  TAGKEYS(                        XK_1,                              1),
  TAGKEYS(                        XK_2,                              2),
  TAGKEYS(                        XK_3,                              3),
  TAGKEYS(                        XK_4,                              4),
  TAGKEYS(                        XK_5,                              5),
  TAGKEYS(                        XK_6,                              6),
  TAGKEYS(                        XK_7,                              7),
  TAGKEYS(                        XK_8,                              8),
  TAGKEYS(                        XK_9,                              9),
  TAGKEYS(                        XK_0,                             10),
  TAGKEYS(                        XK_minus,                         11),
  TAGKEYS(                        XK_equal,                         12),
  TAGKEYS(                        XK_q,                             13),
  TAGKEYS(                        XK_w,                             14),
  TAGKEYS(                        XK_e,                             15),
  TAGKEYS(                        XK_r,                             16),
  TAGKEYS(                        XK_t,                             17),
  TAGKEYS(                        XK_y,                             18),
  TAGKEYS(                        XK_u,                             19),
  TAGKEYS(                        XK_i,                             20),
  TAGKEYS(                        XK_o,                             21),
  TAGKEYS(                        XK_p,                             22),
  TAGKEYS(                        XK_bracketleft,                   23),
  TAGKEYS(                        XK_bracketright,                  24),
  TAGKEYS(                        XK_backslash,                     25),
  TAGKEYS(                        XK_a,                             26),
  TAGKEYS(                        XK_s,                             27),
  TAGKEYS(                        XK_d,                             28),
  TAGKEYS(                        XK_f,                             29),
  TAGKEYS(                        XK_g,                             30),
  TAGKEYS(                        XK_semicolon,                     31),
  TAGKEYS(                        XK_apostrophe,                    32),
  TAGKEYS(                        XK_z,                             33),
  TAGKEYS(                        XK_x,                             34),
  TAGKEYS(                        XK_c,                             35),
  TAGKEYS(                        XK_v,                             36),
  TAGKEYS(                        XK_b,                             37),
  TAGKEYS(                        XK_n,                             38),
  TAGKEYS(                        XK_m,                             39),
  TAGKEYS(                        XK_comma,                         40),
  TAGKEYS(                        XK_period,                        41),
  TAGKEYS(                        XK_slash,                         42),
  TAGKEYS(                        XK_Escape,                        43),
  TAGKEYS(                        XK_F1,                            44),
  TAGKEYS(                        XK_F2,                            45),
  TAGKEYS(                        XK_F3,                            46),
  TAGKEYS(                        XK_F4,                            47),
  TAGKEYS(                        XK_F5,                            48),
  TAGKEYS(                        XK_F6,                            49),
  TAGKEYS(                        XK_F7,                            50),
  TAGKEYS(                        XK_F8,                            51),
  TAGKEYS(                        XK_F9,                            52),
  TAGKEYS(                        XK_F10,                           53),
  TAGKEYS(                        XK_F11,                           54),
  TAGKEYS(                        XK_F12,                           55),
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                  event mask      button          function        argument */
  { ClkClientWin,           MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,           MODKEY,         Button3,        resizemouse,    {0} },
};
