/*
 * ps.c -- printing the "partial" bar encoding
 *
 * Copyright (c) 1999 Alessandro Rubini (rubini@prosa.it)
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

/*
 * How do the "partial" and "textinfo" strings work?
 *
 * The first char in "partial" tells how much extra space to
 * leave. For ean it is used to print the first digit, for example.
 * This is an integer offset from '0'.
 *
 * The next characters are alternating bars and spaces, as multiples
 * of the base dimension which is 1 unless the code is
 * rescaled. Rescaling is calculated as the ratio from the requested
 * width and the calculated width.  Digits represent bar/space
 * dimensions. Lower-case letters represent those bars that should
 * extend lower than the others, as offet from 'a'.
 *
 * The "textinfo" string is made up of fields "%i:%i:%c" separated by
 * blank space. The first integer is the x position of the character,
 * the second is the font size (before rescaling) and the char item is
 * the charcter to be printed.
 */


int Barcode_ps_print(struct Barcode_Item *bc, FILE *f)
{
    int i, j, barlen;
    double scalef=1, xpos, x0, y0, yr;
    unsigned char *ptr;
    unsigned char c;

    if (!bc->partial || !bc->textinfo) {
	bc->error = ENODATA;
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
	else
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
    fprintf(f,"%% Printing barcode for \"%s\", scaled %5.2f",
	    bc->ascii, scalef);
    if (bc->encoding)
	fprintf(f,", encoded using \"%s\"",bc->encoding);
    fprintf(f, "\n");
    fprintf(f,"%% The space/bar succession is represented "
	    "by the following widths (space first):\n"
	    "%% ");
    for (i=0; i<strlen(bc->partial); i++) {
	if (isdigit(bc->partial[i]))
	    putc(bc->partial[i], f);
	if (islower(bc->partial[i]))
	    putc(bc->partial[i]-'a'+'1', f);
	if (isupper(bc->partial[i]))
	    putc(bc->partial[i]-'A'+'1', f);
    }
    putc('\n', f);

    xpos = bc->margin + (bc->partial[0]-'0') * scalef;
    for (ptr = bc->partial+1, i=1; *ptr; ptr++, i++) {
	/* j is the width of this bar/space */
	if (isdigit (*ptr))   j = *ptr-'0';
	else                  j = *ptr-'a'+1;
	if (i%2) { /* bar */
	    x0 = bc->xoff + xpos + (j*scalef)/2;
            y0 = bc->yoff + bc->margin;
            yr = bc->height;
            if (!(bc->flags & BARCODE_NO_ASCII)) {
                /* leave some space for the ascii part */
                y0 += (isdigit(*ptr) ? 10 : 5) * scalef;
                yr -= (isdigit(*ptr) ? 10 : 5) * scalef;
            }
            fprintf(f,"%5.2f setlinewidth "
		    "%6.2f %6.2f moveto "
		    "0 %5.2f rlineto stroke\n",
		    j * scalef, x0, y0, yr);
	}
	xpos += j * scalef;
    }
    fprintf(f,"\n");

    /* Then, the text */

    if (!(bc->flags & BARCODE_NO_ASCII)) {
        for (ptr = bc->textinfo; ptr; ptr = strchr(ptr, ' ')) {
            while (*ptr == ' ') ptr++;
            if (!*ptr) break;
            if (sscanf(ptr, "%i:%i:%c", &i, &j, &c) != 3) {
                fprintf(stderr, "barcode: impossible data: %s\n", ptr);
                continue;
            }
            fprintf(f, "/Courier-Bold findfont %5.2f scalefont setfont\n",
                    j * scalef);
            /* FIXME: a ')' can't be printed this way */
            fprintf(f, "%5.2f %5.2f moveto (%c) show\n",
                    bc->xoff + i * scalef + bc->margin,
                    (double)bc->yoff + bc->margin, c);
        }
    }

    fprintf(f,"\n%% End barcode for \"%s\"\n\n", bc->ascii);

    if (!(bc->flags & BARCODE_OUT_NOHEADERS)) {
	fprintf(f,"showpage\n");
	if (bc->flags & BARCODE_OUT_PS)  {
	    fprintf(f, "%%%%Trailer\n\n");
	}
    }
    return 0;
}




