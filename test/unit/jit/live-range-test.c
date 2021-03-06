/*
 * Copyright (c) 2008  Pekka Enberg
 * 
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "jit/vars.h"
#include <libharness.h>

void test_empty_range_does_not_contain_anything(void)
{
	struct live_range range = { .start = 0, .end = 0 };

	assert_false(in_range(&range, 0));
	assert_false(in_range(&range, 1));
}

void test_empy_range_is_empty(void)
{
	struct live_range range = { .start = 0, .end = 0 };

	assert_true(range_is_empty(&range));
}

void test_range_length_treats_end_as_exclusive(void)
{
	struct live_range range = { .start = 0, .end = 2 };

	assert_int_equals(2, range_len(&range));
}

void test_in_range_treats_end_as_exclusive(void)
{
	struct live_range range = { .start = 0, .end = 2 };

	assert_true(in_range(&range, 0));
	assert_true(in_range(&range, 1));
	assert_false(in_range(&range, 2));
}

void test_range_that_is_within_another_range_intersects(void)
{
	struct live_range range1 = { .start = 0, .end = 3 };
	struct live_range range2 = { .start = 1, .end = 2 };

	assert_true(ranges_intersect(&range1, &range2));
	assert_true(ranges_intersect(&range2, &range1));
}

void test_ranges_that_intersect(void)
{
	struct live_range range1 = { .start = 0, .end = 2 };
	struct live_range range2 = { .start = 1, .end = 3 };

	assert_true(ranges_intersect(&range1, &range2));
	assert_true(ranges_intersect(&range2, &range1));
}

void test_ranges_that_do_not_intersect(void)
{
	struct live_range range1 = { .start = 0, .end = 2 };
	struct live_range range2 = { .start = 2, .end = 4 };

	assert_false(ranges_intersect(&range1, &range2));
	assert_false(ranges_intersect(&range2, &range1));
}

void test_interval_add_range(void)
{
#if 0
	struct live_interval it;
	struct live_range *r;

	INIT_LIST_HEAD(&it.range_list);

	interval_add_range(&it, 1, 3);
	r = interval_first_range(&it);
	assert_int_equals(1, r->start);
	assert_int_equals(3, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));

	interval_add_range(&it, 5, 7);
	r = interval_first_range(&it);
	assert_int_equals(1, r->start);
	assert_int_equals(3, r->end);
	r = next_range(&it.range_list, r);
	assert_int_equals(5, r->start);
	assert_int_equals(7, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));

	interval_add_range(&it, 3, 5);
	r = interval_first_range(&it);
	assert_int_equals(1, r->start);
	assert_int_equals(7, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));

	interval_add_range(&it, 7, 8);
	r = interval_first_range(&it);
	assert_int_equals(1, r->start);
	assert_int_equals(8, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));

	interval_add_range(&it, 10, 13);
	r = interval_first_range(&it);
	assert_int_equals(1, r->start);
	assert_int_equals(8, r->end);
	r = next_range(&it.range_list, r);
	assert_int_equals(10, r->start);
	assert_int_equals(13, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));

	interval_add_range(&it, 0, 14);
	r = interval_first_range(&it);
	assert_int_equals(0, r->start);
	assert_int_equals(14, r->end);
	assert_ptr_equals(NULL, next_range(&it.range_list, r));
#endif
}
