#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <memory.h>

#include "arbdb.h"
#include "arbdbt.h"

/* sort an array
   compare is a compare function:

   compare(2,3) should be  <0;
   compare(x,x)            = 0;
   compare(4,3)            >0;

*/

char *GBT_quicksort(void **array,long start, long end, gb_compare_two_items_type compare, char *client_data)
{
    long        i,j;
    long        mid;
    void        *swap;
    char        *error;
    if (end-1 <= start) return 0;
    i = start;
    j = end-1;
    mid = (i+j)/2;
    while (i<j) {
        for (; i<j; i++) {
            if (compare(array[i],array[mid],client_data) >= 0) break;
        }
        for (;  i<j; j--) {
            if (compare(array[j],array[mid],client_data) < 0) break;
        }
        swap = array[i];
        array[i] = array[j];
        array[j] = swap;
        if (i== mid) mid = j;
        else if (j== mid) mid = i;
    }
    swap = array[i];
    array[i] = array[mid];
    array[mid] = swap;
    error = GBT_quicksort(array,start,i,compare,client_data);
    if (!error) error = GBT_quicksort(array,i+1,end,compare,client_data);
    return error;
}

char *GB_mergesort(void **array,long start, long end, gb_compare_two_items_type compare, char *client_data)
{
    long size;
    long mid;
    long i, j;
    long        dest;
    void **buffer;
    void *ibuf[256];
    char *error;

    size = end - start;
    if (size <= 1) {
        return 0;
    }
    mid = (start+end) / 2;
    error = GB_mergesort(array,start,mid,compare,client_data);
    error = GB_mergesort(array,mid,end,compare,client_data);
    if (size <256) {
        buffer = ibuf;
    }else {
        buffer = (void **)malloc((size_t)(size * sizeof(void *)));
    }

    for ( dest = 0, i = start, j = mid ; i< mid && j < end;) {
        if (compare(array[i],array[j],client_data) < 0) {
            buffer[dest++] = array[i++];
        }else{
            buffer[dest++] = array[j++];
        }
    }
    while(i<mid) buffer[dest++] = array[i++];
    while(j<end) buffer[dest++] = array[j++];

    memcpy( (char *)(array+start),(char *)buffer,(int)size * sizeof(void *));

    if (size>=256) free((char *)buffer);

    return error;
}
