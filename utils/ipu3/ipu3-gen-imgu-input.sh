#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2022, Google Inc.
#
# Author: Umang Jain <umang.jain@ideasonboard.com>
#
# ipu3-gen-imgu-input.sh - Generate input file for IMGU streaming
#
# The scripts makes use of the following tools, which are expected to be
# found in $PATH:
#
# - raw2pnm (from nvt https://github.com/intel/nvt.git)
# - pnm2raw (from nvt https://github.com/intel/nvt.git)
# - ipu-pack (from https://git.libcamera.org/libcamera/libcamera.git/)
# - convert (from https://github.com/ImageMagick/ImageMagick/tree/main/utilities)

#default frame count
frame_count=10

validate_size() {
	local size=$1
	local width=$(echo $size | awk -F 'x' '{print $1}')
	local height=$(echo $size | awk -F 'x' '{print $2}')

	[[ "x${size}" == "x${width}x${height}" ]]
}

# Print usage message
usage() {
	echo "Usage: $(basename $1) --count <count> --size <size>"
	echo "Supported options:"
	echo "--count count     Generate input file containing <count> frames, default=10"
	echo "--size size       Frame size ('width' x 'height')"
	echo ""
}

# Parse command line arguments
while (( "$#" )) ; do
	case $1 in
	--count)
		frame_count=$2
		shift 2
		;;
	--size)
		in_size=$2
		if ! validate_size $in_size ; then
			echo "Invalid size '$in_size'"
			usage $0
			exit 1
		fi
		shift 2
		;;
	-*)
		echo "Unsupported option $1" >&2
		usage $0
		exit 1
		;;
	*)
		break
		;;
	esac
done

if [[ -z ${in_size+x} ]] ; then
	usage $0
	exit 1
fi

echo "Generating Input file frame-$in_size.raw"
for i in `seq 0 $(expr $frame_count - 1)`
do
	convert -background "#000000" -size $in_size -fill "#ffffff" -pointsize 120 -gravity center label:"Frame $i" frame-$i.ppm
	pnm2raw frame-$i.ppm frame-$i.raw
	ipu3-pack frame-$i.raw - >> frame-$in_size.raw

	rm frame-$i.ppm
	rm frame-$i.raw
done
