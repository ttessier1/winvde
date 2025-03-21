#pragma once

/* BITARRAY (C) 2005 Renzo Davoli
 * Licensed under the GPLv2
 * Modified by Ludovico Gardenghi 2005
 *
 * A bitarray is (a pointer to) an array of memory words, can be used as a set.
 * +--------------------------------+--------------------------------+----
 * |33222222222211111111110000000000|66665555555555444444444433333333|999...
 * |10987654321098765432109876543210|32109876543210987654321098765432|543...
 * +--------------------------------+--------------------------------+----
 *
 * e.g. bit number 33 is the second LSB of the second word (in a 32 bit machine)
 *
 * bitarrays must be allocated bu ba_alloc
 * ba_realloc must know the old and the new size of the bitarray
 * ba_check checks a bit (returns 0 if cleared, some value != 0 it set)
 * ba_set sets a bit
 * ba_clr clears a bit
 * ba_FORALL computes an expression for each set bit,
 *           K is an integer var, must be defined in advance.
 *           it is the number of the set bit when the expression is evaluated
 * ba_FORALLFUN calls a function: first arg the index, second arg is ARG
 * ba_card counts how many bits are set
 *
 * bac_ functions allocate one more trailing word to the bit array to store
 * the cardinality of the set (# of set bits).
 * This is useful when dealing with large sparse maybe empy sets.
 * bac_set/CLEAR are slightly more expensive but
 * all the FORALL functions shortcut as soon as no more elements can be found.
 * If the set is empty the BAC FORALL macros exit immediately.
 * *** warning in case of memory leak may loop or segfault if the cardinality is
 *     overwritten ***
 *
 * Macro summary
 *
 * #define ba_alloc(N)
 * #define ba_realloc(B,N,M)
 * #define ba_check(X,I)
 * #define ba_set(X,I)
 * #define ba_clr(X,I)
 * #define ba_zap(X,N)
 * #define ba_FORALLFUN(X,N,F,ARG)
 * #define ba_FORALL(X,N,EXPR,K)
 * #define ba_card(X,N)
 * #define ba_empty(X,N)
 * #define ba_copy(DST,SRC,N)     *** MUST HAVE THE SAME SIZE
 * #define ba_add(DST,SRC,N)      *** MUST HAVE THE SAME SIZE
 * #define ba_remove(DST,SRC,N)   *** MUST HAVE THE SAME SIZE
 * #define ba_negate(X,N)
 *
 * #define bac_alloc(N)
 * #define bac_realloc(B,N,M)
 * #define bac_check(X,I)
 * #define bac_set(X,N,I)
 * #define bac_clr(X,N,I)
 * #define bac_zap(X,N)
 * #define bac_FORALLFUN(X,N,F,ARG)
 * #define bac_FORALL(X,N,EXPR,K)
 * #define bac_card(X,N)
 * #define bac_empty(X,N)
 * #define bac_copy(DST,SRC,N)   *** MUST HAVE THE SAME SIZE
 */

#ifndef _BITARRAY_H
#define _BITARRAY_H
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <limits.h>
#include <string.h>


#if __LONG_MAX__ == 2147483647L || SIZE_MAX == 0xffffffffui32                      /* 32 bits */
# define __VDEWORDSIZE 32
# define __LOG_WORDSIZE (5)
# define __WORDSIZEMASK 0x1f
#elif __LONG_MAX__ == 9223372036854775807L || SIZE_MAX == 0xffffffffffffffffui64    /* 64 bits */
# define __VDEWORDSIZE 64
# define __LOG_WORDSIZE (6)
# define __WORDSIZEMASK 0x3f
#else
# error sorry this program has been tested only on 32 or 64 bit machines
#endif

#define __WORDSIZE_1 (__VDEWORDSIZE-1)
#define __WORDSIZEROUND(VX) ((VX + __WORDSIZE_1) >> __LOG_WORDSIZE)

typedef unsigned long bitarrayelem;
typedef bitarrayelem* bitarray;

/* Simple Bit Array */

static inline bitarray ba_alloc(int n)
{
	return (bitarray)calloc(__WORDSIZEROUND(n), sizeof(unsigned long));
}

static inline bitarray ba_realloc(bitarray b, int n, int m)
{
	int __i;
	bitarray nb = (bitarray)realloc(b, __WORDSIZEROUND(m) * sizeof(unsigned long));
	if (nb != NULL)
	{
		for (__i = __WORDSIZEROUND(n); __i < __WORDSIZEROUND(m); __i++)
		{
			nb[__i] = 0;
		}
		nb[__WORDSIZEROUND(n) - 1] &= 
			(-1) >> (((0 + (n)) % __VDEWORDSIZE)*-1);
	}
	return nb;
}

static inline int ba_check(bitarray x, int index)
{
	return (x && (x[index >> __LOG_WORDSIZE] & (1L << (index & __WORDSIZEMASK))));
}

static inline void ba_set(bitarray x, int index)
{
	x[index >> __LOG_WORDSIZE] |= (1L << (index & __WORDSIZEMASK));
}

static inline void ba_clr(bitarray x, int index)
{
	x[index >> __LOG_WORDSIZE] &= ~(1L << (index & __WORDSIZEMASK));
}

static inline void ba_zap(bitarray x, int n)
{
	unsigned int __i;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		x[__i] = 0;
}

#define ba_FORALLFUN(A,X,N,F,ARG) \
	unsigned int __iffa##A,__jffa##A; \
	 bitarrayelem __vffa##A; \
	 uint32_t maxffa##A=__WORDSIZEROUND(N); \
	 for (__iffa##A=0; __iffa##A< maxffa##A; __iffa##A++) \
	 for (__vffa##A=(X)[__iffa##A],__jffa##A=0; __jffa##A < __VDEWORDSIZE; __vffa##A >>=1, __jffa##A++) \
	 if (__vffa##A & 1) (F)(__iffa##A*__VDEWORDSIZE+__jffa##A,(ARG)); \
	 0;

#define ba_FORALL(A,X,N,EXPR,K) \
	unsigned int __ifa##A,__jfa##A; \
	 bitarrayelem __vfa##A; \
	 uint32_t maxfa##A=__WORDSIZEROUND(N); \
	 for (__ifa##A=0; __ifa##A< maxfa##A; __ifa##A++) \
	 for (__vfa##A=(X)[__ifa##A],__jfa##A=0; __jfa##A < __VDEWORDSIZE; __vfa##A >>=1, __jfa##A++) \
	 if (__vfa##A & 1) {(K)=__ifa##A*__VDEWORDSIZE+__jfa##A;(EXPR);} \
	 (K);

static inline int ba_card(bitarray x, int n)
{
	unsigned int __i, __j, __n = 0;
	bitarrayelem __v;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		for (__v = (x)[__i], __j = 0; __j < __VDEWORDSIZE; __v >>= 1, __j++)
			if (__v & 1) __n++;
	return __n;
}

static inline void ba_empty(bitarray x, int n)
{
	unsigned int __i;
	bitarrayelem __v = 0;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		__v |= (x)[__i]; \
}

static inline void ba_copy(bitarray dst, bitarray src, int n)
{
	memcpy(dst, src, sizeof(bitarrayelem) * __WORDSIZEROUND(n));
}

static inline void ba_add(bitarray dst, bitarray src, int n)
{
	unsigned int __i;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		dst[__i] |= src[__i];
}

static inline void ba_remove(bitarray dst, bitarray src, int n)
{
	unsigned int __i;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		dst[__i] &= ~(src[__i]);
}

static inline void ba_negate(bitarray x, int n)
{
	unsigned int __i;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		x[__i] = ~(x[__i]);
}

/* Bit Array with Cardinality (Count of set bits) */
/* it is stored after the last element */

static inline bitarray bac_alloc(int n)
{
	return (bitarray)calloc(__WORDSIZEROUND(n) + 1, sizeof(unsigned long));
}

static inline bitarray bac_realloc(bitarray b, int n, int m)
{
	int __i;
	int __size = b[__WORDSIZEROUND(n)];
	bitarray nb = (bitarray)realloc(b, (__WORDSIZEROUND(m) + 1) * sizeof(unsigned long));
	if (nb != NULL) {
		b[__WORDSIZEROUND(m)] = __size;
		for (__i = __WORDSIZEROUND(n); __i < __WORDSIZEROUND(n); __i++)
			nb[__i] = 0;
		nb[__WORDSIZEROUND(n) - 1] &= (-1) >> ((0U - n) % __VDEWORDSIZE);
	}
	
	return nb;
}

/* ba_check and bac_check are the same */
static inline int bac_check(bitarray x, int index)
{
	return (x && (x[index >> __LOG_WORDSIZE] & (1L << (index & __WORDSIZEMASK))));
}

static inline void bac_set(bitarray x, int n, int index)
{
	bitarrayelem __v = x[index >> __LOG_WORDSIZE];
	bitarrayelem __w = __v;
	__v |= (1L << (index & __WORDSIZEMASK));
	if (__v != __w) x[index >> __LOG_WORDSIZE] = __v, (x[__WORDSIZEROUND(n)]++);
}

static inline void bac_clr(bitarray x, int n, int index)
{
	bitarrayelem __v = x[index >> __LOG_WORDSIZE];
	bitarrayelem __w = __v;
	__v &= ~(1L << (index & __WORDSIZEMASK));
	if (__v != __w) x[index >> __LOG_WORDSIZE] = __v, (x[__WORDSIZEROUND(n)]--);
}

static inline void bac_zap(bitarray x, int n)
{
	unsigned int __i;
	uint32_t max = __WORDSIZEROUND(n);
	for (__i = 0; __i < max; __i++)
		x[__i] = 0;
	x[__i] = 0;
}

#define bac_FORALLFUN(A,X,N,F,ARG) \
	unsigned int __ifaf##A,__jfaf##A; \
	 bitarrayelem __vfaf##A; \
	 int __nfaf##A=(X)[__WORDSIZEROUND(N)]; \
	 for (__ifaf##A=0; __nfaf##A > 0; __ifaf##A++) \
	 for (__vfaf##A=(X)[__ifaf##A],__jfaf##A=0; __jfaf##A < __VDEWORDSIZE; __vfaf##A >>=1, __jfaf##A++) \
	 if (__vfaf##A & 1) (F)(__ifaf##A*__VDEWORDSIZE+__jfaf##A,(ARG)),__nfaf##A--; \
	 0;

#define bac_FORALL(A,X,N,EXPR,K) \
	unsigned int __ifa##A,__jfa##A; \
	 bitarrayelem __vfa##A; \
	 int __nfa##A=(X)[__WORDSIZEROUND(N)]; \
	 for (__ifa##A=0; __nfa##A > 0; __ifa##A++) \
	 for (__vfa##A=(X)[__ifa##A],__jfa##A=0; __jfa##A < __VDEWORDSIZE; __vfa##A >>=1, __jfa##A++) \
	 if (__vfa##A & 1) (K)=__ifa##A*__VDEWORDSIZE+__jfa##A,(EXPR),__nfa##A--; \
	 0;

static inline int bac_card(bitarray x, int n)
{
	return(x[__WORDSIZEROUND(n)]);
}

static inline int bac_empty(bitarray x, int n)
{
	return(x[__WORDSIZEROUND(n)] == 0);
}

static inline void bac_copy(bitarray dst, bitarray src, int n)
{
	memcpy(dst, src, sizeof(bitarrayelem) * (__WORDSIZEROUND(n) + 1));
}

#if defined TEST_BITARRAY
#include <stdio.h>
/* usage example */
int fun(int i, int arg)
{
	printf("I %d\n", i);
	return -1;
}

// example progname.exe 128
int main(int argc, char* argv[])
{
	bitarray b;
	int k;
	if (argc != 2) return 0;
	int val = atoi(argv[1]);
	if (val < 34) return 0;
	printf("%d -round-> %d\n", val, __WORDSIZEROUND(val));
	b = ba_alloc(val);
	ba_set(b, 31);
	ba_set(b, 33);
	printf("%d -> %d\n", 31, ba_check(b, 31));
	printf("%d -> %d\n", 33, ba_check(b, 33));
	printf("CARD %d\n", ba_card(b, val));
	ba_FORALLFUN(1,b, val, fun, 0);
	ba_FORALL(1,b, val, (printf("E1 %d\n", k)), k);
	printf("RE127\n");
	b = ba_realloc(b, val, 127);
	ba_FORALL(2,b, 127, (printf("E2 %d\n", k)), k);
	printf("RE42\n");
	b = ba_realloc(b, 127, 42);
	ba_FORALL(3,b, 127, (printf("E3 %d\n", k)), k);
	ba_clr(b, 31);
	printf("%d -> %d\n", 31, ba_check(b, 31));
	printf("CARD %d\n", ba_card(b, 42));
	ba_clr(b, 33);
	printf("%d -> %d\n", 33, ba_check(b, 33));
	printf("CARD %d\n", ba_card(b, 42));
	b = bac_alloc(val);
	if (argc != 2) return 0;
	printf("%d -> %d\n", val, __WORDSIZEROUND(val));
	bac_set(b, val, 31);
	bac_set(b, val, 33);
	printf("%d -> %d\n", 31, bac_check(b, 31));
	printf("%d -> %d\n", 33, bac_check(b, 33));
	printf("CARD %d\n", bac_card(b, val));
	bac_FORALLFUN(4,b, val, fun, 0);
	bac_FORALL(5,b, val, (printf("E1 %d\n", k)), k);
	printf("RE127\n");
	printf("CARD %d\n", bac_card(b, val));
	b = bac_realloc(b, val, 127);
	bac_FORALL(6,b, 127, (printf("E2 %d\n", k)), k);
	printf("RE42\n");
	b = bac_realloc(b, 127, 42);
	bac_FORALL(7,b, 42, (printf("E3 %d\n", k)), k);
	bac_clr(b, 42, 31);
	printf("%d -> %d\n", 31, bac_check(b, 31));
	printf("CARD %d\n", bac_card(b, 42));
	bac_clr(b, 42, 33);
	printf("%d -> %d\n", 33, bac_check(b, 33));
	printf("CARD %d\n", bac_card(b, val));
}
#endif
#endif
