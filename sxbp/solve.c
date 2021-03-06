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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "saxbospiral.h"
#include "plot.h"
#include "solve.h"


#ifdef __cplusplus
extern "C"{
#endif

/*
 * private function - takes a pointer to a spiral struct and captures the
 * current CPU clock ticks (should only be called once - when timing is to be
 * started).
 */
static void initialise_spiral_timing(sxbp_spiral_t* spiral) {
    spiral->current_clock_ticks = clock(); // get clock ticks
    spiral->elapsed_clock_ticks = 0; // reset accumulated ticks
}

/*
 * private function - takes a pointer to a spiral struct and captures the CPU
 * time elapsed since this function was called.
 */
static void synchronise_spiral_timing(sxbp_spiral_t* spiral) {
    // get current clock ticks
    clock_t current_clock_ticks = clock();
    // get elapsed clock ticks since last time
    clock_t elapsed_clock_ticks = (
        current_clock_ticks - spiral->current_clock_ticks
    );
    // update the 'current' clock ticks to be stored in spiral
    spiral->current_clock_ticks = current_clock_ticks;
    // add elapsed ticks to the field keeping track of this
    spiral->elapsed_clock_ticks += elapsed_clock_ticks;
    /*
     * do last step only if there is at least one whole second's worth of clock
     * ticks available
     */
    if(spiral->elapsed_clock_ticks >= CLOCKS_PER_SEC) {
        // calculate whole seconds and store in seconds_spent field
        spiral->seconds_spent += (spiral->elapsed_clock_ticks / CLOCKS_PER_SEC);
        // store the remainder in the elapsed_clock_ticks field
        spiral->elapsed_clock_ticks %= CLOCKS_PER_SEC;
    }
}

/*
 * private function, given a pointer to a spiral struct and the index of the
 * highest line to use, check if the latest line would collide with any of the
 * others, given their current directions and jump sizes (using co-ords stored
 * in cache).
 * NOTE: This assumes that all lines except the most recent are valid and
 * don't collide.
 * Returns boolean on whether or not the spiral collides or not. Also, sets the
 * collider field in the spiral struct to the index of the colliding line
 * (if any)
 *
 * Asserts:
 * - That spiral->lines is not NULL
 * - That spiral->co_ord_cache.co_ords.items is not NULL
 * - That index is less than spiral->size
 */
static bool spiral_collides(sxbp_spiral_t* spiral, size_t index) {
    // preconditional assertions
    assert(spiral->lines != NULL);
    assert(spiral->co_ord_cache.co_ords.items != NULL);
    assert(index < spiral->size);
    /*
     * if there are less than 4 lines in the spiral, then there's no way it
     * can collide, so return false early
     */
    if(spiral->size < 4) {
        return false;
    } else {
        // initialise a counter to keep track of what line we're on
        uint32_t line_count = 0;
        uint32_t ttl = spiral->lines[line_count].length + 1; // ttl of line
        size_t last_co_ord = spiral->co_ord_cache.co_ords.size;
        sxbp_line_t last_line = spiral->lines[index];
        uint32_t start_of_last_line = (last_co_ord - last_line.length) - 1;
        // check the co-ords of the last line segment against all the others
        for(uint32_t i = 0; i < start_of_last_line; i++) {
            for(size_t j = start_of_last_line; j < last_co_ord; j++) {
                if(
                    (
                        spiral->co_ord_cache.co_ords.items[i].x ==
                        spiral->co_ord_cache.co_ords.items[j].x
                    )
                    &&
                    (
                        spiral->co_ord_cache.co_ords.items[i].y ==
                        spiral->co_ord_cache.co_ords.items[j].y
                    )
                ) {
                    spiral->collider = line_count;
                    return true;
                }
            }
            // update ttl (and counter if needed)
            ttl--;
            if(ttl == 0) {
                line_count++;
                ttl = spiral->lines[line_count].length;
            }
            /*
             * terminate the loop if the next line would be the line 2 lines
             * before the last one (these two lines can never collide with the
             * last and can be safely ignored, for a small performance increase)
             */
            if(line_count == (spiral->size - 2 - 1)) { // -1 for zero-index
                break;
            }
        }
        return false;
    }
}

/*
 * given a spiral struct that is known to collide, the index of the 'last'
 * segment in the spiral (i.e. the one that was found to be colliding) and a
 * perfection threshold (0 for no perfection, or otherwise the maximmum line
 * length at which to allow aggressive optimisation), return a suggested length
 * to set the segment before this line to.
 *
 * NOTE: This function is not guaranteed to make suggestions that will not
 * collide. Every suggestion that is followed should then have the spiral
 * re-evaluated for collisions before doing any more work.
 *
 * NOTE: This function does not *need* to be called with spirals that collide,
 * but it is pointless to call this function with a spiral that does not collide
 *
 * NOTE: In the context of this function, 'rigid' or 'r' refers to the line that
 * the newly plotted line has collided with and 'previous' or 'p' refers to the
 * line before the newly plotted line.
 *
 * Asserts:
 * - That spiral.lines is not NULL
 * - That spiral.co_ord_cache.co_ords.items is not NULL
 * - That index is less than spiral.size
 */
static sxbp_length_t suggest_resize(
    sxbp_spiral_t spiral, size_t index, sxbp_length_t perfection_threshold
) {
    // preconditional assertions
    assert(spiral.lines != NULL);
    assert(spiral.co_ord_cache.co_ords.items != NULL);
    assert(index < spiral.size);
    // check if collides or not, return same size if no collision
    if(spiral.collides) {
        /*
         * if the perfection threshold is 0, then we can just use our
         * suggestion, as perfection is disabled.
         * otherwise, if the colliding line's length is greater than our
         * perfection threshold, we cannot make any intelligent suggestions on
         * the length to extend the previous line to (without the high
         * likelihood of creating a line that wastes space), so we just return
         * the previous line's length +1
         */
        if(
            (perfection_threshold > 0) &&
            (spiral.lines[index].length > perfection_threshold)
        ) {
            return spiral.lines[index - 1].length + 1;
        }
        // store the 'previous' and 'rigid' lines.
        sxbp_line_t p = spiral.lines[index - 1];
        sxbp_line_t r = spiral.lines[spiral.collider];
        // if pr and r are not parallel, we can return early
        if((p.direction % 2) != (r.direction % 2)) {
            return spiral.lines[index - 1].length + 1;
        }
        // create variables to store the start and end co-ords of these lines
        sxbp_co_ord_t pa, ra, rb;
        /*
         * We need to grab the start and end co-ords of the line previous to the
         * colliding line, and the rigid line that it collided with.
         */
        size_t p_index = sxbp_sum_lines(spiral, 0, index - 1);
        size_t r_index = sxbp_sum_lines(spiral, 0, spiral.collider);
        pa = spiral.co_ord_cache.co_ords.items[p_index];
        ra = spiral.co_ord_cache.co_ords.items[r_index];
        rb = spiral.co_ord_cache.co_ords.items[r_index + r.length];
        /*
         * Apply the rules mentioned in collision_resolution_rules.txt to
         * calculate the correct length to set the previous line and return it.
         */
        if((p.direction == SXBP_UP) && (r.direction == SXBP_UP)) {
            return (ra.y - pa.y) + r.length + 1;
        } else if((p.direction == SXBP_UP) && (r.direction == SXBP_DOWN)) {
            return (rb.y - pa.y) + r.length + 1;
        } else if((p.direction == SXBP_RIGHT) && (r.direction == SXBP_RIGHT)) {
            return (ra.x - pa.x) + r.length + 1;
        } else if((p.direction == SXBP_RIGHT) && (r.direction == SXBP_LEFT)) {
            return (rb.x - pa.x) + r.length + 1;
        } else if((p.direction == SXBP_DOWN) && (r.direction == SXBP_UP)) {
            return (pa.y - rb.y) + r.length + 1;
        } else if((p.direction == SXBP_DOWN) && (r.direction == SXBP_DOWN)) {
            return (pa.y - ra.y) + r.length + 1;
        } else if((p.direction == SXBP_LEFT) && (r.direction == SXBP_RIGHT)) {
            return (pa.x - rb.x) + r.length + 1;
        } else if((p.direction == SXBP_LEFT) && (r.direction == SXBP_LEFT)) {
            return (pa.x - ra.x) + r.length + 1;
        } else {
            // this is the catch-all case, where no way to optimise was found
            return spiral.lines[index - 1].length + 1;
        }
    } else {
        /*
         * If we got here, then no collisions could be found, which means we
         * don't have to extend the previous segment.
         */
        return spiral.lines[index - 1].length;
    }
}

sxbp_status_t sxbp_resize_spiral(
    sxbp_spiral_t* spiral, uint32_t index, sxbp_length_t length,
    sxbp_length_t perfection_threshold
) {
    // preconditional assertions
    assert(spiral->lines != NULL);
    assert(index < spiral->size);
    /*
     * setup state variables, these are used in place of recursion for managing
     * state of which line is being resized, and what size it should be.
     */
    // set result status
    sxbp_status_t result;
    size_t current_index = index;
    sxbp_length_t current_length = length;
    while(true) {
        // set the target line to the target length
        spiral->lines[current_index].length = current_length;
        /*
         * also, set cache validity to this index so we invalidate any invalid
         * entries in the co-ord cache
         */
        spiral->co_ord_cache.validity = (
            current_index < spiral->co_ord_cache.validity
        ) ? current_index : spiral->co_ord_cache.validity;
        // update the spiral's co-ord cache, and catch any errors
        result = sxbp_cache_spiral_points(spiral, current_index + 1);
        // return if errors
        if(result != SXBP_OPERATION_OK) {
            return result;
        }
        spiral->collides = spiral_collides(spiral, current_index);
        if(spiral->collides) {
            /*
             * if we've caused a collision, we need to call the suggest_resize()
             * function to get the suggested length to resize the previous
             * segment to
             */
            current_length = suggest_resize(
                *spiral, current_index, perfection_threshold
            );
            current_index--;
        } else if(current_index != index) {
            /*
             * if we didn't cause a collision but we're not on the top-most
             * line, then we've just resolved a collision situation.
             * we now need to work on the next line and start by setting to 1.
             */
            current_index++;
            current_length = 1;
        } else {
            /*
             * if we're on the top-most line and there's no collision
             * this means we've finished! Set solved_count to this index+1
             * Return OPERATION_OK from function.
             */
            spiral->solved_count = index + 1;
            result = SXBP_OPERATION_OK;
            return result;
        }
        // update time spent solving at every iteration
        synchronise_spiral_timing(spiral);
    }
}

sxbp_status_t sxbp_plot_spiral(
    sxbp_spiral_t* spiral, sxbp_length_t perfection_threshold, uint32_t max_line,
    void(* progress_callback)(
        sxbp_spiral_t* spiral, uint32_t latest_line, uint32_t target_line,
        void* progress_callback_user_data
    ),
    void* progress_callback_user_data
) {
    // preconditional assertions
    assert(spiral->lines != NULL);
    // start up the CPU clock cycle timing
    initialise_spiral_timing(spiral);
    /*
     * update accuracy of the seconds spent field
     * (every run time makes it one second less accurate).
     */
    spiral->seconds_accuracy++;
    // set up result status
    sxbp_status_t result;
    // get index of highest line to plot
    uint32_t max_index = (max_line > spiral->size) ? spiral->size : max_line;
    // calculate the length of each line within range solved_count -> max_index
    for(size_t i = spiral->solved_count; i < max_index; i++) {
        result = sxbp_resize_spiral(spiral, i, 1, perfection_threshold);
        // catch and return error if any
        if(result != SXBP_OPERATION_OK) {
            return result;
        }
        // update time spent solving
        synchronise_spiral_timing(spiral);
        // call callback if given
        if(progress_callback != NULL) {
            progress_callback(spiral, i, max_index, progress_callback_user_data);
        }
    }
    // update time spent solving
    synchronise_spiral_timing(spiral);
    // all ok
    result = SXBP_OPERATION_OK;
    return result;
}

#ifdef __cplusplus
} // extern "C"
#endif
