#! /usr/bin/env bash
$EXTRACTRC *.rc *.ui *.kcfg > rc.cpp
$XGETTEXT gamedata/game_*.txt src/*.cpp -o $podir/kgoldrunner.pot
