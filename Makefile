CFLAGS ?= -g
PKGS=glib-2.0 x11
CFLAGS += -Wall -Wextra -Wno-long-long -pedantic -std=c99 \
		  $(shell pkg-config --cflags $(PKGS)) \
		  $(shell pkg-config --libs $(PKGS))
EXEC=spotify_util_test spotify_playing

all: $(EXEC)
$(EXEC): spotify_util.o 

clean:
	rm -f *.o core $(EXEC)

.PHONY: all test
