#ifndef __LAMA_RUNTIME__
#define __LAMA_RUNTIME__

# include <stdio.h>
# include <stdio.h>
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
# include <sys/mman.h>
# include <assert.h>
# include <errno.h>
# include <regex.h>
# include <time.h>
# include <limits.h>

//#define DEBUG_PRINT

# define STRING_TAG  0x00000001
# define ARRAY_TAG   0x00000003
# define SEXP_TAG    0x00000005
# define CLOSURE_TAG 0x00000007 
# define UNBOXED_TAG 0x00000009 // Not actually a tag; used to return from LkindOf

# define LEN(x) ((x & 0xFFFFFFF8) >> 3)
# define TAG(x)  (x & 0x00000007)

# define TO_DATA(x) ((data*)((char*)(x)-sizeof(int)))
# define TO_SEXP(x) ((sexp*)((char*)(x)-2*sizeof(int)))
#ifdef DEBUG_PRINT // GET_SEXP_TAG is necessary for printing from space
# define GET_SEXP_TAG(x) (LEN(x))
#endif

# define UNBOXED(x)  (((int) (x)) &  0x0001)
# define UNBOX(x)    (((int) (x)) >> 1)
# define BOX(x)      ((((int) (x)) << 1) | 0x0001)

#define WORD_SIZE (CHAR_BIT * sizeof(int))

typedef struct {
  int tag; 
  char contents[0];
} data; 

typedef struct {
  int tag; 
  data contents; 
} sexp;

extern size_t __gc_stack_top, __gc_stack_bottom;

extern void* alloc    (size_t);
extern void* Bsexp    (int n, ...);
extern int   LtagHash (char*);
char* de_hash (int n);

extern void* Bsexp___(int bn, int tag);
extern void* Bstring___(void *p);
extern void* Bclosure___(int bn, void *entry);
extern void* Barray___(int bn);

extern void Bmatch_failure(void *v, char *fname, int line, int col);
extern void* Bstringval (void *p);


extern void __gc_root_scan_stack();

#endif