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
#include <sys/types.h> /* for pid_t */
#include <limits.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "spotify_util.h"

static inline gboolean _spotify_pid(pid_t *);
static inline gboolean _parse_win_title(gchar *, gchar **, gchar **);
static inline gboolean _win_title(pid_t pid, gchar **title);
static void _search(Display *display, Window w, pid_t pid, gchar **win_title);
static inline void _free_playing(spotify *);

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
    
    tmp_pid = g_ascii_strtod(cmd_out, NULL);
    if (tmp_pid == 0)
        return FALSE; /* `g_ascii_strtod' returns 0 on failure. */

    g_free(cmd); 
    g_free(cmd_out);

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
    return res;
}

void spotify_free(spotify *res)
{
    _free_playing(res);
    free(res);
}

static inline void _free_playing(spotify *res)
{
    if (res->playing.artist != NULL)
        g_free(res->playing.artist);
    if (res->playing.title != NULL)
        g_free(res->playing.title);
}

extern gboolean spotify_update_playing(spotify *res)
{
    pid_t pid;
    if (_spotify_pid(&pid) == FALSE) {
        return FALSE;
    }

    gchar *win_title;
    if (_win_title(pid, &win_title) == FALSE) {
        return FALSE;
    }
    
    _free_playing(res);
    if (_parse_win_title(win_title, &res->playing.artist, 
                &res->playing.title) == FALSE) {
        g_free(win_title);
        return FALSE;
    }

    g_free(win_title);
    return TRUE;
}

static inline gboolean 
_parse_win_title(gchar *win_title, gchar **artist, gchar **title)
{
    GRegex *regex = NULL;
    GMatchInfo *match_info = NULL;

    /* XXX: The result will be unexpected when there is a `–' sign in the
     * artist name.
     */
    regex = g_regex_new("Spotify - (?P<ARTIST>.+) – (?P<TITLE>.+)", 0, 0, NULL);
    g_regex_match(regex, win_title, 0, &match_info);
    *artist = g_match_info_fetch_named(match_info, "ARTIST");
    *title = g_match_info_fetch_named(match_info, "TITLE");

    g_match_info_free(match_info);
    g_regex_unref(regex);

    if (*artist == NULL || *title == NULL)
        return FALSE;
    return TRUE;
}

static inline gboolean _win_title(pid_t pid, gchar **title)
{
    Display *display = XOpenDisplay(0);
    Window root = XDefaultRootWindow(display);
    gchar *tmp_title = NULL;

    _search(display, root, pid, &tmp_title);
    XCloseDisplay(display);

    if (tmp_title == NULL)
        return FALSE;
    *title = tmp_title;
    return TRUE;
}

static void _search(Display *display, Window w, pid_t pid, gchar **win_title)
{
    Atom           type;
    int            format;
    unsigned long  n_items;
    unsigned long  bytes_after;
    unsigned char *prop_pid = 0;
    unsigned char *prop_title = 0;

    if(XGetWindowProperty(display, w, 
                XInternAtom(display, "_NET_WM_PID", True),
                0, 1, False, 
                XA_CARDINAL, &type, &format, &n_items, &bytes_after, &prop_pid)
            == Success) {
        if(prop_pid != 0) {
            if(pid == (pid_t)*((unsigned long *)prop_pid)) {
                Atom utf8_string = XInternAtom(display, "UTF8_STRING", 0);
                if (XGetWindowProperty(display, w,
                            XInternAtom(display, "_NET_WM_NAME", 0),
                            0, LONG_MAX, False,
                            utf8_string, &type, &format, &n_items, &bytes_after,
                            &prop_title)
                        == Success) {
                    if (type == utf8_string && format == 8 && n_items != 0) {
                        *win_title = g_strdup((char *)prop_title);
                    }

                    XFree(prop_title);
                }
            }

            XFree(prop_pid);
        }
    }

    if (*win_title == NULL) {
        Window    w_root;
        Window    w_parent;
        Window   *w_child;
        unsigned  n_children;

        if(XQueryTree(display, w, &w_root, &w_parent, &w_child, 
                    &n_children) != 0) {
            for (size_t i = 0; i < n_children && *win_title == NULL; ++i) 
                _search(display, w_child[i], pid, win_title);

            if (w_child != NULL)
                XFree(w_child);
        }
    }
}
