// // /* This file is derived from source code for the Nachos
// //    instructional operating system.  The Nachos copyright notice
// //    is reproduced in full below. */

// // /* Copyright (c) 1992-1996 The Regents of the University of California.
// //    All rights reserved.

// //    Permission to use, copy, modify, and distribute this software
// //    and its documentation for any purpose, without fee, and
// //    without written agreement is hereby granted, provided that the
// //    above copyright notice and the following two paragraphs appear
// //    in all copies of this software.

// //    IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
// //    ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
// //    CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
// //    AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
// //    HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// //    THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
// //    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// //    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// //    PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
// //    BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
// //    PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
// //    MODIFICATIONS.
// //    */


// // #include <stdio.h>
// // #include <string.h>
// // #include "threads/synch.h"
// // #include "threads/interrupt.h"
// // #include "threads/thread.h"



// // //1주차 과제-세마포어
// // // static bool dec_pri_function(const struct list_elem *, const struct list_elem *,void *);
// // // static bool dec_pri_in_sema_function(const struct list_elem *, const struct list_elem *, void *);
// // /* Initializes semaphore SEMA to VALUE.  A semaphore is a
// //    nonnegative integer along with two atomic operators for
// //    manipulating it:

// //    - down or "P": wait for the value to become positive, then
// //    decrement it.

// //    - up or "V": increment the value (and wake up one waiting
// //    thread, if any). */
// // void
// // sema_init (struct semaphore *sema, unsigned value) {
// // 	ASSERT (sema != NULL);

// // 	sema->value = value;
// // 	list_init (&sema->waiters);
// // }

// // /* Down or "P" operation on a semaphore.  Waits for SEMA's value
// //    to become positive and then atomically decrements it.

// //    This function may sleep, so it must not be called within an
// //    interrupt handler.  This function may be called with
// //    interrupts disabled, but if it sleeps then the next scheduled
// //    thread will probably turn interrupts back on. This is
// //    sema_down function. */
// // void
// // sema_down (struct semaphore *sema) {
// // 	enum intr_level old_level;

// // 	ASSERT (sema != NULL);
// // 	ASSERT (!intr_context ());
	
// // 	old_level = intr_disable ();
// // 	while (sema->value == 0) {
// // 		//여기 수정 ->priority
// // 		list_insert_ordered (&sema->waiters, &thread_current ()->elem,priority_cmp,NULL);
// // 		thread_block ();
// // 	}
// // 	sema->value--;
// // 	intr_set_level (old_level);
// // }

// // /* Down or "P" operation on a semaphore, but only if the
// //    semaphore is not already 0.  Returns true if the semaphore is
// //    decremented, false otherwise.

// //    This function may be called from an interrupt handler.

// // } */
// // bool
// // sema_try_down (struct semaphore *sema) {
// // 	enum intr_level old_level;
// // 	bool success;

// // 	ASSERT (sema != NULL);

// // 	old_level = intr_disable ();
// // 	if (sema->value > 0)
// // 	{
// // 		sema->value--;
// // 		success = true;
// // 	}
// // 	else
// // 		success = false;
// // 	intr_set_level (old_level);

// // 	return success;
// // }

// // /* Up or "V" operation on a semaphore.  Increments SEMA's value
// //    and wakes up one thread of those waiting for SEMA, if any.

// //    This function may be called from an interrupt handler. */
// // void
// // sema_up (struct semaphore *sema) {
// // 	enum intr_level old_level;

// // 	ASSERT (sema != NULL);

// // 	old_level = intr_disable ();
// // 	if (!list_empty (&sema->waiters)){
// // 		//여기면 정말 슬플 것 같다.. 여긴 아닌걸로
// // 		list_sort(&sema->waiters, priority_cmp, NULL);//괄호 고침

// // 		thread_unblock (list_entry (list_pop_front (&sema->waiters),
// // 					struct thread, elem));
// // 	}
	
// // 	sema->value++;
// // 	priority_preempt();
// // 	intr_set_level (old_level);
// // }

// // static void sema_test_helper (void *sema_);

// // /* Self-test for semaphores that makes control "ping-pong"
// //    between a pair of threads.  Insert calls to printf() to see
// //    what's going on. */
// // void
// // sema_self_test (void) {
// // 	struct semaphore sema[2];
// // 	int i;

// // 	printf ("Testing semaphores...");
// // 	sema_init (&sema[0], 0);
// // 	sema_init (&sema[1], 0);
// // 	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
// // 	for (i = 0; i < 10; i++)
// // 	{
// // 		sema_up (&sema[0]);
// // 		sema_down (&sema[1]);
// // 	}
// // 	printf ("done.\n");
// // }

// // /* Thread function used by sema_self_test(). */
// // static void
// // sema_test_helper (void *sema_) {
// // 	struct semaphore *sema = sema_;
// // 	int i;

// // 	for (i = 0; i < 10; i++)
// // 	{
// // 		sema_down (&sema[0]);
// // 		sema_up (&sema[1]);
// // 	}
// // }

// // /* Initializes LOCK.  A lock can be held by at most a single
// //    thread at any given time.  Our locks are not "recursive", that
// //    is, it is an error for the thread currently holding a lock to
// //    try to acquire that lock.

// //    A lock is a specialization of a semaphore with an initial
// //    value of 1.  The difference between a lock and such a
// //    semaphore is twofold.  First, a semaphore can have a value
// //    greater than 1, but a lock can only be owned by a single
// //    thread at a time.  Second, a semaphore does not have an owner,
// //    meaning that one thread can "down" the semaphore and then
// //    another one "up" it, but with a lock the same thread must both
// //    acquire and release it.  When these restrictions prove
// //    onerous, it's a good sign that a semaphore should be used,
// //    instead of a lock. */
// // void
// // lock_init (struct lock *lock) {
// // 	ASSERT (lock != NULL);

// // 	lock->holder = NULL;
// // 	sema_init (&lock->semaphore, 1);
// // }

// // /* Acquires LOCK, sleeping until it becomes available if
// //    necessary.  The lock must not already be held by the current
// //    thread.

// //    This function may sleep, so it must not be called within an
// //    interrupt handler.  This function may be called with
// //    interrupts disabled, but interrupts will be turned back on if
// //    we need to sleep. */

// // //lock_acquire
// // void
// // lock_acquire (struct lock *lock)
// // {
// //   ASSERT (lock != NULL);
// //   ASSERT (!intr_context ());
// //   ASSERT (!lock_held_by_current_thread (lock));

// //   struct thread * cur = thread_current();

// //   if(lock->holder) { // if holder exist for this lock
// //     cur->lock_needed = lock; // update thread
// //     list_insert_ordered(&(lock->holder->doner_list), &(cur->donorelem),priority_cmp,0); // add current thread to donor list of holder
// //     donate_priority(); // donate priority
// //   }

// //   sema_down (&lock->semaphore);
// //   // acquire lock
// //   cur->lock_needed = NULL; // update thread
// //   lock->holder = thread_current(); // update lock
// // }

// // //무한루프
// // // void donate_priority(struct thread *t, int new_priority, int depth) {
// // //     if (depth > 8) {
// // //         return; // 중첩 깊이 제한 초과 시 함수 종료
// // //     }

// // //     if (t->priority < new_priority) {
// // //         t->priority = new_priority; // 우선순위 업데이트

// // //         if (t->wait_on_lock != NULL){
// // // 			if(t->wait_on_lock->holder != NULL) {
// // //             // 다음 스레드에게 우선순위 기부
// // //             donate_priority(t->wait_on_lock->holder, new_priority, depth + 1);
// // // 			}
// // // 		}
// // //     }
// // // };



// // /* Tries to acquires LOCK and returns true if successful or false
// //    on failure.  The lock must not already be held by the current
// //    thread.

// //    This function will not sleep, so it may be called within an
// //    interrupt handler. */
// // bool
// // lock_try_acquire (struct lock *lock) {
// // 	bool success;

// // 	ASSERT (lock != NULL);
// // 	ASSERT (!lock_held_by_current_thread (lock));

// // 	success = sema_try_down (&lock->semaphore);
// // 	if (success)
// // 		lock->holder = thread_current ();
// // 	return success;
// // }


// // /* Releases LOCK, which must be owned by the current thread.
// //    This is lock_release function.

// //    An interrupt handler cannot acquire a lock, so it does not
// //    make sense to try to release a lock within an interrupt
// //    handler. */
// // void
// // lock_release (struct lock *lock) 
// // {
// //   ASSERT (lock != NULL);
// //   ASSERT (lock_held_by_current_thread (lock));

// //   lock->holder = NULL;
  
// // //   struct thread * cur = thread_current();
// // //   struct list_elem * e = list_begin(&(cur->doner_list));
// // //   while(e != list_end(&(cur->doner_list))) {
// // //     struct thread * t = list_entry(e, struct thread, donorelem);
// // //     if(t->lock_needed == lock) {
// // //       e = list_remove(e);
// // //     }
// // //     else {
// // //       e = list_next(e);
// // //     }
// // remove_with_lock(lock); //현재 스레드의 donation list에서 lock을 기다리는 스레드를 제거한다.
// // 	refresh_priority(); //현재 스레드의 priority를 재설정한다.

// // 	sema_up (&lock->semaphore); //lock을 기다리는 스레드를 깨운다
// // }


// // /* Returns true if the current thread holds LOCK, false
// //    otherwise.  (Note that testing whether some other thread holds
// //    a lock would be racy.) */
// // bool
// // lock_held_by_current_thread (const struct lock *lock) {
// // 	ASSERT (lock != NULL);

// // 	return lock->holder == thread_current ();
// // }
// // 
// // /* One semaphore in a list. */
// // struct semaphore_elem {
// // 	struct list_elem elem;              /* List element. */
// // 	struct semaphore semaphore;         /* This semaphore. */
// // };

// // /* Initializes condition variable COND.  A condition variable
// //    allows one piece of code to signal a condition and cooperating
// //    code to receive the signal and act upon it. */
// // void
// // cond_init (struct condition *cond) {
// // 	ASSERT (cond != NULL);

// // 	list_init (&cond->waiters);
// // }

// // /* Atomically releases LOCK and waits for COND to be signaled by
// //    some other piece of code.  After COND is signaled, LOCK is
// //    reacquired before returning.  LOCK must be held before calling
// //    this function.

// //    The monitor implemented by this function is "Mesa" style, not
// //    "Hoare" style, that is, sending and receiving a signal are not
// //    an atomic operation.  Thus, typically the caller must recheck
// //    the condition after the wait completes and, if necessary, wait
// //    again.

// //    A given condition variable is associated with only a single
// //    lock, but one lock may be associated with any number of
// //    condition variables.  That is, there is a one-to-many mapping
// //    from locks to condition variables.

// //    This function may sleep, so it must not be called within an
// //    interrupt handler.  This function may be called with
// //    interrupts disabled, but interrupts will be turned back on if
// //    we need to sleep. */
// // void
// // cond_wait (struct condition *cond, struct lock *lock) {
// // 	struct semaphore_elem waiter;

// // 	ASSERT (cond != NULL);
// // 	ASSERT (lock != NULL);
// // 	ASSERT (!intr_context ());
// // 	ASSERT (lock_held_by_current_thread (lock));

// // 	sema_init (&waiter.semaphore, 0);
// // 	list_insert_ordered (&cond->waiters, &waiter.elem,priority_cmp_sema,NULL);
// // 	lock_release (lock);
// // 	sema_down (&waiter.semaphore);
// // 	lock_acquire (lock);
// // }
// // bool priority_cmp_sema(struct list_elem * a, struct list_elem * b, void * aux UNUSED) {
// //   struct semaphore_elem * a_sema = list_entry(a, struct semaphore_elem, elem);
// //   struct semaphore_elem * b_sema = list_entry(b, struct semaphore_elem, elem);

// //   struct thread * a_thread = list_entry(list_begin(&(a_sema->semaphore.waiters)), struct thread, elem);
// //   struct thread * b_thread = list_entry(list_begin(&(b_sema->semaphore.waiters)), struct thread, elem);

// //   return a_thread->priority > b_thread->priority;
// // }
// // /* If any threads are waiting on COND (protected by LOCK), then
// //    this function signals one of them to wake up from its wait.
// //    LOCK must be held before calling this function.

// //    An interrupt handler cannot acquire a lock, so it does not
// //    make sense to try to signal a condition variable within an
// //    interrupt handler. */
// // void
// // cond_signal (struct condition *cond, struct lock *lock UNUSED) {
// // 	ASSERT (cond != NULL);
// // 	ASSERT (lock != NULL);
// // 	ASSERT (!intr_context ());
// // 	ASSERT (lock_held_by_current_thread (lock));

// // 	if (!list_empty (&cond->waiters)){
// // 		list_sort(&cond->waiters,priority_cmp_sema,NULL);
// // 		sema_up (&list_entry (list_pop_front (&cond->waiters),
// // 					struct semaphore_elem, elem)->semaphore);
// // 	}
// // }

// // /* Wakes up all threads, if any, waiting on COND (protected by
// //    LOCK).  LOCK must be held before calling this function.

// //    An interrupt handler cannot acquire a lock, so it does not
// //    make sense to try to signal a condition variable within an
// //    interrupt handler. */
// // void
// // cond_broadcast (struct condition *cond, struct lock *lock) {
// // 	ASSERT (cond != NULL);
// // 	ASSERT (lock != NULL);

// // 	while (!list_empty (&cond->waiters))
// // 		cond_signal (cond, lock);
// // }

// // /*
// //  * 현재 스레드의 기부 리스트에서 지정된 락을 기다리는 스레드를 제거
// // */
// /* This file is derived from source code for the Nachos
//    instructional operating system.  The Nachos copyright notice
//    is reproduced in full below. */

// /* Copyright (c) 1992-1996 The Regents of the University of California.
//    All rights reserved.

//    Permission to use, copy, modify, and distribute this software
//    and its documentation for any purpose, without fee, and
//    without written agreement is hereby granted, provided that the
//    above copyright notice and the following two paragraphs appear
//    in all copies of this software.

//    IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
//    ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
//    CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
//    AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
//    HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

//    THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
//    WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//    PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
//    BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
//    PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
//    MODIFICATIONS.
//    */

// #include "threads/synch.h"
// #include <stdio.h>
// #include <string.h>
// #include "threads/interrupt.h"
// #include "threads/thread.h"

// /* Initializes semaphore SEMA to VALUE.  A semaphore is a
//    nonnegative integer along with two atomic operators for
//    manipulating it:

//    - down or "P": wait for the value to become positive, then
//    decrement it.

//    - up or "V": increment the value (and wake up one waiting
//    thread, if any). */
// void
// sema_init (struct semaphore *sema, unsigned value) {
// 	ASSERT (sema != NULL);

// 	sema->value = value;
// 	list_init (&sema->waiters);
// }

// /* Down or "P" operation on a semaphore.  Waits for SEMA's value
//    to become positive and then atomically decrements it.

//    This function may sleep, so it must not be called within an
//    interrupt handler.  This function may be called with
//    interrupts disabled, but if it sleeps then the next scheduled
//    thread will probably turn interrupts back on. This is
//    sema_down function. */
// void
// sema_down (struct semaphore *sema) {
// 	enum intr_level old_level;

// 	ASSERT (sema != NULL);
// 	ASSERT (!intr_context ());

// 	old_level = intr_disable ();
// 	while (sema->value == 0) {
// 		list_insert_ordered(&sema->waiters, &thread_current ()->elem, cmp_priority, NULL);
// 		// list_push_back (&sema->waiters, &thread_current ()->elem);
// 		thread_block ();
// 	}
// 	sema->value--;
// 	intr_set_level (old_level);
// }

// /* Down or "P" operation on a semaphore, but only if the
//    semaphore is not already 0.  Returns true if the semaphore is
//    decremented, false otherwise.

//    This function may be called from an interrupt handler. */
// bool
// sema_try_down (struct semaphore *sema) {
// 	enum intr_level old_level;
// 	bool success;

// 	ASSERT (sema != NULL);

// 	old_level = intr_disable ();
// 	if (sema->value > 0)
// 	{
// 		sema->value--;
// 		success = true;
// 	}
// 	else
// 		success = false;
// 	intr_set_level (old_level);

// 	return success;
// }

// /* Up or "V" operation on a semaphore.  Increments SEMA's value
//    and wakes up one thread of those waiting for SEMA, if any.

//    This function may be called from an interrupt handler. */
// void
// sema_up (struct semaphore *sema) {
// 	enum intr_level old_level;

// 	ASSERT (sema != NULL);

// 	old_level = intr_disable ();
// 	if (!list_empty (&sema->waiters))
// 	{
// 		list_sort(&sema->waiters, cmp_priority, NULL);
// 		thread_unblock (list_entry (list_pop_front (&sema->waiters),
// 					struct thread, elem));
// 	}
// 	sema->value++;
// 	test_max_priority();
// 	intr_set_level (old_level);
// }

// static void sema_test_helper (void *sema_);

// /* Self-test for semaphores that makes control "ping-pong"
//    between a pair of threads.  Insert calls to printf() to see
//    what's going on. */
// void
// sema_self_test (void) {
// 	struct semaphore sema[2];
// 	int i;

// 	printf ("Testing semaphores...");
// 	sema_init (&sema[0], 0);
// 	sema_init (&sema[1], 0);
// 	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
// 	for (i = 0; i < 10; i++)
// 	{
// 		sema_up (&sema[0]);
// 		sema_down (&sema[1]);
// 	}
// 	printf ("done.\n");
// }

// /* Thread function used by sema_self_test(). */
// static void
// sema_test_helper (void *sema_) {
// 	struct semaphore *sema = sema_;
// 	int i;

// 	for (i = 0; i < 10; i++)
// 	{
// 		sema_down (&sema[0]);
// 		sema_up (&sema[1]);
// 	}
// }
// 
// /* Initializes LOCK.  A lock can be held by at most a single
//    thread at any given time.  Our locks are not "recursive", that
//    is, it is an error for the thread currently holding a lock to
//    try to acquire that lock.

//    A lock is a specialization of a semaphore with an initial
//    value of 1.  The difference between a lock and such a
//    semaphore is twofold.  First, a semaphore can have a value
//    greater than 1, but a lock can only be owned by a single
//    thread at a time.  Second, a semaphore does not have an owner,
//    meaning that one thread can "down" the semaphore and then
//    another one "up" it, but with a lock the same thread must both
//    acquire and release it.  When these restrictions prove
//    onerous, it's a good sign that a semaphore should be used,
//    instead of a lock. */
// void
// lock_init (struct lock *lock) {
// 	ASSERT (lock != NULL);

// 	lock->holder = NULL;
// 	sema_init (&lock->semaphore, 1);
// }

// /* Acquires LOCK, sleeping until it becomes available if
//    necessary.  The lock must not already be held by the current
//    thread.

//    This function may sleep, so it must not be called within an
//    interrupt handler.  This function may be called with
//    interrupts disabled, but interrupts will be turned back on if
//    we need to sleep. */


// void
// lock_acquire (struct lock *lock) {
// 	ASSERT (lock != NULL);
// 	ASSERT (!intr_context ());
// 	ASSERT (!lock_held_by_current_thread (lock));

// 	struct thread *curr = thread_current();
// 	if (lock->holder != NULL)
// 	{
// 		curr->wait_on_lock = lock; //현재 스레드의 wait_on_lock에 lock을 넣어준다.
// 		list_insert_ordered(&lock->holder->donations, &curr->donation_elem, cmp_donation_priority, 0);
// 		if(!thread_mlfqs) donate_priority(); //thread_mlfqs가 아닐때
// 	}
// 	sema_down (&lock->semaphore);
// 	curr->wait_on_lock = NULL;

// 	lock->holder = thread_current ();
// }

// /* Tries to acquires LOCK and returns true if successful or false
//    on failure.  The lock must not already be held by the current
//    thread.

//    This function will not sleep, so it may be called within an
//    interrupt handler. */
// bool
// lock_try_acquire (struct lock *lock) {
// 	bool success;

// 	ASSERT (lock != NULL);
// 	ASSERT (!lock_held_by_current_thread (lock));

// 	success = sema_try_down (&lock->semaphore);
// 	if (success)
// 		lock->holder = thread_current ();
// 	return success;
// }

// /* Releases LOCK, which must be owned by the current thread.
//    This is lock_release function.

//    An interrupt handler cannot acquire a lock, so it does not
//    make sense to try to release a lock within an interrupt
//    handler. */

// void
// lock_release (struct lock *lock) {

// 	ASSERT (lock != NULL);
// 	ASSERT (lock_held_by_current_thread (lock));

// 	lock->holder = NULL;
// 	if(!thread_mlfqs){
// 		remove_with_lock(lock); //현재 스레드의 donation list에서 lock을 기다리는 스레드를 제거한다.
// 		refresh_priority(); //현재 스레드의 priority를 재설정한다.
// 	}
// 	sema_up (&lock->semaphore); //lock을 기다리는 스레드를 깨운다.
// }

// /* Returns true if the current thread holds LOCK, false
//    otherwise.  (Note that testing whether some other thread holds
//    a lock would be racy.) */
// bool
// lock_held_by_current_thread (const struct lock *lock) {
// 	ASSERT (lock != NULL);

// 	return lock->holder == thread_current ();
// }
// 
// /* One semaphore in a list. */
// struct semaphore_elem {
// 	struct list_elem elem;              /* List element. */
// 	struct semaphore semaphore;         /* This semaphore. */
// };

// /* Initializes condition variable COND.  A condition variable
//    allows one piece of code to signal a condition and cooperating
//    code to receive the signal and act upon it. */
// void
// cond_init (struct condition *cond) {
// 	ASSERT (cond != NULL);

// 	list_init (&cond->waiters);
// }

// /* Atomically releases LOCK and waits for COND to be signaled by
//    some other piece of code.  After COND is signaled, LOCK is
//    reacquired before returning.  LOCK must be held before calling
//    this function.

//    The monitor implemented by this function is "Mesa" style, not
//    "Hoare" style, that is, sending and receiving a signal are not
//    an atomic operation.  Thus, typically the caller must recheck
//    the condition after the wait completes and, if necessary, wait
//    again.

//    A given condition variable is associated with only a single
//    lock, but one lock may be associated with any number of
//    condition variables.  That is, there is a one-to-many mapping
//    from locks to condition variables.

//    This function may sleep, so it must not be called within an
//    interrupt handler.  This function may be called with
//    interrupts disabled, but interrupts will be turned back on if
//    we need to sleep. */
// void
// cond_wait (struct condition *cond, struct lock *lock) {
// 	struct semaphore_elem waiter;

// 	ASSERT (cond != NULL);
// 	ASSERT (lock != NULL);
// 	ASSERT (!intr_context ());
// 	ASSERT (lock_held_by_current_thread (lock));

// 	sema_init (&waiter.semaphore, 0);
// 	list_insert_ordered(&cond->waiters, &waiter.elem, cmp_sem_priority, NULL);	
// 	// list_push_back (&cond->waiters, &waiter.elem);
// 	lock_release (lock);
// 	sema_down (&waiter.semaphore);
// 	lock_acquire (lock);
// }


// bool cmp_sem_priority (const struct list_elem *a, const struct list_elem *b, void *aux)
// {
// 	struct semaphore_elem *sa = list_entry(a, struct semaphore_elem, elem);
// 	struct semaphore_elem *sb = list_entry(b, struct semaphore_elem, elem);

// 	struct list_elem *ta = list_begin(&sa->semaphore.waiters);
// 	struct list_elem *tb = list_begin(&sb->semaphore.waiters);
// 	return cmp_priority(ta, tb, NULL);
// }

// /* If any threads are waiting on COND (protected by LOCK), then
//    this function signals one of them to wake up from its wait.
//    LOCK must be held before calling this function.

//    An interrupt handler cannot acquire a lock, so it does not
//    make sense to try to signal a condition variable within an
//    interrupt handler. */
// void
// cond_signal (struct condition *cond, struct lock *lock UNUSED) {
// 	ASSERT (cond != NULL);
// 	ASSERT (lock != NULL);
// 	ASSERT (!intr_context ());
// 	ASSERT (lock_held_by_current_thread (lock));

// 	if (!list_empty (&cond->waiters))
// 	{
// 		list_sort(&cond->waiters, cmp_sem_priority, NULL);
// 		sema_up (&list_entry (list_pop_front (&cond->waiters),
// 					struct semaphore_elem, elem)->semaphore);
// 	}
// }

// /* Wakes up all threads, if any, waiting on COND (protected by
//    LOCK).  LOCK must be held before calling this function.

//    An interrupt handler cannot acquire a lock, so it does not
//    make sense to try to signal a condition variable within an
//    interrupt handler. */
// void
// cond_broadcast (struct condition *cond, struct lock *lock) {
// 	ASSERT (cond != NULL);
// 	ASSERT (lock != NULL);

// 	while (!list_empty (&cond->waiters))
// 		cond_signal (cond, lock);
// };
/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
   */

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* ************************ Project 1 ************************ */
static bool dec_pri_function(const struct list_elem *, const struct list_elem *, void *);
static bool dec_pri_in_sema_function (const struct list_elem *, const struct list_elem *, void *);
static bool dec_pri_in_donate_function (const struct list_elem *, const struct list_elem *, void *);
static void remove_waiting_lock(struct lock *);

/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
   decrement it.

   - up or "V": increment the value (and wake up one waiting
   thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) {
	ASSERT (sema != NULL);

	sema->value = value;
	list_init (&sema->waiters);
}


/*
	이 연산은 주어진 세마포어(SEMA)의 값이 양수가 될 때까지 기다린 다음, 값을 원자적으로 감소시킵니다.

	세마포어의 Down 연산은 일반적으로 어떤 리소스의 사용을 시작하거나,
    크리티컬 섹션에 들어가기 전에 호출됩니다. 세마포어의 값이 양수인 경우에만 Down 연산이 성공하며,
	값이 0이거나 음수인 경우에는 해당 세마포어의 값이 양수가 될 때까지 대기합니다.
	이 함수는 슬립(Sleep)이 발생할 수 있으므로, 이 함수는 인터럽트 핸들러 내에서 호출해서는 안 됩니다.
	즉, 이 함수는 인터럽트 컨텍스트에서 호출되지 않아야 합니다. 
	함수가 슬립하지 않으면 인터럽트 비활성화 상태에서 호출할 수 있지만, 
	스레드가 슬립하면 다음으로 스케줄되는 스레드가 아마도 인터럽트를 다시 활성화할 것입니다.

	이러한 Down 연산은 주로 공유된 자원에 대한 접근을 제어하고, 
	여러 스레드 간에 일관성을 유지하기 위해 사용됩니다.
	Down 연산을 호출하면 세마포어의 값을 감소시켜 해당 자원에 대한 사용 권한을 획득하고,
	다른 스레드는 이 세마포어가 다시 양수가 될 때까지 기다리게 됩니다.

	sema_down은 sleep이 발생할 수 있다?
		- 함수가 실행되는 도중에 스레드가 블록되어(waiting 상태가 되어) 잠들 수 있다
		- 세마포어 값이 0인 경우: 세마포어의 값이 0이면 Down 연산이 불가능하므로 스레드는 해당 
			세마포어의 값이 양수가 될 때까지 대기
*/
void
sema_down (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);
	ASSERT (!intr_context ());

	old_level = intr_disable ();
	while (sema->value == 0) {
		list_push_back (&sema->waiters, &thread_current ()->elem);
		thread_block ();
	}
	sema->value--;
	intr_set_level (old_level);
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) {
	enum intr_level old_level;
	bool success;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (sema->value > 0)
	{
		sema->value--;
		success = true;
	}
	else
		success = false;
	intr_set_level (old_level);

	return success;
}


/*
	주어진 세마포어(SEMA)의 값을 증가시키고, 만약 세마포어를 기다리는 스레드 중 하나가 있다면 깨움

	세마포어의 Up 연산은 주로 어떤 리소스의 사용이 끝났음을 나타내거나, 
	크리티컬 섹션을 빠져나가기 전에 호출

	세마포어의 값이 증가하면서 기다리고 있는 스레드 중 하나가 깨어나서 리소스에 접근할 수 있게 됨
	이 함수는 인터럽트 핸들러 내에서 호출할 수 있음

	즉, 인터럽트 컨텍스트에서도 안전하게 호출될 수 있음
	이는 일반적으로 세마포어 연산이 인터럽트에서 사용되는 경우가 있기 때문에 중요한 특징
*/
void
sema_up (struct semaphore *sema) {
	enum intr_level old_level;

	ASSERT (sema != NULL);

	old_level = intr_disable ();
	if (!list_empty (&sema->waiters)) {
		list_sort(&sema->waiters, dec_pri_function, NULL);
		thread_unblock (list_entry (list_pop_front (&sema->waiters),
					struct thread, elem));
	}
	sema->value++;
	try_yield();
	intr_set_level (old_level);
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) {
	struct semaphore sema[2];
	int i;

	printf ("Testing semaphores...");
	sema_init (&sema[0], 0);
	sema_init (&sema[1], 0);
	thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
	for (i = 0; i < 10; i++)
	{
		sema_up (&sema[0]);
		sema_down (&sema[1]);
	}
	printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) {
	struct semaphore *sema = sema_;
	int i;

	for (i = 0; i < 10; i++)
	{
		sema_down (&sema[0]);
		sema_up (&sema[1]);
	}
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */

/*
   락을 초기화합니다.
   주어진 락에 대한 초기화 작업을 수행하고,
   해당 락의 보유자(holder)를 NULL로, 세마포어를 1로 초기화합니다.
*/
void
lock_init (struct lock *lock) {
	ASSERT (lock != NULL);

	lock->holder = NULL;
	sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */

/*
* priority donation inherit - 
	* old_priority라는 변수를 thread에 추가하여 구현해 보았음
	* multiple donate에서 이전상태의 우선순위를 기억하고 있어야하는 문제 발생
	* 즉, donation 받은 정보에 대한 리스트가 필요
*/
void
lock_acquire (struct lock *lock) {
	struct thread *curr = thread_current();
	
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (!lock_held_by_current_thread (lock));

	if (!thread_mlfqs) {
		if (lock->semaphore.value == 0) {
			curr->wait_on_lock = lock;
			if (lock->holder->priority < curr->priority) {
				list_insert_ordered(&lock->holder->donation, &curr->donation_elem, dec_pri_in_donate_function, NULL);
				donate_priority ();
			}
		}
	}
	

	sema_down (&lock->semaphore);

	if (!thread_mlfqs) {
		curr->wait_on_lock = NULL;
	}
	
	lock->holder = curr;
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock) { 
	bool success;

	ASSERT (lock != NULL);
	ASSERT (!lock_held_by_current_thread (lock));

	success = sema_try_down (&lock->semaphore);
	if (success)
		lock->holder = thread_current ();
	return success;
}

/* Releases LOCK, which must be owned by the current thread.
   This is lock_release function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) {
	struct thread *curr = thread_current();
	ASSERT (lock != NULL);
	ASSERT (lock_held_by_current_thread (lock));

	if (!thread_mlfqs) {
		remove_waiting_lock(lock);

		curr->priority = curr->origin_priority;
		if (!list_empty(&curr->donation)) {
			struct thread *top_thread = list_entry(list_front(&curr->donation),
												struct thread, donation_elem);
			if (top_thread->priority > curr->priority)
				curr->priority = top_thread->priority;
		}
	}
	

	lock->holder = NULL;
	sema_up (&lock->semaphore);
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) {
	ASSERT (lock != NULL);

	return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem {
	struct list_elem elem;              /* List element. */
	struct semaphore semaphore;         /* This semaphore. */
};

/*
   Condition Variable을 초기화합니다.
   주어진 Condition Variable에 대한 대기 리스트를 초기화합니다.
   
   cond: 초기화할 Condition Variable 구조체의 포인터
*/
void
cond_init (struct condition *cond) {
	ASSERT (cond != NULL);

	list_init (&cond->waiters);
}

/* 
   Condition Variable에서 기다리는 스레드를 표현하는 세마포어 요소를 초기화하고,
   해당 세마포어 요소를 Condition Variable의 대기 리스트에 삽입한 후,
   현재 보유한 락을 해제하고 대기 상태로 들어갑니다.
   이 함수는 반드시 인터럽트가 비활성화된 상태에서 호출되어야 하며,
   현재 스레드가 주어진 락을 보유하고 있어야 합니다.
   
   기다리는 동안 세마포어 요소의 세마포어를 0으로 초기화하고,
   대기 리스트에는 스레드들을 우선순위 기준으로 정렬하여 삽입합니다.
   
   락을 해제하고 대기 상태로 들어간 후, 실제로 깨어나려면 다른 스레드에서
   조건 변수에 signal 또는 broadcast를 호출해야 합니다.
*/
void
cond_wait (struct condition *cond, struct lock *lock) {
	struct semaphore_elem waiter;

	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	sema_init (&waiter.semaphore, 0);
	list_insert_ordered (&cond->waiters, &waiter.elem, dec_pri_in_sema_function, NULL);
	lock_release (lock);
	sema_down (&waiter.semaphore);
	lock_acquire (lock);
}

/*
   Condition Variable에서 대기 중인 스레드 중 하나를 깨웁니다.
   대기 중인 스레드가 있을 경우, 대기 리스트를 우선순위 기준으로 정렬하고,
   가장 우선순위가 높은 스레드를 깨워서 대기 상태에서 깨어나게 합니다.
   
   주어진 락이 현재 스레드에 의해 보유되고 있어야 하며,
   인터럽트가 비활성화된 상태에서 호출되어야 합니다.
*/
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);
	ASSERT (!intr_context ());
	ASSERT (lock_held_by_current_thread (lock));

	if (!list_empty (&cond->waiters)) {
		list_sort(&cond->waiters, dec_pri_in_sema_function, NULL);
		sema_up (&list_entry (list_pop_front (&cond->waiters),
					struct semaphore_elem, elem)->semaphore);
	}
}

/*
   Condition Variable에서 대기 중인 모든 스레드를 깨웁니다.
   대기 중인 스레드가 있다면, 그 모든 스레드를 깨워서 대기 상태에서 깨어나게 합니다.
   
   주어진 락이 현재 스레드에 의해 보유되고 있어야 하며,
   인터럽트가 비활성화된 상태에서 호출되어야 합니다.
*/
void
cond_broadcast (struct condition *cond, struct lock *lock) {
	ASSERT (cond != NULL);
	ASSERT (lock != NULL);

	while (!list_empty (&cond->waiters))
		cond_signal (cond, lock);
}
/*
   리스트 정렬에서 사용되는 스레드 우선순위에 따른 비교 함수입니다.
   이 함수는 두 리스트 요소 간의 비교를 수행하고, 정렬 순서를 결정합니다.
*/
static bool 
dec_pri_function(const struct list_elem *a, const struct list_elem *b, void *aux) {
    return list_entry(a, struct thread, elem)->priority > list_entry(b, struct thread, elem)->priority;
}

/*
   세마포어의 대기 리스트에서 스레드 우선순위에 따른 비교 함수입니다.
   이 함수는 두 세마포어 요소의 대기 스레드 중 가장 높은 우선순위를 비교하여 정렬합니다.
*/
static bool 
dec_pri_in_sema_function (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
	struct semaphore_elem *sema_a = list_entry(a, struct semaphore_elem, elem);
	struct semaphore_elem *sema_b = list_entry(b, struct semaphore_elem, elem);

	return list_entry(list_begin(&sema_a->semaphore.waiters), struct thread, elem)->priority 
	> list_entry(list_begin(&sema_b->semaphore.waiters), struct thread, elem)->priority;
}

static bool 
dec_pri_in_donate_function (const struct list_elem *a, const struct list_elem *b, void *aux UNUSED) {
	return list_entry(a, struct thread, donation_elem)->priority > list_entry(b, struct thread, donation_elem)->priority;
}

/*
 * 현재 스레드의 기부 리스트에서 지정된 락을 기다리는 스레드를 제거
*/
static void
remove_waiting_lock(struct lock *lock) {
	struct list *donation = &(thread_current()->donation); 
    struct list_elem *donor_elem;	
    struct thread *donor_thread;

    if (list_empty(donation))
        return;

    donor_elem = list_front(donation);

    while (1)
    {
        donor_thread = list_entry(donor_elem, struct thread, donation_elem);
        if (donor_thread->wait_on_lock == lock)		   
            list_remove(&donor_thread->donation_elem); 
        donor_elem = list_next(donor_elem);
        if (donor_elem == list_end(donation))
            return;
    }
}