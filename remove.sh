#!/bin/bash
# remove.sh
EXECUTABLE=cssdups
CONFIG=csdexcl
BIN=/usr/local/bin
GLOBALCFG=/usr/local/share/"$EXECUTABLE"
LOCALCFG=~/.config/"$EXECUTABLE"
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
