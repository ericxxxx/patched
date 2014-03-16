/*
 * Copyright (C) 2003-2014 The Music Player Daemon Project
 * http://www.musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef MPD_PCM_NEON_HXX
#define MPD_PCM_NEON_HXX

#include "Traits.hxx"

#include <arm_neon.h>

/**
 * Call a NEON intrinsic for each element in the vector.
 *
 * @param func the NEON intrinsic
 * @param result the vector variable that gets assigned the result
 * @param vector the input vector
 */
#define neon_x4_u(func, result, vector) do { \
	result.val[0] = func(vector.val[0]); \
	result.val[1] = func(vector.val[1]); \
	result.val[2] = func(vector.val[2]); \
	result.val[3] = func(vector.val[3]); \
} while (0)

/**
 * Call a NEON intrinsic for each element in the vector.
 *
 * @param func the NEON intrinsic
 * @param result the vector variable that gets assigned the result
 * @param vector the input vector
 * @param arg an additional argument that gets passed to each call
 */
#define neon_x4_b(func, result, vector, arg) do { \
	result.val[0] = func(vector.val[0], arg); \
	result.val[1] = func(vector.val[1], arg); \
	result.val[2] = func(vector.val[2], arg); \
	result.val[3] = func(vector.val[3], arg); \
} while (0)

/**
 * Convert floating point samples to 16 bit signed integer using ARM NEON.
 */
struct NeonFloatTo16 {
	static constexpr SampleFormat src_format = SampleFormat::FLOAT;
	static constexpr SampleFormat dst_format = SampleFormat::S16;
	typedef SampleTraits<src_format> SrcTraits;
	typedef SampleTraits<dst_format> DstTraits;

	typedef typename SrcTraits::value_type SV;
	typedef typename DstTraits::value_type DV;

	static constexpr size_t BLOCK_SIZE = 16;

	void Convert(int16_t *dst, const float *src, const size_t n) const {
		for (unsigned i = 0; i < n / BLOCK_SIZE;
		     ++i, src += BLOCK_SIZE, dst += BLOCK_SIZE) {
			/* load 16 float samples into 4 quad
			   registers */
			float32x4x4_t value = vld4q_f32(src);

			/* convert to 32 bit integer */
			int32x4x4_t ivalue;
			neon_x4_b(vcvtq_n_s32_f32, ivalue, value,
				  DstTraits::BITS - 1);

			/* convert to 16 bit integer with saturation */
			int16x4x4_t nvalue;
			neon_x4_u(vqmovn_s32, nvalue, ivalue);

			/* store result */
			vst4_s16(dst, nvalue);
		}
	}
};

#endif