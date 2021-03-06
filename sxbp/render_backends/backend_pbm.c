/*
 * This source file forms part of libsxbp, a library which generates
 * experimental 2D spiral-like shapes based on input binary data.
 *
 * Copyright (C) 2016, 2017, Joshua Saxby joshua.a.saxby+TNOPLuc8vM==@gmail.com
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../saxbospiral.h"
#include "../render.h"
#include "backend_pbm.h"


#ifdef __cplusplus
extern "C"{
#endif

sxbp_status_t sxbp_render_backend_pbm(
    sxbp_bitmap_t bitmap, sxbp_buffer_t* buffer
) {
    // preconditional assertsions
    assert(bitmap.pixels != NULL);
    assert(buffer->bytes == NULL);
    /*
     * allocate two char arrays for the width and height strings - these may be
     * up to 10 characters each (max uint32_t is 10 digits long), so allocate 2
     * char arrays of 11 chars each (1 extra char for null-terminator)
     */
    char width_string[11], height_string[11];
    // these are used to keep track of how many digits each is
    int width_string_length, height_string_length = 0;
    // convert width and height to a decimal string, store lengths
    width_string_length = sprintf(width_string, "%" PRIu32, bitmap.width);
    height_string_length = sprintf(height_string, "%" PRIu32, bitmap.height);
    /*
     * now that we know the length of the image dimension strings, we can now
     * calculate how much memory we'll have to allocate for the image buffer
     */
    // calculate number of bytes per row - this is ceiling(width / 8)
    size_t bytes_per_row = (size_t)ceil((double)bitmap.width / 8.0);
    // calculate number of bytes for the entire image pixels (rows and columns)
    size_t image_bytes = bytes_per_row * bitmap.height;
    // finally put it all together to get total image buffer size
    size_t image_buffer_size = (
        3 // "P4" magic number + whitespace
        + width_string_length + 1 // width of image in decimal + whitespace
        + height_string_length + 1 // height of image in decimal + whitespace
        + image_bytes // lastly, the bytes which make up the image pixels
    );
    // try and allocate the data for the buffer
    buffer->bytes = calloc(image_buffer_size, sizeof(uint8_t));
    // check fo memory allocation failure
    if(buffer->bytes == NULL) {
        return SXBP_MALLOC_REFUSED;
    } else {
        // set buffer size
        buffer->size = image_buffer_size;
        // otherwise carry on
        size_t index = 0; // this index is used to index the buffer
        // construct magic number + whitespace
        memcpy(buffer->bytes + index, "P4\n", 3);
        index += 3;
        // image width
        memcpy(buffer->bytes + index, width_string, width_string_length);
        index += width_string_length;
        // whitespace
        memcpy(buffer->bytes + index, "\n", 1);
        index += 1;
        // image height
        memcpy(buffer->bytes + index, height_string, height_string_length);
        index += height_string_length;
        // whitespace
        memcpy(buffer->bytes + index, "\n", 1);
        index += 1;
        // now for the image data, packed into rows to the nearest byte
        for(size_t y = 0; y < bitmap.height; y++) { // row loop
            for(size_t x = 0; x < bitmap.width; x++) {
                // byte index is index + floor(x / 8)
                size_t byte_index = index + (x / 8);
                // bit index is x mod 8
                uint8_t bit_index = x % 8;
                // write bits most-significant-bit first
                buffer->bytes[byte_index] |= (
                    // black pixel = bool true = 1, just like in PBM format
                    bitmap.pixels[x][y] << (7 - bit_index)
                );
            }
            // increment index so next row is written in the correct place
            index += bytes_per_row;
        }
        return SXBP_OPERATION_OK;
    }
}

#ifdef __cplusplus
} // extern "C"
#endif
