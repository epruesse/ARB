/* 
 * File:   aw_help.hxx
 * Author: aboeckma
 *
 * Created on December 6, 2012, 1:03 PM
 */

#pragma once

void AW_help_popup(UNFIXED, const char *help_file);
inline WindowCallback makeHelpCallback(const char *helpfile) { return makeWindowCallback(AW_help_popup, helpfile); }

void AW_insert_default_help_entries(AW_window*);
