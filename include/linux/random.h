/*
 * include/linux/random.h
 *
 * Include file for the random number generator.
 */

#ifndef _LINUX_RANDOM_H
#define _LINUX_RANDOM_H

#include <uapi/linux/random.h>

extern void add_device_randomness(const void *, unsigned int);
extern void add_input_randomness(unsigned int type, unsigned int code,
				 unsigned int value) __latent_entropy;
extern void add_interrupt_randomness(int irq, int irq_flags) __latent_entropy;

extern void get_random_bytes(void *buf, int nbytes);
extern void get_random_bytes_arch(void *buf, int nbytes);
void erandom_get_random_bytes(char *buf, size_t count);
void generate_random_uuid(unsigned char uuid_out[16]);
extern int random_int_secret_init(void);

#ifndef MODULE
extern const struct file_operations random_fops, urandom_fops;
#endif

unsigned int get_random_int(void);
unsigned long get_random_long(void);
unsigned long randomize_range(unsigned long start, unsigned long end, unsigned long len);

u32 prandom_u32(void);
void prandom_bytes(void *buf, int nbytes);
void prandom_seed(u32 seed);
void prandom_reseed_late(void);

/*
 * These macros are preserved for backward compatibility and should be
 * removed as soon as a transition is finished.
 */
#define random32() prandom_u32()
#define srandom32(seed) prandom_seed(seed)

u32 prandom_u32_state(struct rnd_state *state);
void prandom_bytes_state(struct rnd_state *state, void *buf, int nbytes);

/*
 * Handle minimum values for seeds
 */
static inline u32 __seed(u32 x, u32 m)
{
	return (x < m) ? x + m : x;
}

/**
 * prandom_seed_state - set seed for prandom_u32_state().
 * @state: pointer to state structure to receive the seed.
 * @seed: arbitrary 64-bit value to use as a seed.
 */
static inline void prandom_seed_state(struct rnd_state *state, u64 seed)
{
	u32 i = (seed >> 32) ^ (seed << 10) ^ seed;

	state->s1 = __seed(i,   2U);
	state->s2 = __seed(i,   8U);
	state->s3 = __seed(i,  16U);
	state->s4 = __seed(i, 128U);
}

#ifdef CONFIG_ARCH_RANDOM
# include <asm/archrandom.h>
#else
static inline int arch_get_random_long(unsigned long *v)
{
	return 0;
}
static inline int arch_get_random_int(unsigned int *v)
{
	return 0;
}
static inline int arch_has_random(void)
{
	return 0;
}
static inline int arch_get_random_seed_long(unsigned long *v)
{
	return 0;
}
static inline int arch_get_random_seed_int(unsigned int *v)
{
	return 0;
}
static inline int arch_has_random_seed(void)
{
	return 0;
}
#endif

#endif /* _LINUX_RANDOM_H */
