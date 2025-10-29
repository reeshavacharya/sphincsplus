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
  unsigned long long ns;
  unsigned long long keygen_times[NTESTS], sign_times[NTESTS], verify_times[NTESTS];
  int i;

  printf("Parameters: SPHINCS+-%s using %s\n", PARAMNAME, CRYPTO_ALGNAME);
  printf("Public Key Bytes: %d\n", CRYPTO_PUBLICKEYBYTES);
  printf("Secret Key Bytes: %d\n", CRYPTO_SECRETKEYBYTES);
  printf("Signature Bytes: %d\n", CRYPTO_BYTES);

  for(i=0; i<NTESTS; i++)
  {
    // Time key generation
    clock_gettime(CLOCK_MONOTONIC, &start);
    crypto_sign_keypair(pk, sk);
    clock_gettime(CLOCK_MONOTONIC, &stop);
    ns = ((stop.tv_sec - start.tv_sec) * 1000000000LL + (stop.tv_nsec - start.tv_nsec));
    keygen_times[i] = ns;

    randombytes(m, MLEN);

    // Time signing
    clock_gettime(CLOCK_MONOTONIC, &start);
    crypto_sign(sm, &smlen, m, MLEN, sk);
    clock_gettime(CLOCK_MONOTONIC, &stop);
    ns = ((stop.tv_sec - start.tv_sec) * 1000000000LL + (stop.tv_nsec - start.tv_nsec));
    sign_times[i] = ns;

    // Time verification
    clock_gettime(CLOCK_MONOTONIC, &start);
    crypto_sign_open(m2, &mlen, sm, smlen, pk);
    clock_gettime(CLOCK_MONOTONIC, &stop);
    ns = ((stop.tv_sec - start.tv_sec) * 1000000000LL + (stop.tv_nsec - start.tv_nsec));
    verify_times[i] = ns;

    if(mlen != MLEN)
    {
      printf("verification failed!\n");
      return -1;
    }
    for(size_t j = 0; j < mlen; j++)
    {
      if(m[j] != m2[j])
      {
        printf("verification failed!\n");
        return -1;
      }
    }
  }

  printf("\nBenchmark results for %d iterations\n", NTESTS);
  printf("All times are in nanoseconds\n");
  
  printf("\nKey Generation:\n");
  printf("Average: %llu\n", average(keygen_times, NTESTS));
  printf("Median: %llu\n", median(keygen_times, NTESTS));
  // printf("Times: ");
  // print_results("", keygen_times, NTESTS);

  printf("\nSigning:\n");
  printf("Average: %llu\n", average(sign_times, NTESTS));
  printf("Median: %llu\n", median(sign_times, NTESTS));
  // printf("Times: ");
  // print_results("", sign_times, NTESTS);

  printf("\nVerification:\n");
  printf("Average: %llu\n", average(verify_times, NTESTS));
  printf("Median: %llu\n", median(verify_times, NTESTS));
  // printf("Times: ");
  // print_results("", verify_times, NTESTS);

  free(m);
  free(sm);
  free(m2);

  return 0;
}