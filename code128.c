#include <errno.h>
#include "barcode.h"

int Barcode_128_verify(char *text)
{
    return -1;
}

int Barcode_128_encode(struct Barcode_Item *bc)
{
    bc->error = ENOSYS;
    return -1;
}

int Barcode_128c_verify(char *text)
{
    return -1;
}

int Barcode_128c_encode(struct Barcode_Item *bc)
{
    bc->error = ENOSYS;
    return -1;
}
