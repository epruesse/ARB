/* 
 * File:   aw_assert.hxx
 * Author: aboeckma
 *
 * Created on November 1, 2012, 2:59 PM
 */

#pragma once

#ifndef ARB_ASSERT_H
#include <arb_assert.h>
#endif

#include <glib.h>

// we use the glib warning / logging system. so these
// are just a couple of defines to rename the calls

#ifdef aw_assert
#error do not redefine aw_assert!
#endif

#define aw_return_if_fail(cond) g_return_if_fail(cond)
#define aw_return_val_if_fail(cond, val) g_return_val_if_fail(cond, val)
#define aw_return_if_reached() g_return_if_reached()
#define aw_return_val_if_reached(val) g_return_val_if_reached(val)
#define aw_warn_if_fail(cond) g_warn_if_fail(cond)
#define aw_warn_if_reached() g_warn_if_reached()
#define aw_debug g_debug
#define AW_BREAKPOINT G_BREAKPOINT

#define aw_assert(cond) aw_warn_if_fail(cond)
