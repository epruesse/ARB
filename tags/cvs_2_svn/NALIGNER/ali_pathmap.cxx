
// #include <malloc.h>
#include <stdlib.h>

#include "ali_pathmap.hxx"
#include "ali_misc.hxx"


ALI_PATHMAP::ALI_PATHMAP(unsigned long w, unsigned long h)
{
    width = w;
    height = h;
    height_real = (h / 2) + 1;

    pathmap = (unsigned char **) CALLOC((unsigned int) (height_real * w), sizeof(unsigned char));
    //pathmap = (unsigned char (*)[1]) CALLOC((unsigned int) (height_real * w), sizeof(unsigned char));
    up_pointers = (ALI_TARRAY < ali_pathmap_up_pointer > ****) CALLOC((unsigned int) w, sizeof(ALI_TARRAY < ali_pathmap_up_pointer > *));
    //up_pointers = (ALI_TARRAY < ali_pathmap_up_pointer > **(*)[1]) CALLOC((unsigned int) w, sizeof(ALI_TARRAY < ali_pathmap_up_pointer > *));
    optimized = (unsigned char **) CALLOC((unsigned int) ((w / 8) + 1), sizeof(unsigned char));
    //optimized = (unsigned char (*)[1]) CALLOC((unsigned int) ((w / 8) + 1), sizeof(unsigned char));
    if (pathmap == 0 || up_pointers == 0 || optimized == 0)
        ali_fatal_error("Out of memory");
}

ALI_PATHMAP::~ALI_PATHMAP(void)
{
    unsigned long   l;

    if (pathmap)
        free((char *) pathmap);
    if (up_pointers) {
        for (l = 0; l < width; l++)
            if ((*up_pointers)[l])
                free((char *) (*up_pointers)[l]);
        free((char *) up_pointers);
    }
    if (optimized)
        free((char *) optimized);
}


/*
 * Set a value in the pathmap
 */
void            ALI_PATHMAP::
set(unsigned long x, unsigned long y, unsigned char val,
    ALI_TARRAY < ali_pathmap_up_pointer > *up_pointer)
{
    if (x >= width || y >= height)
        ali_fatal_error("Out of range", "ALI_PATHMAP::set()");

    if (val & ALI_LUP) {
        if ((*up_pointers)[x] == 0) {
            (*up_pointers)[x] = (ALI_TARRAY < ali_pathmap_up_pointer > **)
                CALLOC((unsigned int) height,
                       sizeof(ALI_TARRAY < ali_pathmap_up_pointer > *));
            if ((*up_pointers)[x] == 0)
                ali_fatal_error("Out of memory");
            (*optimized)[x / 8] &= (unsigned char) ~(0x01 << (7 - (x % 8)));
        }
        if ((*optimized)[x / 8] & (0x01 << (7 - (x % 8))))
            ali_fatal_error("Try to change optimized value", "ALI_PATHMAP::set()");

        (*up_pointers)[x][y] = up_pointer;
    }
    if (y & 0x01)
        val &= 0x0f;
    else
        val <<= 4;
    (*pathmap)[x * height_real + (y >> 1)] |= val;
}

/*
 * Get a value from the pathmap
 */
void            ALI_PATHMAP::
get(unsigned long x, unsigned long y, unsigned char *val,
    ALI_TARRAY < ali_pathmap_up_pointer > **up_pointer)
{
    unsigned long   l, counter;

    *up_pointer = 0;
    if (x >= width || y >= height) {
        ali_fatal_error("Out of range", "ALI_PATHMAP::get()");
    }
    if (y & 0x01)
        *val = (*pathmap)[x * height_real + (y >> 1)] & 0x0f;
    else
        *val = (*pathmap)[x * height_real + (y >> 1)] >> 4;

    if (*val & ALI_LUP) {
        if ((*optimized)[x / 8] & (0x01 << (7 - (x % 8)))) {
            for (l = 0, counter = 0; l < y / 2; l++) {
                if ((*pathmap)[x * height_real + l] & ALI_LUP)
                    counter++;
                if (((*pathmap)[x * height_real + l] >> 4) & ALI_LUP)
                    counter++;
            }
            if (y & 0x01 && ((*pathmap)[x * height_real + l] >> 4) & ALI_LUP)
                counter++;
            *up_pointer = (*up_pointers)[x][counter];
        } else
            *up_pointer = (*up_pointers)[x][y];
    }
}


/*
 * optimize the pathmap (the dynamic field with up_pointers)
 */
void            ALI_PATHMAP::
optimize(unsigned long x)
{
    unsigned long   l, counter;
    ALI_TARRAY < ali_pathmap_up_pointer > *(*buffer)[];

    if ((*up_pointers)[x] == 0 || (*optimized)[x / 8] & (0x01 << (7 - (x % 8))))
        return;

    for (l = 0, counter = 0; l < height; l++)
        if ((*up_pointers)[x][l] != 0)
            counter++;

    if (counter == 0) {
        free((char *) (*up_pointers)[x]);
        (*up_pointers)[x] = 0;
        return;
    }
    (*optimized)[x / 8] |= (unsigned char) (0x01 << (7 - (x % 8)));

    buffer = (ALI_TARRAY < ali_pathmap_up_pointer > *(*)[])
        CALLOC((unsigned int) counter + 1,
               sizeof(ALI_TARRAY < ali_pathmap_up_pointer > *));
    if (buffer == 0)
        ali_fatal_error("Out of memory");

    for (l = 0, counter = 0; l < height; l++)
        if ((*up_pointers)[x][l] != 0)
            (*buffer)[counter++] = (*up_pointers)[x][l];
    (*buffer)[counter] = 0;

    free((char *) (*up_pointers)[x]);
    (*up_pointers)[x] = (ALI_TARRAY < ali_pathmap_up_pointer > **) buffer;
}


void            ALI_PATHMAP::
print(void)
{
    ali_pathmap_up_pointer up;
    unsigned long   x, y, i;
    unsigned char   val;

    printf("PATH_MATRIX:\n");
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            if (y & 0x01)
                val = (*pathmap)[x * height_real + y / 2] & 0x0f;
            else
                val = (*pathmap)[x * height_real + y / 2] >> 4;
            printf("%d ", val);
        }
        printf("\n");
    }

    printf("UP_POINTERS:\n");
    for (x = 0; x < width; x++) {
        if ((*up_pointers)[x]) {
            printf("%3ld : ", x);
            if ((*optimized)[x / 8] & 0x01 << (7 - (x % 8))) {
                for (y = 0; (*up_pointers)[x][y] != 0; y++) {
                    printf("(");
                    for (i = 0; i < (*up_pointers)[x][y]->size(); i++) {
                        up = (*up_pointers)[x][y]->get(i);
                        printf("%ld:%d ", up.start, up.operation);
                    }
                    printf(") ");
                }
            } else {
                for (y = 0; y < height; y++) {
                    printf("(");
                    if ((*up_pointers)[x][y]) {
                        for (i = 0; i < (*up_pointers)[x][y]->size(); i++) {
                            up = (*up_pointers)[x][y]->get(i);
                            printf("%ld:%d ", up.start, up.operation);
                        }
                    }
                    printf(") ");
                }
            }
            printf("\n");
        }
    }
}






/***************************************************************
 *
 * TEST PART
 *
 ***************************************************************

#include "ali_tarray.hxx"

ALI_TARRAY<ali_pathmap_up_pointer> *array1, *array2;

void init_pathmap(ALI_PATHMAP *pmap)
{

   pmap->set(0,0,ALI_LEFT);
        pmap->set(0,1,ALI_DIAG);
        pmap->set(0,2,ALI_UP);
        pmap->set(0,3,ALI_LUP,array1);
   pmap->set(0,4,ALI_LEFT);
        pmap->set(0,5,ALI_DIAG);
        pmap->set(0,6,ALI_UP);
        pmap->set(0,7,ALI_LUP,array2);


        pmap->set(1,0,ALI_LEFT|ALI_DIAG);
        pmap->set(1,1,ALI_LEFT|ALI_UP);
        pmap->set(1,2,ALI_LEFT|ALI_LUP,array2);
        pmap->set(1,3,ALI_LEFT|ALI_DIAG);
        pmap->set(1,4,ALI_LEFT|ALI_UP);
        pmap->set(1,5,ALI_LEFT|ALI_LUP,array1);

        pmap->set(30,23,ALI_LEFT);
        pmap->set(30,24,ALI_DIAG);
        pmap->set(30,25,ALI_UP);
        pmap->set(30,26,ALI_LUP,array1);
   pmap->set(30,27,ALI_LEFT);
        pmap->set(30,28,ALI_DIAG);
        pmap->set(30,29,ALI_UP);
        pmap->set(30,30,ALI_LUP,array2);

        pmap->set(29,25,ALI_LEFT|ALI_DIAG);
        pmap->set(29,26,ALI_LEFT|ALI_UP);
        pmap->set(29,27,ALI_LEFT|ALI_LUP,array2);
        pmap->set(29,28,ALI_LEFT|ALI_DIAG);
        pmap->set(29,29,ALI_LEFT|ALI_UP);
        pmap->set(29,30,ALI_LEFT|ALI_LUP,array1);
}

void print_array(ALI_TARRAY<ali_pathmap_up_pointer> *array)
{
   unsigned long l;
        ali_pathmap_up_pointer up;

   if (array == 0)
                return;

   printf("<");
        for (l = 0; l < array->size(); l++) {
                up = array->get(l);
                printf("%d:%d ",up.start,up.operation);
        }
        printf(">");
}

void check_pathmap(ALI_PATHMAP *pmap)
{
   unsigned long l;

   unsigned char val;
        ALI_TARRAY<ali_pathmap_up_pointer> *array_of_pointer;

   printf("******************\n");
        for (l = 0; l < 8; l++) {
                pmap->get(0,l,&val,&array_of_pointer);
                printf("(0,%d)  %d ",l,val);
                print_array(array_of_pointer);
                printf("\n");
        }
        for (l = 0; l < 6; l++) {
                pmap->get(1,l,&val,&array_of_pointer);
                printf("(1,%d)  %d ",l,val);
                print_array(array_of_pointer);
                printf("\n");
        }
        for (l = 23; l < 31; l++) {
                pmap->get(29,l,&val,&array_of_pointer);
                printf("(29,%d)  %d ",l,val);
                print_array(array_of_pointer);
                printf("\n");
        }
        for (l = 25; l < 31; l++) {
                pmap->get(30,l,&val,&array_of_pointer);
                printf("(30,%d)  %d ",l,val);
                print_array(array_of_pointer);
                printf("\n");
        }
}


main()
{
   ali_pathmap_up_pointer up;
   ALI_PATHMAP *pmap;

        array1 = new ALI_TARRAY<ali_pathmap_up_pointer>(3);
        up.start = 1; up.operation = ALI_LEFT;
        array1->set(0,up);
        up.start = 3; up.operation = ALI_DIAG;
        array1->set(1,up);
        up.start = 5; up.operation = ALI_LEFT;
        array1->set(2,up);
        array2 = new ALI_TARRAY<ali_pathmap_up_pointer>(4);
        up.start = 2; up.operation = ALI_DIAG;
        array2->set(0,up);
        up.start = 4; up.operation = ALI_LEFT;
        array2->set(1,up);
        up.start = 8; up.operation = ALI_DIAG;
        array2->set(2,up);
        up.start = 16; up.operation = ALI_LEFT;
        array2->set(3,up);

        pmap = new ALI_PATHMAP(31,31);

        init_pathmap(pmap);
   pmap->print();
        check_pathmap(pmap);
        pmap->optimize(0);
        pmap->optimize(1);
        pmap->optimize(30);
        pmap->optimize(29);
        pmap->print();
        check_pathmap(pmap);
}



*****************************************************************/
