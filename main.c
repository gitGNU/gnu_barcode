/*
 * main.c - a commandline frontend for the barcode library
 *
 * Copyright (c) 1999 Michele Comitini (mcm@glisco.it)
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
#include <errno.h>

#include "cmdline.h"
#include "barcode.h"

/*
 * Most of this file deals with command line options, by exploiting
 * the cmdline.[ch] engine to offer defaults via environment variables
 * and handling functions for complex options.
 *
 * In order to offer a friendly interface (for those who feel the
 * cmdline *is* friendly, like me), we have to convert names to enums...
 */

struct {
    char *name;
    int type;
} encode_tab[] = {
    {"ean",      BARCODE_EAN},
    {"ean13",    BARCODE_EAN},
    {"upc",      BARCODE_UPC},
    {"upc-a",    BARCODE_UPC},
    {"isbn",     BARCODE_ISBN},
    {"39",       BARCODE_39},
    {"code39",   BARCODE_39},
#if 0 /* These are not implemented, yet */
    {"128",      BARCODE_128},
    {"code128",  BARCODE_128},
    {"128c",     BARCODE_128C},
    {"code128c", BARCODE_128C},
#endif
    {NULL, 0}
};

/*
 * Get encoding type from string rapresentation.
 * Returns -1 on error.
 */
int encode_id(char *encode_name)
{
    int i;
    for (i = 0;  encode_tab[i].name; i++)
	if (!strcasecmp(encode_tab[i].name, encode_name))
	    return encode_tab[i].type;
    return -1;
}

int list_encodes(FILE *f) /* used in the help message */
{
    int prev = -1;
    int i;

    fprintf(f, "Known encodings are (synonyms appear on the same line):");
    for (i = 0;  encode_tab[i].name; i++) {
	if (encode_tab[i].type != prev)
	    fprintf(f, "\n\t");
	else
	    fprintf(f, ", ");
	fprintf(f, "\"%s\"", encode_tab[i].name);
	prev = encode_tab[i].type;
    }
    fprintf(f, "\n");
    return 0;
}


/*
 * Variables to hold cmdline arguments (or defaults)
 */
char *ifilename, *ofilename;
int encoding_type;                    /* filled by get_encoding() */
int code_width, code_height;          /* "-g" for standalone codes */
int lines, columns;                   /* "-t" for tables */
int xmargin, ymargin;                 /* both for "-g" and "-t" */
int ximargin, yimargin;               /* "-m": internal margins */
int eps, ps, noascii, nochecksum;     /* boolean flags */
/*
 * Functions to handle command line arguments
 */

struct encode_item {
    char *string;
    struct encode_item *next;
} *list_head, *list_tail;

/* each "-b" option adds a string to the input pool allocating its space */
int get_input_string(void *arg)
{
    struct encode_item *item = malloc(sizeof(*item));
    if (!item)
	return -1;
    item->string = strdup(arg);
    if (!list_head) {
	list_head = list_tail = item;
    } else {
	list_tail->next = item;
	list_tail = item;
    }
    item->next = NULL;
    return 0;
}

/* and this function extracts strings from the pool */
unsigned char *retrieve_input_string(FILE *ifile)
{
    char *string;
    static char fileline[128];

    struct encode_item *item = list_head;
    if (list_tail) { /* this means at least one "-b" was specified */
	if (!item)
	    return NULL; /* the list is empty */
	string = item->string;
	list_head = item->next;
	free(item);
	return string;
    }

    /* else,  read from the file */
    if (!fgets(fileline, 128, ifile))
	return NULL;
    if (fileline[strlen(fileline)-1]=='\n')
	fileline[strlen(fileline)-1]= '\0';
    return strdup(fileline);
}

/* convert an encoding name to an encoding integer code */
int get_encoding(void *arg)
{
    encoding_type = encode_id((char *)arg);
    if (encoding_type<0) return -1; /* error */
    return 0;
}

/* convert a geometry specification */
int get_geometry(void *arg)
{
    int n;

    n = sscanf((char *)arg, "%dx%d+%d+%d", &code_width, &code_height,
	       &xmargin, &ymargin);

    if (n==4 || n==2) return 0;
    return -1;
}

/* convert a geometry specification */
int get_table(void *arg)
{
    int n;

    n = sscanf((char *)arg, "%dx%d+%d+%d", &columns, &lines,
	       &xmargin, &ymargin);
    if (n==4 || n==2) return 0;
    return -1;
}

/* convert an internal margin specification */
int get_margin(void *arg)
{
    char separator;
    int n;

    /* accept one number or two, separated by any char */
    n = sscanf((char *)arg, "%d%c%d", &ximargin, &separator, &yimargin);
    if (n==1) {
	n=3; yimargin = ximargin;
    }
    if (n!=3)
	return -1;
    return 0;
}


/*
 * The table of possible arguments
 */
struct commandline option_table[] = {
    {'i', CMDLINE_S, &ifilename, NULL, NULL, NULL,
                   "input file (strings to encode), default is stdin"},
    {'o', CMDLINE_S, &ofilename, NULL, NULL, NULL,
                    "output file, default is stdout"},
    {'b', CMDLINE_S, NULL, get_input_string, NULL, NULL,
                   "string to encode (use input file if missing)"},
    {'e', CMDLINE_S, NULL, get_encoding, "BARCODE_ENCODING", NULL,
                   "encoding type (default is best fit for first string)"},
    {'g', CMDLINE_S, NULL, get_geometry, "BARCODE_GEOMETRY", NULL,
                    "geometry on the page: <wid>x<hei>[+<margin>+<margin>]"},
    {'t', CMDLINE_S, NULL, get_table, "BARCODE_TABLE", NULL,
                    "table geometry: <cols>x<lines>[+<margin>+<margin>]"},
    {'m', CMDLINE_S, NULL, get_margin, "BARCODE_MARGIN", "10",
                    "internal margin for each item in a table: <xm>[,<ym>]"},
    {'n', CMDLINE_NONE, &noascii, NULL, NULL, NULL,
                    "\"numeric\": avoid printing text along with the bars"},
    {'c', CMDLINE_NONE, &nochecksum, NULL, NULL, NULL,
                    "no Checksum character, if the chosen encoding allows it"},
    {'E', CMDLINE_NONE, &eps, NULL, NULL, NULL,
                    "print a single code as eps file, else do multi-page ps"},
    {0,}
};
	 
/*
 * The main function
 */
int main(int argc, char **argv)
{
    FILE *ifile = stdin;
    FILE *ofile = stdout;
    char *line;
    int flags=0; /* for the library */
    int page;

    /* First of all, accept "--help" and "-h" as a special case */
    if (argc == 2 && (!strcmp(argv[1],"--help") || !strcmp(argv[1],"-h"))) {
	commandline_errormsg(stderr, option_table, argv[0], "Options:\n");
	fprintf(stderr,"\n");
	list_encodes(stderr);
	exit(1);
    }

    /* Otherwise, parse the commandline */
    if (commandline(option_table, argc, argv, "Use: %s [options]\n")<0) {
	/* error: add information about the encoding types and exit */
	list_encodes(stderr);
	exit(1);
    }

    /* FIXME: print warnings for incompatible options */

    /* open the input stream if specified */
    if (ifilename)
	ifile = fopen(ifilename,"r");
    if (!ifile) {
	fprintf(stderr, "%s: %s: %s\n", argv[0], ifilename,
		strerror(errno));
	exit(1);
    }

    /* open the output stream if specified */
    if (ofilename)
	ofile = fopen(ofilename,"w");
    if (!ofile) {
	fprintf(stderr, "%s: %s: %s\n", argv[0], ofilename,
		strerror(errno));
	exit(1);
    }

    if (encoding_type < 0) { /* unknown type specified */
	fprintf(stderr,"%s: Unknown endoding. Try \"%s --help\"\n",
 		argv[0], argv[0]);
	exit(1);
    }
    flags |= encoding_type;  
    ps = !eps; /* a shortcut */
    if (eps)
	flags |= BARCODE_OUT_EPS; /* print headers too */
    else
	flags |= BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS;
    if (noascii)
	flags |= BARCODE_NO_ASCII;
    if (nochecksum)
	flags |= BARCODE_NO_CHECKSUM;

    /* the table is not available in eps mode */
    if (eps && (lines>1 || columns>1)) {
	fprintf(stderr, "%s: can't print tables in EPS format\n",argv[0]);
	exit(1);
    }

    if (ps) { /* The header is independent of single/table mode */
	/* Headers. Don't let the library do it, we may need multi-page */
	fprintf(ofile, "%%!PS-Adobe-2.0\n");
	fprintf(ofile, "%%%%Creator: \"barcode\", "
		"libbarcode sample frontend\n");
	fprintf(ofile, "%%%%EndComments\n");
	fprintf(ofile, "%%%%EndProlog\n\n");
    }

    /*
     * Here we are, ready to work. Handle the one-per-page case first,
     * as it is shorter.
     */
    if (!lines && !columns) {
	page = 0;
	while ( (line = retrieve_input_string(ifile)) ) {
	    page++;
	    if (ps) {
		fprintf(ofile, "%%%%Page: %i %i\n\n",page,page);
	    }
	    Barcode_Encode_and_Print(line, ofile, code_width, code_height,
				     xmargin, ymargin, flags);
	    if (eps) break; /* if output is eps, do it once only */
	    fprintf(ofile, "showpage\n");
	}
	/* no more lines, print footers */
	if (ps) {
	    fprintf(ofile, "%%%%Trailer\n\n");
	}
    } else {

	/* table mode, the header has been already printed */
	
	/* A4 is 210x297mm, 1in is 25.4mm, 1in is 72p .... */
	int page_wid = 595;  /* FIXME: should support different page sizes */
	int page_hei = 842;
	int xstep = (page_wid - 2 * xmargin)/columns;
	int ystep = (page_hei - 2 * xmargin)/lines;
	int x = columns, y = -1; /* position in the table, start off-page */

	page=0;
	while ( (line = retrieve_input_string(ifile)) ) {
	    x++;  /* fit x and y */
	    if (x >= columns) {
		x=0; y--;
		if (y<0) {
		    y = lines-1; page++;
		    if (page>1) fprintf(ofile, "showpage\n");
		    fprintf(ofile, "%%%%Page: %i %i\n\n",page,page);
		}
	    }

	    /*
	     * Print this code, using the internal margins as spacing.
	     * In order to remove the extra (default) margin, subtract it
	     * in advance (dirty)
	     */
	    Barcode_Encode_and_Print(line, ofile,
		    xstep - 2*ximargin, ystep - 2*yimargin,
		    xmargin + ximargin + x * xstep - BARCODE_DEFAULT_MARGIN,
		    ymargin + yimargin + y * ystep - BARCODE_DEFAULT_MARGIN,
		    flags);
	}
	fprintf(ofile, "showpage\n\n%%%%Trailer\n\n");
    }
    return 0;
}



