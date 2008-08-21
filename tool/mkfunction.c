
/*
** This file contains a standalone program used to generate C code that
** implements a static hash table to store the definitions of built-in
** SQL functions in SQLite. 
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
** The SQLite source file "func.c" is included below.
**
** By defining the 4 macros and typedef below before including "func.c",
** most of the code is excluded. What is left is an array of constant
** strings, aBuiltinFunc[], containing the names of SQLite's built-in 
** SQL functions. i.e.:
**
**   const char aBuiltinFunc[] = { "like", "glob", "min", "max" ... };
**
** The data from aBuiltinFunc[] is used by this program to create the
** static hash table.
*/
#define CREATE_BUILTIN_HASHTABLE 1
#define FUNCTION(zName,w,x,y,z)    #zName
#define AGGREGATE(zName,v,w,x,y,z) #zName
#define LIKEFUNC(zName,x,y,z)      #zName
#define FuncDef const char *

#include "func.c"

/* The number of buckets in the static hash table. */
#define HASHSIZE 127

typedef unsigned char u8;

/* An array to map all upper-case characters into their corresponding
** lower-case character. 
*/
static const u8 sqlite3UpperToLower[] = {
      0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17,
     18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
     36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53,
     54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 97, 98, 99,100,101,102,103,
    104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,
    122, 91, 92, 93, 94, 95, 96, 97, 98, 99,100,101,102,103,104,105,106,107,
    108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,
    126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
    144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,
    162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,
    180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,
    198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,
    216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,
    234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,
    252,253,254,255
};
#define UpperToLower sqlite3UpperToLower

int sqlite3StrICmp(const char *zLeft, const char *zRight){
  register unsigned char *a, *b;
  a = (unsigned char *)zLeft;
  b = (unsigned char *)zRight;
  while( *a!=0 && UpperToLower[*a]==UpperToLower[*b]){ a++; b++; }
  return UpperToLower[*a] - UpperToLower[*b];
}

static int hashstring(const char *zName){
  int ii;
  unsigned int iKey = 0;
  for(ii=0; zName[ii]; ii++){
    iKey = (iKey<<3) + (u8)sqlite3UpperToLower[(u8)zName[ii]];
  }
  iKey = iKey%HASHSIZE;
  return iKey;
}

static void printarray(const char *zName, u8 *aArray, int nArray){
  int ii;
  printf("  static u8 %s[%d] = {", zName, nArray);
  for(ii=0; ii<nArray; ii++){
    if( (ii%16)==0 ){
      printf("\n    ");
    }
    printf("%2d, ", aArray[ii]);
  }
  printf("\n  };\n");
}


int main(int argc, char **argv){
  int nFunc;              /* Number of entries in the aBuiltinFunc array */

  u8 anFunc[256];
  u8 aHash[HASHSIZE];
  u8 aNext[256];
  int ii;
  int iHead;

  nFunc = (sizeof(aBuiltinFunc)/sizeof(const char *));
  assert(nFunc<256);

  memset(aHash, (unsigned char)nFunc, sizeof(aHash));
  memset(aNext, (unsigned char)nFunc, sizeof(aNext));
  memset(anFunc, 0, sizeof(anFunc));

  iHead = -1;
  for(ii=0; ii<nFunc; ii++){
    int iHash;

    if( iHead>=0 && 0==sqlite3StrICmp(aBuiltinFunc[ii], aBuiltinFunc[iHead]) ){
      anFunc[iHead]++;
      continue;
    }else{
      /* The routine generated by this program assumes that if there are
      ** two or more entries in the aBuiltinFunc[] array with the same
      ** name (i.e. two versions of the "max" function), then they must
      ** be stored in adjacent slots. The following block detects the
      ** problem if this is not the case.
      */
      int jj;
      for(jj=0; jj<ii; jj++){
        if( 0==sqlite3StrICmp(aBuiltinFunc[ii], aBuiltinFunc[jj]) ){
          fprintf(stderr, "Error in func.c\n");
          return -1;
        }
      }

      iHead = ii;
      anFunc[iHead] = 1;
    }

    iHash = hashstring(aBuiltinFunc[ii]);
    if( aHash[iHash]!=nFunc ){
      int iNext = aHash[iHash];
      while( aNext[iNext]!=nFunc ){
        iNext = aNext[iNext];
      }
      aNext[iNext] = ii;
    }else{
      aHash[iHash] = ii;
    }
  }

  printf(
  "/******* Automatically Generated code - do not edit **************/\n"
  "int sqlite3GetBuiltinFunction(\n"
  "  const char *zName,   \n"
  "  int nName, \n"
  "  FuncDef **paFunc\n"
  "){\n"
  );

  printarray("aHash", aHash, HASHSIZE);
  printarray("anFunc", anFunc, nFunc);
  printarray("aNext", aNext, nFunc);
  printf("  FuncDef *pNoFunc = &aBuiltinFunc[%d];\n", nFunc);

  printf(
  "  unsigned int iKey = 0;  /* Hash of case-insensitive string zName. */\n"
  "  int ii;\n"
  "  FuncDef *pFunc;\n"
  "\n"
  );
  printf(
  "  assert( (sizeof(aBuiltinFunc)/sizeof(aBuiltinFunc[0]))==%d );\n", nFunc
  );
  printf(
  "  /* Generate the hash of zName */\n"
  "  for(ii=0; ii<nName; ii++){\n"
  "    iKey = (iKey<<3) + (u8)sqlite3UpperToLower[(u8)zName[ii]];\n"
  "  }\n"
  "  iKey = iKey%%127;\n"
  "\n"
  "  pFunc = &aBuiltinFunc[iKey = aHash[iKey]];\n"
  "  while( pFunc!=pNoFunc && sqlite3StrNICmp(pFunc->zName, zName, nName) ){\n"
  "    pFunc = &aBuiltinFunc[iKey = aNext[iKey]];\n"
  "  }\n"
  "\n"
  "  *paFunc = pFunc;\n"
  "  return anFunc[iKey];\n"
  "}\n"
  );

  return 0;
}
