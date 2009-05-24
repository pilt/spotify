#include <stdio.h>

#include "spotify_util.h"

int main(int argc, char **argv)
{
    spotify *s;
    if ((s = spotify_init()) == NULL) {
        puts("unable to initialize");
        return -1;
    }

    if (spotify_update_playing(s))
        printf("artist: %s\ntitle: %s\n", s->playing.artist, s->playing.title);
    else
        puts("unable to update the currently playing song");

    spotify_free(s);
    return 0;
}
