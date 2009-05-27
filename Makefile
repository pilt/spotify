CFLAGS ?= -g
# FIXME: Only use libpurple when it's needed.
PKGS=glib-2.0 x11 
CFLAGS += -Wall -Wextra -Wno-long-long -pedantic -std=c99 \
		  $(shell pkg-config --cflags $(PKGS)) \
		  $(shell pkg-config --libs $(PKGS))
PIDGIN_FLAGS=$(shell pkg-config --cflags --libs purple)
EXEC=spotify_util_test spotify_playing

all: $(EXEC) spotify_playing.so
$(EXEC): spotify_util.o 

clean:
	rm -f *.o *.so core $(EXEC)

.PHONY: all test

spotify_playing.so: pidgin_plugin.c
	cc -shared -fPIC $^ $(CFLAGS) $(PIDGIN_FLAGS) -o $@

