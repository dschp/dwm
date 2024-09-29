/* See LICENSE file for copyright and license details. */

/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 0;        /* 0 means bottom bar */
static const char *fonts[]          = { "sans-serif:size=7" };
static const char dmenufont[]       = "sans-serif:size=11";
static const char col_gray1[]       = "#191919";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#dddddd";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#007799";
static const char col_cyan2[]       = "#00bbff";
static const char col_black[]       = "#000000";
static const char col_white[]       = "#ffffff";

static const char *colors[][3]      = {
  /*                 fg         bg          border   */
  [SchemeNorm]   = { col_gray3, col_gray1,  col_gray2 },
  [SchemeSel]    = { col_cyan2, col_black,  col_cyan  },
  [SchemeWarn]   = { "#ffff00", col_gray1,  col_gray2 },
  [SchemeUrgent] = { "#afff00", col_gray1,  col_gray2 },
  { "#ff0087", col_gray1,  col_gray2 },
  { "#00dddd", col_gray1,  col_gray2 },
};

/* tagging */
static const char *tags[] = {
  "~", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "-", "=", "Esc",
  "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
  "F13", "F14", "F15"
};

static const Rule rules[] = {
  /* xprop(1):
   *	WM_CLASS(STRING) = instance, class
   *	WM_NAME(STRING) = title
   */
  /* class      instance    title       tags mask     isfloating   monitor */
  { "Gimp",     NULL,       NULL,       0,            1,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
  /* symbol     arrange function */
  { "[]=",      tile },    /* first entry is default */
  { "><>",      NULL },    /* no layout function means floating behavior */
  { "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod4Mask
#define TAGKEYS(KEY,TAG)						\
  { MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
  { MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
  { MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
  { MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static char dmenumon[2] = "0"; /* component of dmenucmd, manipulated in spawn() */
static const char *dmenucmd[] = {
  "dmenu_run",
  "-m",   dmenumon,
  "-fn",  dmenufont,
  "-nb",  col_gray1,
  "-nf",  col_gray3,
  "-sb",  col_cyan,
  "-sf",  col_gray4,
  NULL };
static const char *termcmd[]  = { "st", NULL };
static const char *doffcmd[]  = { "doff", NULL };
static const char *lockcmd[]  = { "slock", NULL };

#include "movestack.c"
static Key keys[] = {
  /* modifier                     key        function        argument */
  { MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
  { MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
  { MODKEY,                       XK_s,      spawn,          {.v = doffcmd } },
  { MODKEY|ShiftMask,             XK_s,      spawn,          {.v = lockcmd } },
  { MODKEY,                       XK_b,      togglebar,      {0} },
  { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
  { MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
  { MODKEY|ShiftMask,             XK_j,      movestack,      {.i = +1 } },
  { MODKEY|ShiftMask,             XK_k,      movestack,      {.i = -1 } },
  { MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
  { MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
  { MODKEY|ShiftMask,             XK_h,      setmfact,       {.f = -0.01} },
  { MODKEY|ShiftMask,             XK_l,      setmfact,       {.f = +0.01} },
  { MODKEY,                       XK_Return, zoom,           {0} },
  { MODKEY,                       XK_Tab,    view,           {0} },
  { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
  { MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
  { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
  { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
  { MODKEY,                       XK_space,  setlayout,      {0} },
  { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
  { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
  { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
  { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
  { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
  { MODKEY,                       XK_semicolon,  incnmaster, {.i = +1 } },
  { MODKEY,                       XK_apostrophe, incnmaster, {.i = -1 } },
  { MODKEY,                       XK_Right,  viewnext,       {0} },
  { MODKEY,                       XK_Left,   viewprev,       {0} },
  { MODKEY|ShiftMask,             XK_Right,  tagtonext,      {0} },
  { MODKEY|ShiftMask,             XK_Left,   tagtoprev,      {0} },
  TAGKEYS(                        XK_grave,                  0)
  TAGKEYS(                        XK_1,                      1)
  TAGKEYS(                        XK_2,                      2)
  TAGKEYS(                        XK_3,                      3)
  TAGKEYS(                        XK_4,                      4)
  TAGKEYS(                        XK_5,                      5)
  TAGKEYS(                        XK_6,                      6)
  TAGKEYS(                        XK_7,                      7)
  TAGKEYS(                        XK_8,                      8)
  TAGKEYS(                        XK_9,                      9)
  TAGKEYS(                        XK_0,                      10)
  TAGKEYS(                        XK_minus,                  11)
  TAGKEYS(                        XK_equal,                  12)
  TAGKEYS(                        XK_Escape,                 13)
  TAGKEYS(                        XK_F1,                     14)
  TAGKEYS(                        XK_F2,                     15)
  TAGKEYS(                        XK_F3,                     16)
  TAGKEYS(                        XK_F4,                     17)
  TAGKEYS(                        XK_F5,                     18)
  TAGKEYS(                        XK_F6,                     19)
  TAGKEYS(                        XK_F7,                     20)
  TAGKEYS(                        XK_F8,                     21)
  TAGKEYS(                        XK_F9,                     22)
  TAGKEYS(                        XK_F10,                    23)
  TAGKEYS(                        XK_F11,                    24)
  TAGKEYS(                        XK_F12,                    25)
  TAGKEYS(                        XK_F13,                    26)
  TAGKEYS(                        XK_F14,                    27)
  TAGKEYS(                        XK_F15,                    28)
  { MODKEY|ShiftMask|ControlMask, XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static Button buttons[] = {
  /* click                event mask      button          function        argument */
  { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
  { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
  { ClkWinTitle,          0,              Button2,        zoom,           {0} },
  { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
  { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
  { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
  { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
  { ClkTagBar,            0,              Button1,        view,           {0} },
  { ClkTagBar,            0,              Button3,        toggleview,     {0} },
  { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
  { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};

