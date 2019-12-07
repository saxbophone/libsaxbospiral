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
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#ifdef __cplusplus
#error "This file is ISO C99. It should not be compiled with a C++ Compiler."
#endif

typedef struct CommandLineOptions {
    uint8_t start_problem_size; // size of problem to start with in bits
    uint8_t end_problem_size; // size of problem to end with in bits
    size_t max_ram_per_process; // max RAM allowed per process in bytes
} CommandLineOptions;

/*
 * the data type used to store problems and solutions
 * NOTE: this must be able to store at least 2^N values where N is the maximum
 * desired problem size in bits
 */
typedef uint64_t RepresentationBase;
typedef RepresentationBase Problem;
typedef RepresentationBase Solution;

/*
 * stores all the valid solutions for a given problem
 * (problem is not stored in this particular struct)
 */
typedef struct SolutionSet {
    size_t count; // how many solutions there are
    Solution* solutions; // dynamically allocated array of count many solutions
} SolutionSet;

typedef struct ProblemSet {
    uint8_t bits; // how many bits wide these problems are
    size_t count; // how many problems there are
    SolutionSet* problem_solutions; // dynamic array, problem is index number
} ProblemSet;

// private constants

// these constants are calculated from A-B-exponential regression on search data
static const long double MEAN_VALIDITY_A_B_EXPONENTIAL_REGRESSION_CURVE_A = 1.56236069184829962203;
static const long double MEAN_VALIDITY_A_B_EXPONENTIAL_REGRESSION_CURVE_B = 0.8329257011252032045966;

// private functions which are used directly by main()

// attempts to parse command line and return options, calls exit() if bad args
static CommandLineOptions parse_command_line_options(
    int argc,
    char const *argv[]
);

/*
 * finds the largest problem size (in bits) which can be represented with the
 * given RAM limit per-process
 */
static uint8_t find_largest_cacheable_problem_size(size_t ram_limit);

int main(int argc, char const *argv[]) {
    CommandLineOptions options = parse_command_line_options(argc, argv);
    uint8_t largest_cacheable = find_largest_cacheable_problem_size(
        options.max_ram_per_process
    );
    printf("Maximum cacheable problem size: %" PRIu8 " bits\n", largest_cacheable);
    return 0;
}

/*
 * private functions which are used only by other private functions which are
 * used directly by main()
 */

/*
 * returns the number of bytes needed to cache the solutions to all problems of
 * given problem size
 */
static size_t get_cache_size_of_problem(uint8_t problem_size);

/*
 * generate all the problems and solutions of a given bit size and populate the
 * given problem_set with them
 */
static bool generate_problems_and_solutions(
    ProblemSet* problem_set,
    uint8_t bits
);

/*
 * returns the expected number of mean valid solutions per problem for the given
 * problem size in bits
 */
static size_t predict_number_of_valid_solutions(uint8_t problem_size);

/*
 * almost too simple to put in a function, but added for readability.
 * quickly calculate powers of two
 * NOTE: this overflows without warning if a power greater than 1 less of the
 * maximum bit width of integer supported by the system is given. (so a 64-bit
 * system will overflow if 64 is passed).
 */
static uintmax_t two_to_the_power_of(uint8_t power);

/*
 * uses A-B-exponential using magic constants derived from regression of
 * existing exhaustive test data to get the validity percentage for a problem of
 * a given size in bits
 * float percentage is returned where 0.0 is 0% and 1.0% is 100%
 */
static long double mean_validity(uint8_t problem_size);

// implementations of all private functions

static CommandLineOptions parse_command_line_options(
    int argc,
    char const *argv[]
) {
    if (argc < 4) exit(-1); // we need 3 additional arguments besides file name
    CommandLineOptions options = {0};
    options.start_problem_size = strtoul(argv[1], NULL, 10);
    options.end_problem_size = strtoul(argv[2], NULL, 10);
    options.max_ram_per_process = strtoul(argv[3], NULL, 10);
    if (
        options.start_problem_size == 0
        || options.end_problem_size == 0
        || options.max_ram_per_process == 0
    ) {
        exit(-1); // none of them can be zero
    }
    return options;
}

static uint8_t find_largest_cacheable_problem_size(size_t ram_limit) {
    uint8_t problem_size;
    // this typically won't actually get to 32, 22 bits gives ~1TiB size!
    for (problem_size = 1; problem_size < 32; problem_size++) {
        size_t cache_size = get_cache_size_of_problem(problem_size);
        // XXX: debug
        printf("%02" PRIu8 ": %10zu\n", problem_size, cache_size);
        // stop when we find a problem size that exceeds our ram limit
        if (cache_size > ram_limit) return problem_size - 1;
    }
    // probably will never be reached, but in case it is, return something
    return problem_size - 1;
}

static size_t get_cache_size_of_problem(uint8_t problem_size) {
    if (problem_size < 6) { // problem sizes below this do not follow the trend
        return (
            sizeof(ProblemSet) + (
                two_to_the_power_of(problem_size) * // number of problems
                (
                    sizeof(SolutionSet) +
                    (
                        two_to_the_power_of(problem_size) * // solutions
                        sizeof(Solution)
                    )
                )
            )
        );
    }
    // all problems of size 6 or above do follow the trend line we have plotted
    return (
        sizeof(ProblemSet) + (
            two_to_the_power_of(problem_size) * // number of problems
            (
                sizeof(SolutionSet) +
                (
                    predict_number_of_valid_solutions(problem_size) * // solutions
                    sizeof(Solution)
                )
            )
        )
    );
}

static size_t predict_number_of_valid_solutions(uint8_t problem_size) {
    return (size_t)ceill( // round up for a conservative estimate
        two_to_the_power_of(problem_size) * mean_validity(problem_size)
    );
}

static uintmax_t two_to_the_power_of(uint8_t power) {
    return 1U << power;
}

static long double mean_validity(uint8_t problem_size) {
    // A-B-exponential regression means that value is `A * B^problem_size`
    return (
        MEAN_VALIDITY_A_B_EXPONENTIAL_REGRESSION_CURVE_A *
        powl(MEAN_VALIDITY_A_B_EXPONENTIAL_REGRESSION_CURVE_B, problem_size)
    );
}
