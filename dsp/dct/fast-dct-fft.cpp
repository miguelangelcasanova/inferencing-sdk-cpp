/*
 * Fast discrete cosine transform algorithms (C)
 *
 * Copyright (c) 2017 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/fast-discrete-cosine-transform-algorithms
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include "fast-dct-fft.h"
#include "../returntypes.hpp"
#include "../numpy.hpp"
#include "../memory.hpp"

// DCT type II, unscaled
int ei::dct::transform(float vector[], size_t len) {
	const size_t fft_data_out_size = (len / 2 + 1) * sizeof(ei::fft_complex_t);
	const size_t fft_data_in_size = len * sizeof(float);

	// Allocate KissFFT input / output buffer
    ei::fft_complex_t *fft_data_out =
		(ei::fft_complex_t*)ei_dsp_calloc(fft_data_out_size, 1);
	if (!fft_data_out) {
		return ei::EIDSP_OUT_OF_MEM;
	}

    float *fft_data_in = (float*)ei_dsp_calloc(fft_data_in_size, 1);
	if (!fft_data_in) {
		ei_dsp_free(fft_data_out, fft_data_out_size);
		return ei::EIDSP_OUT_OF_MEM;
	}

	// Preprocess the input buffer with the data from the vector
	size_t halfLen = len / 2;
	for (size_t i = 0; i < halfLen; i++) {
		fft_data_in[i] = vector[i * 2];
		fft_data_in[len - 1 - i] = vector[i * 2 + 1];
	}
	if (len % 2 == 1) {
		fft_data_in[halfLen] = vector[len - 1];
	}

	int r = ei::numpy::rfft(fft_data_in, len, fft_data_out, (len / 2 + 1), len);
	if (r != 0) {
		ei_dsp_free(fft_data_in, fft_data_in_size);
		ei_dsp_free(fft_data_out, fft_data_out_size);
		return r;
	}

	for (size_t i = 0; i < len / 2 + 1; i++) {
		float temp = i * M_PI / (len * 2);
		vector[i] = fft_data_out[i].r * cos(temp) + fft_data_out[i].i * sin(temp);
	}

	ei_dsp_free(fft_data_in, fft_data_in_size);
	ei_dsp_free(fft_data_out, fft_data_out_size);

	return 0;
}

// DCT type III, unscaled
int ei::dct::inverse_transform(float vector[], size_t len) {
	const size_t fft_data_out_size = len * sizeof(kiss_fft_cpx);
	const size_t fft_data_in_size = len * sizeof(kiss_fft_cpx);

	// Allocate KissFFT input / output buffer
    kiss_fft_cpx *fft_data_out = (kiss_fft_cpx*)ei_dsp_calloc(fft_data_out_size, 1);
	if (!fft_data_out) {
		return ei::EIDSP_OUT_OF_MEM;
	}

    kiss_fft_cpx *fft_data_in = (kiss_fft_cpx*)ei_dsp_calloc(fft_data_in_size, 1);
	if (!fft_data_in) {
		ei_dsp_free(fft_data_out, fft_data_out_size);
		return ei::EIDSP_OUT_OF_MEM;
	}

	size_t kiss_fftr_mem_length;

    // Allocate KissFFT configuration
    kiss_fft_cfg cfg = kiss_fft_alloc(len, 0, NULL, NULL, &kiss_fftr_mem_length);
	if (!cfg) {
		ei_dsp_free(fft_data_in, fft_data_in_size);
		ei_dsp_free(fft_data_out, fft_data_out_size);
		return ei::EIDSP_OUT_OF_MEM;
	}

	ei_dsp_register_alloc(kiss_fftr_mem_length);

	// Preprocess and transform
	if (len > 0) {
		vector[0] /= 2;
	}

	for (size_t i = 0; i < len; i++) {
		float temp = i * M_PI / (len * 2);
		fft_data_in[i].r = vector[i] * cos(temp);
		fft_data_in[i].i *= -sin(temp);
	}

	kiss_fft(cfg, fft_data_in, fft_data_out);

	// Postprocess the vectors
	size_t halfLen = len / 2;
	for (size_t i = 0; i < halfLen; i++) {
		vector[i * 2 + 0] = fft_data_out[i].r;
		vector[i * 2 + 1] = fft_data_out[len - 1 - i].r;
	}

	if (len % 2 == 1) {
		vector[len - 1] = fft_data_out[halfLen].r;
	}

	ei_dsp_free(cfg, kiss_fftr_mem_length);
	ei_dsp_free(fft_data_in, fft_data_in_size);
	ei_dsp_free(fft_data_out, fft_data_out_size);

	return ei::EIDSP_OK;
}
