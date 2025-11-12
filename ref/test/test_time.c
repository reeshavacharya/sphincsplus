#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../params.h"
#include "../hash.h"
#include "../thash.h"
#include "../address.h"
#include "../randombytes.h"
#include "../merkle.h"
#include "../wots.h"
#include "../wotsx1.h"
#include "../utils.h"
#include "../utilsx1.h"
#include "../fors.h"
#include "../api.h"

// Define PARAMNAME based on the PARAMS define
#define XSTR(s) STR(s)
#define STR(s) #s
#define PARAMNAME XSTR(PARAMS)

#define NTESTS 100
#define MLEN 32

#include "cycles.h"

/* Reuse the same MEASURE macros as the reference benchmark to measure
 * cycles and wall time. These expect the following variables to exist
 * in scope: unsigned long long t[NTESTS+1]; struct timespec start, stop;
 * double result; int i; and an init_cpucycles() call prior to use.
 */
#define MEASURE_GENERIC(TEXT, MUL, FNCALL, CORR)\
    printf(TEXT);\
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);\
    for(i = 0; i < NTESTS; i++) {\
        t[i] = cpucycles() / CORR;\
        FNCALL;\
    }\
    t[NTESTS] = cpucycles();\
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &stop);\
    result = ((stop.tv_sec - start.tv_sec) * 1e6 + \
        (stop.tv_nsec - start.tv_nsec) / 1e3) / (double)CORR;\
    /* print a compact single-line summary (avg us and median cycles) */\
    {\
      /* compute median of deltas */\
      unsigned long long tmp[NTESTS+1];\
      for (int _j = 0; _j <= NTESTS; _j++) tmp[_j] = t[_j];\
      /* delta */\
      for (int _j = 0; _j < NTESTS; _j++) tmp[_j] = tmp[_j+1] - tmp[_j];\
      /* sort */\
      qsort(tmp, NTESTS, sizeof(unsigned long long), cmp_llu);\
      unsigned long long med = (NTESTS % 2) ? tmp[NTESTS/2] : (tmp[NTESTS/2-1] + tmp[NTESTS/2]) / 2;\
  /* convert average from microseconds to milliseconds with 5 decimal places */\
  printf("avg. %11.5lf ms; median %llu cycles\n", (result / NTESTS) / 1000.0, med);\
    }
#define MEASURT(TEXT, MUL, FNCALL)\
    MEASURE_GENERIC(\
        TEXT, MUL,\
        do {\
          for (int j = 0; j < 1000; j++) {\
            FNCALL;\
          }\
        } while (0);,\
    1000);
#define MEASURE(TEXT, MUL, FNCALL) MEASURE_GENERIC(TEXT, MUL, FNCALL, 1)

static int cmp_llu(const void *a, const void*b)
{
  if(*(unsigned long long *)a < *(unsigned long long *)b) return -1;
  if(*(unsigned long long *)a > *(unsigned long long *)b) return 1;
  return 0;
}

static unsigned long long median(unsigned long long *l, size_t llen)
{
  qsort(l, llen, sizeof(unsigned long long), cmp_llu);

  if(llen%2) return l[llen/2];
  else return (l[llen/2-1]+l[llen/2])/2;
}

static unsigned long long average(unsigned long long *t, size_t tlen)
{
  unsigned long long acc=0;
  size_t i;
  for(i=0;i<tlen;i++)
    acc += t[i];
  return acc/(tlen);
}

static void print_results(const char *s, unsigned long long *t, size_t tlen)
{
  size_t i;
  printf("%s", s);
  for(i=0;i<tlen-1;i++)
  {
    printf("%llu,", t[i]);
  }
  printf("%llu\n", t[tlen-1]);
}

int main()
{
  unsigned char pk[CRYPTO_PUBLICKEYBYTES];
  unsigned char sk[CRYPTO_SECRETKEYBYTES];
  unsigned char *m = malloc(MLEN);
  unsigned char *sm = malloc(MLEN + CRYPTO_BYTES);
  unsigned char *m2 = malloc(MLEN + CRYPTO_BYTES);
  unsigned long long smlen;
  unsigned long long mlen;
  struct timespec start, stop;
  unsigned long long t[NTESTS+1];
  double result;
  int i;

  printf("Parameters: SPHINCS+-%s using %s\n", PARAMNAME, CRYPTO_ALGNAME);
  printf("Public Key Bytes: %d\n", CRYPTO_PUBLICKEYBYTES);
  printf("Secret Key Bytes: %d\n", CRYPTO_SECRETKEYBYTES);
  printf("Signature Bytes: %d\n", CRYPTO_BYTES);

  /* Print resolved SPHINCS+ parameters from the included params header */
  printf("SPHINCS+ parameters: n=%d, full_height=%d, d=%d, fors_height=%d, fors_trees=%d, wots_w=%d\n",
         SPX_N, SPX_FULL_HEIGHT, SPX_D, SPX_FORS_HEIGHT, SPX_FORS_TREES, SPX_WOTS_W);

  /* Initialize cycle counter used by MEASURE macros */
  init_cpucycles();

  /* Prepare a random message once (MEASURE will call the functions repeatedly)
   * Note: crypto_sign will overwrite `sm` on each iteration; verify will use
   * the last-produced signature. This mirrors the approach in benchmark.c.
   */
  randombytes(m, MLEN);

  /* Measure key generation, signing and verification using MEASURE macros */
  MEASURE("Key generation:     ", 1, crypto_sign_keypair(pk, sk));
  MEASURE("Signing:            ", 1, crypto_sign(sm, &smlen, m, MLEN, sk));
  MEASURE("Verification:       ", 1, crypto_sign_open(m2, &mlen, sm, smlen, pk));

  /* Run one final functional check to ensure the last signature verifies */
  if (crypto_sign_open(m2, &mlen, sm, smlen, pk) != 0 || mlen != MLEN) {
    printf("verification failed!\n");
    return -1;
  }
  for (size_t j = 0; j < mlen; j++) {
    if (m[j] != m2[j]) {
      printf("verification failed!\n");
      return -1;
    }
  }

  free(m);
  free(sm);
  free(m2);

  return 0;
}