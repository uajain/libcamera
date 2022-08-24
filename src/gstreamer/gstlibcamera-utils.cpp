/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2020, Collabora Ltd.
 *     Author: Nicolas Dufresne <nicolas.dufresne@collabora.com>
 *
 * gstlibcamera-utils.c - GStreamer libcamera Utility Function
 */

#include "gstlibcamera-utils.h"

#include <libcamera/formats.h>

using namespace libcamera;

static struct {
	GstVideoFormat gst_format;
	PixelFormat format;
} format_map[] = {
	/* Compressed */
	{ GST_VIDEO_FORMAT_ENCODED, formats::MJPEG },

	/* RGB */
	{ GST_VIDEO_FORMAT_RGB, formats::BGR888 },
	{ GST_VIDEO_FORMAT_BGR, formats::RGB888 },
	{ GST_VIDEO_FORMAT_ARGB, formats::BGRA8888 },

	/* YUV Semiplanar */
	{ GST_VIDEO_FORMAT_NV12, formats::NV12 },
	{ GST_VIDEO_FORMAT_NV21, formats::NV21 },
	{ GST_VIDEO_FORMAT_NV16, formats::NV16 },
	{ GST_VIDEO_FORMAT_NV61, formats::NV61 },
	{ GST_VIDEO_FORMAT_NV24, formats::NV24 },

	/* YUV Planar */
	{ GST_VIDEO_FORMAT_I420, formats::YUV420 },
	{ GST_VIDEO_FORMAT_YV12, formats::YVU420 },
	{ GST_VIDEO_FORMAT_Y42B, formats::YUV422 },

	/* YUV Packed */
	{ GST_VIDEO_FORMAT_UYVY, formats::UYVY },
	{ GST_VIDEO_FORMAT_VYUY, formats::VYUY },
	{ GST_VIDEO_FORMAT_YUY2, formats::YUYV },
	{ GST_VIDEO_FORMAT_YVYU, formats::YVYU },

	/* \todo NV42 is used in libcamera but is not mapped in GStreamer yet. */
};

static GstVideoColorimetry
colorimetry_from_colorspace(const ColorSpace &colorSpace)
{
	GstVideoColorimetry colorimetry;

	switch (colorSpace.primaries) {
	case ColorSpace::Primaries::Raw:
		colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_UNKNOWN;
		break;
	case ColorSpace::Primaries::Smpte170m:
		colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_SMPTE170M;
		break;
	case ColorSpace::Primaries::Rec709:
		colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_BT709;
		break;
	case ColorSpace::Primaries::Rec2020:
		colorimetry.primaries = GST_VIDEO_COLOR_PRIMARIES_BT2020;
		break;
	}

	switch (colorSpace.transferFunction) {
	case ColorSpace::TransferFunction::Linear:
		colorimetry.transfer = GST_VIDEO_TRANSFER_GAMMA10;
		break;
	case ColorSpace::TransferFunction::Srgb:
		colorimetry.transfer = GST_VIDEO_TRANSFER_SRGB;
		break;
	case ColorSpace::TransferFunction::Rec709:
		colorimetry.transfer = GST_VIDEO_TRANSFER_BT709;
		break;
	}

	switch (colorSpace.ycbcrEncoding) {
	case ColorSpace::YcbcrEncoding::None:
		colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_RGB;
		break;
	case ColorSpace::YcbcrEncoding::Rec601:
		colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_BT601;
		break;
	case ColorSpace::YcbcrEncoding::Rec709:
		colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_BT709;
		break;
	case ColorSpace::YcbcrEncoding::Rec2020:
		colorimetry.matrix = GST_VIDEO_COLOR_MATRIX_BT2020;
		break;
	}

	switch (colorSpace.range) {
	case ColorSpace::Range::Full:
		colorimetry.range = GST_VIDEO_COLOR_RANGE_0_255;
		break;
	case ColorSpace::Range::Limited:
		colorimetry.range = GST_VIDEO_COLOR_RANGE_16_235;
		break;
	}

	return colorimetry;
}

static std::optional<ColorSpace>
colorspace_from_colorimetry(const GstVideoColorimetry &colorimetry)
{
	std::optional<ColorSpace> colorspace;

	switch (colorimetry.primaries) {
	case GST_VIDEO_COLOR_PRIMARIES_UNKNOWN:
		/* Unknown primaries map to raw colorspace in gstreamer */
		return ColorSpace::Raw;
	case GST_VIDEO_COLOR_PRIMARIES_SMPTE170M:
		colorspace->primaries = ColorSpace::Primaries::Smpte170m;
		break;
	case GST_VIDEO_COLOR_PRIMARIES_BT709:
		colorspace->primaries = ColorSpace::Primaries::Rec709;
		break;
	case GST_VIDEO_COLOR_PRIMARIES_BT2020:
		colorspace->primaries = ColorSpace::Primaries::Rec2020;
		break;
	default:
		GST_WARNING("Colorimetry primaries %d not mapped in gstlibcamera",
			    colorimetry.primaries);
		return std::nullopt;
	}

	switch (colorimetry.transfer) {
	/* Transfer function mappings inspired from v4l2src plugin */
	case GST_VIDEO_TRANSFER_GAMMA18:
	case GST_VIDEO_TRANSFER_GAMMA20:
	case GST_VIDEO_TRANSFER_GAMMA22:
	case GST_VIDEO_TRANSFER_GAMMA28:
		GST_WARNING("GAMMA 18, 20, 22, 28 transfer functions not supported");
	/* fallthrough */
	case GST_VIDEO_TRANSFER_GAMMA10:
		colorspace->transferFunction = ColorSpace::TransferFunction::Linear;
		break;
	case GST_VIDEO_TRANSFER_SRGB:
		colorspace->transferFunction = ColorSpace::TransferFunction::Srgb;
		break;
	case GST_VIDEO_TRANSFER_BT601:
	case GST_VIDEO_TRANSFER_BT2020_12:
	case GST_VIDEO_TRANSFER_BT2020_10:
	case GST_VIDEO_TRANSFER_BT709:
		colorspace->transferFunction = ColorSpace::TransferFunction::Rec709;
		break;
	default:
		GST_WARNING("Colorimetry transfer function %d not mapped in gstlibcamera",
			    colorimetry.transfer);
		return std::nullopt;
	}

	switch (colorimetry.matrix) {
	case GST_VIDEO_COLOR_MATRIX_RGB:
		colorspace->ycbcrEncoding = ColorSpace::YcbcrEncoding::None;
		break;
	/* FCC is about the same as BT601 with less digit */
	case GST_VIDEO_COLOR_MATRIX_FCC:
	case GST_VIDEO_COLOR_MATRIX_BT601:
		colorspace->ycbcrEncoding = ColorSpace::YcbcrEncoding::Rec601;
		break;
	case GST_VIDEO_COLOR_MATRIX_BT709:
		colorspace->ycbcrEncoding = ColorSpace::YcbcrEncoding::Rec709;
		break;
	case GST_VIDEO_COLOR_MATRIX_BT2020:
		colorspace->ycbcrEncoding = ColorSpace::YcbcrEncoding::Rec2020;
		break;
	default:
		GST_WARNING("Colorimetry matrix %d not mapped in gstlibcamera",
			    colorimetry.matrix);
		return std::nullopt;
	}

	switch (colorimetry.range) {
	case GST_VIDEO_COLOR_RANGE_0_255:
		colorspace->range = ColorSpace::Range::Full;
		break;
	case GST_VIDEO_COLOR_RANGE_16_235:
		colorspace->range = ColorSpace::Range::Limited;
		break;
	default:
		GST_WARNING("Colorimetry range %d not mapped in gstlibcamera",
			    colorimetry.range);
		return std::nullopt;
	}

	return colorspace;
}

static GstVideoFormat
pixel_format_to_gst_format(const PixelFormat &format)
{
	for (const auto &item : format_map) {
		if (item.format == format)
			return item.gst_format;
	}
	return GST_VIDEO_FORMAT_UNKNOWN;
}

static PixelFormat
gst_format_to_pixel_format(GstVideoFormat gst_format)
{
	if (gst_format == GST_VIDEO_FORMAT_ENCODED)
		return PixelFormat{};

	for (const auto &item : format_map)
		if (item.gst_format == gst_format)
			return item.format;
	return PixelFormat{};
}

static GstStructure *
bare_structure_from_format(const PixelFormat &format)
{
	GstVideoFormat gst_format = pixel_format_to_gst_format(format);

	if (gst_format == GST_VIDEO_FORMAT_UNKNOWN)
		return nullptr;

	if (gst_format != GST_VIDEO_FORMAT_ENCODED)
		return gst_structure_new("video/x-raw", "format", G_TYPE_STRING,
					 gst_video_format_to_string(gst_format), nullptr);

	switch (format) {
	case formats::MJPEG:
		return gst_structure_new_empty("image/jpeg");
	default:
		return nullptr;
	}
}

GstCaps *
gst_libcamera_stream_formats_to_caps(const StreamFormats &formats)
{
	GstCaps *caps = gst_caps_new_empty();

	for (PixelFormat pixelformat : formats.pixelformats()) {
		g_autoptr(GstStructure) bare_s = bare_structure_from_format(pixelformat);

		if (!bare_s) {
			GST_WARNING("Unsupported DRM format %" GST_FOURCC_FORMAT,
				    GST_FOURCC_ARGS(pixelformat));
			continue;
		}

		for (const Size &size : formats.sizes(pixelformat)) {
			GstStructure *s = gst_structure_copy(bare_s);
			gst_structure_set(s,
					  "width", G_TYPE_INT, size.width,
					  "height", G_TYPE_INT, size.height,
					  nullptr);
			gst_caps_append_structure(caps, s);
		}

		const SizeRange &range = formats.range(pixelformat);
		if (range.hStep && range.vStep) {
			GstStructure *s = gst_structure_copy(bare_s);
			GValue val = G_VALUE_INIT;

			g_value_init(&val, GST_TYPE_INT_RANGE);
			gst_value_set_int_range_step(&val, range.min.width, range.max.width, range.hStep);
			gst_structure_set_value(s, "width", &val);
			gst_value_set_int_range_step(&val, range.min.height, range.max.height, range.vStep);
			gst_structure_set_value(s, "height", &val);
			g_value_unset(&val);

			gst_caps_append_structure(caps, s);
		}
	}

	return caps;
}

GstCaps *
gst_libcamera_stream_configuration_to_caps(const StreamConfiguration &stream_cfg)
{
	GstCaps *caps = gst_caps_new_empty();
	GstStructure *s = bare_structure_from_format(stream_cfg.pixelFormat);

	gst_structure_set(s,
			  "width", G_TYPE_INT, stream_cfg.size.width,
			  "height", G_TYPE_INT, stream_cfg.size.height,
			  nullptr);

	if (stream_cfg.colorSpace) {
		GstVideoColorimetry colorimetry = colorimetry_from_colorspace(stream_cfg.colorSpace.value());
		gchar *colorimetry_str = gst_video_colorimetry_to_string(&colorimetry);

		if (colorimetry_str)
			gst_structure_set(s, "colorimetry", G_TYPE_STRING, colorimetry_str, nullptr);
		else
			g_error("Got invalid colorimetry from ColorSpace: %s",
				ColorSpace::toString(stream_cfg.colorSpace).c_str());
	}

	gst_caps_append_structure(caps, s);

	return caps;
}

void
gst_libcamera_configure_stream_from_caps(StreamConfiguration &stream_cfg,
					 GstCaps *caps)
{
	GstVideoFormat gst_format = pixel_format_to_gst_format(stream_cfg.pixelFormat);
	guint i;
	gint best_fixed = -1, best_in_range = -1;
	GstStructure *s;

	/*
	 * These are delta weight computed from:
	 *   ABS(width - stream_cfg.size.width) * ABS(height - stream_cfg.size.height)
	 */
	guint best_fixed_delta = G_MAXUINT;
	guint best_in_range_delta = G_MAXUINT;

	/* First fixate the caps using default configuration value. */
	g_assert(gst_caps_is_writable(caps));

	/* Lookup the structure for a close match to the stream_cfg.size */
	for (i = 0; i < gst_caps_get_size(caps); i++) {
		s = gst_caps_get_structure(caps, i);
		gint width, height;
		guint delta;

		if (gst_structure_has_field_typed(s, "width", G_TYPE_INT) &&
		    gst_structure_has_field_typed(s, "height", G_TYPE_INT)) {
			gst_structure_get_int(s, "width", &width);
			gst_structure_get_int(s, "height", &height);

			delta = ABS(width - (gint)stream_cfg.size.width) * ABS(height - (gint)stream_cfg.size.height);

			if (delta < best_fixed_delta) {
				best_fixed_delta = delta;
				best_fixed = i;
			}
		} else {
			gst_structure_fixate_field_nearest_int(s, "width", stream_cfg.size.width);
			gst_structure_fixate_field_nearest_int(s, "height", stream_cfg.size.height);
			gst_structure_get_int(s, "width", &width);
			gst_structure_get_int(s, "height", &height);

			delta = ABS(width - (gint)stream_cfg.size.width) * ABS(height - (gint)stream_cfg.size.height);

			if (delta < best_in_range_delta) {
				best_in_range_delta = delta;
				best_in_range = i;
			}
		}
	}

	/* Prefer reliable fixed value over ranges */
	if (best_fixed >= 0)
		s = gst_caps_get_structure(caps, best_fixed);
	else
		s = gst_caps_get_structure(caps, best_in_range);

	if (gst_structure_has_name(s, "video/x-raw")) {
		const gchar *format = gst_video_format_to_string(gst_format);
		gst_structure_fixate_field_string(s, "format", format);
	}

	/* Then configure the stream with the result. */
	if (gst_structure_has_name(s, "video/x-raw")) {
		const gchar *format = gst_structure_get_string(s, "format");
		gst_format = gst_video_format_from_string(format);
		stream_cfg.pixelFormat = gst_format_to_pixel_format(gst_format);
	} else if (gst_structure_has_name(s, "image/jpeg")) {
		stream_cfg.pixelFormat = formats::MJPEG;
	} else {
		g_critical("Unsupported media type: %s", gst_structure_get_name(s));
	}

	gint width, height;
	gst_structure_get_int(s, "width", &width);
	gst_structure_get_int(s, "height", &height);
	stream_cfg.size.width = width;
	stream_cfg.size.height = height;

	/* Configure colorimetry */
	if (gst_structure_has_field(s, "colorimetry")) {
		const gchar *colorimetry_str = gst_structure_get_string(s, "colorimetry");
		GstVideoColorimetry colorimetry;

		if (!gst_video_colorimetry_from_string(&colorimetry, colorimetry_str))
			g_critical("Invalid colorimetry %s", colorimetry_str);

		stream_cfg.colorSpace = colorspace_from_colorimetry(colorimetry);
	}
}

#if !GST_CHECK_VERSION(1, 17, 1)
gboolean
gst_task_resume(GstTask *task)
{
	/* We only want to resume the task if it's paused. */
	GLibLocker lock(GST_OBJECT(task));
	if (GST_TASK_STATE(task) != GST_TASK_PAUSED)
		return FALSE;

	GST_TASK_STATE(task) = GST_TASK_STARTED;
	GST_TASK_SIGNAL(task);
	return TRUE;
}
#endif

G_LOCK_DEFINE_STATIC(cm_singleton_lock);
static std::weak_ptr<CameraManager> cm_singleton_ptr;

std::shared_ptr<CameraManager>
gst_libcamera_get_camera_manager(int &ret)
{
	std::shared_ptr<CameraManager> cm;

	G_LOCK(cm_singleton_lock);

	cm = cm_singleton_ptr.lock();
	if (!cm) {
		cm = std::make_shared<CameraManager>();
		cm_singleton_ptr = cm;
		ret = cm->start();
	} else {
		ret = 0;
	}

	G_UNLOCK(cm_singleton_lock);

	return cm;
}
