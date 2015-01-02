#!/bin/bash

#
#
#

#
# temporary directory
#

mkdir TMP

#
# decompose a video into individual frames
#

ffmpeg -i $1 TMP/%06d.jpg

#
# transform frames to ASCII art
#

COUNTER=0

for frame in TMP/*.jpg
do
	#
	printf "$frame\n"

	#
	./n3lar $frame $frame.ascii.png

	#
	let COUNTER=COUNTER+1
done

#
# stitch the individual frames to a video
#

ffmpeg -i TMP/%06d.jpg.ascii.png $2

#
# clean temporary files
#

rm TMP/*.jpg
rm TMP/*.ascii.png

rmdir TMP