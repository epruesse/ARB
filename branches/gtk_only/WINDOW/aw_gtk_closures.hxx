/* 
 * File:   aw_gtk_closures.hxx
 * Author: mpimm
 *
 * Created on March 20, 2013, 2:14 PM
 * 
 * This file contains all gtk closure marshal functions.
 * 
 * A closure marshal function is used to translate arrays of 
 * parameters into callback invocations supported by C.
 * There are already a number of closure functions defined by glib.
 * Check the glib documentation first before adding a new function here.
 * 
 * The functions in this file are generated by the glib-genmarshal program.
 * It is recommended to use this program to generate closures.
 * 
 */

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

/* VOID:OBJECT,OBJECT (/dev/stdin:1) */
extern void g_cclosure_arb_marshal_VOID__OBJECT_OBJECT  (GClosure     *closure,
                                                         GValue       *return_value,
                                                         guint         n_param_values,
                                                         const GValue *param_values,
                                                         gpointer      invocation_hint,
                                                         gpointer      marshal_data);

G_END_DECLS