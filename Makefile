CFLAGS = -Wall -O2 -std=c99
GLIB_CFLAGS = $(shell pkg-config glib-2.0 --cflags)
GLIB_LDFLAGS = $(shell pkg-config glib-2.0 --libs)
YAML_LDFLAGS = $(shell pkg-config --libs yaml-0.1)

OBJECTS = get.o parse.o set.o write.o iconfig2yaml.o

%.o: %.c
	$(CC) $(CFLAGS) $(GLIB_CFLAGS) -c $<

iconfig2yaml: $(OBJECTS)
	$(CC) $(CFLAGS) $(GLIB_LDFLAGS) $(YAML_LDFLAGS) $(OBJECTS) -o $@

all: iconfig2yaml

clean:
	rm -rf *.o iconfig2yaml || true

.default: all
