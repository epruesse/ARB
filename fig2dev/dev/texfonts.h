/*
 * TransFig: Facility for Translating Fig code
 * Copyright (c) 1985 Supoj Sutantavibul
 * Copyright (c) 1991 Micah Beck
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

static char		*texfontnames[] = {
			"rm",		/* default */
			"rm",			/* roman */
			"bf",			/* bold */
			"it",			/* italic */
			"sf", 			/* sans serif */
			"tt"			/* typewriter */
		};

/* The selection of font names may be site dependent.
 * Not all fonts are preloaded at all sizes.
 */

static char		texfontsizes[] = {
                       11,            /* default */
                       5, 5, 5, 5,    /* 1-4: small fonts */
                       5,			/* five point font */
                       6, 7, 8,	/* etc */
                       9, 10, 11,
                       12, 12, 14,
                       14, 14, 17,
                       17, 17, 20,
                       20, 20, 20, 20, 25,
                       25, 25, 25, 29,
                       29, 29, 29, 29,
                       34, 34, 34, 34,
                       34, 34, 34, 41,
                       41, 41
  			};

#define MAXFONTSIZE 	42

#define TEXFONT(F)	(texfontnames[((F) <= MAX_FONT) ? (F) : (MAX_FONT-1)])
#define TEXFONTSIZE(S)	(texfontsizes[((S) <= MAXFONTSIZE) ? round(S)\
				      				: (MAXFONTSIZE-1)])
#define TEXFONTMAG(T)	TEXFONTSIZE(T->size*(rigid_text(T) ? 1.0 : mag))

