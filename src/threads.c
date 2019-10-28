#include "function_table.c"

#define MAX_TASK_QUEUE_SIZE 64

#define atomic_read_aligned_8(x, mem) __asm__("movb %1, %0" : "=r" (x) : "m" (*mem))
#define atomic_read_aligned_16(x, mem) __asm__("movw %1, %0" : "=r" (x) : "m" (*mem))
#define atomic_read_aligned_32(x, mem) __asm__("movl %1, %0" : "=r" (x) : "m" (*mem))
#define atomic_read_aligned_64(x, mem) __asm__("movq %1, %0" : "=r" (x) : "m" (*mem))

#define atomic_write_aligned_8(x, mem) __asm__("movb %1, %0" : "=m" (*mem) : "r" (x))
#define atomic_write_aligned_16(x, mem) __asm__("movw %1, %0" : "=m" (*mem) : "r" (x))
#define atomic_write_aligned_32(x, mem) __asm__("movl %1, %0" : "=m" (*mem) : "r" (x))
#define atomic_write_aligned_64(x, mem) __asm__("movq %1, %0" : "=m" (*mem) : "r" (x))

typedef struct {
	void **buf;
	uint64_t push_count;
	uint64_t pop_count;
} Task_queue;

typedef struct {
	unsigned char *thread_page;
	pthread_t pthread_id;
	Task_queue queue;
	uint8_t sig;
	unsigned char pad[64 - sizeof(unsigned char *) - sizeof(pthread_t) - sizeof(Task_queue) - sizeof(uint8_t)];
} Thread_info;

typedef struct {
	uint64_t task_id;
	uint64_t thread_id;
} Task_info;

typedef struct {
	uint64_t num;
	Task_info array[];
} Deps;

static inline uint64_t Task_queue_size(Task_queue *queue)
{
	uint64_t *push_count_adr = &queue->push_count;
	uint64_t *pop_count_adr = &queue->pop_count;
	uint64_t push_count;
	uint64_t pop_count;
	atomic_read_aligned_64(push_count, push_count_adr);
	atomic_read_aligned_64(pop_count, pop_count_adr);
	return push_count - pop_count;
}

static inline uint64_t Task_queue_next_push(Task_queue *queue)
{
	uint64_t *push_count_adr = &queue->push_count;
	uint64_t push_count;
	atomic_read_aligned_64(push_count, push_count_adr);
	uint64_t idx = push_count & (MAX_TASK_QUEUE_SIZE - 1);
	return idx;
}

static inline uint64_t Task_queue_next_pop(Task_queue *queue)
{
	uint64_t *pop_count_adr = &queue->pop_count;
	uint64_t pop_count;
	atomic_read_aligned_64(pop_count, pop_count_adr);
	uint64_t idx = pop_count & (MAX_TASK_QUEUE_SIZE - 1);
	return idx;
}

static inline uint64_t Task_queue_next_push_id(Task_queue *queue)
{
	uint64_t *push_count_adr = &queue->push_count;
	uint64_t push_count;
	atomic_read_aligned_64(push_count, push_count_adr);
	return push_count;
}

static inline uint64_t Task_queue_next_pop_id(Task_queue *queue)
{
	uint64_t *pop_count_adr = &queue->pop_count;
	uint64_t pop_count;
	atomic_read_aligned_64(pop_count, pop_count_adr);
	return pop_count;
}

static inline void Task_queue_push(Task_queue *queue)
{
	uint64_t *push_count_adr = &queue->push_count;
	uint64_t push_count;
	atomic_read_aligned_64(push_count, push_count_adr);
	++push_count;
	atomic_write_aligned_64(push_count, push_count_adr);
}

static inline void Task_queue_pop(Task_queue *queue)
{
	uint64_t *pop_count_adr = &queue->pop_count;
	uint64_t pop_count;
	atomic_read_aligned_64(pop_count, pop_count_adr);
	++pop_count;
	atomic_write_aligned_64(pop_count, pop_count_adr);
}

static void *threads_startup_task(void *restrict param)
{
	static void (*const func_table[])(void *) = {f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13};
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	uintptr_t id = (uintptr_t)param;
	Thread_info *cur_tr_adr = &threads[id];
	Task_queue* queue_adr = &cur_tr_adr->queue;
	for(;;) {
		uint64_t size = Task_queue_size(queue_adr);
		while(size) {
			uint64_t idx = Task_queue_next_pop(queue_adr);
			void **mem = queue_adr->buf + idx * 2;

			void *restrict args = *mem;
			Deps *restrict deps = (void *)*(mem + 1);
			int func = *(int *)args;

			for(uint64_t i = 0; i < deps->num; ++i) {
				uint64_t task_id = deps->array[i].task_id;
				uint64_t thread_id = deps->array[i].thread_id;
				Task_queue* dep_queue_adr = &threads[thread_id].queue;
				while(task_id >= Task_queue_next_pop_id(dep_queue_adr)) {
					_mm_pause();
				}
			}

			func_table[func](args);

			_ReadWriteBarrier();

			Task_queue_pop(queue_adr);
			--size;
		}
		if(cur_tr_adr->sig == 4) {
			return NULL;
		}
		if(cur_tr_adr->sig == 1) {
			uint8_t *sig_adr = &cur_tr_adr->sig;
			uint8_t sig = 2;
			atomic_write_aligned_8(sig, sig_adr);
			atomic_read_aligned_8(sig, sig_adr);
			while(sig != 0) {
				_mm_pause();
				atomic_read_aligned_8(sig, sig_adr);
			}
			sig = 3;
			atomic_write_aligned_8(sig, sig_adr);
		}
		_mm_pause();
	}
}

static void threads_init(void)
{
	uint64_t *thread_num_adr = page_alloc_lines(THREAD_SPACE, 1);
	unsigned long size = 0x1FFC0;
	PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX buffer = page_alloc_lines(THREAD_SPACE, size);
	ASSERT(GetLogicalProcessorInformationEx(RelationProcessorCore, buffer, &size));
	*thread_num_adr = (uint64_t)size / buffer[0].Size - 1;
	uint64_t thread_num = *thread_num_adr;
	page_free_lines(THREAD_SPACE, 0x1FFC0);
	Thread_info *threads = page_alloc_lines(THREAD_SPACE, thread_num);
	void **mem = page_alloc(THREAD_SPACE, sizeof(void *) * 2 * MAX_TASK_QUEUE_SIZE, thread_num);
	pthread_attr_t attr;
	ASSERT_POSIX(pthread_attr_init(&attr));
	for(uint64_t i = 0; i < thread_num; ++i) {
		threads[i].thread_page = allocator_alloc_pages(16);
		threads[i].queue.buf = &mem[i * 2 * MAX_TASK_QUEUE_SIZE];
		threads[i].queue.push_count = 1;
		threads[i].queue.pop_count = 1;
		threads[i].sig = 0;
		ASSERT_POSIX(pthread_create(&(threads[i].pthread_id), &attr, threads_startup_task, (void *)(uintptr_t)i));
	}
}

static void threads_deinit(void)
{
	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	for(uint64_t i = 0; i < thread_num; ++i) {
		uint8_t* sig_adr = &threads[i].sig;
		uint8_t sig = 4;
		atomic_write_aligned_8(sig, sig_adr);
	}
	for(uint64_t i = 0; i < thread_num; ++i) {
		ASSERT_POSIX(pthread_join(threads[i].pthread_id, NULL));
		allocator_free_pages(threads[i].thread_page);
	}
}

#define THREADS_SCHEDULE_TASK(...) \
__extension__ ({ \
	__VA_ARGS__ \
	__threads_schedule_task(args, sizeof(*args), deps, sizeof(*deps) + sizeof(Task_info) * deps->num); \
})

static Task_info __threads_schedule_task(void *restrict args, size_t size_args, Deps *restrict deps, size_t size_deps)
{
	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	static uint64_t cnt = 0;
	Thread_info *cur_tr_adr;
	for(;;) {
		for(; cnt < thread_num; ++cnt) {
			uint64_t size = Task_queue_size(&threads[cnt].queue);
			if(size < MAX_TASK_QUEUE_SIZE) {
				cur_tr_adr = &threads[cnt];
				goto exit_loop;
			}
		}
		cnt = 0;
	}
	exit_loop:;
	Task_queue* queue_adr = &cur_tr_adr->queue;
	uint64_t task_id = Task_queue_next_push_id(queue_adr);
	uint64_t idx = task_id & (MAX_TASK_QUEUE_SIZE - 1);
	void **mem = queue_adr->buf + idx * 2;
	
	*mem = page_alloc_wrap(cur_tr_adr->thread_page, size_args, 1);
	memcpy(*mem, args, size_args);

	*(mem + 1) = page_alloc_wrap(cur_tr_adr->thread_page, size_deps, 1);
	memcpy(*(mem + 1), deps, size_deps);

	_ReadWriteBarrier();

	Task_queue_push(queue_adr);
	
	return (Task_info){task_id, cnt};
}

#define THREADS_WAIT_FOR_TASKS(...) \
__extension__ ({ \
	__VA_ARGS__ \
	__threads_wait_for_tasks(deps); \
})

static void __threads_wait_for_tasks(Deps *restrict deps)
{
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	for(uint64_t i = 0; i < deps->num; ++i) {
		uint64_t task_id = deps->array[i].task_id;
		uint64_t thread_id = deps->array[i].thread_id;
		Task_queue* dep_queue_adr = &threads[thread_id].queue;
		while(task_id >= Task_queue_next_pop_id(dep_queue_adr))
			_mm_pause();
	}
}

static void threads_pause(void)
{
	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	for(uint64_t i = 0; i < thread_num; ++i) {
		uint8_t* sig_adr = &threads[i].sig;
		uint8_t sig = 1;
		atomic_write_aligned_8(sig, sig_adr);
	}
}

static void threads_resume(void)
{
	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	for(uint64_t i = 0; i < thread_num; ++i) {
		uint8_t* sig_adr = &threads[i].sig;
		uint8_t sig = 0;
		atomic_write_aligned_8(sig, sig_adr);
	}
}

static void threads_sync(uint8_t sig)
{
	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);
	
	for(uint64_t i = 0; i < thread_num; ++i) {
		uint8_t* sig_adr = &threads[i].sig;
		uint8_t _sig;
		atomic_read_aligned_8(_sig, sig_adr);
		while(_sig != sig) {
			_mm_pause();
			atomic_read_aligned_8(_sig, sig_adr);
		}
	}
}

static void threads_clear(void)
{
	threads_pause();
	threads_sync(2);

	uint64_t thread_num = *(uint64_t *)(void *)(THREAD_SPACE);
	Thread_info *threads = (void *)(THREAD_SPACE + 64);

	for(uint64_t i = 0; i < thread_num; ++i) {
		Thread_info* cur_tr_adr = &threads[i];
		Task_queue* queue_adr = &cur_tr_adr->queue;
		while(Task_queue_size(queue_adr))
			_mm_pause();
		queue_adr->push_count = 0;
		queue_adr->pop_count = 0;
	}

	threads_resume();
	threads_sync(3);
}
