/*
 * ean.c -- encoding for ean, upc and isbn
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

/*
 * IMPORTANT NOTE: if you are reading this file to learn how to add a
 * new encoding type, this is the wrong place as there are too many
 * special cases. Please refer to code39.c instead. If you want to
 * learn how UPC, EAN, ISBN work, on the other hand, I did my best to
 * commend things and hope you enjoy it.
 */

/*
 * These following static arrays are used to describe the barcode.
 *
 * The various forms of UPC and EAN are documented as using three
 * different alphabets to encode the ten digits. However, each digit
 * has exactly one encoding; only, it is sometimes mirrored. Moreover,
 * if you represent the width of each symbol (bar/space) instead of
 * the sequence of 1's and 0's, you find that even-parity and odd-parity
 * encoding are exactly the same. So, here are the digits: */
static char *digits[] = {
     "3211","2221","2122","1411","1132",
     "1231","1114","1312","1213","3112"};

/*
 * What EAN encoding does is adding a leading digit (the 13th digit).
 * Such an extra digit is encoded by mirroring three of the six digits that
 * appear in the left half of the UPC code. Here how mirroring works:
 */
static char *ean_mirrortab[] = {
     "------","--1-11","--11-1","--111-","-1--11",
     "-11--1","-111--","-1-1-1","-1-11-","-11-1-"
};

/*
 * UPC-E (the 6-digit one), instead, encodes the check character as
 * a mirroring of the symbols. This is similar, but the encoding for "0" is
 * different (EAN uses no mirroring for "0" to be compatible with UPC).
 * The same rule is used for UPC-5 (the supplemental digits for ISBN)
 */
static char *upc_mirrortab[] = {
     "---111","--1-11","--11-1","--111-","-1--11",
     "-11--1","-111--","-1-1-1","-1-11-","-11-1-"
};
/* UPC-2 has just two digits to mirror */
static char *upc_mirrortab2[] = {
    "11","1-","-1","--"
};

/*
 * initial, middle, final guard bars (first symbol is a a space).
 * EAN-13 overwrites the first "0" with "9" to make space for the extra digit.
 */
static char *guard[] = {"0a1a","1a1a1","a1a"};

/* initial, final guard bars for UPC-E*/
static char *guardE[] = {"0a1a","1a1a1a"};

/* initial and inter-char guard bars for supplementals (first is space) */
static char *guardS[] = {"9112","11"};

/*
 * Check that the text can be encoded. Returns 0 or -1.
 * The checksum is added later on, accept 12 digits (EAN-13) or 8 (EAN-8).
 * For the 12-digit case, accept an addon of 2 or 5 digits, separated by ' '
 */
int Barcode_ean_verify(unsigned char *text)
{
    int i, len = strlen(text);
    if (len != 12 && len != 7 && len != 15 && len != 18) {
        return -1;
    }
    if (len >12) {/* add-2 or add-5 */
	if (text[12] != ' ' || !isdigit(text[13]) || !isdigit(text[14]))
	    return -1;
	if (len == 18) /* add-5 */
	    if (!isdigit(text[15]) || !isdigit(text[16]) || !isdigit(text[17]))
		return -1;
	/* ok, now check the initial digits */
	len = 12;
    }

    for (i=0; i<len; i++)
        if (!isdigit(text[i]))
            return -1;
    return 0;
}

/*
 * Upc is the same, but accept 11 digits (UPC-A) or 6 (UPC-E) or the add-on
 */
int Barcode_upc_verify(unsigned char *text)
{
    int i, len = strlen(text);
    if (len != 11 && len != 6 && len != 14 && len != 17) {
        return -1;
    }
    if (len >11) {/* add-2 or add-5 */
	if (text[11] != ' ' || !isdigit(text[12]) || !isdigit(text[13]))
	    return -1;
	if (len == 17) /* add-5 */
	    if (!isdigit(text[14]) || !isdigit(text[15]) || !isdigit(text[16]))
		return -1;
	/* ok, now check the initial digits */
	len = 11;
    }

    for (i=0; i<len; i++)
        if (!isdigit(text[i]))
            return -1;
    return 0;
}

/*
 * Isbn is the same as EAN, just shorter. Dashes are accepted, the
 * check character (if specified) is skipped, the extra 5 digits are
 * accepted after a blank.
 */
int Barcode_isbn_verify(unsigned char *text)
{
    int i, ndigit=0;

    for (i=0; i < strlen(text); i++) {
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
    if (ndigit!=9) return -1; /* too short */

    /* skip an hyphen, if any */
    if (text[i] == '-')
	i++;
    /* accept one more char if any (the checksum) */
    if (isdigit(text[i]) || toupper(text[i])=='X')
	i++;
    if (text[i] == '\0')
	return 0; /* Ok */

    /* and accept the extra price tag (blank + 5 digits), if any */
    if (strlen(text+i) != 6)
	return -1;
    if (text[i] != ' ')
	return -1;
    i++; /* skip the blank */
    while (text[i]) {
	if (!isdigit(text[i]))
	    return -1;
	i++;
    }
    return 0; /* Ok: isbn + 5-digit addon */
}

/*
 * These functions are shortcuts I use in the encoding engine
 */
static int ean_make_checksum(char *text, int mode)
{
    int esum = 0, osum = 0, i;
    int even=1; /* last char is even */

    if (strchr(text, ' '))
	i = strchr(text, ' ') - text; /* end of first part */
    else 
	i = strlen(text); /* end of all */

    while (i-- > 0) {
	if (even) esum += text[i]-'0';
	else      osum += text[i]-'0';
	even = !even;
    }
    if (!mode) { /* standard upc/ean checksum */
	i = (3*esum + osum) % 10;
	return (10-i) % 10; /* complement to 10 */
    } else { /* add-5 checksum */
	i = (3*esum + 9*osum);
	return i%10;
    }
}

/* The checksum for UPC-E is calculated using its UPC-A equivalent */
static char *upc_e_to_a(char *text)
{
    static char result[16];
    strcpy(result, "00000000000"); /* 11 0's */

    switch(text[5]) { /* last char */
        case '0': case '1': case '2':
	    memcpy(result+1, text,   2); result[3]=text[5]; /* Manuf. */
	    memcpy(result+8, text+2, 3); /* Product */
        case '3':
	    memcpy(result+1, text,   3); /* Manufacturer */
	    memcpy(result+9, text+3, 2); /* Product */
        case '4':
	    memcpy(result+1,  text,   4); /* Manufacturer */
	    memcpy(result+10, text+4, 1); /* Product */
        default:
	    memcpy(result+1,  text,   5); /* Manufacturer */
	    memcpy(result+10, text+5, 1); /* Product */
    }
    return result;
}

static int width_of_partial(unsigned char *partial)
{
    int i=0;
    while (*partial) {
	if (isdigit(*partial))
	    i += *partial - '0';
	else if (islower(*partial))
	    i += *partial - 'a' + 1;
	partial++;
    }
    return i;
}

/*
 * The encoding functions fills the "partial" and "textinfo" fields.
 * This one deals with both upc (-A and -E) and ean (13 and 8).
 */
int Barcode_ean_encode(struct Barcode_Item *bc)
{
    static char text[24];
    static char partial[256];
    static char textinfo[256];
    char *mirror, *ptr1, *ptr2, *tptr = textinfo; /* where text is written */

    enum {UPCA, UPCE, EAN13, EAN8, ISBN} encoding = ISBN;
    int i, xpos, checksum;

    if (!bc->ascii) {
	bc->error = EINVAL;
	return -1;
    }
    if (!bc->encoding) {
	/* ISBN already wrote what it is; if unknown, find it out */
	switch(strlen(bc->ascii)) {
	    case 12:
		bc->encoding = strdup("EAN-13");
		encoding = EAN13;
		break;
	    case 11:
		bc->encoding = strdup("UPC-A");
		encoding = UPCA;
		break;
	    case 7:
		bc->encoding = strdup("EAN-8");
		encoding = EAN8;
		break;
	    case 6:
		bc->encoding = strdup("UPC-E");
		encoding = UPCE;
		break;
	    default:
		/* it may be UPC or EAN13 with an addon */
		if (strlen(bc->ascii) > 13) {
		    if (bc->ascii[12]==' ') {
			bc->encoding = strdup("EAN-13");
			encoding = EAN13; break;
		    }
		    if (bc->ascii[11]==' ') {
			bc->encoding = strdup("UPC-A");
			encoding = UPCA; break;
		    }
		}
		/* else, it's wrong (impossible, as the text is checked) */
		bc->error = -EINVAL;
		return -1;
	}
    }

    /* better safe than sorry */
    if (bc->partial)	free(bc->partial);  bc->partial =  NULL;
    if (bc->textinfo)	free(bc->textinfo); bc->textinfo = NULL;

    if (encoding == UPCA) { /* add the leading 0 (not printed) */
	text[0] = '0';
	strcpy(text+1, bc->ascii);
    } else {
	strcpy(text, bc->ascii);
    }

    /*
     * build the checksum and the bars: any encoding is slightly different
     */
    if (encoding == UPCA || encoding == EAN13 || encoding == ISBN) {

	checksum = ean_make_checksum(text, 0);
	text[12] = '0' + checksum; /* add it to the text */
	text[13] = '\0';

	strcpy(partial, guard[0]);
	if (encoding == EAN13 || encoding == ISBN) { /* The first digit */
	    sprintf(tptr,"0:12:%c ",text[0]);
	    tptr += strlen(tptr);
	    partial[0] = '9'; /* extra space for the digit */
	} else if (encoding == UPCA)
	    partial[0] = '9'; /* UPC has one digit before the symbol, too */
	xpos = width_of_partial(partial);
	mirror = ean_mirrortab[text[0]-'0'];

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
	    /*
	     * Write the ascii digit. UPC has a special case
	     * for the first digit, which is out of the bars
	     */
	    if (encoding == UPCA && i==1) {
		sprintf(tptr, "0:10:%c ", text[i]);
		tptr += strlen(tptr);
		ptr1[1] += 'a'-'1'; /* bars are long */
		ptr1[3] += 'a'-'1';
	    } else {
		sprintf(tptr, "%i:12:%c ", xpos, text[i]);
		tptr += strlen(tptr);
	    }
	    /* count the width of the symbol */
	    xpos += 7; /* width_of_partial(ptr2) */
	}

	strcat(partial, guard[1]); /* middle */
	xpos += width_of_partial(guard[1]);
    
	/* right part */
	for (i=7;i<13;i++) {  
	    ptr1 = partial + strlen(partial); /* target */
	    ptr2 =  digits[text[i]-'0'];      /* source */
	    strcpy(ptr1, ptr2);
	    /*
	     * Ascii digit. Once again, UPC has a special
	     * case for the last digit
	     */
	    if (encoding == UPCA && i==12) {
		sprintf(tptr, "%i:10:%c ", xpos+13, text[i]);
		tptr += strlen(tptr);
		ptr1[0] += 'a'-'1'; /* bars are long */
		ptr1[2] += 'a'-'1';
	    } else {
		sprintf(tptr, "%i:12:%c ", xpos, text[i]);
		tptr += strlen(tptr);
	    }
	    xpos += 7; /* width_of_partial(ptr2) */
	}
	tptr[-1] = '\0'; /* overwrite last space */
	strcat(partial, guard[2]); /* end */
	xpos += width_of_partial(guard[2]);

	/*
	 * And that's it. Now, in case some add-on is specified it
	 * must be encoded too. Look for it.
	 */
	if ( (ptr1 = strchr(bc->ascii, ' ')) ) {
	    ptr1++;
	    if (strlen(ptr1) != 2 && strlen(ptr1) != 5) {
		bc->error = EINVAL; /* impossible, actually */
		return -1;
	    }
	    strcpy(text, ptr1);
	    if (strlen(ptr1)==5) {
		checksum = ean_make_checksum(text, 1 /* special way */);
		mirror = upc_mirrortab[checksum]+1; /* only last 5 digits */
	    } else {
		checksum = atoi(text)%4;
		mirror = upc_mirrortab2[checksum];
	    }
	    strcat(textinfo, " +"); strcat(partial, "+");
	    tptr = textinfo + strlen(textinfo);
	    for (i=0; i<strlen(text); i++) {
		if (!i) {
		    strcat(partial, guardS[0]); /* separation and head */
		    xpos += width_of_partial(guardS[0]);
		} else {
		    strcat(partial, guardS[1]);
		    xpos += width_of_partial(guardS[1]);
		}
		ptr1 = partial + strlen(partial); /* target */
		ptr2 =  digits[text[i]-'0'];      /* source */
		strcpy(ptr1, ptr2);
		if (mirror[i] != '1') { /* negated wrt EAN13 */
		    /* mirror this */
		    ptr1[0] = ptr2[3];
		    ptr1[1] = ptr2[2];
		    ptr1[2] = ptr2[1];
		    ptr1[3] = ptr2[0];
		}
		/* and the text */
		sprintf(tptr, " %i:12:%c", xpos, text[i]);
		tptr += strlen(tptr);
		xpos += 7; /* width_of_partial(ptr2) */
	    }
	}

    } else if (encoding == UPCE) {

	checksum = ean_make_checksum(upc_e_to_a(text), 0);

	strcpy(partial, guardE[0]);
	xpos = width_of_partial(partial);
	mirror = upc_mirrortab[checksum];

	for (i=0;i<6;i++) {      
	    ptr1 = partial + strlen(partial); /* target */
	    ptr2 =  digits[text[i]-'0'];      /* source */
	    strcpy(ptr1, ptr2);
	    if (mirror[i] != '1') { /* negated wrt EAN13 */
		/* mirror this */
		ptr1[0] = ptr2[3];
		ptr1[1] = ptr2[2];
		ptr1[2] = ptr2[1];
		ptr1[3] = ptr2[0];
	    }
	    sprintf(tptr, "%i:12:%c ", xpos, text[i]);
	    tptr += strlen(tptr);
	    xpos += 7; /* width_of_partial(ptr2) */
	}
	tptr[-1] = '\0'; /* overwrite last space */
	strcat(partial, guardE[1]); /* end */

    } else { /* EAN-8  almost identical to EAN-13 but no mirroring */

	checksum = ean_make_checksum(text, 0);
	text[7] = '0' + checksum; /* add it to the text */
	text[8] = '\0';

	strcpy(partial, guard[0]);
	xpos = width_of_partial(partial);

	/* left part */
	for (i=0;i<4;i++) {      
	    strcpy(partial + strlen(partial), digits[text[i]-'0']);
	    sprintf(tptr, "%i:12:%c ", xpos, text[i]);
	    tptr += strlen(tptr);
	    xpos += 7; /* width_of_partial(digits[text[i]-'0' */
	}
	strcat(partial, guard[1]); /* middle */
	xpos += width_of_partial(guard[1]);
    
	/* right part */
	for (i=4;i<8;i++) {      
	    strcpy(partial + strlen(partial), digits[text[i]-'0']);
	    sprintf(tptr, "%i:12:%c ", xpos, text[i]);
	    tptr += strlen(tptr);
	    xpos += 7; /* width_of_partial(digits[text[i]-'0' */
	}
	tptr[-1] = '\0'; /* overwrite last space */
	strcat(partial, guard[2]); /* end */
    }

    /* all done, copy results to the data structure */
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
	bc->width = width_of_partial(partial);

    return 0; /* success */
}

int Barcode_upc_encode(struct Barcode_Item *bc)
{
    return Barcode_ean_encode(bc); /* UPC is folded into EAN */
}

int Barcode_isbn_encode(struct Barcode_Item *bc)
{
    /* For ISBN we must normalize the string and prefix "978" */
    unsigned char *text = malloc(24); /* 13 + ' ' + 5 plus some slack */
    unsigned char *otext;
    int i, j, retval;

    if (!text) {
	bc->error = ENOMEM;
	return -1;
    }
    strcpy(text, "978"); j=3;

    otext = bc->ascii;
    for (i=0; otext[i]; i++) {
	if (isdigit(otext[i]))
	    text[j++] = otext[i];
	if (j == 12) /* checksum added later */
	    break;
    }
    text[j]='\0';
    if (strchr(otext, ' '))
	strcat(text, strchr(otext, ' '));
    bc->ascii = text;
    bc->encoding = strdup("ISBN");
    retval = Barcode_ean_encode(bc);
    bc->ascii = otext; /* restore ascii for the ps comments */
    free(text);
    return retval;
}
    
