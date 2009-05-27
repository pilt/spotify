CFLAGS ?= -g
# FIXME: Only use libpurple when it's needed.
PKGS=glib-2.0 x11 
CFLAGS += -Wall -Wextra -Wno-long-long -pedantic -std=c99 -fPIC \
		  $(shell pkg-config --cflags --libs $(PKGS))
PIDGIN_FLAGS=$(shell pkg-config --cflags --libs purple)
EXEC=spotify_util_test spotify_playing
PIDGIN_PLUGIN=spotify_playing.so

all: $(EXEC) $(PIDGIN_PLUGIN)
$(EXEC): spotify_util.o 

clean:
	rm -f *.o *.so core $(EXEC)

.PHONY: all test

$(PIDGIN_PLUGIN): pidgin_plugin.c spotify_util.o
	cc -shared $^ $(CFLAGS) $(PIDGIN_FLAGS) -o $@

