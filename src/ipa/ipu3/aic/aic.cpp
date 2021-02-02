/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Google Inc.
 *
 * aic.cpp - Intel IA Imaging library C++ wrapper
 *
 * Automatic IPU Configuration
 */

#include <ia_imaging/ia_cmc_parser.h>

#include "aic.h"

#include "libcamera/internal/log.h"

#include "binary_data.h"
#include "parameter_encoder.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define ALIGN128(x) (((x) + 127) & ~127)

namespace libcamera {

LOG_DEFINE_CATEGORY(AIC)

namespace ipa::ipu3::aic {

/*
 * Only a Single Pipeline instance of the AIC is currently supported.
 * The CrOS implementation defines a set of AIC to run for both STILL and VIDEO
 * allowing improved perfomance on preview streams while taking an image
 * capture.
 */

AIC::~AIC()
{
	if (iaCmc_)
		ia_cmc_parser_deinit(iaCmc_);
}

int AIC::init(BinaryData &aiqb)
{
	LOG(AIC, Debug) << "Initialising IA AIC Wrapper";

	pipe_ = std::make_unique<IPU3ISPPipe>();

	CLEAR(runtimeOutFrameParams_);
	CLEAR(runtimeResCfgParams_);
	CLEAR(runtimeInFrameParams_);
	CLEAR(runtimeParamsRec_);
	CLEAR(runtimeParams_);

	runtimeParams_.output_frame_params = &runtimeOutFrameParams_;
	runtimeParams_.frame_resolution_parameters = &runtimeResCfgParams_;
	runtimeParams_.input_frame_params = &runtimeInFrameParams_;
	runtimeParams_.focus_rect = &runtimeParamsRec_;

	/*
	 * \todo: Both the AIC and the AIQ use the iaCmc_.
	 * Can this be the same instance or do they need their own instances?
	 */
	iaCmc_ = ia_cmc_parser_init(aiqb.data());
	if (iaCmc_ == nullptr) {
		LOG(AIC, Error) << "Failed to initialise CMC Parser";
		return -EINVAL;
	}

	/* \todo: Initialise the mRuntimeParams with ia_aiq_frame_params before
	 * constructing the KBL_AIC.
	 * In CrOS, GraphConfig::getSensorFrameParams provides all these
	 * details. Start looking from ParameterWorker::configure()
	 */
	ISPPipe *pipe = static_cast<ISPPipe *>(pipe_.get());
	skyCam_ = std::make_unique<KBL_AIC>(&pipe, 1, iaCmc_, aiqb.data(),
					    runtimeParams_, 0, 0);

	return 0;
}

int AIC::configure(const struct IPAConfigInfo &configInfo)
{
	LOG(AIC, Debug) << "IA AIC configure(): "
			<< " bds: " << configInfo.bdsOutputSize.width << "x" << configInfo.bdsOutputSize.height
			<< " ifSize: " << configInfo.iif.width << "x" << configInfo.iif.height
			<< " cropRegion: " << configInfo.sensorInfo.analogCrop.width
			<< "x" << configInfo.sensorInfo.analogCrop.height;

	//Fill AIC input frame params
	runtimeParams_.frame_use = ia_aiq_frame_use_still;
	runtimeParams_.mode_index = AIC_MODE_STILL;
	runtimeInFrameParams_.sensor_frame_params.horizontal_crop_offset = 0;
	runtimeInFrameParams_.sensor_frame_params.vertical_crop_offset = 0;
	runtimeInFrameParams_.sensor_frame_params.cropped_image_width = configInfo.sensorInfo.analogCrop.width;
	runtimeInFrameParams_.sensor_frame_params.cropped_image_height = configInfo.sensorInfo.analogCrop.height;
	runtimeInFrameParams_.sensor_frame_params.horizontal_scaling_numerator = 1;
	runtimeInFrameParams_.sensor_frame_params.horizontal_scaling_denominator = 1;
	runtimeInFrameParams_.sensor_frame_params.vertical_scaling_numerator = 1;
	runtimeInFrameParams_.sensor_frame_params.vertical_scaling_denominator = 1;
	runtimeInFrameParams_.fix_flip_x = 0;
	runtimeInFrameParams_.fix_flip_y = 0;

	runtimeOutFrameParams_.width = configInfo.sensorInfo.analogCrop.width;
	runtimeOutFrameParams_.height = configInfo.sensorInfo.analogCrop.height;

	runtimeResCfgParams_.BDSin_img_width = configInfo.iif.width;
	runtimeResCfgParams_.BDSin_img_height = configInfo.iif.height;
	runtimeResCfgParams_.BDSout_img_width = configInfo.bdsOutputSize.width;
	runtimeResCfgParams_.BDSout_img_height = configInfo.bdsOutputSize.height;

	runtimeResCfgParams_.horizontal_IF_crop =
		(configInfo.sensorInfo.outputSize.width - configInfo.iif.width) / 2;
	runtimeResCfgParams_.vertical_IF_crop =
		(configInfo.sensorInfo.outputSize.height - configInfo.iif.height) / 2;
	runtimeResCfgParams_.BDS_horizontal_padding =
		static_cast<uint16_t>(ALIGN128(configInfo.bdsOutputSize.width) - configInfo.bdsOutputSize.width);

	return 0;
}

void AIC::reset()
{
}

int AIC::run(ipu3_uapi_params *params)
{
	LOG(AIC, Debug) << "IA AIC Run()";
	skyCam_->Run(&runtimeParams_, 1);

	/* IPU3 firmware specific encoding for ISP controls. */
	ParameterEncoder::encode(GetAicConfig(), params);

	return 0;
}

std::string AIC::version()
{
	return "";
}

aic_config_t *AIC::GetAicConfig()
{
	pipe_->dump();
	return pipe_->GetAicConfig();
}

void AIC::updateRuntimeParams(aiq::AiqResults &results)
{
	runtimeParams_.pa_results = results.pa();
	runtimeParams_.sa_results = results.sa();

	const ia_aiq_ae_results *ae = results.ae();
	runtimeParams_.exposure_results = ae->exposures->exposure;
	runtimeParams_.weight_grid = ae->weight_grid;

	runtimeParams_.isp_vamem_type = 0;
	runtimeParams_.awb_results = results.awb();
	runtimeParams_.gbce_results = results.gbce();

	/* \todo: Set below parameters from capture settings
	params->time_stamp = 0; //microsecond unit
	params->manual_brightness = settings->ispSettings.manualSettings.manualBrightness;
	params->manual_contrast = settings->ispSettings.manualSettings.manualContrast;
	params->manual_hue = settings->ispSettings.manualSettings.manualHue;
	params->manual_saturation = settings->ispSettings.manualSettings.manualSaturation;
	params->manual_sharpness = settings->ispSettings.manualSettings.manualSharpness;
	*/
}

} /* namespace ipa::ipu3::aic */

} /* namespace libcamera */
