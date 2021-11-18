/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Limited
 *
 * color_space.cpp - color spaces.
 */

#include <libcamera/color_space.h>

#include <algorithm>
#include <sstream>
#include <vector>

/**
 * \file color_space.h
 * \brief Class and enums to represent color spaces
 */

namespace libcamera {

/**
 * \class ColorSpace
 * \brief Class to describe a color space
 *
 * The ColorSpace class defines the color primaries, the Y'CbCr encoding,
 * the transfer function associated with the color space, and the range
 * (sometimes also referred to as the quantisation) of the color space.
 *
 * Certain combinations of these fields form well-known standard color
 * spaces such as "JPEG" or "REC709".
 *
 * In the strictest sense a "color space" formally only refers to the
 * color primaries and white point. Here, however, the ColorSpace class
 * adopts the common broader usage that includes the transfer function,
 * Y'CbCr encoding method and quantisation.
 *
 * For more information on the specific color spaces described here, please
 * see:
 *
 * <a href="https://www.kernel.org/doc/html/v5.10/userspace-api/media/v4l/colorspaces-details.html#col-srgb">sRGB</a>
 * <a href="https://www.kernel.org/doc/html/v5.10/userspace-api/media/v4l/colorspaces-details.html#col-jpeg">JPEG</a>
 * <a href="https://www.kernel.org/doc/html/v5.10/userspace-api/media/v4l/colorspaces-details.html#col-smpte-170m">SMPTE 170M</a>
 * <a href="https://www.kernel.org/doc/html/v5.10/userspace-api/media/v4l/colorspaces-details.html#col-rec709">Rec.709</a>
 * <a href="https://www.kernel.org/doc/html/v5.10/userspace-api/media/v4l/colorspaces-details.html#col-bt2020">Rec.2020</a>
 */

/**
 * \enum ColorSpace::Primaries
 * \brief The color primaries for this color space
 *
 * \var ColorSpace::Primaries::Raw
 * \brief These are raw colors directly from a sensor
 * \var ColorSpace::Primaries::Smpte170m
 * \brief SMPTE 170M color primaries
 * \var ColorSpace::Primaries::Rec709
 * \brief Rec.709 color primaries
 * \var ColorSpace::Primaries::Rec2020
 * \brief Rec.2020 color primaries
 */

/**
 * \enum ColorSpace::YcbcrEncoding
 * \brief The Y'CbCr encoding
 *
 * \var ColorSpace::YcbcrEncoding::Rec601
 * \brief Rec.601 Y'CbCr encoding
 * \var ColorSpace::YcbcrEncoding::Rec709
 * \brief Rec.709 Y'CbCr encoding
 * \var ColorSpace::YcbcrEncoding::Rec2020
 * \brief Rec.2020 Y'CbCr encoding
 */

/**
 * \enum ColorSpace::TransferFunction
 * \brief The transfer function used for this color space
 *
 * \var ColorSpace::TransferFunction::Linear
 * \brief This color space uses a linear (identity) transfer function
 * \var ColorSpace::TransferFunction::Srgb
 * \brief sRGB transfer function
 * \var ColorSpace::TransferFunction::Rec709
 * \brief Rec.709 transfer function
 */

/**
 * \enum ColorSpace::Range
 * \brief The range (sometimes "quantisation") for this color space
 *
 * \var ColorSpace::Range::Full
 * \brief This color space uses full range pixel values
 * \var ColorSpace::Range::Limited
 * \brief This color space uses limited range pixel values, being
 * 16 to 235 for Y' and 16 to 240 for Cb and Cr (8 bits per sample)
 * or 64 to 940 for Y' and 16 to 960 for Cb and Cr (10 bits)
 */

/**
 * \fn ColorSpace::ColorSpace(Primaries p, Encoding e, TransferFunction t, Range r)
 * \brief Construct a ColorSpace from explicit values
 * \param[in] p The color primaries
 * \param[in] e The Y'CbCr encoding
 * \param[in] t The transfer function for the color space
 * \param[in] r The range of the pixel values in this color space
 */

/**
 * \brief Assemble and return a readable string representation of the
 * ColorSpace
 * \return A string describing the ColorSpace
 */
const std::string ColorSpace::toString() const
{
	/* Print out a brief name only for standard color sapces. */

	static const std::vector<std::pair<ColorSpace, const char *>> colorSpaceNames = {
		{ ColorSpace::Raw, "Raw" },
		{ ColorSpace::Jpeg, "Jpeg" },
		{ ColorSpace::Srgb, "Srgb" },
		{ ColorSpace::Smpte170m, "Smpte170m" },
		{ ColorSpace::Rec709, "Rec709" },
		{ ColorSpace::Rec2020, "Rec2020" },
	};
	auto it = std::find_if(colorSpaceNames.begin(), colorSpaceNames.end(),
			       [this](const auto &item) {
				       return *this == item.first;
			       });
	if (it != colorSpaceNames.end())
		return std::string(it->second);

	static const char *primariesNames[] = {
		"Raw",
		"Smpte170m",
		"Rec709",
		"Rec2020",
	};
	static const char *encodingNames[] = {
		"Rec601",
		"Rec709",
		"Rec2020",
	};
	static const char *transferFunctionNames[] = {
		"Linear",
		"Srgb",
		"Rec709",
	};
	static const char *rangeNames[] = {
		"Full",
		"Limited",
	};

	std::stringstream ss;
	ss << std::string(primariesNames[static_cast<int>(primaries)]) << "/"
	   << std::string(encodingNames[static_cast<int>(ycbcrEncoding)]) << "/"
	   << std::string(transferFunctionNames[static_cast<int>(transferFunction)]) << "/"
	   << std::string(rangeNames[static_cast<int>(range)]);

	return ss.str();
}

/**
 * \brief Assemble and return a readable string representation of an
 * optional ColorSpace
 * \return A string describing the optional ColorSpace
 */
const std::string ColorSpace::toString(const std::optional<ColorSpace> &colorSpace)
{
	if (!colorSpace)
		return "Unknown";

	return colorSpace->toString();
}

/**
 * \var ColorSpace::primaries
 * \brief The color primaries
 */

/**
 * \var ColorSpace::ycbcrEncoding
 * \brief The Y'CbCr encoding
 */

/**
 * \var ColorSpace::transferFunction
 * \brief The transfer function for this color space
 */

/**
 * \var ColorSpace::range
 * \brief The pixel range used by this color space
 */

/**
 * \var ColorSpace::Undefined
 * \brief A constant representing a fully undefined color space
 */

/**
 * \var ColorSpace::Raw
 * \brief A constant representing a raw color space (from a sensor)
 */

/**
 * \var ColorSpace::Jpeg
 * \brief A constant representing the JPEG color space used for
 * encoding JPEG images (and regarded as being the same as the sRGB
 * color space)
 */

/**
 * \var ColorSpace::Smpte170m
 * \brief A constant representing the SMPTE170M color space
 */

/**
 * \var ColorSpace::Rec709
 * \brief A constant representing the Rec.709 color space
 */

/**
 * \var ColorSpace::Rec2020
 * \brief A constant representing the Rec.2020 color space
 */

/**
 * \brief Compare color spaces for equality
 * \return True if the two color spaces are identical, false otherwise
 */
bool operator==(const ColorSpace &lhs, const ColorSpace &rhs)
{
	return lhs.primaries == rhs.primaries &&
	       lhs.ycbcrEncoding == rhs.ycbcrEncoding &&
	       lhs.transferFunction == rhs.transferFunction &&
	       lhs.range == rhs.range;
}

/**
 * \fn bool operator!=(const ColorSpace &lhs, const ColorSpace &rhs)
 * \brief Compare color spaces for inequality
 * \return True if the two color spaces are not identical, false otherwise
 */

} /* namespace libcamera */
