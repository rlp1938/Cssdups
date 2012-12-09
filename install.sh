#!/bin/bash
# install.sh
EXECUTABLE=cssdups
CONFIG=csdexcl
BIN=/usr/local/bin
GLOBALCFG=/usr/local/share/"$EXECUTABLE"
#
cp $EXECUTABLE $BIN
if [ ! -d $GLOBALCFG ];then
	mkdir $GLOBALCFG
	sync
fi
cp $CONFIG $GLOBALCFG

