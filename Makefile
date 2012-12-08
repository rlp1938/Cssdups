CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=cssdups.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=cssdups

.PHONY: clean install remove

all: $(SOURCES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *o cssdups
	
install:
	cp cssdups /usr/local/bin/

remove:
	if [-f /usr/local/bin/cssdups ];then 
		rm /usr/local/bin/cssdups
	fi
	if [-f /usr/local/share/cssdups/csdexcl ];then 
		rm /usr/local/bin/cssdups/csdexcl
	fi
	
