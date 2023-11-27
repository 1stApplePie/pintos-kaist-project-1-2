#include "threads/thread.h"
#include <debug.h>
#include <stddef.h>
#include <random.h>
#include <stdio.h>
#include <string.h>
#include "threads/flags.h"
#include "threads/interrupt.h"
#include "threads/intr-stubs.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "intrinsic.h"
#ifdef USERPROG
#include "userprog/process.h"
#endif

/* 스레드 구조체의 `magic` 멤버를 위한 임의의 값.
   스택 오버플로우를 감지하는 데 사용됩니다. 자세한 내용은
   thread.h 상단의 큰 주석을 참조하세요. */
#define THREAD_MAGIC 0xcd6abf4b //스텍 오버플로우 감지

/* 기본 스레드를 위한 임의의 값
   이 값을 수정하지 마십시오. */
#define THREAD_BASIC 0xd42df210 //스레드에 사용되는 특정 값이 임의로 설정되어 있으며, 변경x

/* THREAD_READY 상태인 프로세스들의 목록, 즉 실행할 준비는 되었으나
   실제로 실행 중이지 않은 프로세스들입니다. */
static struct list ready_list; //THREAD_READY 상태인 프로세스들의 목록
//THREAD_READY : 프로세스가 실행을 준비는 마쳤지만, 현재는 실행 중이지 않는 상태

/* Idle thread. */
static struct thread *idle_thread;

/* 초기 스레드, init.c:main()을 실행하는 스레드입니다. */
static struct thread *initial_thread; //시스템이 시작될 때 가장 먼저 실행되는 초기 스레드

/* allocate_tid()에 의해 사용되는 락입니다. */
static struct lock tid_lock; //lock-> 동시성 제어를 위해 사용되는 동기화 메커니즘

/* 스레드 소멸 요청 */
static struct list destruction_req; //스레드의 생명 주기를 관리->스레드를 소멸시키기 위한 요청

/* Statistics. */
static long long idle_ticks;    /* # of timer ticks spent idle. */
static long long kernel_ticks;  /* # of timer ticks in kernel threads. */
static long long user_ticks;    /* # of timer ticks in user programs. */

/* Scheduling. */
#define TIME_SLICE 4            /* 각 스레드에 할당할 타이머 틱의 수. ->스레드에 주어진 cpu시간을 측정하는 단위*/
static unsigned thread_ticks;   /* 마지막 yield 이후의 타이머 틱 수. */
//yield->스레드가 cpu사용을 자발적으로 양보하는 것

/* false(기본값)일 경우, 라운드-로빈 스케줄러를 사용합니다.
   true일 경우, 다중 레벨 피드백 큐 스케줄러를 사용합니다.
   커널 커맨드 라인 옵션 "-o mlfqs"에 의해 제어됩니다. */
//라운드-로빈 스케줄러는 기본값, 다중 레벨 피드백 큐 스케줄러는 커널 커맨드 라인 옵션을 통해 활성화 가능
bool thread_mlfqs; //false:라운드-로빈, true:다중 레벨 피드백 큐

static void kernel_thread (thread_func *, void *aux); //커널 스레드 생성 함수

static void idle (void *aux UNUSED); //시스템이 유휴 상태일 때 실행되는 함수
static struct thread *next_thread_to_run (void); //다음에 실행될 스레드를 결정하는 함수
static void init_thread (struct thread *, const char *name, int priority); //스레드를 초기화하는 함수 (이름+우선순위 결정)
static void do_schedule(int status); //주어진 상태에 따라 스케줄링 작업을 수행하는 함수
static void schedule (void); //스레드 스케줄링을 수행하는 함수
static tid_t allocate_tid (void); //새로운 스레드ID를 할당하는 함수

// 1주차 -> alarm 구현
static struct list sleep_list; // sleeping 스레드 리스트
static int64_t tick_to_awake;

/* T가 유효한 스레드를 가리키는 것으로 보이면 true를 반환합니다. */
#define is_thread(t) ((t) != NULL && (t)->magic == THREAD_MAGIC)

/* 실행 중인 스레드를 반환합니다.
 * CPU의 스택 포인터 `rsp`를 읽은 다음,
 * 그 값을 페이지의 시작으로 반올림합니다. `struct thread`는
 * 항상 페이지의 시작 부분에 위치하고 스택 포인터는 중간 어딘가에 있기 때문에,
 * 이 방법으로 현재 스레드를 찾을 수 있습니다. */
#define running_thread() ((struct thread *) (pg_round_down (rrsp ()))) 
//1.cpu의 스택 포인터를 반환하는 'rrsp()'함수를 호출
//2.그 결과를 pg_round_downg함수를 사용하여 페이지의 시작 주소로 반올림
//3.계산된 주소는 struct thread타입으로 캐스팅되어 현재 실행 중인 스레드의 구조체 포인터를 제공
//->현재 실행중인 스레드의 'struct thread'구조체를 찾는 데 사용


// 스레드 시작을 위한 전역 디스크립터 테이블(GDT).
// GDT는 스레드 초기화 이후에 설정될 것이므로,
// 임시 GDT를 먼저 설정해야 합니다.
static uint64_t gdt[3] = { 0, 0x00af9a000000ffff, 0x00cf92000000ffff }; 
//운영체제의 스레드 시작 과정에서 사용되는 전역 디스크립터 테이블(GDT)에 관한 것
//GDT는 메모리 세그먼트에 대한 정보를 저장하는 테이블
//시스템의 스레드 초기화가 완료된 후에 설정되지만, 초기 스레드 시작 시 임시 gdt를 먼저 설정해야 함을 설정
//목적 : 스레드가 올바르게 실행될 수 있도록 메모리 접근과 관련된 정보를 제공



/* 현재 실행 중인 코드를 스레드로 변환함으로써 스레딩 시스템을 초기화합니다.
   이 방법은 일반적으로 작동하지 않지만, loader.S가 스택의 하단을
   페이지 경계에 맞추어 놓았기 때문에 이 경우에만 가능합니다.

   또한 실행 큐와 tid 락도 초기화합니다.

   이 함수를 호출한 후에는 thread_create()를 사용하여 어떠한 스레드도
   생성하기 전에 페이지 할당자를 초기화해야 합니다.

   이 함수가 완료될 때까지 thread_current()를 호출하는 것은 안전하지 않습니다. */
void
thread_init (void) {
	ASSERT (intr_get_level () == INTR_OFF); //인터럽트가 비활성화되었는지 확인

	/* 임시 GDT를 커널용으로 다시 로드합니다.
	 * 이 GDT는 사용자 컨텍스트를 포함하지 않습니다.
	 * 커널은 사용자 컨텍스트를 포함한 GDT를 gdt_init()에서 다시 구축할 것입니다. */
	struct desc_ptr gdt_ds = {
		.size = sizeof (gdt) - 1, //gdt의 크기 설정
		.address = (uint64_t) gdt //gdt의 주소 설정
	};
	lgdt (&gdt_ds); //gdt를 로드

	/* 전역 스레드 컨텍스트를 초기화합니다 */
	lock_init (&tid_lock);// 스레드 ID 락을 초기화합니다.
	list_init (&ready_list);// 준비 상태의 스레드 목록을 초기화합니다.
	list_init (&sleep_list);
	list_init (&destruction_req);// 소멸 요청 목록을 초기화합니다.

	/* 실행 중인 스레드를 위한 스레드 구조체를 설정합니다. */
	initial_thread = running_thread ();// 현재 실행 중인 스레드를 가져옴
	init_thread (initial_thread, "main", PRI_DEFAULT);// 초기 스레드를 초기화, 이름은 "main", 우선순위는 기본값으로 설정
	initial_thread->status = THREAD_RUNNING; //초기 스레드의 상태를 실행중으로 설정
	initial_thread->tid = allocate_tid (); //초기 스레드에 스레드 ID를 할당
}

/* 인터럽트를 활성화하여 선점형 스레드 스케줄링을 시작합니다.
   또한 idle 스레드를 생성합니다. */
void
thread_start (void) {
	/* idle 스레드 생성. */
	struct semaphore idle_started; //idle스레드가 시작되었는지를 나타내는 세마포어 선언
	sema_init (&idle_started, 0); //idle스레드 시작 세마포어를 초기화
	thread_create ("idle", PRI_MIN, idle, &idle_started); //최소 우선순위로 idle스레드 생성
	
	//메타메타몽 알람
	tick_to_awake=	INT64_MAX; //다음 깨어날 스레드의 시간을 최대값으로 설정

	/* 선점형 스레드 스케줄링 시작. */
	intr_enable (); //인터럽트를 활성화하여 선점형 스케줄링을 가능하게 함

	/* idle스레드가 idle_thread를 초기화할 때까지 대기*/
	sema_down (&idle_started); //idle스레드가 준비될 때까지 대기
}

/* 타이머 인터럽트 핸들러가 각 타이머 틱마다 호출합니다.
   따라서, 이 함수는 외부 인터럽트 컨텍스트에서 실행됩니다. */
void
thread_tick (void) {
	struct thread *t = thread_current (); //현재 실행 중인 스레드를 가져옴

	/* Update statistics. */
	if (t == idle_thread)
		idle_ticks++; //현재 스레드가 idle스레드인 경우 idle_ticks증가
#ifdef USERPROG
	else if (t->pml4 != NULL)
		user_ticks++; //사용자 프로그램이 실행 중인 경우 user_ticks증가
#endif
	else
		kernel_ticks++; //그 외의 경우(커널 모드에서 실행 중인 경우) kernel_ticks 증가

	/* 선점 강제 실행 */
	if (++thread_ticks >= TIME_SLICE)
		intr_yield_on_return (); //실행 중인 스레드의 틱 수가 TIME_SLICE초과하면 선점을 강제
}

/* Prints thread statistics. */
void
thread_print_stats (void) {
	printf ("Thread: %lld idle ticks, %lld kernel ticks, %lld user ticks\n",
			idle_ticks, kernel_ticks, user_ticks);
}

/* 주어진 초기 우선순위 PRIORITY로 NAME이라는 이름의 새 커널 스레드를 생성하며,
   FUNCTION을 실행하여 AUX를 인자로 전달하고, 준비 큐에 추가합니다. 
   새 스레드의 스레드 식별자를 반환하거나, 생성에 실패하면 TID_ERROR를 반환합니다.

   thread_start()가 호출되었다면, 새 스레드는 thread_create()가 반환되기 전에
   스케줄될 수 있습니다. 심지어 thread_create()가 반환되기 전에 종료될 수도 있습니다.
   반대로, 원래 스레드는 새 스레드가 스케줄되기 전에 어느 정도 시간 동안 실행될 수 있습니다.
   순서를 보장하고 싶다면 세마포어 또는 다른 형태의 동기화를 사용하세요.

   제공된 코드는 새 스레드의 `priority' 멤버를 PRIORITY로 설정하지만,
   실제 우선순위 스케줄링은 구현되어 있지 않습니다.
   우선순위 스케줄링은 문제 1-3의 목표입니다. */

//새로운 커널 스레드 생성 및 초기화 수행
//1. 스레드에 필요한 메모리를 할당
//2. 스레드의 기본 속성을 설정
//3. 실행 큐에 추가하여 스케줄링될 수 있도록 준비
tid_t
thread_create (const char *name, int priority, thread_func *function, void *aux) {
	struct thread *t; //새 스레드를 위한 구조체 포인터 선언
	tid_t tid; //스레드의 식별자 선언

	ASSERT (function != NULL);//전달된 function포인터가 null이 아닌지 확인

	/* 스레드 할당 */
	t = palloc_get_page (PAL_ZERO);//페이지 할당 함수를 사용하여 스레드에 메모리 할당,초기화
	if (t == NULL)
		return TID_ERROR; //할당 실패 시 TID_ERROR반환

	/* 스레드 초기화 */
	init_thread (t, name, priority); //스레드 초기화 함수 호출(이름과 우선순위 설정)
	tid = t->tid = allocate_tid (); //스레드 식별자 할당 및 설정

	/* 스케줄된 경우 kernel_thread 호출
	 * 주의)rdi는 첫 번째 인자, rsi는 두 번째 인자 */
	t->tf.rip = (uintptr_t) kernel_thread;//스레드의 명령 포인터를 kernel_thread 함수로 설정
	t->tf.R.rdi = (uint64_t) function; //첫 번째 인자로 function설정
	t->tf.R.rsi = (uint64_t) aux; //두 번째 인자로 aux설정
	t->tf.ds = SEL_KDSEG; //데이터 세그먼트 셀렉터 설정
	t->tf.es = SEL_KDSEG; //추가 데이터 세그먼트 셀렉터 설정
	t->tf.ss = SEL_KDSEG; //스택 세그먼트 셀렉터 설정
	t->tf.cs = SEL_KCSEG; //코드 세그먼트 셀렉터 설정
	t->tf.eflags = FLAG_IF; //EFLAGS 레지스터 설정(인터럽트 활성화)

	/* 실행 큐에 추가 */
	thread_unblock (t); //스레드를 실행 큐에 추가하여 실행 가능 상태로 만듦

	return tid; //할당된 스레드 식별자 반환
}

/* 현재 스레드를 수면 상태로 전환합니다. thread_unblock()에 의해 깨어날 때까지 스케줄되지 않습니다.

   이 함수는 인터럽트가 꺼진 상태에서 호출되어야 합니다.
   synch.h에 있는 동기화 기본 요소 중 하나를 사용하는 것이 보통 더 좋은 방법입니다. */
void
thread_block (void) {
	ASSERT (!intr_context ()); //인터럽트 컨텍스트 내부가 아닌지 확인
	ASSERT (intr_get_level () == INTR_OFF); //인터럽트가 꺼져 있는지 확인
	thread_current ()->status = THREAD_BLOCKED; //현재 스레드의 상태를 blocked로 설정
	schedule (); //다음 스레드로 스케줄링
}

/* 차단된 상태의 스레드 T를 준비 상태로 전환합니다.
   T가 차단되지 않은 상태라면 에러입니다. (실행 중인 스레드를 준비 상태로 만들기 위해서는 thread_yield()를 사용하세요.)

   이 함수는 실행 중인 스레드를 선점하지 않습니다. 이는 중요할 수 있습니다:
   만약 호출자가 직접 인터럽트를 비활성화했다면, 스레드를 원자적으로 차단 해제하고
   다른 데이터를 업데이트할 수 있다고 기대할 수 있습니다. */
void
thread_unblock (struct thread *t) {
	enum intr_level old_level;

	ASSERT (is_thread (t)); //t가 유효한 스레드인지 확인

	old_level = intr_disable ();//인터럽트 비활성화, 이전 인터럽트 레벨 저장
	ASSERT (t->status == THREAD_BLOCKED);//스레드가 blocked상태인지 확인
	list_push_back (&ready_list, &t->elem);//스레드를 준비 리스트에 추가
	
	// list_insert_ordered(&ready_list, &t->elem,thread_get_priority,NULL);
	t->status = THREAD_READY;//스레드의 상태를 thread_ready로 변경
	intr_set_level (old_level); //인터럽트 레벨을 원래대로 복구
}

/* 실행 중인 스레드의 이름을 반환합니다. */
const char *
thread_name (void) {
	return thread_current ()->name; //현재 스레드의 'name'필드를 반환
}

/* 실행 중인 스레드를 반환합니다.
   이 함수는 running_thread()에 몇 가지 정합성 검사를 추가한 것입니다.
   자세한 내용은 thread.h 파일 상단의 주석을 참조하세요. */
//스택 오버플로우와 같은 잠재적인 문제를 감지하는 데 도움
struct thread *
thread_current (void) {
	struct thread *t = running_thread (); //현재 실행 중인 스레드를 반환

	/* T가 실제로 스레드인지 확인합니다.
	   이 두 가지 단언문 중 하나라도 실패하면, 해당 스레드는
	   스택을 오버플로우했을 수 있습니다. 각 스레드는 4kB 미만의 스택을 가지므로,
	   큰 자동 배열이나 적당한 재귀는 스택 오버플로우를 일으킬 수 있습니다. */
	ASSERT (is_thread (t)); //t가 유효한 스레드인지 확인
	ASSERT (t->status == THREAD_RUNNING);//t의 상태가 '실행 중'인지 확인

	return t; //현재 실행중인 스레드를 반환
}

/* 실행 중인 스레드의 tid를 반환합니다. */
tid_t
thread_tid (void) {
	return thread_current ()->tid; //현재 스레드의 tid 필드를 선언
}

/* 현재 스레드를 일정에서 제외하고 파괴합니다. 호출자에게는 절대 반환되지 않습니다. */
void
thread_exit (void) {
	ASSERT (!intr_context ()); //인터럽트 컨텍스트가 아닌지 확인

#ifdef USERPROG
	process_exit (); //사용자 프로그램의 경우, 관련 프로세스 종료 처리를 수행
#endif

		/* 단순히 우리의 상태를 '죽음'으로 설정하고 다른 프로세스를 스케줄합니다.
	   schedule_tail() 호출 중에 파괴될 것입니다. */
	intr_disable (); //인터럽트 비활성화
	do_schedule (THREAD_DYING); //스레드의 상태를 thread_dying으로 설정하고 스케줄링
	NOT_REACHED (); //이 지점에 도달해서는 안됨(?)
}

/* CPU를 양보합니다. 현재 스레드는 수면 상태로 전환되지 않으며,
   스케줄러의 판단에 따라 즉시 다시 스케줄될 수 있습니다. */
void
thread_yield (void) {
	struct thread *curr = thread_current (); //현재 실행중인 스레드를 가져옴
	enum intr_level old_level;

	ASSERT (!intr_context ()); //인터럽트 컨텍스트가 아닌지 확인

	old_level = intr_disable (); //인터럽트를 비활성화, 이전 인터럽트 레벨 저장
	if (curr != idle_thread) //현재 스레드가 idle_thread가 아니라면
		list_push_back (&ready_list, &curr->elem); //준비 리스트에 추가
	do_schedule (THREAD_READY); //스레드의 상태를 'thread_ready'로 변경 후 스케줄링
	intr_set_level (old_level); //인터럽트 레벨을 원래대로 복구
} //->현재 스레드가 cpu사용을 자발적으로 양보하도록 함.
//스레드를 수면 상태로 전환x, 준비 리스트에 다시 추가하여 즉시 혹은 나중에 다시 스케줄링 할 수 있도록 함
//목적: 멀티태스킹 환경에서 스레드 간의 공정한 cpu시간 분배를 돕는 데 중요한 역할을 함

/* 현재 스레드의 우선순위를 NEW_PRIORITY로 설정합니다. */
void
thread_set_priority (int new_priority) {
	thread_current ()->priority = new_priority;//현재 스레드의 우선순위를 새로운 값으로 변경
}

/* 현재 스레드의 우선순위를 반환합니다. */
int
thread_get_priority (void) {
	return thread_current ()->priority; //현재 스레드의 우선순위를 반환
}

/* 현재 스레드의 nice 값을 NICE로 설정합니다. */
void
thread_set_nice (int nice UNUSED) {
	/* TODO: Your implementation goes here */
}

/* 현재 스레드의 nice 값을 반환합니다. */
int
thread_get_nice (void) {
	/* TODO: Your implementation goes here */
	return 0;
}

/* Returns 100 times the system load average. */
int
thread_get_load_avg (void) { //시스템의 평균 부하(load average)를 계산하여 반환
	/* TODO: Your implementation goes here */
	return 0;
}

/* Returns 100 times the current thread's recent_cpu value. */
int
thread_get_recent_cpu (void) {//호출된 스레드의 최근 cpu사용량을 게산하여 반환
	/* TODO: Your implementation goes here */
	return 0;
}

/* Idle thread.  Executes when no other thread is ready to run.

   The idle thread is initially put on the ready list by
   thread_start().  It will be scheduled once initially, at which
   point it initializes idle_thread, "up"s the semaphore passed
   to it to enable thread_start() to continue, and immediately
   blocks.  After that, the idle thread never appears in the
   ready list.  It is returned by next_thread_to_run() as a
   special case when the ready list is empty. */
static void
idle (void *idle_started_ UNUSED) {
	struct semaphore *idle_started = idle_started_;

	idle_thread = thread_current ();
	sema_up (idle_started);

	for (;;) {
		/* Let someone else run. */
		intr_disable ();
		thread_block ();

		/* Re-enable interrupts and wait for the next one.

		   The `sti' instruction disables interrupts until the
		   completion of the next instruction, so these two
		   instructions are executed atomically.  This atomicity is
		   important; otherwise, an interrupt could be handled
		   between re-enabling interrupts and waiting for the next
		   one to occur, wasting as much as one clock tick worth of
		   time.

		   See [IA32-v2a] "HLT", [IA32-v2b] "STI", and [IA32-v3a]
		   7.11.1 "HLT Instruction". */
		asm volatile ("sti; hlt" : : : "memory");
	}
}

/* Function used as the basis for a kernel thread. */
static void
kernel_thread (thread_func *function, void *aux) {
	ASSERT (function != NULL);

	intr_enable ();       /* The scheduler runs with interrupts off. */
	function (aux);       /* Execute the thread function. */
	thread_exit ();       /* If function() returns, kill the thread. */
}


/* Does basic initialization of T as a blocked thread named
   NAME. */
static void
init_thread (struct thread *t, const char *name, int priority) {
	ASSERT (t != NULL);
	ASSERT (PRI_MIN <= priority && priority <= PRI_MAX);
	ASSERT (name != NULL);

	memset (t, 0, sizeof *t);
	t->status = THREAD_BLOCKED;
	strlcpy (t->name, name, sizeof t->name);
	t->tf.rsp = (uint64_t) t + PGSIZE - sizeof (void *);
	t->priority = priority;
	t->magic = THREAD_MAGIC;
}

/* Chooses and returns the next thread to be scheduled.  Should
   return a thread from the run queue, unless the run queue is
   empty.  (If the running thread can continue running, then it
   will be in the run queue.)  If the run queue is empty, return
   idle_thread. */
static struct thread *
next_thread_to_run (void) {
	if (list_empty (&ready_list))
		return idle_thread;
	else
		return list_entry (list_pop_front (&ready_list), struct thread, elem);
}

/* Use iretq to launch the thread */
void
do_iret (struct intr_frame *tf) {
	__asm __volatile(
			"movq %0, %%rsp\n"
			"movq 0(%%rsp),%%r15\n"
			"movq 8(%%rsp),%%r14\n"
			"movq 16(%%rsp),%%r13\n"
			"movq 24(%%rsp),%%r12\n"
			"movq 32(%%rsp),%%r11\n"
			"movq 40(%%rsp),%%r10\n"
			"movq 48(%%rsp),%%r9\n"
			"movq 56(%%rsp),%%r8\n"
			"movq 64(%%rsp),%%rsi\n"
			"movq 72(%%rsp),%%rdi\n"
			"movq 80(%%rsp),%%rbp\n"
			"movq 88(%%rsp),%%rdx\n"
			"movq 96(%%rsp),%%rcx\n"
			"movq 104(%%rsp),%%rbx\n"
			"movq 112(%%rsp),%%rax\n"
			"addq $120,%%rsp\n"
			"movw 8(%%rsp),%%ds\n"
			"movw (%%rsp),%%es\n"
			"addq $32, %%rsp\n"
			"iretq"
			: : "g" ((uint64_t) tf) : "memory");
}

/* Switching the thread by activating the new thread's page
   tables, and, if the previous thread is dying, destroying it.

   At this function's invocation, we just switched from thread
   PREV, the new thread is already running, and interrupts are
   still disabled.

   It's not safe to call printf() until the thread switch is
   complete.  In practice that means that printf()s should be
   added at the end of the function. */
static void
thread_launch (struct thread *th) {
	uint64_t tf_cur = (uint64_t) &running_thread ()->tf;
	uint64_t tf = (uint64_t) &th->tf;
	ASSERT (intr_get_level () == INTR_OFF);

	/* The main switching logic.
	 * We first restore the whole execution context into the intr_frame
	 * and then switching to the next thread by calling do_iret.
	 * Note that, we SHOULD NOT use any stack from here
	 * until switching is done. */
	__asm __volatile (
			/* Store registers that will be used. */
			"push %%rax\n"
			"push %%rbx\n"
			"push %%rcx\n"
			/* Fetch input once */
			"movq %0, %%rax\n"
			"movq %1, %%rcx\n"
			"movq %%r15, 0(%%rax)\n"
			"movq %%r14, 8(%%rax)\n"
			"movq %%r13, 16(%%rax)\n"
			"movq %%r12, 24(%%rax)\n"
			"movq %%r11, 32(%%rax)\n"
			"movq %%r10, 40(%%rax)\n"
			"movq %%r9, 48(%%rax)\n"
			"movq %%r8, 56(%%rax)\n"
			"movq %%rsi, 64(%%rax)\n"
			"movq %%rdi, 72(%%rax)\n"
			"movq %%rbp, 80(%%rax)\n"
			"movq %%rdx, 88(%%rax)\n"
			"pop %%rbx\n"              // Saved rcx
			"movq %%rbx, 96(%%rax)\n"
			"pop %%rbx\n"              // Saved rbx
			"movq %%rbx, 104(%%rax)\n"
			"pop %%rbx\n"              // Saved rax
			"movq %%rbx, 112(%%rax)\n"
			"addq $120, %%rax\n"
			"movw %%es, (%%rax)\n"
			"movw %%ds, 8(%%rax)\n"
			"addq $32, %%rax\n"
			"call __next\n"         // read the current rip.
			"__next:\n"
			"pop %%rbx\n"
			"addq $(out_iret -  __next), %%rbx\n"
			"movq %%rbx, 0(%%rax)\n" // rip
			"movw %%cs, 8(%%rax)\n"  // cs
			"pushfq\n"
			"popq %%rbx\n"
			"mov %%rbx, 16(%%rax)\n" // eflags
			"mov %%rsp, 24(%%rax)\n" // rsp
			"movw %%ss, 32(%%rax)\n"
			"mov %%rcx, %%rdi\n"
			"call do_iret\n"
			"out_iret:\n"
			: : "g"(tf_cur), "g" (tf) : "memory"
			);
}

/* Schedules a new process. At entry, interrupts must be off.
 * This function modify current thread's status to status and then
 * finds another thread to run and switches to it.
 * It's not safe to call printf() in the schedule(). */
static void
do_schedule(int status) { //스레드 상태 변경 및 스케줄러 호출
	ASSERT (intr_get_level () == INTR_OFF); //인터럽트 비활성화 확인
	ASSERT (thread_current()->status == THREAD_RUNNING); //현재 스레드가 실행중 상태인지 확인
	while (!list_empty (&destruction_req)) { //destruction_req목록이 비어있지 않는 동안 반복
		//'destruction_req'목록의 첫 번째 요소 제거
		//해당 요소를 victim이라는 스레드 구조체로 변환
		//victim은 파괴되어야 할 스레드
		struct thread *victim = 
			list_entry (list_pop_front (&destruction_req), struct thread, elem);
		
		palloc_free_page(victim); //victim 스레드가 사용하고 있던 메모리 페이지 해제
	}
	thread_current ()->status = status; //현재 스레드의 상태를 새로운 상태로 변경
	schedule (); //스케줄러 호출하여 다음에 실행할 스레드 결정 및 스위치
}

static void
schedule (void) { //현재 실행 중인 스레드를 다음 실행할 스레드로 전환하는 함수
	struct thread *curr = running_thread (); //현재 실행 중인 스레드 가져오기
	struct thread *next = next_thread_to_run (); //다음에 실행할 스레드 결정

	ASSERT (intr_get_level () == INTR_OFF); //인터럽트가 비활성되어 있는지 확인
	ASSERT (curr->status != THREAD_RUNNING); //현재 스레드가 '실행 중' 상태가 아닌지 확인
	ASSERT (is_thread (next)); //next 변수에 저장된 스레드가 유효한 스레드인지 확인
	/* Mark us as running. */
	next->status = THREAD_RUNNING; //next스레드의 상태를 '실행 중'으로 변경

	/* Start new time slice. */
	thread_ticks = 0; //새로운 타임 슬라이스 시작 ->스레드가 cpu에서 실행되는 시간을 추적

#ifdef USERPROG
	/* Activate the new address space. */
	process_activate (next);
#endif

	if (curr != next) {
		/* If the thread we switched from is dying, destroy its struct
		   thread. This must happen late so that thread_exit() doesn't
		   pull out the rug under itself.
		   We just queuing the page free reqeust here because the page is
		   currently used by the stack.
		   The real destruction logic will be called at the beginning of the
		   schedule(). */
		if (curr && curr->status == THREAD_DYING && curr != initial_thread) {
			ASSERT (curr != next);
			list_push_back (&destruction_req, &curr->elem);
		}

		/* Before switching the thread, we first save the information
		 * of current running. */
		thread_launch (next);
	}
}

/* Returns a tid to use for a new thread. */
static tid_t
allocate_tid (void) {
	static tid_t next_tid = 1;
	tid_t tid;

	lock_acquire (&tid_lock);
	tid = next_tid++;
	lock_release (&tid_lock);

	return tid;
}
//아이고
//1주차 과제 알람구현
//현재 스레드를 block시키고 sleep 상태로 넣기

void thread_sleep(int64_t ticks){
	enum intr_level old=intr_disable(); //인터럽트 레벨 저장 후 인터럽트 끄기
	struct thread *cur= thread_current(); //현재 thread 가져오기

	ASSERT(intr_get_level()==INTR_OFF);//인터럽트가 비활성 상태인지 학인
	//ASSERT(cur && cur->status==THREAD_RUNNING); //현재 스레드가 유효하고, 실행 중인 상태인지 확인
	//ASSERT(cur != idle_thread); //현재 스레드가 아이들 스레드가 아닌지 확인

	cur->wake_up_tick=ticks; //스레드가 깨어날 시간 설정, wake_up_tick에 도달했을 때 wait_up_tick ready
	list_push_back(&sleep_list,&(cur->elem)); //현재 스레드를 sleep 큐에 삽입
	if(ticks<tick_to_awake){//ticks:현재 스레드가 깨어나야 할 시간, tick_to_awake 현재까지 알려진 스레드 중 가장 빨리 깨어나야 할 시간
		tick_to_awake=ticks; //다음에 깨어날 스레드의 시간 업데이트
		// printf("%d\n",tick_to_awake);
	}
	thread_block(); //현재 스레드를 block
	intr_set_level(old); //인터럽트 레벨을 원래대로 복구
}

//wake_up_tick에 도달한 모든 스레드 깨우기
void thread_awake(int64_t ticks){
	tick_to_awake=INT64_MAX; //다음 깨어날 스레드의 시간을 최대로 설정
	for(struct list_elem *e=list_begin(&sleep_list); e!=list_end(&sleep_list);){
		struct thread *t=list_entry(e,struct thread,elem);
		if(ticks>=t->wake_up_tick){ //현재 시간이 스레드의 깨어날 시간과 같거나 크다면
			struct list_elem *hi=e;
			list_remove(hi);
			e=list_next(e); //sleep큐에서 제거

			thread_unblock(t); //스레드의 상태를 블록 해제
		}
		else{
			e=list_next(e); //다음으로 이동
			if(t->wake_up_tick<tick_to_awake){
				tick_to_awake=t->wake_up_tick; //다음 깨어날 스레드의 시간 업데이트
			}
		}
	}
}
int64_t get_tick_to_awake(){
	return tick_to_awake;
}
