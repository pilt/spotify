/*
 * Spotify "Now Listening" Plugin
 *
 * Copyright (C) 2009 Simon Pantzare
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

#include "spotify_util.h"

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
    purple_timeout_remove(spotify_handle);
    spotify_free(spot);
    purple_util_set_current_song (NULL, NULL, NULL);
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
    "Spotify \"Now Listening\" Plugin",
    "2.5.5",

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
