#define ASSERT(expr) \
	(!!(expr)) ? (void)1 : (debug_print_error(__FILE__, __LINE__))

#define ASSERT_POSIX(expr) \
	(!(expr)) ? (void)1 : (debug_function_print_error(__FILE__, (expr), __LINE__))

_Noreturn static void debug_function_print_error(const char *file, long code, int line)
{
	char buf[300];
    sprintf(buf, "File: %s\r\nError code: %ld\r\nLine: %d\r\n", file, code, line);
	HANDLE fp = CreateFile(L"log_error.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long num;
    WriteFile(fp, buf, (uint32_t)strlen(buf), &num, NULL);
    CloseHandle(fp);
    abort();
}

_Noreturn static void debug_print_error(const char *file, int line)
{
	char buf[300];
    sprintf(buf, "File: %s\r\nError code: %u\r\nLine: %d\r\n", file, (uint32_t)GetLastError(), line);
	HANDLE fp = CreateFile(L"log_error.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	unsigned long num;
    WriteFile(fp, buf, (uint32_t)strlen(buf), &num, NULL);
    CloseHandle(fp);
    abort();
}

static void memdump(void* mem, size_t size)
{
	unsigned char* umem = mem;
	printf("Start\n");
	for(size_t i = 0; i < size; ++i) {
		printf("%u ", umem[i]);
		if(!((i + 1) & 15))
			printf("\n");
		if(!((i + 1) & 63))
			printf("\n");
	}
	printf("End\n");
}
