/* 
 * File:   aw_help.hxx
 * Author: aboeckma
 *
 * Created on December 6, 2012, 1:03 PM
 */

#pragma once

void AW_help_popup(AW_window *aw, const char *help_file);
__ATTR__DEPRECATED("use AW_help_popup or makeHelpCallback") inline void AW_POPUP_HELP(AW_window *aw, AW_CL helpfile) { AW_help_popup(aw, (const char *)helpfile); }
inline WindowCallback makeHelpCallback(const char *helpfile) { return makeWindowCallback(AW_help_popup, helpfile); }

void AW_insert_default_help_entries(AW_window*);
