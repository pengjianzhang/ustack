/*
 * config.h 
 *
 *
 * Author:	Peng Jianzhang
 * Email:	pengjianzhang@gmail.com
 * Time:	2012.12.31
 * */

#ifndef _CONFIG_H
#define _CONFIG_H

#define MALLOC	malloc
#define FREE	free
#define CALLOC	calloc

#define CONFIG_CON_TAB_BITS 10  /*recommend 20 */


/* For GCC 4.x*/

#define likely(x) (x)
#define unlikely(x) (__builtin_expect((unsigned long)(x), 0))

#define prefetch(x) (x) 
//#define prefetch(x) __builtin_prefetch(x)


/* timer.c */
#define DEFAULT_CPU_FREQUENCY_MHZ	(2600)
#define DEFAULT_SECONDS	(60*5)

#define CAS(_a, _o, _n)  __sync_val_compare_and_swap(_a, _o, _n)
#define bCAS(_a,_o,_n)  __sync_bool_compare_and_swap(_a,_o,_n)


#define barrier() __asm__ __volatile__("" : : : "memory")

#define mb() asm volatile("mfence":::"memory")
#define rmb() asm volatile("lfence":::"memory")
#define wmb() asm volatile("sfence" ::: "memory")



#define ____cacheline_aligned __attribute__((__aligned__(64)))

#endif
