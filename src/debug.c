#define ASSERT(expr) \
	if(__builtin_expect(!(expr), 0)) \
		do { \
			debug_print_error(__FILE__, __LINE__); \
			assert(!(expr)); \
		} while(0)

#define ASSERT_POSIX(expr) \
	if(__builtin_expect(!!(expr), 0)) \
		do { \
			debug_print_error(__FILE__, __LINE__); \
			assert(!!(expr)); \
		} while(0)

static void debug_print_error(const char *file, int line)
{
	char buf[300];
    sprintf(buf, "File: %s\r\nLine: %d\r\n", file, line);
	const HANDLE fp = CreateFile(TEXT("log_error.txt"), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long num;
    WriteFile(fp, buf, (uint32_t)strlen(buf), &num, NULL);
    CloseHandle(fp);
}

#ifdef DEBUG
static void memdump(void *const mem, size_t size)
{
	unsigned char *const restrict umem = mem;
	printf("Start\n");
	for(size_t i = 0; i < size; ++i) {
		printf("%x ", umem[i]);
		if(!((i + 1) & 15))
			printf("\n");
		if(!((i + 1) & 63))
			printf("\n");
	}
	printf("End\n");
}

static void print_m128(__m128 val)
{
	float out[4];
	_mm_store_ps(out, val);
	printf("%.6f %.6f %.6f %.6f\n", (double)out[0], (double)out[1], (double)out[2], (double)out[3]);
}
#endif

#ifdef DEBUG
#define DEBUG_MEASURE(expr) \
	do { \
 		LARGE_INTEGER measure_li; \
 		QueryPerformanceFrequency(&measure_li); \
 		double measure_freq = measure_li.QuadPart; \
 		QueryPerformanceCounter(&measure_li); \
        int64_t measure_time_start = measure_li.QuadPart; \
        expr; \
        QueryPerformanceCounter(&measure_li); \
        int64_t measure_time_end = measure_li.QuadPart; \
 		printf("Measured: %f us\n", ((double)((measure_time_end - measure_time_start) * 1000000) / measure_freq)); \
	} while(0)
#else
#define DEBUG_MEASURE(expr) \
	do { \
 		expr; \
	} while(0)
#endif
