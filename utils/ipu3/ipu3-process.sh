#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2018, Google Inc.
#
# Author: Laurent Pinchart <laurent.pinchart@ideasonboard.com>
#
# ipu3-process.sh - Process raw frames with the Intel IPU3
#
# The scripts makes use of the following tools, which are expected to be
# found in $PATH:
#
# - media-ctl (from v4l-utils git://linuxtv.org/v4l-utils.git)
# - raw2pnm (from nvt https://github.com/intel/nvt.git)
# - v4l2-ctl (from http://git.linuxtv.org/v4l-utils.git)

imgu_entity="ipu3-imgu 0"

# Locate the media device
find_media_device() {
	local mdev

	for mdev in /dev/media* ; do
		media-ctl -d $mdev -p | grep -q "^driver[ \t]*ipu3-imgu$" && break
		mdev=
	done

	if [[ -z $mdev ]] ; then
		echo "IPU3 media device not found." >&2
		exit 1
	fi

	echo $mdev
}

# Configure the pipeline
configure_pipeline() {
	local enable_3a=1
	local enable_out=1
	local enable_vf=1
	local enable_param=1
	local mode=0

	# Configure the links
	$mediactl -r
	$mediactl -l "\"$imgu_entity input\":0 -> \"$imgu_entity\":0[1]"
	$mediactl -l "\"$imgu_entity parameters\":0 -> \"$imgu_entity\":1[$enable_param]"
	$mediactl -l "\"$imgu_entity\":2 -> \"$imgu_entity output\":0[$enable_out]"
	$mediactl -l "\"$imgu_entity\":3 -> \"$imgu_entity viewfinder\":0[$enable_vf]"
	$mediactl -l "\"$imgu_entity\":4 -> \"$imgu_entity 3a stat\":0[$enable_3a]"

	# Select processing mode (0 for video, 1 for still image)
	v4l2-ctl -d $($mediactl -e "$imgu_entity") -c 0x009819c1=$mode

	# Set formats. The media bus code doesn't matter as it is ignored by the
	# driver. We should use the FIXED format, but media-ctl doesn't support
	# it.
	$mediactl -V "\"$imgu_entity\":0 [fmt:SBGGR10_1X10/$out_size crop:(0,0)/$in_size compose:(0,0)/$in_size]"
	$mediactl -V "\"$imgu_entity\":2 [fmt:SBGGR10_1X10/$out_size]"
	$mediactl -V "\"$imgu_entity\":3 [fmt:SBGGR10_1X10/$vf_size]"
	$mediactl -V "\"$imgu_entity\":4 [fmt:SBGGR10_1X10/$out_size]"
}

# Perform frame processing through the IMGU
process_frames() {
	configure_pipeline

	local stream_setting=" --stream-mmap $nbufs --stream-count=$frame_count"
	local out_width=$(echo $out_size | awk -F 'x' '{print $1}')
	local out_height=$(echo $out_size | awk -F 'x' '{print $2}')
	local vf_width=$(echo $vf_size | awk -F 'x' '{print $1}')
	local vf_height=$(echo $vf_size | awk -F 'x' '{print $2}')
	local in_width=$(echo $in_size | awk -F 'x' '{print $1}')
	local in_height=$(echo $in_size | awk -F 'x' '{print $2}')

	# Save the main and viewfinder outputs to disk, capture and drop 3A
	# statistics. Sleep 500ms between each execution of v4l2-ctl to keep the
	# stdout messages readable.
	v4l2-ctl -d$($mediactl -e "$imgu_entity output") \
		--set-fmt-video=pixelformat=$IMGU_OUT_PIXELFORMAT,width=$out_width,height=$out_height \
		$stream_setting --stream-to=$output_dir/frames-out.bin &
	sleep 0.5
	v4l2-ctl -d$($mediactl -e "$imgu_entity viewfinder") \
		--set-fmt-video=pixelformat=$IMGU_OUT_PIXELFORMAT,width=$vf_width,height=$vf_height \
		$stream_setting --stream-to=$output_dir/frames-vf.bin &
	sleep 0.5
	v4l2-ctl -d $($mediactl -e "$imgu_entity 3a stat") $stream_setting &
	sleep 0.5

	# Feed the IMGU input.

	# To stream out $frame_count on output and vf nodes, input and parameters nodes need to atleast
	# stream ($frame_count + $nbufs) frames.
	local in_stream_count=$(expr $nbufs + $frame_count)
	v4l2-ctl -d $($mediactl -e "$imgu_entity input") \
		--set-fmt-video-out=width=$in_width\,height=$in_height\,pixelformat=$IMGU_IN_PIXELFORMAT \
		--stream-out-mmap $nbufs --stream-from=$in_file --stream-count=$in_stream_count \
		--stream-loop --stream-poll &

	# The input node would not stream until parameters start streaming.
	v4l2-ctl -d$($mediactl -e "$imgu_entity parameters") --stream-out-mmap 1  \
		--stream-count=$in_stream_count --stream-poll
}

# Convert captured files to ppm
convert_files() {
	local index=$1
	local type=$2
	local size=$3
	local format=$4

	local width=$(echo $size | awk -F 'x' '{print $1}')
	local height=$(echo $size | awk -F 'x' '{print $2}')
	local padded_width=$(expr $(expr $width + 63) / 64 \* 64)

	local bytes_offset=$(expr $index \* $(expr $padded_width \* $height \* 12) / 8)
	echo $bytes_offset
	raw2pnm -x$padded_width -y$height -f$format -b $bytes_offset \
		$output_dir/frames-$type.bin \
		$output_dir/frame-$type-$index.ppm
}

run_test() {
	IMGU_IN_PIXELFORMAT=ip3G
	IMGU_OUT_PIXELFORMAT=NV12
	IMGU_VF_PIXELFORMAT=NV12

	echo "==== Test ===="
	echo "input:  $in_file"
	echo "output: $IMGU_OUT_PIXELFORMAT/$out_size"
	echo "vf:     $IMGU_VF_PIXELFORMAT/$vf_size"

	process_frames

	for i in `seq -f '%06.0f' 0 $(($frame_count - 1))`; do
		convert_files $i out $out_size $IMGU_OUT_PIXELFORMAT
		convert_files $i vf $vf_size $IMGU_VF_PIXELFORMAT
	done
}

validate_size() {
	local size=$1
	local width=$(echo $size | awk -F 'x' '{print $1}')
	local height=$(echo $size | awk -F 'x' '{print $2}')

	[[ "x${size}" == "x${width}x${height}" ]]
}

# Print usage message
usage() {
	echo "Usage: $(basename $1) [options] <input-file>"
	echo "Supported options:"
	echo "--out size        output frame size (defaults to input size)"
	echo "--vf size         viewfinder frame size (defaults to input size)"
	echo ""
	echo "Where the input file name and size are"
	echo ""
	echo "input-file = prefix '-' width 'x' height '.' extension"
	echo "size = width 'x' height"
}

# Parse command line arguments
while (( "$#" )) ; do
	case $1 in
	--out)
		out_size=$2
		if ! validate_size $out_size ; then
			echo "Invalid size '$out_size'"
			usage $0
			exit 1
		fi
		shift 2
		;;
	--vf)
		vf_size=$2
		if ! validate_size $vf_size ; then
			echo "Invalid size '$vf_size'"
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

if [ $# != 1 ] ; then
	usage $0
	exit 1
fi

in_file=$1

# Parse the size from the input file name and perform minimal sanity
# checks.
in_size=$(echo $in_file | sed 's/.*-\([0-9]*\)x\([0-9]*\)\.[a-z0-9]*$/\1x\2/')
validate_size $in_size
if [[ $? != 0 ]] ; then
	echo "Invalid input file name $in_file" >&2
	usage $0
	exit 1
fi

out_size=${out_size:-$in_size}
vf_size=${vf_size:-$in_size}

mdev=$(find_media_device) || exit
mediactl="media-ctl -d $mdev"
echo "Using device $mdev"

output_dir="/tmp"
frame_count=10
nbufs=4
run_test
