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
    Barcode_Encode_and_Print("800894002700",stdout, 70, 70, 160, 55, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* smallest, bottom right */
    Barcode_Encode_and_Print("800894002700",stdout, 40, 40, 270, 70, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* A bigger all-0 */
    Barcode_Encode_and_Print("000000000000",stdout, 170, 0, 40, 170, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* Still bigger all-0 */
    Barcode_Encode_and_Print("000000000000",stdout, 300, 0, 240, 170, 
                      BARCODE_EAN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* A few code-39 ones */
    Barcode_Encode_and_Print("prosa srl",stdout, 0, 0, 40, 350, 
                      BARCODE_39 | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);
    Barcode_Encode_and_Print("SAMPLE CODES",stdout, 60, 30, 400, 100, 
                      BARCODE_39 | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* ISBN with add-5 */
    Barcode_Encode_and_Print("1-56592-292-1 90000",stdout, 0, 0, 40, 500, 
                      BARCODE_ISBN | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);

    /* UPC with add-2 */
    Barcode_Encode_and_Print("07447084452 07",stdout, 0, 0, 300, 450, 
                      BARCODE_UPC | BARCODE_OUT_PS | BARCODE_OUT_NOHEADERS);


    printf("\nshowpage\n");
    printf("%%%%Trailer\n\n");
    return 0;
}







