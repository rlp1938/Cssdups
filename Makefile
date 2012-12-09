CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=cssdups.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cssdups
CONFIG=csdexcl

.PHONY: clean install remove

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o cssdups
	
install:
	./install.sh

remove:
	./remove.sh

