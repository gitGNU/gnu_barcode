/*
 * ps.c -- printing the "partial" bar encoding
 *
 * Copyright (c) 1999 Alessandro Rubini (rubini@gnu.org)
 * Copyright (c) 1999 Prosa Srl. (prosa@prosa.it)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "barcode.h"

#define SHRINK_AMOUNT 0.15 /* shrink the bars to account for ink spreading */


/*
 * How do the "partial" and "textinfo" strings work?
 *
 * The first char in "partial" tells how much extra space to add to the
 * left of the bars (for EAN-13, it is used to leave space to print the
 * first digit), other codes may have '0' for no-extra-space-needed.
 *
 * The next characters are alternating bars and spaces, as multiples
 * of the base dimension which is 1 unless the code is
 * rescaled. Rescaling is calculated as the ratio from the requested
 * width and the calculated width.  Digits represent bar/space
 * dimensions. Lower-case letters represent those bars that should
 * extend lower than the others: 'a' is equivalent to '1', 'b' is '2' and
 * so on.
 *
 * The "textinfo" string is made up of fields "%i:%i:%c" separated by
 * blank space. The first integer is the x position of the character,
 * the second is the font size (before rescaling) and the char item is
 * the charcter to be printed.
 *
 * Both the "partial" and "textinfo" strings may include "-" or "+" as
 * special characters (in "textinfo" the char should be a standalone
 * word).  They state where the text should be printed: below the bars
 * ("-", default) or above the bars. This is used, for example, to
 * print the add-5 and add-2 codes to the right of UPC or EAN codes
 * (the add-5 extension is mostly used in ISBN codes.
 */


int Barcode_ps_print(struct Barcode_Item *bc, FILE *f)
{
    int i, j, k, barlen, printable=1;
    double f1, f2, fsav=0;
    int mode = '-'; /* text below bars */
    double scalef=1, xpos, x0, y0, yr;
    unsigned char *ptr;
    unsigned char c;

    if (!bc->partial || !bc->textinfo) {
	bc->error = EINVAL;
	return -1;
    }


    /*
     * Maybe this first part can be made common to several printing back-ends,
     * we'll see how that works when other ouput engines are added
     */

    /* First, calculate barlen */
    barlen = bc->partial[0] - '0';
    for (ptr = bc->partial+1; *ptr; ptr++)
	if (isdigit(*ptr)) 
	    barlen += (*ptr - '0');
	else if (islower(*ptr))
	    barlen += (*ptr - 'a'+1);

    /* The scale factor depends on bar length */
    if (!bc->scalef) {
        if (!bc->width) bc->width = barlen; /* default */
        scalef = bc->scalef = (double)bc->width / (double)barlen;
    }

    /* The width defaults to "just enough" */
    if (!bc->width) bc->width = barlen * scalef +1;

    /* But it can be too small, in this case enlarge and center the area */
    if (bc->width < barlen * scalef) {
        int wid = barlen * scalef + 1;
        bc->xoff -= (wid - bc->width)/2 ;
        bc->width = wid;
        /* Can't extend too far on the left */
        if (bc->xoff < 0) {
            bc->width += -bc->xoff;
            bc->xoff = 0;
        }
    }

    /* The height defaults to 80 points (rescaled) */
    if (!bc->height) bc->height = 80 * scalef;

    /* If too small (20 + text), enlarge and center */
    i = 20 + 20 * ((bc->flags & BARCODE_NO_ASCII)==0);
    if (bc->height < i * scalef ) {
	int hei = i * scalef;
        bc->yoff -= hei/2;
        bc->height = hei;
        if (bc->yoff < 0) {
            bc->height += -bc->yoff;
            bc->yoff = 0;
        }
    }

    /*
     * Ok, then deal with actual ps (eps) output
     */

    if (!(bc->flags & BARCODE_OUT_NOHEADERS)) { /* spit a header first */
	if (bc->flags & BARCODE_OUT_EPS) 
	    fprintf(f, "%%!PS-Adobe-2.0 EPSF-1.2\n");
	else
	    fprintf(f, "%%!PS-Adobe-2.0\n");
	fprintf(f, "%%%%Creator: libbarcode\n");
	if (bc->flags & BARCODE_OUT_EPS)  {
	    fprintf(f, "%%%%BoundingBox: %i %i %i %i\n",
		    bc->xoff,
		    bc->yoff,
		    bc->xoff + bc->width + 2* bc->margin,
		    bc->yoff + bc->height + 2* bc->margin);
	}
	fprintf(f, "%%%%EndComments\n");
	if (bc->flags & BARCODE_OUT_PS)  {
	    fprintf(f, "%%%%EndProlog\n\n");
	    fprintf(f, "%%%%Page: 1 1\n\n");
	}
    }

    /* Print some informative comments */
    for (i=0; bc->ascii[i]; i++)
	if (bc->ascii[i] < ' ')
	    printable = 0;

    fprintf(f,"%% Printing barcode for \"%s\", scaled %5.2f",
	    printable ? bc->ascii : "<unprintable string>", scalef);
    if (bc->encoding)
	fprintf(f,", encoded using \"%s\"",bc->encoding);
    fprintf(f, "\n");
    fprintf(f,"%% The space/bar succession is represented "
	    "by the following widths (space first):\n"
	    "%% ");
    for (i=0; i<strlen(bc->partial); i++) {
        unsigned char c = bc->partial[i];
	if (isdigit(c)) putc(c, f);
	if (islower(c)) putc(c-'a'+'1', f);
	if (isupper(c)) putc(c-'A'+'1', f);
    }
    putc('\n', f);

    xpos = bc->margin + (bc->partial[0]-'0') * scalef;
    for (ptr = bc->partial+1, i=1; *ptr; ptr++, i++) {
	/* special cases: '+' and '-' */
	if (*ptr == '+' || *ptr == '-') {
	    mode = *ptr; /* don't count it */ i++; continue;
	}
	/* j is the width of this bar/space */
	if (isdigit (*ptr))   j = *ptr-'0';
	else                  j = *ptr-'a'+1;
	if (i%2) { /* bar */
	    x0 = bc->xoff + xpos + (j*scalef)/2;
            y0 = bc->yoff + bc->margin;
            yr = bc->height;
            if (!(bc->flags & BARCODE_NO_ASCII)) { /* leave space for text */
		if (mode == '-') {
		    /* text below bars: 10 points or five points */
		    y0 += (isdigit(*ptr) ? 10 : 5) * scalef;
		    yr -= (isdigit(*ptr) ? 10 : 5) * scalef;
		} else { /* '+' */
		    /* text above bars: 10 or 0 from bottom, and 10 from top */
		    y0 += (isdigit(*ptr) ? 10 : 0) * scalef;
		    yr -= (isdigit(*ptr) ? 20 : 10) * scalef; 
		}
	    }
            fprintf(f,"%5.2f setlinewidth "
		    "%6.2f %6.2f moveto "
		    "0 %5.2f rlineto stroke\n",
		    (j * scalef) - SHRINK_AMOUNT, x0, y0, yr);
	}
	xpos += j * scalef;
    }
    fprintf(f,"\n");

    /* Then, the text */

    mode = '-'; /* reinstantiate default */
    if (!(bc->flags & BARCODE_NO_ASCII)) {
        k=0; /* k is the "previous font size" */
        for (ptr = bc->textinfo; ptr; ptr = strchr(ptr, ' ')) {
            while (*ptr == ' ') ptr++;
            if (!*ptr) break;
	    if (*ptr == '+' || *ptr == '-') {
		mode = *ptr; continue;
	    }
            if (sscanf(ptr, "%lf:%lf:%c", &f1, &f2, &c) != 3) {
                fprintf(stderr, "barcode: impossible data: %s\n", ptr);
                continue;
            }
	    if (fsav!=f2) { /* Don't repeat "findfont" if unneeded */
		fprintf(f, "/Helvetica findfont %5.2f scalefont setfont\n",
			f2 * scalef);
	    }
	    fsav = f2; /* for next time */

            fprintf(f, "%5.2f %5.2f moveto (",
                    bc->xoff + f1 * scalef + bc->margin,
		    mode == '-'
                       ? (double)bc->yoff + bc->margin
		       : (double)bc->yoff + bc->margin+bc->height - 8*scalef);
	    /* Both the backslash and the close parens are special */
	    if (c=='\\' || c==')')
		fprintf(f, "\\%c) show\n", c);
	    else
		fprintf(f, "%c) show\n", c);
        }
    }

    fprintf(f,"\n%% End barcode for \"%s\"\n\n",
	    printable ? bc->ascii : "<unprintable string>");

    if (!(bc->flags & BARCODE_OUT_NOHEADERS)) {
	fprintf(f,"showpage\n");
	if (bc->flags & BARCODE_OUT_PS)  {
	    fprintf(f, "%%%%Trailer\n\n");
	}
    }
    return 0;
}




