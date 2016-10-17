#include <stdint.h>
#include <stdlib.h>

#include "saxbospiral.h"
#include "initialise.h"


#ifdef __cplusplus
extern "C"{
#endif

/*
 * when facing the direction specified by current, return the direction that
 * will be faced when turning in the rotational direction specified by turn.
 */
sxbp_direction_t sxbp_change_direction(
    sxbp_direction_t current, sxbp_rotation_t turn
) {
    return (current + turn) % 4U;
}

/*
 * returns a spiral struct with all fields initialised to 0
 */
sxbp_spiral_t sxbp_blank_spiral() {
    return (sxbp_spiral_t){0, NULL, {{NULL, 0}, 0}, false, 0, 0, 0};
}

/*
 * given a buffer_t full of data, and a pointer to a blank spiral_t
 * struct, populates the spiral struct from the data in the buffer
 * this converts the 0s and 1s in the data into UP, LEFT, DOWN, RIGHT
 * instructions which are then used to build the pattern.
 * returns a status_t struct with error information (if needed)
 */
sxbp_status_t sxbp_init_spiral(sxbp_buffer_t buffer, sxbp_spiral_t* spiral) {
    // result status object
    sxbp_status_t result;
    // number of lines is number of bits of the data, + 1 for the first UP line
    size_t line_count = (buffer.size * 8) + 1;
    // populate spiral struct
    spiral->size = line_count;
    spiral->collides = -1;
    // allocate enough memory for a line_t struct for each bit
    spiral->lines = calloc(sizeof(sxbp_line_t), line_count);
    // check for memory allocation failure
    if(spiral->lines == NULL) {
        result.location = DEBUG;
        result.diagnostic = MALLOC_REFUSED;
        return result;
    }
    // First line is always an UP line - this is for orientation purposes
    sxbp_direction_t current = UP;
    spiral->lines[0].direction = current;
    spiral->lines[0].length = 0;
    /*
     * now, iterate over all the bits in the data and convert to directions that
     * make the spiral pattern, storing these directions in the result lines
     */
    for(size_t s = 0; s < buffer.size; s++) {
        // byte-level loop
        for(size_t b = 0; b < 8; b++) {
            // bit level loop
            uint8_t e = 7 - b; // which power of two to use with bit mask
            uint8_t bit = (buffer.bytes[s] & (1 << e)) >> e; // the current bit
            size_t index = (s * 8) + b + 1; // line index
            sxbp_rotation_t rotation; // the rotation we're going to make
            // set rotation direction based on the current bit
            rotation = (bit == 0) ? CLOCKWISE : ANTI_CLOCKWISE;
            // calculate the change of direction
            current = sxbp_change_direction(current, rotation);
            // store direction in result struct
            spiral->lines[index].direction = current;
            // set length to 0 initially
            spiral->lines[index].length = 0;
        }
    }
    // all ok
    result.diagnostic = OPERATION_OK;
    return result;
}

#ifdef __cplusplus
} // extern "C"
#endif
