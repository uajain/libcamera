/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2022, Ideas on Board Oy.
 *
 * stream_colorspace.cpp - Stream colorspace tests
 */

#include <iostream>

#include <libcamera/camera.h>
#include <libcamera/formats.h>
#include <libcamera/stream.h>

#include "test.h"

using namespace libcamera;
using namespace std;

class TestCameraConfiguration : public CameraConfiguration
{
public:
	TestCameraConfiguration()
		: CameraConfiguration()
	{
	}

	Status validate() override
	{
		return validateColorSpaces();
	}
};

class StreamColorSpaceTest : public Test
{
protected:
	int run()
	{
		StreamConfiguration cfg;
		cfg.size = { 640, 320 };
		cfg.pixelFormat = formats::YUV422;
		cfg.colorSpace = ColorSpace::Srgb;
		config_.addConfiguration(cfg);

		StreamConfiguration &streamCfg = config_.at(0);

		/*
		 * YUV stream with sRGB colorspace should have Y'CbCr encoding
		 * adjusted.
		 */
		config_.validate();
		if (streamCfg.colorSpace->ycbcrEncoding == ColorSpace::YcbcrEncoding::None)
			return TestFail;

		/* For RGB pixelFormat, sRGB colorspace shouldn't get adjusted */
		streamCfg.pixelFormat = formats::RGB888;
		streamCfg.colorSpace = ColorSpace::Srgb;
		config_.validate();
		if (streamCfg.colorSpace != ColorSpace::Srgb)
			return TestFail;

		/*
		 * For YUV pixelFormat, encoding should picked up according to
		 * primaries.
		 */
		streamCfg.pixelFormat = formats::YUV422;
		streamCfg.colorSpace = ColorSpace(ColorSpace::Primaries::Rec2020,
						  ColorSpace::TransferFunction::Rec709,
						  ColorSpace::YcbcrEncoding::None,
						  ColorSpace::Range::Limited);
		config_.validate();
		if (streamCfg.colorSpace->ycbcrEncoding != ColorSpace::YcbcrEncoding::Rec2020)
			return TestFail;

		return TestPass;
	}

	TestCameraConfiguration config_;
};

TEST_REGISTER(StreamColorSpaceTest)
