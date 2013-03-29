/*
 * code39.c -- encoding for code39
 *
 * Copyright (c) 1999 Alessandro Rubini (rubini@gnu.org)
 * Copyright (c) 1999 Prosa Srl. (prosa@prosa.it)
 * Copyright (c) 2005 Ian Ward (ian@excess.org)
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
#include <errno.h>


/* this is ordered in decades to simplify encoding */
static char alphabet[] = 
   "1234567890" "ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZ-. *" "$/+%";

/* the checksum alphabet has a different order */
static char checkbet[] = 
   "0123456789" "ABCDEFGHIJ" "KLMNOPQRST" "UVWXYZ-. $" "/+%";

/* The first 40 symbols repeat this bar pattern */
static char *bars[] = {
   "31113","13113","33111","11313","31311",
   "13311","11133","31131","13131","11331"};

/* The first 4 decades use these space patterns */
static char *spaces[] = {"1311","1131","1113","3111"};

/* the last four symbols are special */
static char *specialbars[] = {
   "11111","11111","11111","11111"};

static char *specialspaces[] = {
   "3331","3313","3133","1333"};

static char *fillers[]= {
    "0a3a1c1c1a",
    "1a3a1c1c1a"
};

/* extended code39 translation table */
static char *code39ext[] = {
    "%U", /*for completeness only, NUL cannot be encoded by barcode*/
    "$A","$B","$C","$D","$E","$F","$G","$H","$I","$J","$K","$L","$M",
    "$N","$O","$P","$Q","$R","$S","$T","$U","$V","$W","$X","$Y","$Z",
    "%A","%B","%C","%D","%E"," ",
    "/A","/B","/C","/D","/E","/F","/G","/H","/I","/J","/K","/L","-",
    ".","/O","0","1","2","3","4","5","6","7","8","9","/Z",
    "%F","%G","%H","%I","%J","%V",
    "A","B","C","D","E","F","G","H","I","J","K","L","M",
    "N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
    "%K","%L","%M","%N","%O","%W",
    "+A","+B","+C","+D","+E","+F","+G","+H","+I","+J","+K","+L","+M",
    "+N","+O","+P","+Q","+R","+S","+T","+U","+V","+W","+X","+Y","+Z",
    "%P","%Q","%R","%S","%T"
};

/*
 * Check that the text can be encoded. Returns 0 or -1.
 * If it's all lowecase convert to uppercase and accept it
 */
int Barcode_39_verify(unsigned char *text)
{
    int i, lower=0, upper=0;

    if (text[0] == '\0')
	return -1;
    for (i=0; text[i]; i++) {
        if (isupper(text[i])) upper++;
        if (islower(text[i])) lower++;
        if (!strchr(alphabet,toupper(text[i])))
            return -1;
    }
    if (lower && upper)
        return -1;
    return 0;
}

/*
 * Check that the text can be encoded. Returns 0 or -1.
 * Accept all standard ASCII
 */
int Barcode_39ext_verify(unsigned char *text)
{
    int i;

    if (text[0] == '\0')
	return -1;
    for (i=0; text[i]; i++) {
        if (text[i]>127)
	    return -1;
    }
    return 0;
}

static int add_one(char *ptr, int code)
{
    char *b, *s;

    if (code < 40) {
        b = bars[code%10];
        s = spaces[code/10];
    } else {
        b = specialbars[code-40];
        s = specialspaces[code-40];
    }
    sprintf(ptr,"1%c%c%c%c%c%c%c%c%c", /* separator */
            b[0], s[0], b[1], s[1], b[2], s[2],
            b[3], s[3], b[4]);
    return 0;
}

/*
 * The encoding functions fills the "partial" and "textinfo" fields.
 * Lowercase chars are converted to uppercase
 */
int Barcode_39_encode(struct Barcode_Item *bc)
{
    static char *text;
    static char *partial;  /* dynamic */
    static char *textinfo; /* dynamic */
    char *c, *ptr, *textptr;
    int i, code, textpos, checksum = 0;

    if (bc->partial)
	free(bc->partial);
    if (bc->textinfo)
	free(bc->textinfo);
    bc->partial = bc->textinfo = NULL; /* safe */

    if (!bc->encoding)
	bc->encoding = strdup("code 39");

    text = bc->ascii;
    if (!text) {
        bc->error = EINVAL;
        return -1;
    }
    /* the partial code is 10* (head + text + check + tail) + margin + term. */
    partial = malloc( (strlen(text) + 3) * 10 +2);
    if (!partial) {
        bc->error = errno;
        return -1;
    }

    /* the text information is at most "nnn:fff:c " * strlen +term */
    textinfo = malloc(10*strlen(text) + 2);
    if (!textinfo) {
        bc->error = errno;
        free(partial);
        return -1;
    }

    strcpy(partial, fillers[0]);
    ptr = partial + strlen(partial);
    textptr = textinfo;
    textpos = 22;

    for (i=0; text[i]; i++) {
        c = strchr(alphabet, toupper(text[i]));
        if (!c) {
            bc->error = EINVAL; /* impossible if text is verified */
            free(partial);
            free(textinfo);
            return -1;
        }
        code = c - alphabet;
        add_one(ptr, code);
        c = strchr(checkbet,*c);
        if (c) /* the '*' is not there */
            checksum += (c-checkbet);
        sprintf(textptr, "%i:12:%c ", textpos, toupper(text[i]));
        
        textpos += 16; /* width of each code */
        textptr += strlen(textptr);
        ptr += strlen(ptr); 
    }
    /* Add the checksum */
    if ( (bc->flags & BARCODE_NO_CHECKSUM)==0 ) {
	code = (strchr(alphabet, checkbet[checksum % 43]) - alphabet);
	add_one(ptr, code);
    }
    strcat(ptr, fillers[1]); /* end */
    bc->partial = partial;
    bc->textinfo = textinfo;

    return 0;
}

/*
 * The encoding functions fills the "partial" and "textinfo" fields.
 * Replace the ascii with extended coding and call 39_encode
 */
int Barcode_39ext_encode(struct Barcode_Item *bc)
{
    static char *eascii;
    static char *text;
    char c, *ptr;
    int i;
    
    text = bc->ascii;
    if (!text) {
        bc->error = EINVAL;
        return -1;
    }
    
    /* worst case 2 chars per original text */
    eascii = malloc( strlen(text)*2 +1 );
    if (!eascii) {
	bc->error = errno;
	return -1;
    }

    ptr = eascii;
    
    for (i=0; text[i]; i++) {
	c = text[i];
	if (c<0) {
	    bc->error = EINVAL;
	    free(eascii);
	    return -1;
	}
	
	strcpy(ptr, code39ext[(int)c]);
	ptr += strlen(ptr);
    }

    /* replace original with encoded version */
    bc->ascii = eascii;
    free(text);

    return Barcode_39_encode(bc);
}
