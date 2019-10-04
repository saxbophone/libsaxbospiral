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
    uint32_t range; // the number of discrete values to pick from
    uint32_t samples; // how many values we will pick from it
    uint32_t next; // the next value we will attempt to pick
    uint32_t chosen; // how many we have chosen so far
} ProblemGenerator;

// private data structure for tracking program self-timing state
typedef struct TimingData {
    long double last_estimate; // the last time estimate we made
    long double seconds_elapsed; // the amount of time elapsed in the current timing period
    time_t start_time;
} TimingData;

// private structure used for tracking validity data before it has been finished
typedef struct BookKeepingData {
    uint64_t lowest_validity;
    uint64_t highest_validity;
    uint64_t cumulative_validity;
} BookKeepingData;

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
    // divide validity count by the number of samples in the data
} ValidSolutionsStatistics;

static const long double MINUTE_SECONDS = 60.0L;
static const long double HOUR_SECONDS = 60.0L * 60.0L;
static const long double DAY_SECONDS = 60.0L * 60.0L * 24.0L;
static const long double WEEK_SECONDS = 60.0L * 60.0L * 24.0L * 7.0L;
static const long double MONTH_SECONDS = 60.0L * 60.0L * 24.0L * 30.44L;
static const long double YEAR_SECONDS = 60.0L * 60.0L * 24.0L * 365.2425L;

static uint32_t two_to_the_power_of(uint8_t power) {
    return (uint32_t)powl(2.0L, (long double)power);
}

static ProblemGenerator init_problem_generator(
    uint32_t range, // the number of discrete values to pick from
    uint32_t samples // how many values to pick
) {
    ProblemGenerator generator = {
        .range = range,
        .samples = samples,
        .next = 0,
        .chosen = 0,
    };
    return generator;
}

// picks the next problem out of the problem generator
static uint32_t get_next_problem(ProblemGenerator* generator) {
    // NOTE: cheap error-handling
    if (generator->chosen == generator->samples) {
        abort(); // run out of samples!
    }
    uint32_t picked_value = 0;
    bool value_found = false;
    do {
        /*
         * WHERE N is the total number of values to pick from and M the sample size:
         * the probability of displaying each number starts at N/M, but changes
         * depending on how many left we need to pick and how many we have left
         * to pick from --it is this property which gaurantees we will pick
         * exactly the right number of integers.
         */
        long double chance = (long double)(generator->samples - generator->chosen) / (generator->range - generator->next);
        long double coin_flip = (long double)rand() / RAND_MAX;
        if (coin_flip <= chance) {
            // this number has been chosen!
            picked_value = generator->next;
            // increment chosen count
            generator->chosen++;
            // mark value as picked
            value_found = true;
        }
        // always increment the value that comes next
        generator->next++;
    } while (!value_found);

    return picked_value;
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

static void init_book_keeping_data(BookKeepingData* book_keeping_data) {
    book_keeping_data->lowest_validity = UINT64_MAX;
    book_keeping_data->highest_validity = 0;
    book_keeping_data->cumulative_validity = 0;
}

static void update_book_keeping_data(
    BookKeepingData* book_keeping_data,
    uint32_t buffer[],
    size_t buffer_size
) {
    for (size_t i = 0; i < buffer_size; i++) {
        // update lowest, highest and cumulative total validity values
        if (buffer[i] < book_keeping_data->lowest_validity) {
            book_keeping_data->lowest_validity = buffer[i];
        }
        if (buffer[i] > book_keeping_data->highest_validity) {
            book_keeping_data->highest_validity = buffer[i];
        }
        book_keeping_data->cumulative_validity += buffer[i];
        // TODO: store lowest, highest and mean validity values with problem size
    }
}

// returns the most convenient way of describing a given time unit
static const char* convenient_time_unit(long double seconds) {
    if (seconds < MINUTE_SECONDS) {
        return "s";
    } else if (seconds < HOUR_SECONDS) {
        return " mins";
    } else if (seconds < DAY_SECONDS) {
        return " hours";
    } else if (seconds < WEEK_SECONDS) {
        return " days";
    } else if (seconds < MONTH_SECONDS) {
        return " weeks";
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
    } else if (seconds < WEEK_SECONDS) {
        return seconds / DAY_SECONDS;
    } else if (seconds < MONTH_SECONDS) {
        return seconds / WEEK_SECONDS;
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

static void update_and_print_completion_estimate(TimingData* timing_data, uint8_t last_solved, uint8_t max_problem_size) {
    time_t now = time(NULL);
    char time_buffer[21];
    strftime(time_buffer, sizeof(time_buffer), "%FT%TZ", gmtime(&now));
    // print error of estimate
    printf("============================= %s =============================\n", time_buffer);
    printf(
        "Solved problem size: %" PRIu8
        " - Time taken:\t%Lf%s (%.2Lf%% of estimate)\n",
        last_solved,
        convenient_time_value(timing_data->seconds_elapsed),
        convenient_time_unit(timing_data->seconds_elapsed),
        ((timing_data->seconds_elapsed / timing_data->last_estimate) - 0.0L) * 100.0L
    );
    long double completion_estimate = estimated_completion_time(timing_data->seconds_elapsed, last_solved, max_problem_size - last_solved);
    printf(
        "Estimated time til completion:\t\t%Lf%s\n",
        convenient_time_value(completion_estimate),
        convenient_time_unit(completion_estimate)
    );
    if (last_solved < max_problem_size) {
        timing_data->last_estimate = estimated_completion_time(timing_data->seconds_elapsed, last_solved, 1);
        printf(
            "Estimated time til next solved:\t\t%Lf%s\n",
            convenient_time_value(timing_data->last_estimate),
            convenient_time_unit(timing_data->last_estimate)
        );
    }
    printf("================================================================================\n\n");
}

static void log_node_message(char* message) {
    int rank, name_length;
    char name[MPI_MAX_PROCESSOR_NAME];
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(name, &name_length);
    // use of MPI_Barrier() makes it less likely for output to interleave
    MPI_Barrier(MPI_COMM_WORLD);
    printf("[%s:%d] %s\n", name, rank, message);
    MPI_Barrier(MPI_COMM_WORLD);
}

int main(int argc, char *argv[]) {
    // initialise MPI and discover our place in the world
    MPI_Init(&argc, &argv);
    // seed the mediocre random number generator for picking problems randomly
    srand((unsigned int)time(NULL));
    /*
     * we need three additional arguments:
     * - the filename to write output to
     * - the minimum size of problem to solve (bits)
     * - the maximum size of problem to solve (bits)
     */
    if (argc < 4) {
        fprintf(stderr, "Need filename, min and max problem size arguments!\n");
        return -1;
    }
    const char* filename = argv[1]; // grab filename out of arguments
    const uint8_t min_problem_size = strtoul(argv[2], NULL, 10);
    const uint8_t max_problem_size = strtoul(argv[3], NULL, 10);
    // TODO: change sample proportion to be retrieved from the command-line
    double sample_proportion = 1.0;
    // sanity-checks on the chosen problem sizes
    if (
        !(min_problem_size > 0) || // no point testing a problem of size 0
        !(min_problem_size <= max_problem_size) || // max mustn't be < min
        !(max_problem_size <= 32) // this program works on problem sizes up to 32 bits
    ) {
        fprintf(stderr, "Invalid values or combination of min/max problem sizes!\n");
        fprintf(stderr, "Min must be smaller than or equal to max and both must be no greater than 32\n");
        return -1;
    }
    size_t world_rank, world_size;
    {
        int signed_world_rank, signed_world_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &signed_world_rank);
        MPI_Comm_size(MPI_COMM_WORLD, &signed_world_size);
        world_rank = (size_t)signed_world_rank;
        world_size = (size_t)signed_world_size;
    }
    log_node_message("START");

    ValidSolutionsStatistics* statistics = NULL;
    // only master node has to keep track of statistics
    if (world_rank == 0) {
        // allocate a data structure for tallying % of valid solutions / size
        statistics = calloc(
            (size_t)((max_problem_size - min_problem_size) + 1),
            sizeof(ValidSolutionsStatistics)
        );
        if (statistics == NULL) abort();
    }
    // allocate data structure for storing problem and solution bit strings
    bool* problem = calloc(max_problem_size, sizeof(bool));
    bool* solution = calloc(max_problem_size, sizeof(bool));
    // let it abort if any memory allocations were refused
    if (problem == NULL) abort();
    if (solution == NULL) abort();
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
    for (uint8_t z = min_problem_size; z <= (max_problem_size); z++) {
        if (world_rank == 0) {
            // start the "stopwatch"
            stopwatch_start(&timing_data);
        }
        // how many problems of that size exist
        uint32_t problem_size = two_to_the_power_of(z);
        uint32_t sample_size = problem_size * sample_proportion; // how many problems will we test?
        // "turns" is how many times we need all nodes to work to solve problems
        uint32_t turns = sample_size / world_size;
        // "extra" is the remainder of problems / nodes
        uint32_t extra = sample_size % world_size;
        // this generates problems for us to solve
        ProblemGenerator problem_generator = init_problem_generator(problem_size, sample_size);
        // init highest, lowest and cumulative validity counters
        BookKeepingData book_keeping_data = {0};
        init_book_keeping_data(&book_keeping_data);
        // NOTE: we buffer problems to test in here until we have enough to scatter
        uint32_t* problems_buffer = NULL;
        uint32_t* solutions_buffer = NULL;
        // only the master node needs to allocate these buffers
        if (world_rank == 0) {
            problems_buffer = calloc(world_size, sizeof(uint32_t));
            solutions_buffer = calloc(world_size, sizeof(uint32_t));
            if (problems_buffer == NULL) abort();
            if (solutions_buffer == NULL) abort();
        }
        // for every "turn", populate a full buffer of problems and distribute
        for (uint32_t t = 0; t < turns; t++) {
            if (world_rank == 0) {
                for (uint32_t n = 0; n < world_size; n++) {
                    problems_buffer[n] = get_next_problem(&problem_generator);
                }
            }
            // Scatter problems
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
            // Solve this node's problem
            uint32_t solutions_to_problem = count_solutions_to_problem(
                z,
                problem_size,
                our_problem,
                problem,
                solution
            );
            // Gather results
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
            if (world_rank == 0) {                
                // Update book-keeping
                update_book_keeping_data(&book_keeping_data, solutions_buffer, world_size);
            }
        }

        // only process "extra" problems if there are any
        if (extra > 0) {
            // allocate an array of ints, which tell MPI_Scatterv which nodes to use
            int* send_counts = calloc(world_size, sizeof(int));
            // this array tells MPI_Scatterv the index of each item to send
            int* displacements = calloc(world_size, sizeof(int));
            // lowly allocation failure handling!
            if (send_counts == NULL) abort();
            if (displacements == NULL) abort();
            // for every "extra" turn, add an additional item to the buffer
            for (uint32_t e = 0; e < extra; e++) {
                if (world_rank == 0) {
                    problems_buffer[e] = get_next_problem(&problem_generator);
                }
                send_counts[e] = 1; // send to this process!
                displacements[e] = (int)e; // 0, 1, 2... etc...
            }
            // Scatter problems, with *Scatterv* so not all nodes receive one
            uint32_t our_problem = 0;
            MPI_Scatterv(
                problems_buffer,
                send_counts,
                displacements,
                MPI_UINT32_T,
                &our_problem,
                send_counts[world_rank],
                MPI_UINT32_T,
                0,
                MPI_COMM_WORLD
            );
            uint32_t solutions_to_problem = 0;
            // solve extra problems only on the nodes that were selected to do
            if (world_rank < extra) {
                solutions_to_problem = count_solutions_to_problem(
                    z,
                    problem_size,
                    our_problem,
                    problem,
                    solution
                );
            }
            // Gather problems with *Gatherv* because not all nodes send one
            MPI_Gatherv(
                &solutions_to_problem,
                send_counts[world_rank],
                MPI_UINT32_T,
                solutions_buffer,
                send_counts,
                displacements,
                MPI_UINT32_T,
                0,
                MPI_COMM_WORLD
            );
            if (world_rank == 0) {
                // Update book-keeping
                update_book_keeping_data(&book_keeping_data, solutions_buffer, extra);
            }
            // clean up allocated memory
            free(send_counts);
            free(displacements);
        }
        // update statistics on master node only
        if (world_rank == 0) {
            statistics[z - min_problem_size].problem_size = z; // store size in bits, not raw size!
            statistics[z - min_problem_size].lowest_validity = book_keeping_data.lowest_validity;
            statistics[z - min_problem_size].highest_validity = book_keeping_data.highest_validity;
            /*
             * divide cumulative total validity by number of problems tested
             * this calculation produces the mean validity for this size
             */
            statistics[z - min_problem_size].mean_validity = (long double)book_keeping_data.cumulative_validity / sample_size;
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
                two_to_the_power_of(statistics[z - min_problem_size].problem_size),
                statistics[z - min_problem_size].lowest_validity,
                statistics[z - min_problem_size].highest_validity,
                statistics[z - min_problem_size].mean_validity
            );
            csv_file = close_file(csv_file);
            update_and_print_completion_estimate(&timing_data, z, max_problem_size);
        }


        // XXX: end of new implementation (old one below)
    }
    // deallocate memory
    // TODO: unable to deallocate memory without crashing for some reason
    MPI_Finalize();
    return 0;
}
