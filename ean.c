/*
 * ean.c -- encoding for ean13, upc-a and isbn
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
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "barcode.h"


/*
 * these static arrays are used to describe the code.
 */

static char *digits[] = { /* width of bars and/or spaces */
     "3211","2221","2122","1411","1132",
     "1231","1114","1312","1213","3112"};

static char *fillers[]= {"5a1a","1a1a1","a1a"}; /* head, middle, end */

static char *mirrortab[]={
     "------","--1-11","--11-1","--111-","-1--11",
     "-11--1","-111--","-1-1-1","-1-11-","-11-1-"
};

/*
 * Check that the text can be encoded. Returns 0 or -1.
 * The checksum is added later on, accept 12 digits
 */
int Barcode_ean_verify(char *text)
{
    int i;
    if (strlen(text) != 12)
        return -1;
    for (i=0; i<12; i++)
        if (!isdigit(text[i]))
            return -1;
    return 0;
}

/*
 * Upc is the same, but accept 11 digits
 */
int Barcode_upc_verify(char *text)
{
    int i;
    if (strlen(text) != 11)
        return -1;
    for (i=0; i<11; i++)
        if (!isdigit(text[i]))
            return -1;
    return 0;
}

/*
 * Isbn is the same as EAN, just shorter. We accept blanks and dashes
 */
int Barcode_isbn_verify(char *text)
{
    int i, ndigit=0;

    for (i=0; i < strlen(text); i++) {
	if (text[i] == ' ')
	    continue;
	if (text[i] == '-')
	    continue;
	if (isdigit(text[i])) {
	    ndigit++;
	    if (ndigit == 9) { /* got it all */
		i++; break;
	    }
	    continue;
	}
	return -1; /* found non-digit */
    }
    /* skip an hyphen, if any */
    if (text[i]=='-')
	i++;
    /* accept one more char, the checksum, ignoring its value */
    if (strlen(text+i) < 2)
	return 0;
    return -1;
}

/*
 * The encoding functions fills the "partial" and "textinfo" fields.
 * This one deals with both upc and ean.
 */
int Barcode_ean_encode(struct Barcode_Item *bc)
{
    static char text[16];
    static char partial[128];
    static char textinfo[128];
    char *mirror, *ptr1, *ptr2, *tptr;
    int i, xpos, esum, osum, upc = 0;

    /* UPC is special, ISBN is not */
    if ((bc->flags & BARCODE_ENCODING_MASK) == BARCODE_UPC)
	upc = 1;
    if (!bc->encoding)
	bc->encoding = strdup("EAN-13");

    if (bc->partial)
	free(bc->partial);
    if (bc->textinfo)
	free(bc->textinfo);
    bc->partial = bc->textinfo = NULL; /* safe */

    if (!bc->ascii) {
	bc->error = ENODATA;
	return -1;
    }

    if (upc) {
	text[0] = '0';
	strcpy(text+1, bc->ascii);
    } else {
	strcpy(text, bc->ascii);
    }

    /* create the checksum */
    esum = osum = 0;
    for (i=0; i<12; i++)
	if (i%2) osum += text[i]-'0';
	else     esum += text[i]-'0';
    esum = (3*osum + esum) % 10; /* last digit */
    i = (10-esum) % 10; /* complement to 10 */
    text[12] = '0' + i;
    text[13] = '\0';

    tptr = textinfo;
    if (!upc) { /* The first digit */
	tptr += sprintf(tptr,"0:10:%c ",text[0]);
    }
    strcpy(partial, fillers[0]);
    mirror = mirrortab[text[0]-'0'];
    xpos = 8;

    /* left part */
    for (i=1;i<7;i++) {      
        ptr1 = partial + strlen(partial); /* target */
        ptr2 =  digits[text[i]-'0'];      /* source */
        strcpy(ptr1, ptr2);
        if (mirror[i-1] == '1') {
            /* mirror this */
            ptr1[0] = ptr2[3];
            ptr1[1] = ptr2[2];
            ptr1[2] = ptr2[1];
            ptr1[3] = ptr2[0];
        }
	tptr += sprintf(tptr, "%i:12:%c ", xpos, text[i]);
	xpos += 7;
			
    }

    strcat(partial, fillers[1]); /* middle */
    xpos += 5;
    
    /* right part */
    for (i=7;i<13;i++) {  
        ptr1 = partial + strlen(partial); /* target */
        ptr2 =  digits[text[i]-'0'];      /* source */
        strcpy(ptr1, ptr2);
	tptr += sprintf(tptr, "%i:12:%c ", xpos, text[i]);
	xpos += 7;
    }
    tptr[-1] = '\0'; /* overwrite last space */
    strcat(partial, fillers[2]); /* end */

    bc->partial = strdup(partial);
    if (!bc->partial) {
	bc->error = errno;
	return -1;
    }
    bc->textinfo = strdup(textinfo);
    if (!bc->textinfo) {
	bc->error = errno;
	free(bc->partial);
	bc->partial = NULL;
	return -1;
    }
    if (!bc->width)
	bc->width = 5+3+6*7+5+6*7+3; /* 5 + barlen */
    textinfo[strlen(textinfo)-2]='\0'; /* remove the trailing blank */

    return 0;
}

int Barcode_upc_encode(struct Barcode_Item *bc)
{
    bc->encoding = strdup("UPC-A");
    return Barcode_ean_encode(bc);
}

int Barcode_isbn_encode(struct Barcode_Item *bc)
{
    /* For ISBN we must normalize the string and prefix "978" */
    char *text = malloc(14); /* 13 plus terminator */
    int i, j;

    if (!text) {
	bc->error = ENOMEM;
	return -1;
    }
    strcpy(text, "978"); j=3;

    for (i=0; bc->ascii[i]; i++) {
	if (isdigit(bc->ascii[i]))
	    text[j++] = bc->ascii[i];
	if (j == 13)
	    break;
    }
    text[j]='\0';
    free(bc->ascii);
    bc->ascii = text;
    bc->encoding = strdup("ISBN");
    return Barcode_ean_encode(bc);
}
    
