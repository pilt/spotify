#include <stdio.h>
#include <glib.h>

#include "spotify_util.h"

int main(int argc, char **argv)
{
    spotify *s;
    if ((s = spotify_init()) == NULL)
        return -1;

    if (spotify_update_playing(s) == FALSE) {
        spotify_free(s);
        return -1;
    }

    printf("%s – %s\n", s->playing.artist, s->playing.title);

    spotify_free(s);
    return 0;
}
