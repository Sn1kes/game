//Written once at startup
struct {
    size_t MAX_SPACE_SIZE;
    void *STATE_SPACE;
    void *FRAME_SPACE;
    void *THREAD_SPACE;
    void *STRING_SPACE;
} static ALLOCATOR_CONSTANTS;

struct Allocator {
	unsigned char *p_start;
	size_t offset;
};

struct Page {
	size_t size;
	size_t offset;
};

static struct Allocator allocator_init(void)
{
    /*const HANDLE proc_handle = GetCurrentProcess();
    {
        HANDLE token_handle;
        ASSERT(OpenProcessToken(proc_handle, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token_handle));
        TOKEN_USER user_info;
        DWORD req_size = 0;
        ASSERT(GetTokenInformation(token_handle, TokenUser, NULL, 0, &req_size));
        ASSERT(GetTokenInformation(token_handle, TokenUser, &user_info, req_size, &req_size));
        const PSID user_sid_addr = user_info.User.Sid;
        LSA_HANDLE policy_handle;
        LSA_OBJECT_ATTRIBUTES loa;
        ZeroMemory(&loa, sizeof(LSA_OBJECT_ATTRIBUTES));
        ASSERT_POSIX(LsaOpenPolicy(NULL, &loa, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES, &policy_handle));
        LSA_UNICODE_STRING privilege_str = {
            .Length = sizeof(SE_LOCK_MEMORY_NAME) - sizeof(wchar_t),
            .MaximumLength = sizeof(SE_LOCK_MEMORY_NAME),
            .Buffer = SE_LOCK_MEMORY_NAME
        };
        ASSERT_POSIX(LsaAddAccountRights(policy_handle, user_sid_addr, &privilege_str, 1));

        LUID lock_privilege_luid;
        ASSERT(LookupPrivilegeValue(NULL, SE_LOCK_MEMORY_NAME, &lock_privilege_luid));
        TOKEN_PRIVILEGES privileges = {
            .PrivilegeCount = 1,
            .Privileges     = {{.Luid = lock_privilege_luid, .Attributes = SE_PRIVILEGE_ENABLED}}
        };
        ASSERT(AdjustTokenPrivileges(token_handle, FALSE, &privileges, 0, NULL, NULL));
        ASSERT(CloseHandle(token_handle));
    }*/
	const size_t MAX_SPACE_SIZE = 0x10000000;
    unsigned char *const restrict start = VirtualAlloc(NULL, MAX_SPACE_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	ASSERT(start);

    {
        struct Page *const restrict page_1 = (void *)start;
        struct Page *const restrict page_2 = (void *)(start + 0x800000);
        struct Page *const restrict page_3 = (void *)(start + 0x1000000);
        struct Page *const restrict page_4 = (void *)(start + 0x1800000);
        page_1->size = 0x800000;
        page_1->offset = 64;
        page_2->size = 0x800000;
        page_2->offset = 64;
        page_3->size = 0x800000;
        page_3->offset = 64;
        page_4->size = 0x800000;
        page_4->offset = 64;
    }

	ALLOCATOR_CONSTANTS.MAX_SPACE_SIZE 	= MAX_SPACE_SIZE;
    ALLOCATOR_CONSTANTS.STATE_SPACE  	= start + 0x40;
    ALLOCATOR_CONSTANTS.FRAME_SPACE  	= start + 0x800040;
    ALLOCATOR_CONSTANTS.THREAD_SPACE 	= start + 0x1000040;
    ALLOCATOR_CONSTANTS.STRING_SPACE 	= start + 0x1800040;

	return (struct Allocator){.p_start = start, .offset = 0x2000000};
}

__vectorcall static void allocator_deinit(const struct Allocator allocator)
{
	ASSERT(VirtualFree(allocator.p_start, 0, MEM_RELEASE));
}

__vectorcall static void *page_alloc(unsigned char *const restrict page_u, const size_t size, const size_t count)
{
	struct Page *const restrict page = (void *)(page_u - 64);
	const size_t page_offset         = page->offset;
	const size_t page_size           = page->size;
	const size_t all_size            = ((size + 63) & (size_t)-64) * count;

	ASSERT(all_size + page_offset <= page_size);
	unsigned char *const bytes = (unsigned char *)((void *)page) + page_offset;
	page->offset += all_size;
	return bytes;
}

__vectorcall static void page_free(unsigned char *const restrict page_u, const size_t size, const size_t count)
{
	struct Page *const restrict page = (void *)(page_u - 64);
	const size_t page_offset         = page->offset;
    const size_t all_size            = ((size + 63) & (size_t)-64) * count;

	ASSERT(page_offset >= all_size + 64);
	page->offset -= all_size;
}

__vectorcall static void *page_alloc_lines(unsigned char *const restrict page_u, const size_t lines)
{
	struct Page *const restrict page = (void *)(page_u - 64);
    const size_t page_offset         = page->offset;
    const size_t page_size           = page->size;
    const size_t all_size            = lines * 64;

	ASSERT(all_size + page_offset <= page_size);
	unsigned char *const bytes = (unsigned char *)((void *)page) + page_offset;
	page->offset += all_size;
	return bytes;
}

__vectorcall static void page_free_lines(unsigned char *const restrict page_u, size_t lines)
{
	struct Page *const restrict page = (void *)(page_u - 64);
	const size_t page_offset         = page->offset;
    const size_t all_size            = lines * 64;

	ASSERT(page_offset >= all_size + 64);
	page->offset -= all_size;
}

__vectorcall static void page_free_all(unsigned char *const restrict page_u)
{
	struct Page *const restrict page = (void *)(page_u - 64);
	page->offset = 64;
}
