#ifndef _RANDOM_H
#define _RANDOM_H


#define SEED() ({  unsigned long long  __t; __asm__ __volatile__ ("rdtsc" : "=A" (__t)); __t; })


static inline unsigned pseudo_random32(unsigned seed)
{
        /* Pseudo random number generator from numerical recipes */
        return seed * 1664525 + 1013904223;
}

#endif
