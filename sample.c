#include <stdio.h>
#include <stdlib.h>

#include "barcode.h"

int main(int argc, char **argv)
{

    printf("%%!PS-Adobe-2.0\n");
    printf("%%%%Creator: barcode sample program\n");
    printf("%%%%EndComments\n");
    printf("%%%%EndProlog\n\n");
    printf("%%%%Page: 1 1\n\n");

    /* Print a few barcodes in several places in the page */

    /* default size, bottom left */
    Barcode_Encode_and_Print("800894002700",stdout, 0, 0, 40, 40, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* smaller */
    Barcode_Encode_and_Print("800894002700",stdout, 70, 50, 160, 55, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* smallest */
    Barcode_Encode_and_Print("800894002700",stdout, 40, 30, 270, 70, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* A bigger all-0 */
    Barcode_Encode_and_Print("000000000000",stdout, 170, 0, 40, 160, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* Still bigger all-0 (but UPC, this time) */
    Barcode_Encode_and_Print("00000000000",stdout, 300, 0, 240, 160, 
                      BARCODE_UPC | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* A few code-39 ones */
    Barcode_Encode_and_Print("silly code",stdout, 0, 0, 40, 320, 
                      BARCODE_39 | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);
    Barcode_Encode_and_Print("SAMPLE CODES",stdout, 100, 30, 400, 80, 
                      BARCODE_39 | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* ISBN with add-5 */
    Barcode_Encode_and_Print("1-56592-292-1 90000",stdout, 0, 0, 40, 430, 
                      BARCODE_ISBN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* UPC with add-2 */
    Barcode_Encode_and_Print("07447084452 07",stdout, 0, 0, 300, 410, 
                      BARCODE_UPC | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* code 128-C */
    Barcode_Encode_and_Print("12345678900123456789",stdout, 0, 0, 40, 530, 
                      BARCODE_128C | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* and my data as code-128B autodetected */
    Barcode_Encode_and_Print("RBNLSN68T11E897W",stdout, 0, 60, 240, 510, 
                BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);
    /* same as code-39, forced */
    Barcode_Encode_and_Print("RBNLSN68T11E897W",stdout, 0, 60, 240, 590, 
                BARCODE_NO_CHECKSUM | BARCODE_39 |\
			     BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* and one interleaved 2 of 5 */
    Barcode_Encode_and_Print("0123456789",stdout, 0, 0, 40, 640, 
                BARCODE_I25 | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);


    printf("\nshowpage\n");
    printf("%%%%Trailer\n\n");
    return 0;
}







