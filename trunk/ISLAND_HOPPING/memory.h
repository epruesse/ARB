void     clearUp(void);
void    *newBlock(size_t s);
void     freeBlock(void **v);
void   **newMatrix(size_t nrow,size_t ncol,size_t s);
void     freeMatrix(void ***m);

#define newVector(n,s) newBlock((n)*(s))

#define freeBlock(v) freeBlock((void **)(v))

#define newDoubleVector(n) (double *)(newVector((size_t)(n),sizeof(double)))
#define newFloatVector(n)  (float *)(newVector((size_t)(n),sizeof(float)))
#define newShortVector(n)  (short *)(newVector((size_t)(n),sizeof(short)))
#define newLongVector(n)   (long *)(newVector((size_t)(n),sizeof(long)))
#define newIntVector(n)    (int *)(newVector((size_t)(n),sizeof(int)))
#define newCharVector(n)   (char *)(newVector((size_t)(n),sizeof(char)))

#define newDoubleMatrix(r,c) (double **)(newMatrix((size_t)(r),(size_t)(c),sizeof(double)))
#define newFloatMatrix(r,c) (float **)(newMatrix((size_t)(r),(size_t)(c),sizeof(float)))
#define newShortMatrix(r,c) (short **)(newMatrix((size_t)(r),(size_t)(c),sizeof(short)))
#define newLongMatrix(r,c) (long **)(newMatrix((size_t)(r),(size_t)(c),sizeof(long)))
#define newIntMatrix(r,c) (int **)(newMatrix((size_t)(r),(size_t)(c),sizeof(int)))
#define newCharMatrix(r,c) (char **)(newMatrix((size_t)(r),(size_t)(c),sizeof(char)))
#define freeMatrix(m) freeMatrix((void ***)(m))


