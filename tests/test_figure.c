/*
 * This source file forms part of sxbp, a library which generates experimental
 * 2D spiral-like shapes based on input binary data.
 *
 * This compilation unit provides unit tests for the sxbp_fiigure_t data type.
 *
 * Copyright (C) Joshua Saxby <joshua.a.saxby@gmail.com> 2016-2017, 2018
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "check_wrapper.h"

#include "../sxbp/sxbp.h"

#include "test_suites.h"


START_TEST(test_blank_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_init_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_init_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_free_figure_unallocated) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_free_figure_allocated) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_copy_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_copy_figure_from_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_copy_figure_to_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_begin_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_begin_figure_data_too_big) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_begin_figure_data_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_begin_figure_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_refine_figure) {
    /*
     * NOTE: This function may be rather hard to unit-test as it suffers from
     * the halting problem
     */
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_refine_figure_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_refine_figure_no_lines) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_refine_figure_unimplemented_method) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_dump_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_dump_figure_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_dump_figure_buffer_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_load_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_load_figure_buffer_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_load_figure_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_buffer_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_render_callback_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_pbm) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_pbm_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_pbm_buffer_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_svg) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_svg_figure_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

START_TEST(test_render_figure_to_svg_buffer_null) {
    // TODO: replace this body with actual test code
    ck_abort_msg("Test not implemented!");
} END_TEST

Suite* make_figure_suite(void) {
    // Test cases for figure data type
    Suite* test_suite = suite_create("Figure");

    TCase* blank_figure = tcase_create("sxbp_blank_figure()");
    tcase_add_test(blank_figure, test_blank_figure);
    suite_add_tcase(test_suite, blank_figure);

    TCase* init_figure = tcase_create("sxbp_init_figure()");
    tcase_add_test(init_figure, test_init_figure);
    tcase_add_test(init_figure, test_init_figure_null);
    suite_add_tcase(test_suite, init_figure);

    TCase* free_figure = tcase_create("sxbp_free_figure()");
    tcase_add_test(free_figure, test_free_figure_unallocated);
    tcase_add_test(free_figure, test_free_figure_allocated);
    suite_add_tcase(test_suite, free_figure);

    TCase* copy_figure = tcase_create("sxbp_copy_figure()");
    tcase_add_test(copy_figure, test_copy_figure);
    tcase_add_test(copy_figure, test_copy_figure_from_null);
    tcase_add_test(copy_figure, test_copy_figure_to_null);
    suite_add_tcase(test_suite, copy_figure);

    TCase* begin_figure = tcase_create("sxbp_begin_figure()");
    tcase_add_test(begin_figure, test_begin_figure);
    tcase_add_test(begin_figure, test_begin_figure_data_too_big);
    tcase_add_test(begin_figure, test_begin_figure_data_null);
    tcase_add_test(begin_figure, test_begin_figure_figure_null);
    suite_add_tcase(test_suite, begin_figure);

    TCase* refine_figure = tcase_create("sxbp_refine_figure()");
    tcase_add_test(refine_figure, test_refine_figure);
    tcase_add_test(refine_figure, test_refine_figure_figure_null);
    tcase_add_test(refine_figure, test_refine_figure_no_lines);
    tcase_add_test(refine_figure, test_refine_figure_unimplemented_method);
    suite_add_tcase(test_suite, refine_figure);

    TCase* dump_figure = tcase_create("sxbp_dump_figure()");
    tcase_add_test(dump_figure, test_dump_figure);
    tcase_add_test(dump_figure, test_dump_figure_figure_null);
    tcase_add_test(dump_figure, test_dump_figure_buffer_null);
    suite_add_tcase(test_suite, dump_figure);

    TCase* load_figure = tcase_create("sxbp_load_figure()");
    tcase_add_test(load_figure, test_load_figure);
    tcase_add_test(load_figure, test_load_figure_buffer_null);
    tcase_add_test(load_figure, test_load_figure_figure_null);
    suite_add_tcase(test_suite, load_figure);

    TCase* render_figure = tcase_create("sxbp_render_figure()");
    tcase_add_test(render_figure, test_render_figure);
    tcase_add_test(render_figure, test_render_figure_figure_null);
    tcase_add_test(render_figure, test_render_figure_buffer_null);
    tcase_add_test(render_figure, test_render_figure_render_callback_null);
    suite_add_tcase(test_suite, render_figure);

    TCase* render_figure_to_null = tcase_create("sxbp_render_figure_to_null()");
    tcase_add_test(render_figure_to_null, test_render_figure_to_null);
    suite_add_tcase(test_suite, render_figure_to_null);

    TCase* render_figure_to_pbm = tcase_create("sxbp_render_figure_to_pbm()");
    tcase_add_test(render_figure_to_pbm, test_render_figure_to_pbm);
    tcase_add_test(render_figure_to_pbm, test_render_figure_to_pbm_figure_null);
    tcase_add_test(render_figure_to_pbm, test_render_figure_to_pbm_buffer_null);
    suite_add_tcase(test_suite, render_figure_to_pbm);

    TCase* render_figure_to_svg = tcase_create("sxbp_render_figure_to_svg()");
    tcase_add_test(render_figure_to_svg, test_render_figure_to_svg);
    tcase_add_test(render_figure_to_svg, test_render_figure_to_svg_figure_null);
    tcase_add_test(render_figure_to_svg, test_render_figure_to_svg_buffer_null);
    suite_add_tcase(test_suite, render_figure_to_svg);

    return test_suite;
}
