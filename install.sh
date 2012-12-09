#!/bin/bash
# install.sh
EXECUTABLE=cssdups
CONFIG=csdexcl
BIN=/usr/local/bin
GLOBALCFG=/usr/local/share/"$EXECUTABLE"
MANDIR=/usr/local/share/man
#
cp $EXECUTABLE $BIN
if [ ! -d $GLOBALCFG ];then
	mkdir $GLOBALCFG
	sync
fi
cp $CONFIG $GLOBALCFG
if [ ! -d "$MANDIR"/man1 ];then
	mkdir "$MANDIR"/man1
fi
cp "$EXECUTABLE".1 "$MANDIR"/man1
sync
# this will work ok on Ubuntu and most likely on other Debian based
# systems. I don't know about Fedora etc.
if [ -f /usr/bin/mandb ];then
	/usr/bin/mandb -p	# bring the man page database up to date.
fi

