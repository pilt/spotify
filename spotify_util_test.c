#include <stdio.h>
#include <unistd.h>

#include "spotify_util.h"

spotify *s;

void print_playing(spotify *s)
{
    if (spotify_update_playing(s))
        printf("artist: %s\ntitle: %s\n", s->playing.artist, s->playing.title);
    else
        puts("unable to update the currently playing song");
}

int main(int argc, char **argv)
{
    if ((s = spotify_init()) == NULL) {
        puts("unable to initialize\n");
        return -1;
    }

    for (;;) {
        print_playing(s);
        sleep(1);
    }

    spotify_free(s);
    return 0;
}
