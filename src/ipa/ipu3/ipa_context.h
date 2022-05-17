/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * ipa_context.h - IPU3 IPA Context
 *
 */

#pragma once

#include <vector>

#include <linux/intel-ipu3.h>

#include <libcamera/base/utils.h>

#include <libcamera/controls.h>
#include <libcamera/geometry.h>

namespace libcamera {

namespace ipa::ipu3 {

struct IPASessionConfiguration {
	struct {
		ipu3_uapi_grid_config bdsGrid;
		Size bdsOutputSize;
		uint32_t stride;
	} grid;

	struct {
		ipu3_uapi_grid_config afGrid;
	} af;

	struct {
		utils::Duration minShutterSpeed;
		utils::Duration maxShutterSpeed;
		double minAnalogueGain;
		double maxAnalogueGain;
	} agc;

	struct {
		int32_t defVBlank;
		utils::Duration lineDuration;
	} sensor;
};

struct IPAActiveState {
	struct {
		uint32_t focus;
		double maxVariance;
		bool stable;
	} af;

	struct {
		uint32_t exposure;
		double gain;
	} agc;

	struct {
		struct {
			double red;
			double green;
			double blue;
		} gains;

		double temperatureK;
	} awb;

	struct {
		double gamma;
		struct ipu3_uapi_gamma_corr_lut gammaCorrection;
	} toneMapping;
};

struct IPAFrameContext {
	IPAFrameContext(uint32_t frame, const ControlList &reqControls);

	struct {
		uint32_t exposure;
		double gain;
	} sensor;

	uint32_t frame;
	ControlList frameControls;
};

struct IPAContext {
	IPASessionConfiguration configuration;
	IPAActiveState activeState;

	//std::queue<IPAFrameContext> frameContextQueue;
	std::vector<IPAFrameContext> frameContexts;
};

} /* namespace ipa::ipu3 */

} /* namespace libcamera*/
