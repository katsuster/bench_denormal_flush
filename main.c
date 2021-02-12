#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#if defined(__x86_64)
#include <pmmintrin.h>
#endif

#define SIZE    10000

union uuu {
	double f;
	long long n;
};

volatile double zero = 0.0f;
volatile union uuu a, b, c, d, ab, ac, ca, dc;
union uuu normal[SIZE], denormal[SIZE],	dst[SIZE];

void set_fpu_mode(int ftz, int daz)
{
#if defined(__x86_64)
	if (ftz) {
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
	} else {
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_OFF);
	}
	if (daz) {
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	} else {
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_OFF);
	}
#elif defined(__ARM_NEON)
	uint32_t fpscr;
	__asm__ volatile("mrs %0, fpcr" : "=r"(fpscr));
	if (ftz) {
		fpscr |= (1 << 24);
	} else {
		fpscr &= ~(1 << 24);
	}
	__asm__ volatile("msr fpcr, %0" : : "r"(fpscr));
#endif
}

void test_flush(void)
{
	a.n = 0x0008000000000000ULL;
	b.n = 0x0004000000000000ULL;
	c.n = 0x0010000000000000ULL;
	d.n = 0x0014000000000000ULL;
	ab.f = a.f + b.f;
	ac.f = a.f + c.f;
	ca.f = c.f - a.f;
	dc.f = d.f - c.f;

	printf("  a   : %e(0x%08llx)\n"
		"  b   : %e(0x%08llx)\n"
		"  c   : %e(0x%08llx)\n"
		"  d   : %e(0x%08llx)\n"
		"  a+b : %e(0x%08llx)\n"
		"  a+c : %e(0x%08llx)\n"
		"  c-a : %e(0x%08llx)\n"
		"  d-c : %e(0x%08llx)\n"
		"  a==0: %d\n\n",
		a.f, a.n, b.f, b.n, c.f, c.n, d.f, d.n,
		ab.f, ab.n, ac.f, ac.n, ca.f, ca.n, dc.f, dc.n,
		a.f == zero);
}

void test_flush_for_all(void)
{
#if defined(__x86_64) || defined(__ARM_NEON)
	printf("FTZ:OFF, DAZ:OFF\n");
	set_fpu_mode(0, 0);
	test_flush();
#endif

#if defined(__x86_64)
	printf("FTZ:OFF, DAZ:ON\n");
	set_fpu_mode(0, 1);
	test_flush();
#endif

#if defined(__x86_64)
	printf("FTZ:ON, DAZ:OFF\n");
	set_fpu_mode(1, 0);
	test_flush();
#endif

#if defined(__x86_64) || defined(__ARM_NEON)
	printf("FTZ:ON, DAZ:ON\n");
	set_fpu_mode(1, 1);
	test_flush();
#endif
}

void test_speed_init(void)
{
	union uuu a, b;

	a.f = 1.0f;
	b.n = 0x0000000000000100ULL;

	for (int i = 0; i < SIZE; i++) {
		normal[i].n = a.n;
		denormal[i].n = b.n;
	}
}

void test_speed_result(struct timeval *tv_start)
{
	struct timeval tv_end, tv_ela;

	gettimeofday(&tv_end, NULL);
	timersub(&tv_end, tv_start, &tv_ela);

	printf("  %d.%06d[s]\n", (int)tv_ela.tv_sec, (int)tv_ela.tv_usec);

	printf("  dst : %e(0x%08llx)\n", dst[0].f, dst[0].n);
}

void test_speed(union uuu *val)
{
	for (int i = 0; i < SIZE; i++) {
		dst[i].f =
			val[i].f / 1.08f -
			val[i].f / 1.07f +
			val[i].f / 1.06f -
			val[i].f / 1.05f +
			val[i].f / 1.04f -
			val[i].f / 1.03f +
			val[i].f / 1.02f -
			val[i].f / 1.01f;
	}
}

void test_speed_for_all(int count, union uuu *val)
{
	struct timeval tv_start;
	int i;

	test_speed_init();

#if defined(__x86_64) || defined(__ARM_NEON)
	printf("FTZ:OFF, DAZ:OFF\n");
	set_fpu_mode(0, 0);
	gettimeofday(&tv_start, NULL);
	for (i = 0; i < count; i++) {
		test_speed(val);
	}
	test_speed_result(&tv_start);
#endif

#if defined(__x86_64)
	printf("FTZ:OFF, DAZ:ON\n");
	set_fpu_mode(0, 1);
	gettimeofday(&tv_start, NULL);
	for (i = 0; i < count; i++) {
		test_speed(val);
	}
	test_speed_result(&tv_start);
#endif

#if defined(__x86_64)
	printf("FTZ:ON, DAZ:OFF\n");
	set_fpu_mode(1, 0);
	gettimeofday(&tv_start, NULL);
	for (i = 0; i < count; i++) {
		test_speed(val);
	}
	test_speed_result(&tv_start);
#endif

#if defined(__x86_64) || defined(__ARM_NEON)
	printf("FTZ:ON, DAZ:ON\n");
	set_fpu_mode(1, 1);
	gettimeofday(&tv_start, NULL);
	for (i = 0; i < count; i++) {
		test_speed(val);
	}
	test_speed_result(&tv_start);
#endif
}

int main(int argc, char *argv[])
{
	int count;

	if (argc <= 1) {
		printf("usage:\n  %s loops\n", argv[0]);
		return -1;
	}
	count = atoi(argv[1]);

	test_flush_for_all();

	printf("%d loops\n", count);

	printf("\nNormal -----\n");
	test_speed_for_all(count, normal);

	printf("\nDenormal -----\n");
	test_speed_for_all(count, denormal);

	return 0;
}
