/*
 * Hello World Plugin
 *
 * Copyright (C) 2004, Gary Kramlich <grim@guifications.org>,
 *               2007, John Bailey <rekkanoryo@cpw.pidgin.im>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

/* config.h may define PURPLE_PLUGINS; protect the definition here so that we
 * don't get complaints about redefinition when it's not necessary. */
#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>

/*********** BEGIN SPOTIFY UTIL LIB CODE ********************/

/* FIXME: Figure out how to link to lib or put in this code automatically from 
 * 'spotify_util' source.
 */

#include <glib.h>
#include <sys/types.h> /* for pid_t */
#include <X11/Xlib.h>
#include <X11/Xatom.h>

struct spotify_playing;
struct spotify;

typedef struct spotify_playing {
    gchar *artist;
    gchar *title;
} spotify_playing;

typedef struct spotify {
    spotify_playing playing;
    pid_t pid;
    gchar *win_title;
    Display *display;
    Window window;
} spotify;

spotify * spotify_init(void); 
void spotify_free(spotify *); 
gboolean spotify_update_playing(spotify *);

#include <stdlib.h>
#include <limits.h>

static inline gboolean _spotify_pid(pid_t *);
static inline gboolean _artist_title(spotify *);
static inline gboolean _window(spotify *);
static inline void _free_window(spotify *);
static void _search_window(Display *display, Window w, spotify *res);
static inline void _free_artist_title(spotify *);
static inline void _free_win_title(spotify *);
static gboolean _win_title(spotify *);
static gboolean _refresh(spotify *);

static inline gboolean 
_spotify_pid(pid_t *pid) 
{
    gchar *user;
    gchar *cmd;
    gchar *cmd_out;
    gint cmd_return;
    gdouble tmp_pid;

    user = getenv("USER");
    if (user == NULL) {
        return FALSE;
    }
    
    /* There can be at most one running instance of Spotify per user so this
     * must do.
     */
    cmd = g_strconcat("pgrep -u ", user, " spotify.exe", NULL);
    if (g_spawn_command_line_sync(cmd, 
                &cmd_out, NULL, &cmd_return, NULL) == FALSE) {
        return FALSE;
    }
    g_free(cmd); 
    
    tmp_pid = g_ascii_strtod(cmd_out, NULL);
    g_free(cmd_out);
    if (tmp_pid == 0) {
        return FALSE; /* `g_ascii_strtod' returns 0 on failure. */
    }

    /* FIXME: We should check that `tmp_pid' is in range. */
    *pid = tmp_pid;
    return TRUE;
}

extern spotify * spotify_init(void)
{
    spotify *res = malloc(sizeof(spotify));
    if (res == NULL)
        return NULL;

    res->playing.artist = NULL;
    res->playing.title = NULL;
    res->pid = -1;
    res->win_title = NULL;
    res->display = NULL;
    res->window = NULL;
    return res;
}

void spotify_free(spotify *res)
{
    _free_artist_title(res);
    _free_win_title(res);
    _free_window(res);
    free(res);
}

static inline void _free_artist_title(spotify *res)
{
    if (res->playing.artist != NULL) {
        g_free(res->playing.artist);
        res->playing.artist = NULL;
    }
    if (res->playing.title != NULL) {
        g_free(res->playing.title);
        res->playing.title = NULL;
    }
}

static inline gboolean _pid(spotify *res)
{
    res->pid = -1;
    if (_spotify_pid(&res->pid) == FALSE)
        return FALSE;
    return TRUE;
}

static gboolean _refresh(spotify *res)
{
    gboolean pid_changed = FALSE;
    gboolean got_title = FALSE;

    if (res->pid == -1) {
        pid_changed = TRUE;
        if (_pid(res) == FALSE)
            return FALSE;
    }
    else {
        /* FIXME: It is unnecessary to fetch a new PID all the time. We should
         * start by trying to "ping" the previously used process.
         */
        pid_t last_pid = res->pid;

        if (_pid(res) == FALSE)
            return FALSE;

        if (last_pid != res->pid) 
            pid_changed = TRUE;
    }

    if (pid_changed || res->window == NULL) {
        if (_window(res) == FALSE)
            return FALSE;
    }
    
    if (got_title == FALSE) {
        if (_win_title(res) == FALSE)
            return FALSE;
    }

    if (_artist_title(res) == FALSE) {
        return FALSE;
    }

    return TRUE;
}

extern gboolean spotify_update_playing(spotify *res)
{
    if (_refresh(res) == FALSE)
        return FALSE;
    return TRUE;
}

static inline gboolean 
_artist_title(spotify *res)
{
    _free_artist_title(res);

    GRegex *regex = NULL;
    GMatchInfo *match_info = NULL;

    /* XXX: The result will be unexpected when there is a `–' sign in the
     * artist name.
     */
    regex = g_regex_new("Spotify - (?P<ARTIST>.+) – (?P<TITLE>.+)", 0, 0, NULL);
    g_regex_match(regex, res->win_title, 0, &match_info);
    res->playing.artist = g_match_info_fetch_named(match_info, "ARTIST");
    res->playing.title = g_match_info_fetch_named(match_info, "TITLE");

    g_match_info_free(match_info);
    g_regex_unref(regex);

    if (res->playing.artist == NULL || res->playing.title == NULL)
        return FALSE;
    return TRUE;
}

static inline gboolean _window(spotify *res)
{
    _free_window(res);

    /* FIXME: What about multi-head setups? */
    Display *display = XOpenDisplay(NULL);
    Window root = XDefaultRootWindow(display);

    _search_window(display, root, res);
    if (res->window == NULL || res->display == NULL) {
        XCloseDisplay(display);
        return FALSE;
    }
    return TRUE;
}

static inline void _free_window(spotify *res)
{
    if (res->display != NULL) {
        XCloseDisplay(res->display);
        res->display = NULL;
    }
    if (res->window != NULL) {
        res->window = NULL;
    }
}

static void _search_window(Display *display, Window w, spotify *res)
{
    Atom           type;
    int            format;
    unsigned long  n_items;
    unsigned long  bytes_after;
    unsigned char *prop_pid = 0;

    if(XGetWindowProperty(display, w, 
                XInternAtom(display, "_NET_WM_PID", True),
                0, 1, False, 
                XA_CARDINAL, &type, &format, &n_items, &bytes_after, &prop_pid)
            == Success) {
        if(prop_pid != 0) {
            if(res->pid == (pid_t)*((unsigned long *)prop_pid)) {
                res->display = display;
                res->window = w;
            }

            XFree(prop_pid);
        }
    }

    if (res->window == NULL) {
        Window    w_root;
        Window    w_parent;
        Window   *w_child;
        unsigned  n_children;
        size_t i;

        if(XQueryTree(display, w, &w_root, &w_parent, &w_child, 
                    &n_children) != 0) {
            for (i = 0; i < n_children && res->window == NULL; ++i) 
                _search_window(display, w_child[i], res);

            if (w_child != NULL)
                XFree(w_child);
        }
    }
}

static gboolean _win_title(spotify *res)
{
    if (res->display == NULL || res->window == NULL)
        return FALSE;

    _free_win_title(res);
    gboolean to_return = FALSE;

    Atom           type;
    int            format;
    unsigned long  n_items;
    unsigned long  bytes_after;
    unsigned char *prop_title = 0;

    Atom utf8_string = XInternAtom(res->display, "UTF8_STRING", 0);
    if (XGetWindowProperty(res->display, res->window,
                XInternAtom(res->display, "_NET_WM_NAME", 0),
                0, LONG_MAX, False,
                utf8_string, &type, &format, &n_items, &bytes_after,
                &prop_title)
            == Success) {
        if (type == utf8_string && format == 8 && n_items != 0) {
            res->win_title = g_strdup((char *)prop_title);
            to_return = TRUE;
        }

        XFree(prop_title);
    }

    return to_return;
}

static inline void _free_win_title(spotify *res)
{
    if (res->win_title != NULL) {
        g_free(res->win_title);
        res->win_title = NULL;
    }
}

/*********** END SPOTIFY UTIL LIB CODE **********************/


/* This will prevent compiler errors in some instances and is better explained in the
 * how-to documents on the wiki */
#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

#include <notify.h>
#include <plugin.h>
#include <version.h>
#include <util.h>
#include <debug.h>

#define PLUGIN_ID "core-spotify"

/* FIXME: Should put these two somewhere else. */
guint spotify_handle;
spotify *spot;

/* FIXME: Put this one somewhere else as well maybe. */
PurplePlugin *spotify_plugin = NULL;


    static gboolean
spotify_poll (gpointer data)
{
    spotify *spotc = (spotify *)data;
    if (spotify_update_playing (spotc)) {
        purple_util_set_current_song (spotc->playing.title, 
                spotc->playing.artist, NULL);
    }
    else {
        purple_util_set_current_song (NULL, NULL, NULL);
    }

    return TRUE;
}

    static gboolean
plugin_load (PurplePlugin * plugin)
{
    guint poll_interval = 3000; 
    gpointer poll_data = NULL;

    spotify_plugin = plugin; /* assign this here so we have a valid handle later */

    if ((spot = spotify_init()) == NULL)
        return FALSE;

    poll_data = (gpointer)spot;
    spotify_handle = purple_timeout_add(poll_interval, spotify_poll, poll_data);

    return TRUE;
}

    static gboolean
plugin_unload (PurplePlugin * plugin)
{
    spotify_free(spotify_handle);
    return TRUE;
}

/* For specific notes on the meanings of each of these members, consult the C Plugin Howto
 * on the website. */
static PurplePluginInfo info = {
    PURPLE_PLUGIN_MAGIC,
    PURPLE_MAJOR_VERSION,
    PURPLE_MINOR_VERSION,
    PURPLE_PLUGIN_STANDARD,
    NULL,
    0,
    NULL,
    PURPLE_PRIORITY_DEFAULT,

    PLUGIN_ID,
    "Spotify \"Now Listening\" Plugin!",
    DISPLAY_VERSION, /* This constant is defined in config.h, but you shouldn't use it for
                        your own plugins.  We use it here because it's our plugin. And we're lazy. */

    "Spotify \"Now Listening\" Plugin",
    "Spotify \"Now Listening\" Plugin",
    "Simon Pantzare <simon@spantz.org>", /* correct author */
    "http://spantz.org",


    plugin_load,
    plugin_unload,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,		/* func to initialize actions */
    NULL,
    NULL,
    NULL,
    NULL
};

    static void
init_plugin (PurplePlugin * plugin)
{
}

PURPLE_INIT_PLUGIN (spotify, init_plugin, info)
