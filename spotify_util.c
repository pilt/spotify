/*
Copyright (C) 2009 Simon Pantzare <simon@spantz.org>

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include <limits.h>
#include "spotify_util.h"
#include <stdio.h>

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
        puts("just started, find pid");
        pid_changed = TRUE;
        if (_pid(res) == FALSE)
            return FALSE;
    }
    else {
        puts("find new pid");
        pid_t last_pid = res->pid;

        if (_pid(res) == FALSE)
            return FALSE;

        if (last_pid != res->pid) 
            pid_changed = TRUE;
    }

    if (pid_changed || res->window == NULL) {
        puts("just started OR pid changed, find window");
        if (_window(res) == FALSE)
            return FALSE;
    }
    
    if (got_title == FALSE) {
        puts("fetch title");
        if (_win_title(res) == FALSE)
            return FALSE;
    }

    if (_artist_title(res) == FALSE) {
        return FALSE;
    }
    puts("artist / title found");

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

        if(XQueryTree(display, w, &w_root, &w_parent, &w_child, 
                    &n_children) != 0) {
            for (size_t i = 0; i < n_children && res->window == NULL; ++i) 
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
