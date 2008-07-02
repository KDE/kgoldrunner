#! /usr/bin/env bash
$EXTRACTRC */*.rc >> rc.cpp || exit 11
$XGETTEXT rc.cpp gamedata/game_*.txt src/*.cpp -o $podir/kgoldrunner.pot
rm -f rc.cpp
