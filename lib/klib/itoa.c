

char *itoa_16 (unsigned int val, char *buf)
{
    char *ptr, *rc;
    rc = ptr = buf;

    do {
        *ptr++ = "0123456789abcdef"[val & 0xF];
        val >>= 4;
    } while (val);
    *ptr-- = '\0';

    while (buf < ptr) {
        char tmp = *buf;
        *buf++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

char *itoa_2 (unsigned int val, char *buf)
{
    char *ptr, *rc;
    rc = ptr = buf;

    do {
        *ptr++ = val & 1 ? '1' : '0';
        val >>= 1;
    } while (val);
    *ptr-- = '\0';

    while (buf < ptr) {
        char tmp = *buf;
        *buf++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}

/* swiped from osdev.org wiki */
/*-----------------------------------------------------------------------------
 * Converts an integer in a given base to a string */
//-----------------------------------------------------------------------------
char *itoa (int val, char *str, int base) {
    char *rc, *ptr, *low;
    
    // check for supported base
    if ( base < 2 || base > 36 ) {
        *str = '\0';
        return str;
    }

    rc = ptr = str;

    // set '-' for negative decimals
    if ( val < 0 && base == 10 )
        *ptr++ = '-';

    // remember where the numbers start
    low = ptr;

    // the actual conversion
    do {
        // modulo is negative for negative value; this makes abs() unnecessary
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmno"
                    "pqrstuvwxyz"[35 + val % base];
        val /= base;
    } while ( val );

    // terminate the string
    *ptr-- = '\0';

    // invert the numbers
    while ( low < ptr ) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    return rc;
}
