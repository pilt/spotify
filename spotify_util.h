#ifndef SPOTIFY_UTIL_H
#define SPOTIFY_UTIL_H

#include <glib.h>

struct spotify_playing;
struct spotify;

typedef struct spotify_playing {
    gchar *artist;
    gchar *title;
} spotify_playing;

typedef struct spotify {
    spotify_playing playing;
} spotify;

extern spotify * spotify_init(void); 
extern void spotify_free(spotify *); 
extern gboolean spotify_update_playing(spotify *);

#endif
