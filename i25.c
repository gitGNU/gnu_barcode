/*
 * i25.c -- "interleaved 2 of 5"
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

static char *codes[] = {
    "11331", "31113", "13113", "33111", "11313",
    "31311", "13311", "11133", "31131", "13131"
};

static char *guard[] = {"a1a1", "c1a"}; /* begin end */

int Barcode_i25_verify(unsigned char *text)
{
    if (!strlen(text))
	return -1;
    while (*text && isdigit(*text))
	text++;
    if (*text)
	return -1; /* a non-digit char */
    return 0; /* ok */
}

int Barcode_i25_encode(struct Barcode_Item *bc)
{
    unsigned char *text;
    unsigned char *partial;  /* dynamic */
    unsigned char *textinfo; /* dynamic */
    unsigned char *textptr, *p1, *p2, *pd;
    int i, sum[2], textpos, usesum = 0;


    if (bc->partial)
	free(bc->partial);
    if (bc->textinfo)
	free(bc->textinfo);
    bc->partial = bc->textinfo = NULL; /* safe */

    if (!bc->encoding)
	bc->encoding = strdup("interleaved 2 of 5");

    text = bc->ascii;
    if (!bc->ascii) {
        bc->error = EINVAL;
        return -1;
    }

    if ((bc->flags & BARCODE_NO_CHECKSUM)) usesum = 0; else usesum = 1;

    /* create the real text string, padded to an even number of digits */
    text = malloc(strlen(bc->ascii) + 3); /* leading 0, checksum, term. */
    if (!text) {
	bc->error = errno;
	return -1;
    }
    /* add the leading 0 if needed */
    i = strlen(bc->ascii) + usesum;
    if (i % 2) {
	/* add a leading 0 */
	text[0] = '0';
	strcpy(text+1, bc->ascii);
    } else {
	strcpy(text, bc->ascii);
    }
    /* add the trailing checksum if needed */
    if (usesum) {
	sum[0] = sum[1] = 0;
	for (i=0; text[i]; i++) 
	    sum[i%2] += text[i]-'0';
	if (strlen(text) % 2)
	    i = sum[0] * 3 + sum[1];
	else
	    i = sum[1] * 3 + sum[2];
	strcat(text, "0");
	if (i%10)
	    text[strlen(text)-1] += 10-(i%10);
    }

    /* the partial code is 5 * (text + check) + 4(head) + 3(tail) + term. */
    partial = malloc( (strlen(text) + 3) * 5 +2); /* be large... */
    if (!partial) {
        bc->error = errno;
	free(text);
        return -1;
    }

    /* the text information is at most "nnn:fff:c " * (strlen+1) +term */
    textinfo = malloc(10*(strlen(text)+1) + 2);
    if (!textinfo) {
        bc->error = errno;
        free(partial);
	free(text);
        return -1;
    }


    strcpy(partial, "0"); /* the first space */
    strcat(partial, guard[0]); /* start */
    textpos = 4; /* width of initial guard */
    textptr = textinfo;

    for (i=0; i<strlen(text); i+=2) {
        if (!isdigit(text[i]) || !isdigit(text[i+1])) {
            bc->error = EINVAL; /* impossible if text is verified */
            free(partial);
            free(textinfo);
            free(text);
            return -1;
        }
	/* interleave two digits */
	p1 = codes[text[i]-'0'];
	p2 = codes[text[i+1]-'0'];
	pd = partial + strlen(partial); /* destination */
	while (*p1) {
	    *(pd++) = *(p1++);
	    *(pd++) = *(p2++);
	}
	*pd = '\0';
	/* and print the ascii text (but don't print the checksum, if any */
	if (usesum && strlen(text+i)==2) {
	    /* print only one digit, discard the checksum */
	    sprintf(textptr, "%i:12:%c ", textpos, text[i]);
	} else {
	    sprintf(textptr, "%i:12:%c %i:12:%c ", textpos, text[i],
		    textpos+9, text[i+1]);
	}
        textpos += 18; /* width of two codes */
        textptr += strlen(textptr);
    }
    strcat(partial, guard[1]);

    bc->partial = partial;
    bc->textinfo = textinfo;
    free(text);

    return 0;
}
    
