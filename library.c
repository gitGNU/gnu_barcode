/*
 * library.c -- external functions of libbarcode
 *
 * Copyright (c) 1999 Alessandro Rubini (rubini@gnu.org)
 * Copyright (c) 1999 Prosa Srl. (prosa@prosa.it)
 * Copyright (c) 2010, 2011 Giuseppe Scrivano (gscrivano@gnu.org)
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "barcode.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H /* sometimes (windows, for instance) it's missing */
#  include <unistd.h>
#endif
#include <errno.h>

/*
 * This function allocates a barcode structure and strdup()s the
 * text string. It returns NULL in case of error
 */
struct Barcode_Item *Barcode_Create(char *text)
{
    struct Barcode_Item *bc;

    bc = malloc(sizeof(*bc));
    if (!bc) return NULL;

    memset(bc, 0, sizeof(*bc));
    bc->ascii = strdup(text);
    bc->margin = BARCODE_DEFAULT_MARGIN; /* default margin */
    return bc;
}


/*
 * Free a barcode structure
 */
int Barcode_Delete(struct Barcode_Item *bc)
{
    if (bc->ascii)
	free(bc->ascii);
    if (bc->partial)
	free(bc->partial);
    if (bc->textinfo)
	free(bc->textinfo);
    if (bc->encoding)
	free(bc->encoding);
    free(bc);
    return 0; /* always success */
}


/*
 * The various supported encodings.  This might be extended to support
 * dynamic addition of extra encodings
 */
extern int Barcode_ean_verify(char *text);
extern int Barcode_ean_encode(struct Barcode_Item *bc);
extern int Barcode_upc_verify(char *text);
extern int Barcode_upc_encode(struct Barcode_Item *bc);
extern int Barcode_isbn_verify(char *text);
extern int Barcode_isbn_encode(struct Barcode_Item *bc);
extern int Barcode_39_verify(char *text);
extern int Barcode_39_encode(struct Barcode_Item *bc);
extern int Barcode_39ext_verify(char *text);
extern int Barcode_39ext_encode(struct Barcode_Item *bc);
extern int Barcode_128b_verify(char *text);
extern int Barcode_128b_encode(struct Barcode_Item *bc);
extern int Barcode_128c_verify(char *text);
extern int Barcode_128c_encode(struct Barcode_Item *bc);
extern int Barcode_128_verify(char *text);
extern int Barcode_128_encode(struct Barcode_Item *bc);
extern int Barcode_128raw_verify(char *text);
extern int Barcode_128raw_encode(struct Barcode_Item *bc);
extern int Barcode_i25_verify(char *text);
extern int Barcode_i25_encode(struct Barcode_Item *bc);
extern int Barcode_cbr_verify(char *text);
extern int Barcode_cbr_encode(struct Barcode_Item *bc);
extern int Barcode_msi_verify(char *text);
extern int Barcode_msi_encode(struct Barcode_Item *bc);
extern int Barcode_pls_verify(char *text);
extern int Barcode_pls_encode(struct Barcode_Item *bc);
extern int Barcode_93_verify(char *text);
extern int Barcode_93_encode(struct Barcode_Item *bc);
extern int Barcode_11_verify(char *text);
extern int Barcode_11_encode(struct Barcode_Item *bc);


struct encoding {
    int type;
    int (*verify)(char *text);
    int (*encode)(struct Barcode_Item *bc);
};

struct encoding encodings[] = {
    {BARCODE_EAN,    Barcode_ean_verify,    Barcode_ean_encode},
    {BARCODE_UPC,    Barcode_upc_verify,    Barcode_upc_encode},
    {BARCODE_ISBN,   Barcode_isbn_verify,   Barcode_isbn_encode},
    {BARCODE_128B,   Barcode_128b_verify,   Barcode_128b_encode},
    {BARCODE_128C,   Barcode_128c_verify,   Barcode_128c_encode},
    {BARCODE_128RAW, Barcode_128raw_verify, Barcode_128raw_encode},
    {BARCODE_39,     Barcode_39_verify,     Barcode_39_encode},
    {BARCODE_39EXT,  Barcode_39ext_verify,  Barcode_39ext_encode},
    {BARCODE_I25,    Barcode_i25_verify,    Barcode_i25_encode},
    {BARCODE_128,    Barcode_128_verify,    Barcode_128_encode},
    {BARCODE_CBR,    Barcode_cbr_verify,    Barcode_cbr_encode},
    {BARCODE_PLS,    Barcode_pls_verify,    Barcode_pls_encode},
    {BARCODE_MSI,    Barcode_msi_verify,    Barcode_msi_encode},
    {BARCODE_93,     Barcode_93_verify,     Barcode_93_encode},
	{BARCODE_11,     Barcode_11_verify,     Barcode_11_encode},
    {0,              NULL,                  NULL}
};

/*
 * A function to encode a string into bc->partial, ready for
 * postprocessing to the output file. Meaningful bits for "flags" are
 * the encoding mask and the no-checksum flag. These bits
 * get saved in the data structure.
 */
int Barcode_Encode(struct Barcode_Item *bc, int flags)
{
    int validbits = BARCODE_ENCODING_MASK | BARCODE_NO_CHECKSUM;
    struct encoding *cptr;

    /* If any flag is cleared in "flags", inherit it from "bc->flags" */
    if (!(flags & BARCODE_ENCODING_MASK))
	flags |= bc->flags & BARCODE_ENCODING_MASK;
    if (!(flags & BARCODE_NO_CHECKSUM))
	flags |= bc->flags & BARCODE_NO_CHECKSUM;
    flags = bc->flags = (flags & validbits) | (bc->flags & ~validbits);

    if (!(flags & BARCODE_ENCODING_MASK)) {
	/* get the first code able to handle the text */
	for (cptr = encodings; cptr->verify; cptr++)
	    if (cptr->verify((char *)bc->ascii)==0)
		break;
	if (!cptr->verify) {
	    bc->error = EINVAL; /* no code can handle this text */
	    return -1;
	}
	flags |= cptr->type; /* this works */
	bc->flags |= cptr->type;
    }
    for (cptr = encodings; cptr->verify; cptr++)
	if (cptr->type == (flags & BARCODE_ENCODING_MASK))
	    break;
    if (!cptr->verify) {
	bc->error = EINVAL; /* invalid barcode type */
	return -1;
    }
    if (cptr->verify(bc->ascii) != 0) {
	bc->error = EINVAL;
	return -1;
    }
    return cptr->encode(bc);
}


/* 
 * When multiple output formats are supported, there will
 * be a jumpt table like the one for the types. Now we don't need it
 */
extern int Barcode_ps_print(struct Barcode_Item *bc, FILE *f);
extern int Barcode_pcl_print(struct Barcode_Item *bc, FILE *f);
extern int Barcode_svg_print(struct Barcode_Item *bc, FILE *f);

/*
 * A function to print a partially decoded string. Meaningful bits for
 * "flags" are the output mask etc. These bits get saved in the data
 * structure. 
 */
int Barcode_Print(struct Barcode_Item *bc, FILE *f, int flags)
{
    int validbits = BARCODE_OUTPUT_MASK | BARCODE_NO_ASCII
	| BARCODE_OUT_NOHEADERS;

    /* If any flag is clear in "flags", inherit it from "bc->flags" */
    if (!(flags & BARCODE_OUTPUT_MASK))
	flags |= bc->flags & BARCODE_OUTPUT_MASK;
    if (!(flags & BARCODE_NO_ASCII))
	flags |= bc->flags & BARCODE_NO_ASCII;
    if (!(flags & BARCODE_OUT_NOHEADERS))
	flags |= bc->flags & BARCODE_OUT_NOHEADERS;
    flags = bc->flags = (flags & validbits) | (bc->flags & ~validbits);

    if (bc->flags & BARCODE_OUT_PCL)
       return Barcode_pcl_print(bc, f);
    if (bc->flags & BARCODE_OUT_SVG)
       return Barcode_svg_print(bc, f);
    return Barcode_ps_print(bc, f);
}

/*
 * Choose the position
 */
int Barcode_Position(struct Barcode_Item *bc, int wid, int hei,
		     int xoff, int yoff, double scalef)
{
    bc->width = wid; bc->height = hei;
    bc->xoff = xoff; bc->yoff = yoff;
    bc->scalef = scalef;
    return 0;
}

/*
 * Do it all in one step
 */
int Barcode_Encode_and_Print(char *text, FILE *f, int wid, int hei,
				    int xoff, int yoff, int flags)
{
    struct Barcode_Item * bc;
    
    if (!(bc=Barcode_Create(text))) {
	errno = -ENOMEM;
	return -1;
    }
    if (     Barcode_Position(bc, wid, hei, xoff, yoff, 0.0) < 0
	  || Barcode_Encode(bc, flags) < 0
	  || Barcode_Print(bc, f, flags) < 0) {
	errno = bc->error;
	Barcode_Delete(bc);
	return -1;
    }
    Barcode_Delete(bc);
    return 0;
}

/*
 * Return the version
 */

int Barcode_Version(char *vptr)
{
    const char *it;
    int ret = 0;

    if (vptr)
	strcpy(vptr, PACKAGE_VERSION);

    for (it = PACKAGE_VERSION; it; it++)
      {
        int d;
      repeat:
        switch (*it)
          {
            /* Poor Peano, nobody ensures that in every locale, we have:
               '1' = '0' + 1,..., '9' = '8' + 1.  But fortunately we are not
               using base 1000 so:  */
          case '0':
            d = 0;
            break;
          case '1':
            d = 1;
            break;
          case '2':
            d = 2;
            break;
          case '3':
            d = 3;
            break;
          case '4':
            d = 4;
            break;
          case '5':
            d = 5;
            break;
          case '6':
            d = 6;
            break;
          case '7':
            d = 7;
            break;
          case '8':
            d = 8;
            break;
          case '9':
            d = 9;
            break;
          case '.':
            it++;
            goto repeat;
          }

        ret = ret * 10 + d;
      }

    return ret;
}
