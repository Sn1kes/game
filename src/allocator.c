static struct Allocator {
	unsigned char *p_start;
	size_t offset;
} allocator;

#define MAX_SPACE_SIZE (size_t)0x10000000
#define STATE_SPACE    (unsigned char *)(allocator.p_start + 0x40)
#define STATIC_SPACE   (unsigned char *)(allocator.p_start + 0x800040)
#define FRAME_SPACE    (unsigned char *)(allocator.p_start + 0x1000040)
#define THREAD_SPACE   (unsigned char *)(allocator.p_start + 0x1800040)
#define STRING_SPACE   (unsigned char *)(allocator.p_start + 0x2000040)

typedef struct {
	size_t size;
	size_t offset;
	size_t offset_front;
} Page;

static void allocator_init(void)
{
	ASSERT(allocator.p_start = VirtualAlloc(NULL, MAX_SPACE_SIZE, MEM_RESERVE, PAGE_READWRITE));
	Page *page;
	ASSERT(page = VirtualAlloc(STATE_SPACE - 0x40, 0x800000, MEM_COMMIT, PAGE_READWRITE));
	page->size = 0x800000;
	page->offset = 64;
	ASSERT(page = VirtualAlloc(STATIC_SPACE - 0x40, 0x800000, MEM_COMMIT, PAGE_READWRITE));
	page->size = 0x800000;
	page->offset = 64;
	ASSERT(page = VirtualAlloc(FRAME_SPACE - 0x40, 0x800000, MEM_COMMIT, PAGE_READWRITE));
	page->size = 0x800000;
	page->offset = 64;
	ASSERT(page = VirtualAlloc(THREAD_SPACE - 0x40, 0x800000, MEM_COMMIT, PAGE_READWRITE));
	page->size = 0x800000;
	page->offset = 64;
	ASSERT(page = VirtualAlloc(STRING_SPACE - 0x40, 0x800000, MEM_COMMIT, PAGE_READWRITE));
	page->size = 0x800000;
	page->offset = 64;
	allocator.offset = 0x5000000;
}

static void allocator_deinit(void)
{
	ASSERT(VirtualFree(allocator.p_start, 0, MEM_RELEASE));
}

static unsigned char *allocator_alloc_pages(size_t pages_num)
{
	Page *page;
	size_t size = pages_num * 4096;
	ASSERT(allocator.offset + size <= MAX_SPACE_SIZE);
	ASSERT(page = VirtualAlloc(allocator.p_start + allocator.offset, size, MEM_COMMIT, PAGE_READWRITE));
	allocator.offset += size;
	page->size = size;
	page->offset = 64;
	return (unsigned char *)(void *)page + 64;
}

static void allocator_free_pages(unsigned char *page_u)
{
	Page *page = (void *)(page_u - 64);
	ASSERT(VirtualFree(page, page->size, MEM_DECOMMIT));
}

static void *page_alloc(unsigned char *page_u, size_t size, size_t count)
{
	Page *page = (void *)(page_u - 64);
	size = (size + 63) & (size_t)-64;
	size *= count;
	ASSERT(size + page->offset <= page->size);
	unsigned char *bytes = (void *)page;
	bytes += page->offset;
	page->offset += size;
	return bytes;
}

static void *page_alloc_wrap(unsigned char *page_u, size_t size, size_t count)
{
	Page *page = (void *)(page_u - 64);
	size = (size + 63) & (size_t)-64;
	size *= count;
	if(size + page->offset <= page->size) {
		unsigned char *bytes = (void *)page;
		bytes += page->offset;
		page->offset += size;
		return bytes;
	}
	page->offset = 64 + size;
	return page_u;
}

static void page_free(unsigned char *page_u, size_t size, size_t count)
{
	Page *page = (void *)(page_u - 64);
	size = (size + 63) & (size_t)-64;
	size *= count;
	ASSERT(page->offset >= size + 64);
	page->offset -= size;
}

static void *page_alloc_lines(unsigned char *page_u, size_t lines)
{
	Page *page = (void *)(page_u - 64);
	size_t size = lines * 64;
	ASSERT(size + page->offset <= page->size);
	unsigned char *bytes = (void *)page;
	bytes += page->offset;
	page->offset += size;
	return bytes;
}

static void *page_alloc_lines_wrap(unsigned char *page_u, size_t lines)
{
	Page *page = (void *)(page_u - 64);
	size_t size = lines * 64;
	if(size + page->offset <= page->size) {
		unsigned char *bytes = (void *)page;
		bytes += page->offset;
		page->offset += size;
		return bytes;
	}
	page->offset = 64 + size;
	return page_u;
}

static inline void page_free_lines(unsigned char *page_u, size_t lines)
{
	Page *page = (void *)(page_u - 64);
	size_t size = lines * 64;
	ASSERT(page->offset >= size + 64);
	page->offset -= size;
}

static inline void page_free_all(unsigned char* page_u)
{
	Page *page = (void *)(page_u - 64);
	page->offset = 64;
}
