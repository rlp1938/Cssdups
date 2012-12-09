#!/bin/bash
# remove.sh
EXECUTABLE=cssdups
CONFIG=csdexcl
BIN=/usr/local/bin
GLOBALCFG=/usr/local/share/"$EXECUTABLE"
LOCALCFG=~/.config/"$EXECUTABLE"
MANDIR=/usr/local/share/man
#
if [ -f "$BIN"/"$EXECUTABLE" ];then 
	rm "$BIN"/"$EXECUTABLE"
fi
if [ -f "$GLOBALCFG"/"$CONFIG" ];then 
	rm "$GLOBALCFG"/"$CONFIG"
fi
if [ -f "$LOCALCFG"/"$CONFIG" ]; then
	rm "$LOCALCFG"/"$CONFIG"
fi
# man page
if [ -f "$MANDIR"/man1/"$EXECUTABLE".1 ]; then
	rm "$MANDIR"/man1/"$EXECUTABLE".1
fi
sync
# this will work ok on Ubuntu and most likely on other Debian based
# systems. I don't know about Fedora etc.
if [ -f /usr/bin/mandb ];then
	/usr/bin/mandb -p	# bring the man page database up to date.
fi
