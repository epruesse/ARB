/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
 * Copyright (c) 1992 Uri Blumenthal, IBM
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation. The authors make no representations about the suitability 
 * of this software for any purpose.  It is provided "as is" without express 
 * or implied warranty.
 *
 * THE AUTHORS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#define              ULIMIT_FONT_SIZE        300
#define	       DEFAULT_PICFONT	     	13
#define PICPSFONT(F)      (PICfontnames[ \
		PICPSfontmap[(((F)->font) <= MAXFONT(F)) ? \
		((F)->font)+1 : \
		DEFAULT_PICFONT]])
extern int v2_flag, v21_flag;
#define ROMAN_DEFAULT 0
#define ROMAN 	1
#define ITALIC 	2
#define BOLD    3
#define ITABOL  4
#define HELVET  5
#define HELBOL  6
#define HELOBL  7
#define HELBOB  8
#define COUR    9
#define COURBL  10
#define COUROB  11
#define COURBO  12
#define SYMBOL  13
#define BRAKET  14
int    PICPSfontmap[] = {
			ROMAN_DEFAULT, ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			COUR,
			COUROB,
			COURBL,
			COURBO,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			HELVET,
			HELOBL,
			HELBOL,
			HELBOB,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			ROMAN,
			ITALIC,
			BOLD,
			ITABOL,
			SYMBOL,
			ITALIC,
			BRAKET
		};
char		*PICfontnames[] = {
		"R", "R",     /* default */
		"I",
		"B",
		"BI",
		"H",
		"HB",
		"HO",
		"HX",
		"C",
		"CB",
		"CO",
		"CX",
		"S",
		"S2"
		};
