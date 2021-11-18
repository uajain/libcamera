/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * Copyright (C) 2021, Raspberry Pi (Trading) Limited
 *
 * color_space.h - color space definitions
 */

#ifndef __LIBCAMERA_COLOR_SPACE_H__
#define __LIBCAMERA_COLOR_SPACE_H__

#include <optional>
#include <string>

namespace libcamera {

class ColorSpace
{
public:
	enum class Primaries : int {
		Raw,
		Smpte170m,
		Rec709,
		Rec2020,
	};

	enum class YcbcrEncoding : int {
		Rec601,
		Rec709,
		Rec2020,
	};

	enum class TransferFunction : int {
		Linear,
		Srgb,
		Rec709,
	};

	enum class Range : int {
		Full,
		Limited,
	};

	constexpr ColorSpace(Primaries p, YcbcrEncoding e, TransferFunction t, Range r)
		: primaries(p), ycbcrEncoding(e), transferFunction(t), range(r)
	{
	}

	static const ColorSpace Raw;
	static const ColorSpace Jpeg;
	static const ColorSpace Srgb;
	static const ColorSpace Smpte170m;
	static const ColorSpace Rec709;
	static const ColorSpace Rec2020;

	Primaries primaries;
	YcbcrEncoding ycbcrEncoding;
	TransferFunction transferFunction;
	Range range;

	const std::string toString() const;
	static const std::string toString(const std::optional<ColorSpace> &colorSpace);
};

constexpr ColorSpace ColorSpace::Raw = { Primaries::Raw, YcbcrEncoding::Rec601, TransferFunction::Linear, Range::Full };
constexpr ColorSpace ColorSpace::Jpeg = { Primaries::Rec709, YcbcrEncoding::Rec601, TransferFunction::Srgb, Range::Full };
constexpr ColorSpace ColorSpace::Srgb = { Primaries::Rec709, YcbcrEncoding::Rec601, TransferFunction::Srgb, Range::Limited };
constexpr ColorSpace ColorSpace::Smpte170m = { Primaries::Smpte170m, YcbcrEncoding::Rec601, TransferFunction::Rec709, Range::Limited };
constexpr ColorSpace ColorSpace::Rec709 = { Primaries::Rec709, YcbcrEncoding::Rec709, TransferFunction::Rec709, Range::Limited };
constexpr ColorSpace ColorSpace::Rec2020 = { Primaries::Rec2020, YcbcrEncoding::Rec2020, TransferFunction::Rec709, Range::Limited };

bool operator==(const ColorSpace &lhs, const ColorSpace &rhs);
static inline bool operator!=(const ColorSpace &lhs, const ColorSpace &rhs)
{
	return !(lhs == rhs);
}

} /* namespace libcamera */

#endif /* __LIBCAMERA_COLOR_SPACE_H__ */
