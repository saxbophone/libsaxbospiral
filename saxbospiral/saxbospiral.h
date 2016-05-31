#ifndef SAXBOPHONE_SAXBOSPIRAL_SAXBOSPIRAL_H
#define SAXBOPHONE_SAXBOSPIRAL_SAXBOSPIRAL_H

#include <stddef.h>
#include <stdint.h>


#ifdef __cplusplus
extern "C"{
#endif

typedef struct version_t {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
} version_t;

extern const version_t VERSION;

// type for representing a cartesian direction
typedef uint8_t direction_t;

// cartesian direction constants
#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

// type for representing a rotational direction
typedef int8_t rotation_t;

// rotational direction constants
#define CLOCKWISE 1
#define ANTI_CLOCKWISE -1

// type for representing the length of a line segment of a spiral
typedef uint32_t length_t;

/* 
 * struct for representing one line segment in the spiral structure, including 
 * the direction of the line and it's length (initially set to 0) 
 * the whole struct uses bitfields to occupy 32 bits of memory
 */
typedef struct line_t {
    direction_t direction : 2; // as there are only 4 directions, use 2 bits
    length_t length : 30; // use 30 bits for the length, nice and long
} line_t;

typedef struct tuple_t {
    int64_t x;
    int64_t y;
} tuple_t;

typedef tuple_t vector_t;
typedef tuple_t co_ord_t;

typedef struct co_ord_array_t {
    co_ord_t * items;
    size_t size;
} co_ord_array_t;

typedef struct co_ord_cache_t {
    co_ord_array_t co_ords;
    size_t validity;
} co_ord_cache_t;

typedef struct spiral_t {
    size_t size;
    line_t * lines;
    co_ord_cache_t co_ord_cache;
} spiral_t;

typedef struct buffer_t {
    uint8_t * bytes;
    size_t size;
} buffer_t;

// vector direction constants
extern const vector_t VECTOR_DIRECTIONS[4];

#ifdef __cplusplus
} // extern "C"
#endif

// end of header file
#endif