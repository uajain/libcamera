/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Raspberry Pi Ltd
 *
 * color_space.cpp - color spaces.
 */

#include <libcamera/color_space.h>

#include <algorithm>
#include <array>
#include <map>
#include <sstream>
#include <utility>

#include <libcamera/stream.h>

#include "libcamera/internal/formats.h"

/**
 * \file color_space.h
 * \brief Class and enums to represent color spaces
 */

namespace libcamera {

/**
 * \class ColorSpace
 * \brief Class to describe a color space
 *
 * The ColorSpace class defines the color primaries, the transfer function,
 * the Y'CbCr encoding associated with the color space, and the range
 * (sometimes also referred to as the quantisation) of the color space.
 *
 * Certain combinations of these fields form well-known standard color
 * spaces such as "sRGB" or "Rec709".
 *
 * In the strictest sense a "color space" formally only refers to the
 * color primaries and white point. Here, however, the ColorSpace class
 * adopts the common broader usage that includes the transfer function,
 * Y'CbCr encoding method and quantisation.
 *
 * More information on color spaces is available in the V4L2 documentation, see
 * in particular
 *
 * - <a href="https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/colorspaces-details.html#col-srgb">sRGB</a>
 * - <a href="https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/colorspaces-details.html#col-jpeg">JPEG</a>
 * - <a href="https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/colorspaces-details.html#col-smpte-170m">SMPTE 170M</a>
 * - <a href="https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/colorspaces-details.html#col-rec709">Rec.709</a>
 * - <a href="https://www.kernel.org/doc/html/latest/userspace-api/media/v4l/colorspaces-details.html#col-bt2020">Rec.2020</a>
 *
 * Note that there is no guarantee of a 1:1 mapping between color space names
 * and definitions in libcamera and V4L2. A notable difference is that the sYCC
 * libcamera color space is called JPEG in V4L2 due to historical reasons. On
 * a similar note, the sRGB colorspace defined in the kernel assumes a Y'CbCr
 * encoding to it which is not true. Hence the ColorSpace::sRGB is defined
 * differently in libcamera (with no Y'CbCr encoding and full range).
 *
 * \todo Define the color space fully in the libcamera API to avoid referencing
 * V4L2
 */

/**
 * \enum ColorSpace::Primaries
 * \brief The color primaries for this color space
 *
 * \var ColorSpace::Primaries::Raw
 * \brief These are raw colors directly from a sensor, the primaries are
 * unspecified
 *
 * \var ColorSpace::Primaries::Smpte170m
 * \brief SMPTE 170M color primaries
 *
 * \var ColorSpace::Primaries::Rec709
 * \brief Rec.709 color primaries
 *
 * \var ColorSpace::Primaries::Rec2020
 * \brief Rec.2020 color primaries
 */

/**
 * \enum ColorSpace::TransferFunction
 * \brief The transfer function used for this color space
 *
 * \var ColorSpace::TransferFunction::Linear
 * \brief This color space uses a linear (identity) transfer function
 *
 * \var ColorSpace::TransferFunction::Srgb
 * \brief sRGB transfer function
 *
 * \var ColorSpace::TransferFunction::Rec709
 * \brief Rec.709 transfer function
 */

/**
 * \enum ColorSpace::YcbcrEncoding
 * \brief The Y'CbCr encoding
 *
 * \var ColorSpace::YcbcrEncoding::None
 * \brief There is no defined Y'CbCr encoding (used for non-YUV formats)
 *
 * \var ColorSpace::YcbcrEncoding::Rec601
 * \brief Rec.601 Y'CbCr encoding
 *
 * \var ColorSpace::YcbcrEncoding::Rec709
 * \brief Rec.709 Y'CbCr encoding
 *
 * \var ColorSpace::YcbcrEncoding::Rec2020
 * \brief Rec.2020 Y'CbCr encoding
 */

/**
 * \enum ColorSpace::Range
 * \brief The range (sometimes "quantisation") for this color space
 *
 * \var ColorSpace::Range::Full
 * \brief This color space uses full range pixel values
 *
 * \var ColorSpace::Range::Limited
 * \brief This color space uses limited range pixel values, being
 * 16 to 235 for Y' and 16 to 240 for Cb and Cr (8 bits per sample)
 * or 64 to 940 for Y' and 16 to 960 for Cb and Cr (10 bits)
 */

/**
 * \fn ColorSpace::ColorSpace(Primaries p, TransferFunction t, Encoding e, Range r)
 * \brief Construct a ColorSpace from explicit values
 * \param[in] p The color primaries
 * \param[in] t The transfer function for the color space
 * \param[in] e The Y'CbCr encoding
 * \param[in] r The range of the pixel values in this color space
 */

/**
 * \brief Assemble and return a readable string representation of the
 * ColorSpace
 *
 * If the color space matches a standard ColorSpace (such as ColorSpace::Sycc)
 * then the short name of the color space ("sYCC") is returned. Otherwise
 * the four constituent parts of the ColorSpace are assembled into a longer
 * string.
 *
 * \return A string describing the ColorSpace
 */
std::string ColorSpace::toString() const
{
	/* Print out a brief name only for standard color spaces. */

	static const std::array<std::pair<ColorSpace, const char *>, 6> colorSpaceNames = { {
		{ ColorSpace::Raw, "RAW" },
		{ ColorSpace::Srgb, "sRGB" },
		{ ColorSpace::Sycc, "sYCC" },
		{ ColorSpace::Smpte170m, "SMPTE170M" },
		{ ColorSpace::Rec709, "Rec709" },
		{ ColorSpace::Rec2020, "Rec2020" },
	} };
	auto it = std::find_if(colorSpaceNames.begin(), colorSpaceNames.end(),
			       [this](const auto &item) {
				       return *this == item.first;
			       });
	if (it != colorSpaceNames.end())
		return std::string(it->second);

	/* Assemble a name made of the constituent fields. */

	static const std::map<Primaries, std::string> primariesNames = {
		{ Primaries::Raw, "RAW" },
		{ Primaries::Smpte170m, "SMPTE170M" },
		{ Primaries::Rec709, "Rec709" },
		{ Primaries::Rec2020, "Rec2020" },
	};
	static const std::map<TransferFunction, std::string> transferNames = {
		{ TransferFunction::Linear, "Linear" },
		{ TransferFunction::Srgb, "sRGB" },
		{ TransferFunction::Rec709, "Rec709" },
	};
	static const std::map<YcbcrEncoding, std::string> encodingNames = {
		{ YcbcrEncoding::None, "None" },
		{ YcbcrEncoding::Rec601, "Rec601" },
		{ YcbcrEncoding::Rec709, "Rec709" },
		{ YcbcrEncoding::Rec2020, "Rec2020" },
	};
	static const std::map<Range, std::string> rangeNames = {
		{ Range::Full, "Full" },
		{ Range::Limited, "Limited" },
	};

	auto itPrimaries = primariesNames.find(primaries);
	std::string primariesName =
		itPrimaries == primariesNames.end() ? "Invalid" : itPrimaries->second;

	auto itTransfer = transferNames.find(transferFunction);
	std::string transferName =
		itTransfer == transferNames.end() ? "Invalid" : itTransfer->second;

	auto itEncoding = encodingNames.find(ycbcrEncoding);
	std::string encodingName =
		itEncoding == encodingNames.end() ? "Invalid" : itEncoding->second;

	auto itRange = rangeNames.find(range);
	std::string rangeName =
		itRange == rangeNames.end() ? "Invalid" : itRange->second;

	std::stringstream ss;
	ss << primariesName << "/" << transferName << "/" << encodingName << "/" << rangeName;

	return ss.str();
}

/**
 * \brief Adjust the colorspace depending on the stream configuration
 * \param[in] config Stream configuration
 *
 * This function adjust the stream's colorspace depending on various factors
 * as reflected by the \a config.
 *
 * - If the stream's colorspace consists a YUV stream and has no Y'Cbcr
 *   encoding specified, the Y'Cbcr encoding is updated based on the transfer
 *   function and primaries fields.
 */
void ColorSpace::adjust(const StreamConfiguration &config)
{
	ColorSpace *cs = this;
	bool isYUV = (PixelFormatInfo::info(config.pixelFormat).colourEncoding ==
		      PixelFormatInfo::ColourEncodingYUV);

	if (isYUV && cs->ycbcrEncoding == YcbcrEncoding::None) {
		if (cs->transferFunction == TransferFunction::Rec709) {
			switch (cs->primaries) {
			/* Raw should never happen */
			case Primaries::Raw:
			case Primaries::Smpte170m:
				cs->ycbcrEncoding = YcbcrEncoding::Rec601;
				break;
			case Primaries::Rec709:
				cs->ycbcrEncoding = YcbcrEncoding::Rec709;
				break;
			case Primaries::Rec2020:
				cs->ycbcrEncoding = YcbcrEncoding::Rec2020;
				break;
			}
		} else if (cs->transferFunction == TransferFunction::Srgb) {
			cs->ycbcrEncoding = YcbcrEncoding::Rec601;
		}
	}
}

/**
 * \brief Assemble and return a readable string representation of an
 * optional ColorSpace
 *
 * This is a convenience helper to easily obtain a string representation
 * for a ColorSpace in parts of the libcamera API where it is stored in a
 * std::optional<>. If the ColorSpace is set, this function returns
 * \a colorSpace.toString(), otherwise it returns "Unset".
 *
 * \return A string describing the optional ColorSpace
 */
std::string ColorSpace::toString(const std::optional<ColorSpace> &colorSpace)
{
	if (!colorSpace)
		return "Unset";

	return colorSpace->toString();
}

/**
 * \var ColorSpace::primaries
 * \brief The color primaries of this color space
 */

/**
 * \var ColorSpace::transferFunction
 * \brief The transfer function used by this color space
 */

/**
 * \var ColorSpace::ycbcrEncoding
 * \brief The Y'CbCr encoding used by this color space
 */

/**
 * \var ColorSpace::range
 * \brief The pixel range used with by color space
 */

/**
 * \brief A constant representing a raw color space (from a sensor)
 */
const ColorSpace ColorSpace::Raw = {
	Primaries::Raw,
	TransferFunction::Linear,
	YcbcrEncoding::None,
	Range::Full
};

/**
 * \brief A constant representing the sRGB color space (RGB formats only)
 */
const ColorSpace ColorSpace::Srgb = {
	Primaries::Rec709,
	TransferFunction::Srgb,
	YcbcrEncoding::None,
	Range::Full
};

/**
 * \brief A constant representing the sYCC color space, typically used for
 * encoding JPEG images
 */
const ColorSpace ColorSpace::Sycc = {
	Primaries::Rec709,
	TransferFunction::Srgb,
	YcbcrEncoding::Rec601,
	Range::Full
};

/**
 * \brief A constant representing the SMPTE170M color space
 */
const ColorSpace ColorSpace::Smpte170m = {
	Primaries::Smpte170m,
	TransferFunction::Rec709,
	YcbcrEncoding::Rec601,
	Range::Limited
};

/**
 * \brief A constant representing the Rec.709 color space
 */
const ColorSpace ColorSpace::Rec709 = {
	Primaries::Rec709,
	TransferFunction::Rec709,
	YcbcrEncoding::Rec709,
	Range::Limited
};

/**
 * \brief A constant representing the Rec.2020 color space
 */
const ColorSpace ColorSpace::Rec2020 = {
	Primaries::Rec2020,
	TransferFunction::Rec709,
	YcbcrEncoding::Rec2020,
	Range::Limited
};

/**
 * \brief Compare color spaces for equality
 * \return True if the two color spaces are identical, false otherwise
 */
bool operator==(const ColorSpace &lhs, const ColorSpace &rhs)
{
	return lhs.primaries == rhs.primaries &&
	       lhs.transferFunction == rhs.transferFunction &&
	       lhs.ycbcrEncoding == rhs.ycbcrEncoding &&
	       lhs.range == rhs.range;
}

/**
 * \fn bool operator!=(const ColorSpace &lhs, const ColorSpace &rhs)
 * \brief Compare color spaces for inequality
 * \return True if the two color spaces are not identical, false otherwise
 */

} /* namespace libcamera */
