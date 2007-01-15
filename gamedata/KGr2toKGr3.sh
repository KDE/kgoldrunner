#!/bin/bash
# This script converts game and level data from KGoldrunner 2 to KGoldrunner 3
# format.  It must run in the directory where "games.dat" is located, with the
# level-data files untarred into the "levels" sub-directory.  It is provided for
# convenience only and must NOT be installed.  The "game_*.txt" files it creates
# do get released and installed and the "games.dat" and "levels.tar" get
# dropped from SVN and future releases.
#
# This script will be needed again whenever someone has created a new
# KGoldrunner game using the game editor, because the editor continues
# to use KGoldrunner 2 format.
#
# Pass 1: in games.dat, convert '\n' character sequences to actual newlines,
# then copy the game-data and description to the new game-file's header.
sed -e 's/\\n/\\\\n/g' games.dat | while read line
do
    case "$line" in
	0*|1*)	echo "Pass 1: $line"
		set -- $line				# Re-parse game-data.
		prefix="$3"				# Get fixed ID of game.
		game="game_$prefix.txt"			# Generate file name.
		echo "// G$1 $2 $3" >"$game"		# Write game-data line.
		shift; shift; shift
		echo " i18n(\"$*\");" >>$game		# Append name of game.
		;;
	*)	echo " i18n(\"$line\");" >>"$game"	# Append game-desc line.
		;;
    esac
done

# Pass 2: append header and data for each level to the new game-files.
while read n rules prefix name
do
    case $n in
	0*|1*)	echo "Pass 2: $n $rules $prefix $name"
		game="game_$prefix.txt"
		i=1
		while [ "$i" -le "$n" ]			# Loop thru all levels.
		do
		    lev="$i"				# Get the level-number.
		    if [ "$i" -lt 100 ]			# Add leading zeroes.
		    then
			lev="0""$lev"
		    fi
		    if [ "$i" -lt 10 ]
		    then
			lev="0""$lev"
		    fi

		    echo "// L$lev" >>"$game"		# Append level-header.
		    lev="levels/$prefix$lev.grl"	# Get level's file path.
		    case "$prefix" in
		    plws|tute*)
			# Normal or tutorial games ...
			# Append level-data with a leading comment, level-name
			# with 'i18n("");' and hint (if any) with 'i18n("");'.
			sed -e '
				1s:^://  :
				2,3s/^/ i18n("/
				2s/$/");/
				3,$s/$/\\n"/
				4,$s/^/ "/
				$s/\\n"/");/
			' "$lev" >>"$game"
			;;
		    *)
			# Championship games ...
			# Append level-data with a leading comment, level-name
			# with 'i18n("");' and hint with 'NOTi18n("");'.
			sed -e '
				1s:^://  :
				2s/^/ i18n("/
				2s/$/");/
				3s/^/ NOTi18n("/
				3,$s/$/\\n"/
				4,$s/^/ "/
				$s/\\n"/");/
			' "$lev" >>"$game"
			;;
		    esac

		    let "i = $i + 1"
		done
		;;
	*)	;;					# Skip game-desc lines.
    esac
done <games.dat
