/*
 * code128.c -- encoding for code128 (A, B, C)
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

static char *codeset[] = {
    "212222", "222122", "222221", "121223", "121322",  /*  0 -  4 */
    "131222", "122213", "122312", "132212", "221213",
    "221312", "231212", "112232", "122132", "122231",  /* 10 - 14 */
    "113222", "123122", "123221", "223211", "221132",
    "221231", "213212", "223112", "312131", "311222",  /* 20 - 24 */
    "321122", "321221", "312212", "322112", "322211",
    "212123", "212321", "232121", "111323", "131123",  /* 30 - 34 */
    "131321", "112313", "132113", "132311", "211313",
    "231113", "231311", "112133", "112331", "132131",  /* 40 - 44 */
    "113123", "113321", "133121", "313121", "211331",
    "231131", "213113", "213311", "213131", "311123",  /* 50 - 54 */
    "311321", "331121", "312113", "312311", "332111",
    "314111", "221411", "431111", "111224", "111422",  /* 60 - 64 */
    "121124", "121421", "141122", "141221", "112214",
    "112412", "122114", "122411", "142112", "142211",  /* 70 - 74 */
    "241211", "221114", "413111", "241112", "134111",
    "111242", "121142", "121241", "114212", "124112",  /* 80 - 84 */
    "124211", "411212", "421112", "421211", "212141",
    "214121", "412121", "111143", "111341", "131141",  /* 90 - 94 */
    "114113", "114311", "411113", "411311", "113141",
    "114131", "311141", "411131", "b1a4a2", "b1a2a4",  /* 100 - 104 */
    "b1a2c2", "b3c1a1b"
};

#define START_A 103
#define START_B 104
#define START_C 105
#define STOP    106
#define SHIFT    98 /* only A and B */
#define CODE_A  101 /* only B and C */
#define CODE_B  100 /* only A and C */
#define CODE_C   99 /* only A and B */
#define FUNC_1  102 /* all of them */
#define FUNC_2   97 /* only A and B */
#define FUNC_3   96 /* only A and B */
/* FUNC_4 is CODE_A when in A and CODE_B when in B */

#define SYMBOL_WID 11 /* all of them are 11-bar wide */

#if 0
int Barcode_128_verify(char *text)
{
    /* not implemented */
    return -1;
}

int Barcode_128_encode(struct Barcode_Item *bc)
{
    /* not implemented */
    bc->error = ENOSYS;
    return -1;
}
#endif

int Barcode_128c_verify(unsigned char *text)
{
    if (!strlen(text))
	return -1;
    /* must be an even number of digits */
    if (strlen(text)%2)
	return -1;
    /* and must be all digits */
    for (; *text; text++)
	if (!isdigit(*text))
	    return -1;
    return 0;
}

int Barcode_128c_encode(struct Barcode_Item *bc)
{
    static char *text;
    static char *partial;  /* dynamic */
    static char *textinfo; /* dynamic */
    char *textptr;
    int i, code, textpos, checksum = 0;

    if (bc->partial)
	free(bc->partial);
    if (bc->textinfo)
	free(bc->textinfo);
    bc->partial = bc->textinfo = NULL; /* safe */

    if (!bc->encoding)
	bc->encoding = strdup("code 128-C");

    text = bc->ascii;
    if (!text) {
        bc->error = ENODATA;
        return -1;
    }
    /* the partial code is 6* (head + text + check + tail) + final + term. */
    partial = malloc( (strlen(text) + 3) * 6 +2);
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

    strcpy(partial, "0"); /* the first space */
    strcat(partial, codeset[START_C]);
    checksum += START_C; /* the start char is counted in the checksum */
    textptr = textinfo;
    textpos = SYMBOL_WID;

    for (i=0; i<strlen(text); i+=2) {
        if (!isdigit(text[i]) || !isdigit(text[i+1])) {
            bc->error = EINVAL; /* impossible if text is verified */
            free(partial);
            free(textinfo);
            return -1;
        }
        code = (text[i]-'0') * 10 + text[i+1]-'0';
	strcat(partial, codeset[code]);
	checksum += code * (i/2+1); /* first * 1 + second * 2 + third * 3... */

        sprintf(textptr, "%g:9:%c %g:9:%c ", (double)textpos, text[i],
		textpos + (double)SYMBOL_WID/2,	text[i+1]);
        textpos += SYMBOL_WID; /* width of each code */
        textptr += strlen(textptr);
    }
    /* Add the checksum, independent of BARCODE_NO_CHECKSUM */
    checksum %= 103;
    strcat(partial, codeset[checksum]);
    /* and the end marker */
    strcat(partial, codeset[STOP]);

    bc->partial = partial;
    bc->textinfo = textinfo;

    return 0;
}

#if 0
int Barcode_128_verify(char *text)
{
    /* not implemented */
    return -1;
}

int Barcode_128_encode(struct Barcode_Item *bc)
{
    /* not implemented */
    bc->error = ENOSYS;
    return -1;
}
#endif
