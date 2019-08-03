/*
 * This source file forms part of sxbp, a library which generates experimental
 * 2D spiral-like shapes based on input binary data.
 *
 * Copyright (C) Joshua Saxby <joshua.a.saxby@gmail.com> 2019
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <mpi.h>

#include "sxbp/figure_collides.h"
#include "sxbp/sxbp.h"


#ifdef __cplusplus
#error "This file is ISO C99. It should not be compiled with a C++ Compiler."
#endif

// private data structure for maintaining state for problem generation
typedef struct ProblemGenerator {
    uint32_t next; // the id of the next problem that will be generated
} ProblemGenerator;

// private data structure for tracking program self-timing state
typedef struct TimingData {
    long double last_estimate; // the last time estimate we made
    long double seconds_elapsed; // the amount of time elapsed in the current timing period
    time_t start_time;
} TimingData;

// private data structure for storing proportion of valid solutions for problems
typedef struct ValidSolutionsStatistics {
    uint8_t problem_size; // for what size of problem (in bits) is this data?
    // the fewest number of valid solutions found across problems of this size
    uint64_t lowest_validity;
    // the highest number of valid solutions found across problems of this size
    uint64_t highest_validity;
    // the mean number of valid solutions found across problems of this size
    long double mean_validity;
    // NOTE: to get validity rates as percentages:
    // divide validity count by 2^problem_size
} ValidSolutionsStatistics;

// tweak these variables to change which range of problem sizes to test
static const uint8_t MIN_PROBLEM_SIZE = 3;
static const uint8_t MAX_PROBLEM_SIZE = 18;

static const long double MINUTE_SECONDS = 60.0L;
static const long double HOUR_SECONDS = 60.0L * 60.0L;
static const long double DAY_SECONDS = 60.0L * 60.0L * 24.0L;
static const long double MONTH_SECONDS = 60.0L * 60.0L * 24.0L * 30.44L;
static const long double YEAR_SECONDS = 60.0L * 60.0L * 24.0L * 365.2425L;

static uint32_t two_to_the_power_of(uint8_t power) {
    return (uint32_t)powl(2.0L, (long double)power);
}

static ProblemGenerator init_problem_generator(uint8_t problem_size) {
    ProblemGenerator generator = {
        .next = 0,
    };
    return generator;
}

// picks the next problem out of the problem generator
static uint32_t get_next_problem(ProblemGenerator* generator) {
    uint32_t problem = generator->next;
    // NOTE: this will overflow for problems larger than 32 bits!
    generator->next++;
    return problem;
}

// unpacks all the bits up to `size` from the given `source` integer into `dest`
static void integer_to_bit_string(uint32_t source, bool* dest, uint8_t size) {
    // NOTE: we handle integers big-endian, but only handle the x lowest bits
    for (uint8_t i = 0; i < size; i++) {
        uint32_t mask = 1U << (size - i - 1);
        if ((source & mask) != 0) {
            dest[i] = true;
        } else {
            dest[i] = false;
        }
    }
}

static bool is_solution_valid_for_problem(
    uint8_t size,
    bool* solution,
    bool* problem
) {
    // create and allocate memory for a figure of the correct size
    sxbp_Figure figure = sxbp_blank_figure();
    figure.size = size + 1U; // inlcudes 1 additional starter line as orientation
    if (!sxbp_success(sxbp_init_figure(&figure))) {
        abort(); // XXX: Cheap allocation failure exit!
    }
    // hardcode the first line, which is always the same
    figure.lines[0].direction = SXBP_UP;
    figure.lines[0].length = 3;
    // set the line lengths and directions from the problem and solution
    sxbp_Direction current_direction = SXBP_UP;
    for (uint8_t i = 0; i < size; i++) {
        // if bit is 1, turn right, otherwise, turn left
        if (solution[i]) {
            current_direction = (current_direction - 1) % 4;
        } else {
            current_direction = (current_direction + 1) % 4;
        }
        figure.lines[i + 1].length = problem[i] ? 2 : 1;
        figure.lines[i + 1].direction = current_direction;
    }
    // check if figure collides and store result
    bool figure_collides = false;
    if (!sxbp_success(sxbp_figure_collides(&figure, &figure_collides))) {
        abort(); // XXX: Cheap allocation failure exit!
    }

    // XXX: DEBUGGING CODE FOR PRINTING EVERY SINGLE TESTED CANDIDATE
    // if (!figure_collides){
    //     sxbp_Bitmap bitmap = sxbp_blank_bitmap();
    //     sxbp_render_figure_to_bitmap(&figure, &bitmap);
    //     sxbp_print_bitmap(&bitmap, stdout);
    //     sxbp_free_bitmap(&bitmap);
    // }

    // free memory for figure
    sxbp_free_figure(&figure);
    // return result
    return !figure_collides;
}

static FILE* open_file_for_appending(const char* filename) {
    FILE* file_handle = fopen(filename, "a");
    // abort if file couldn't be opened
    if (file_handle == NULL) {
        fprintf(stderr, "Can't open file for appending!\n");
        abort();
    }
    return file_handle;
}

static FILE* close_file(FILE* file_handle) {
    // try to close file, abort if unsuccessful
    if (fclose(file_handle) != 0) {
        fprintf(stderr, "Can't close open file!\n");
        abort();
    }
    return NULL;
}

static void stopwatch_start(TimingData* timing_data) {
    timing_data->start_time = time(NULL);
    timing_data->seconds_elapsed = 1.0L / 0.0L; // NAN
}

static void stopwatch_stop(TimingData* timing_data) {
    time_t now = time(NULL);
    long double seconds_elapsed = difftime(now, timing_data->start_time);
    timing_data->seconds_elapsed = seconds_elapsed;
}

static long double estimated_completion_time_of_next(
    long double latest_run_time,
    uint8_t completed_factor
) {
    uint8_t next_factor = completed_factor + 1;
    return latest_run_time / (powl(4.0L, completed_factor) * completed_factor)
                           * (powl(4.0L, next_factor) * next_factor);
}

static long double estimated_completion_time(
    long double latest_run_time,
    uint8_t completed_factor,
    uint8_t factors_left
) {
    long double estimate = 0.0L;
    long double last_estimated = latest_run_time;
    for (uint8_t f = 0; f < factors_left; f++) {
        last_estimated = estimated_completion_time_of_next(
            last_estimated,
            completed_factor + f
        );
        estimate += last_estimated;
    }
    return estimate;
}

// returns the most convenient way of describing a given time unit
static const char* convenient_time_unit(long double seconds) {
    if (seconds < MINUTE_SECONDS) {
        return "s";
    } else if (seconds < HOUR_SECONDS) {
        return " mins";
    } else if (seconds < DAY_SECONDS) {
        return " hours";
    } else if (seconds < MONTH_SECONDS) {
        return " days";
    } else if (seconds < YEAR_SECONDS) {
        return " months";
    } else {
        return " years";
    }
}

// returns the time in seconds, converted if needed to most convenient unit
static long double convenient_time_value(long double seconds) {
    if (seconds < MINUTE_SECONDS) {
        return seconds;
    } else if (seconds < HOUR_SECONDS) {
        return seconds / MINUTE_SECONDS;
    } else if (seconds < DAY_SECONDS) {
        return seconds / HOUR_SECONDS;
    } else if (seconds < MONTH_SECONDS) {
        return seconds / DAY_SECONDS;
    } else if (seconds < YEAR_SECONDS) {
        return seconds / MONTH_SECONDS;
    } else {
        return seconds / YEAR_SECONDS;
    }
}

// counts how many valid solutions there are to a problem via brute-force search
static uint32_t count_solutions_to_problem(
    uint8_t problem_bits,
    uint32_t problem_size,
    uint32_t problem_number,
    bool* problem_scratch_pad,
    bool* solution_scratch_pad
) {
    uint32_t solutions_to_problem = 0;
    // generate a problem for bit string p
    integer_to_bit_string(problem_number, problem_scratch_pad, problem_bits);
    // for every potential solution for a problem of that size...
    for (uint32_t s = 0; s < problem_size; s++) {
        // generate a solution for bit string s
        integer_to_bit_string(s, solution_scratch_pad, problem_bits);
        // check if the solution is valid for the problem
        if (
            is_solution_valid_for_problem(
                problem_bits,
                solution_scratch_pad,
                problem_scratch_pad
            )
        ) {
            // increment number of solutions if valid
            solutions_to_problem++;
        }
    }
    return solutions_to_problem;
}

void update_and_print_completion_estimate(TimingData* timing_data, uint8_t last_solved) {
    time_t now = time(NULL);
    char time_buffer[21];
    strftime(time_buffer, sizeof(time_buffer), "%FT%TZ", gmtime(&now));
    // print error of estimate
    printf("============================= %s =============================\n", time_buffer);
    // printf("Time Factor: %f\n", seconds_elapsed / previous_time);
    printf(
        "Solved problem size: %" PRIu8
        " - Time taken:\t%Lf%s (%.2Lf%% of estimate)\n",
        last_solved,
        convenient_time_value(timing_data->seconds_elapsed),
        convenient_time_unit(timing_data->seconds_elapsed),
        ((timing_data->seconds_elapsed / timing_data->last_estimate) - 0.0L) * 100.0L
    );
    long double completion_estimate = estimated_completion_time(timing_data->seconds_elapsed, last_solved, MAX_PROBLEM_SIZE - last_solved);
    printf(
        "Estimated time til completion:\t\t%Lf%s\n",
        convenient_time_value(completion_estimate),
        convenient_time_unit(completion_estimate)
    );
    if (last_solved < MAX_PROBLEM_SIZE) {
        timing_data->last_estimate = estimated_completion_time(timing_data->seconds_elapsed, last_solved, 1);
        printf(
            "Estimated time til next solved:\t\t%Lf%s\n",
            convenient_time_value(timing_data->last_estimate),
            convenient_time_unit(timing_data->last_estimate)
        );
    }
    printf("================================================================================\n\n");
    // previous_time = seconds_elapsed;
}

int main(int argc, char *argv[]) {
    // pre-conditional assertions
    assert(MIN_PROBLEM_SIZE > 0); // no point testing a problem of size 0
    assert(MIN_PROBLEM_SIZE <= MAX_PROBLEM_SIZE); // max mustn't be < min
    // this program works on problem sizes up to 32 bits
    assert(MAX_PROBLEM_SIZE <= 32);
    // we need one additional argument --for the file name to output to
    if (argc < 2) {
        fprintf(stderr, "Need filename argument!\n");
        return -1;
    }
    const char* filename = argv[1]; // grab filename out of arguments

    // initialise MPI and discover our place in the world
    MPI_Init(&argc, &argv);
    int world_rank, world_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    printf("Process %i of %i\n", world_rank, world_size);

    ValidSolutionsStatistics* statistics = NULL;
    // only master node has to keep track of statistics
    if (world_rank == 0) {
        // allocate a data structure for tallying % of valid solutions / size
        statistics = calloc(
            (size_t)((MAX_PROBLEM_SIZE - MIN_PROBLEM_SIZE) + 1),
            sizeof(ValidSolutionsStatistics)
        );
        assert(statistics != NULL);
    }
    // allocate data structure for storing problem and solution bit strings
    bool* problem = calloc(MAX_PROBLEM_SIZE, sizeof(bool));
    bool* solution = calloc(MAX_PROBLEM_SIZE, sizeof(bool));
    // let it abort if any memory allocations were refused
    assert(problem != NULL);
    assert(solution != NULL);
    // keep track of program self-timing data
    TimingData timing_data = {0};

    // only the master node has to write out the results to file
    if (world_rank == 0) {
        // write out the CSV file row headings
        FILE* csv_file = open_file_for_appending(filename);
        fprintf(
            csv_file,
            "Timestamp,Bits,Problem Size,Lowest Validity,Highest Validity,Mean Validity\n"
        );
        csv_file = close_file(csv_file);
    }
    // for every size of problem...
    for (uint8_t z = MIN_PROBLEM_SIZE; z <= (MAX_PROBLEM_SIZE); z++) {
        // start the "stopwatch"
        stopwatch_start(&timing_data);
        // how many problems of that size exist
        uint32_t problem_size = two_to_the_power_of(z);
        // init highest, lowest and cumulative validity counters
        uint64_t lowest_validity = UINT64_MAX;
        uint64_t highest_validity = 0;
        uint64_t cumulative_validity = 0;
        // NOTE: we buffer problems to test in here until we have enough to scatter
        uint32_t* problems_buffer = NULL;
        uint32_t* solutions_buffer = NULL;
        size_t problems_buffer_size = 0;
        // the master node populates the buffer
        if (world_rank == 0) {
            problems_buffer = calloc((size_t)world_size, sizeof(uint32_t));
            solutions_buffer = calloc((size_t)world_size, sizeof(uint32_t));
            assert(problems_buffer != NULL);
            assert(solutions_buffer != NULL);
        }
        // for every problem of that size...
        for (uint32_t p = 0; p < problem_size; p++) {
            // on the master node, add this problem to the end of the buffer
            if (world_rank == 0) {
                problems_buffer[problems_buffer_size] = p;
            }
            problems_buffer_size++;
            // whenever we have enough problems stored in the buffer, scatter them
            if (problems_buffer_size == (size_t)world_size) {
                // scatter the buffer contents to all nodes (including us)
                uint32_t our_problem;
                MPI_Scatter(
                    problems_buffer,
                    1,
                    MPI_UINT32_T,
                    &our_problem,
                    1,
                    MPI_UINT32_T,
                    0,
                    MPI_COMM_WORLD
                );
                // brute-force our problem
                uint32_t solutions_to_problem = count_solutions_to_problem(
                    z,
                    problem_size,
                    our_problem,
                    problem,
                    solution
                );
                // gather, to return the data to the master node
                MPI_Gather(
                    &solutions_to_problem,
                    1,
                    MPI_UINT32_T,
                    solutions_buffer,
                    1,
                    MPI_UINT32_T,
                    0,
                    MPI_COMM_WORLD
                );
                // store results on master node only
                if (world_rank == 0) {
                    for (size_t i = 0; i < problems_buffer_size; i++) {
                        // update lowest, highest and cumulative total validity values
                        if (solutions_buffer[i] < lowest_validity) {
                            lowest_validity = solutions_buffer[i];
                        }
                        if (solutions_buffer[i] > highest_validity) {
                            highest_validity = solutions_buffer[i];
                        }
                        cumulative_validity += solutions_buffer[i];
                        // TODO: store lowest, highest and mean validity values with problem size
                    }
                }
                // reset buffer size
                problems_buffer_size = 0;
            }
        }
        // only collate data on master node
        if (world_rank == 0) {
            statistics[z].problem_size = z; // store size in bits, not raw size!
            statistics[z].lowest_validity = lowest_validity;
            statistics[z].highest_validity = highest_validity;
            /*
             * divide cumulative total validity by number of problems tested
             * this calculation produces the mean validity for this size
             */
            statistics[z].mean_validity = (long double)cumulative_validity / problem_size;
            // stop the "stopwatch"
            stopwatch_stop(&timing_data);
            time_t now = time(NULL);
            char time_buffer[21];
            strftime(time_buffer, sizeof(time_buffer), "%FT%TZ", gmtime(&now));
            // update file (on master node only)
            FILE* csv_file = open_file_for_appending(filename);
            fprintf(
                csv_file,
                "%s,%" PRIu8 ",%" PRIu32 ",%" PRIu64 ",%" PRIu64 ",%Lf\n",
                time_buffer,
                z,
                two_to_the_power_of(statistics[z].problem_size),
                statistics[z].lowest_validity,
                statistics[z].highest_validity,
                statistics[z].mean_validity
            );
            csv_file = close_file(csv_file);
            update_and_print_completion_estimate(&timing_data, z);
        }
    }
    // deallocate memory
    assert(statistics != NULL);
    assert(problem != NULL);
    assert(solution != NULL);
    // FIXME: for some reason, all of these free()s cause memory error crashes!
    // free(solution);
    // free(problem);
    // free(statistics);
    MPI_Finalize();
    return 0;
}
