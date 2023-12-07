#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "filesys/filesys.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "intrinsic.h"

void syscall_entry (void);
void syscall_handler (struct intr_frame *);

/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

enum {
	STD_INPUT,
	STD_OUTPUT,
	STD_ERROR
};

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
// syscall 명령어를 실행하면 커널로 진입
// syscall 명령어를 만나면 cpu는 사용자 모드에서 커널 모드로 전환
// 커널 내부에서는 시스템 콜 번호를 확인하고 해당하는 시스템 콜 핸들러 함수로 이동
// 해당 함수는 즉 커널 내부에서 작동하는 함수다.
void
syscall_handler (struct intr_frame *f UNUSED) {
	// TODO: Your implementation goes here.
	// page fault, divide zero
	// printf ("system call!\n");
	// printf ("system call No.%d\n", f->R.rax);
	switch (f->R.rax) {
    case SYS_HALT:
	{
		halt();
		break;
	}

    case SYS_EXIT:
	{
		exit (f->R.rdi);
        break;
	}

    case SYS_FORK:
        break;

	case SYS_EXEC:
		f->R.rax = exec (f->R.rdi);
        break;

	case SYS_WAIT:
		break;

    case SYS_CREATE:
	{
		f->R.rax = create (f->R.rdi, f->R.rsi);
		break;
	}

	case SYS_REMOVE:
	{
		f->R.rax = remove (f->R.rdi);
		break;
	}

	case SYS_OPEN:
        break;
	
	case SYS_FILESIZE:
        break;

	case SYS_READ:
        break;

	case SYS_WRITE:
	{
		f->R.rax = write (f->R.rdi, f->R.rsi, f->R.rdx);
		break;
	}

	case SYS_SEEK:
	{
		// int fd = f->R.rsi;
		// unsigned position = f->R.rdi;
		// seek (fd, position);
        break;
	}

	case SYS_TELL:
        break;

	case SYS_CLOSE:
        break;

	case SYS_DUP2:
        break;

    default:
        break;
}
}

/*
	power_off()를 호출하여 핀토스를 종료합니다 (src/include/threads/init.h에 선언됨).
	교착 상태에 빠질 수 있는 상황 등에 대한 일부 정보를 잃게 되므로 거의 사용하지 않는 것이 좋습니다.
*/
void halt(void) {
	power_off();
	thread_exit();
}

/*
	현재 사용자 프로그램을 종료하여 커널에 상태를 반환합니다. 
	프로세스의 부모가 기다리는 경우(아래 참조) 반환되는 상태가 이 상태입니다.
	일반적으로 상태 0은 성공을 나타내고 0이 아닌 값은 오류를 나타냅니다.
*/
void
exit (int status) {
	struct thread *curr = thread_current();
	curr->exit_flag = true;
	curr->exit_status = status;

	printf("%s: exit(%d)\n", thread_name(), status); 
	thread_exit();
}

/*
	현재 프로세스의 복제본인 새 프로세스를 THREAD_NAME이라는 이름으로 생성합니다.
	호출자가 저장한 레지스터인 %RBX, %RSP, %RBP, %R12 - %R15를 제외한 레지스터의 값은 복제할 필요가 없습니다.
	자식 프로세스의 pid를 반환해야 하며, 그렇지 않으면 유효한 pid가 아니어야 합니다.
	자식 프로세스에서 반환 값은 0이어야 합니다. 
	자식 프로세스는 파일 기술자 및 가상 메모리 공간을 포함한 중복 리소스를 가져야 합니다.
	부모 프로세스는 자식 프로세스가 성공적으로 복제되었는지 여부를 알기 전까지는 포크에서 반환하지 않아야 합니다.
	즉, 자식 프로세스가 리소스를 복제하는 데 실패하면 부모의 fork() 호출은 TID_ERROR를 반환해야 합니다.
	템플릿은 threads/mmu.c의 pml4_for_each()를 사용하여 
	해당 페이지 테이블 구조를 포함한 전체 사용자 메모리 공간을 복사하지만, 
	에 해당 페이지 테이블 구조를 포함하지만, 전달된 pte_for_each_func의 누락된 부분을 채워야 합니다.
*/
pid_t
fork (const char *thread_name){
	return (pid_t) 0;
}

/*
	지정된 인수를 전달하여 현재 프로세스를 cmd_line에 지정된 이름의 실행 파일로 변경합니다. 
	성공하면 절대 반환되지 않습니다. 
	그렇지 않으면 어떤 이유로든 프로그램을 로드하거나 
	실행할 수 없는 경우 종료 상태 -1로 프로세스가 종료됩니다. 
	이 함수는 exec를 호출한 스레드의 이름을 변경하지 않습니다. 
	파일 설명자는 실행 호출 내내 열려 있다는 점에 유의하세요.
*/
int
exec (const char *cmd_line) {
	if(process_exec(cmd_line) == -1) {
		return -1;
	}
	return 0;
}

/*
	자식 프로세스 pid를 기다렸다가 자식의 종료 상태를 검색합니다.
	pid가 아직 살아있다면 종료될 때까지 기다립니다. 
	그런 다음 pid가 종료하기 위해 전달한 상태를 반환합니다. 
	pid가 exit()를 호출하지 않았지만 커널에 의해 종료된 경우(예: 예외로 인해 종료된 경우), 
	wait(pid)는 -1을 반환해야 합니다. 
	부모 프로세스가 wait를 호출할 때 이미 종료된 자식 프로세스를 기다리는 것은 완전히 합법적이지만, 
	커널은 여전히 부모가 자식의 종료 상태를 검색하거나 
	자식이 커널에 의해 종료되었음을 알 수 있도록 허용해야 합니다.
*/
int
wait (pid_t pid) {
	struct thread *parent_process = thread_current();
	struct thread *child_process;
	struct thread *t;

	struct list_elem *e;
	for (e = list_begin (&parent_process->child_process); e != list_end (&parent_process->child_process); e = list_next (e)) {
		t = list_entry (e, struct thread, child_elem);
		if (t->tid == pid) {
			child_process = t;
		}
  	}

	while (child_process->exit_flag == false) {
	}
	
	return child_process->exit_status;
}

/*
	초기 initial_size 바이트 크기의 file이라는 새 파일을 생성합니다. 
	성공하면 참을 반환하고, 그렇지 않으면 거짓을 반환합니다. 
	새 파일을 만든다고 해서 파일이 열리지는 않습니다. 
	새 파일을 열려면 시스템 호출이 필요한 별도의 작업입니다.
*/
bool
create (const char *file, unsigned initial_size) {
	if (file == NULL) {
		exit(-1);
	}
	return filesys_create(file, initial_size);
}

/*
	file이라는 파일을 삭제합니다. 
	성공하면 참을 반환하고, 그렇지 않으면 거짓을 반환합니다. 
	파일은 열려 있는지 여부에 관계없이 제거할 수 있으며 열려 있는 파일을 제거해도 닫히지 않습니다. 
	자세한 내용은 FAQ에서 열린 파일 제거하기를 참조하세요.
*/
bool
remove (const char *file) {
	return filesys_remove(file);
}

/*
	Opens the file called file. Returns a nonnegative integer handle called a "file descriptor" (fd),
	or -1 if the file could not be opened. 
	File descriptors numbered 0 and 1 are reserved for the console:
	fd 0 (STDIN_FILENO) is standard input, fd 1 (STDOUT_FILENO) is standard output. 
	The open system call will never return either of these file descriptors, 
	which are valid as system call arguments only as explicitly described below. 
	Each process has an independent set of file descriptors. 
	File descriptors are inherited by child processes. 
	When a single file is opened more than once, 
	whether by a single process or different processes, 
	each open returns a new file descriptor. 
	Different file descriptors for a single file are closed independently 
	in separate calls to close and they do not share a file position.
*/
int
open (const char *file) {
	struct thread *curr = thread_current();
	return syscall1 (SYS_OPEN, file);
}


/*
	Returns the size, in bytes, of the file open as fd.
*/
// int
// filesize (int fd) {
// 	return syscall1 (SYS_FILESIZE, fd);
// }

/*
	Reads size bytes from the file open as fd into buffer. 
	Returns the number of bytes actually read (0 at end of file), 
	or -1 if the file could not be read (due to a condition other than end of file). 
	fd 0 reads from the keyboard using input_getc().
*/
// int
// read (int fd, void *buffer, unsigned size) {
// 	return syscall3 (SYS_READ, fd, buffer, size);
// }

/*
	Writes size bytes from buffer to the open file fd. 
	Returns the number of bytes actually written, 
	which may be less than size if some bytes could not be written. 
	Writing past end-of-file would normally extend the file, 
	but file growth is not implemented by the basic file system. 
	The expected behavior is to write as many bytes as possible up to end-of-file 
	and return the actual number written, or 0 if no bytes could be written at all. 
	fd 1 writes to the console. Your code to write to the console should 
	write all of buffer in one call to putbuf(), at least as long as size 
	is not bigger than a few hundred bytes (It is reasonable to break up larger buffers).
	Otherwise, lines of text output by different processes may end up interleaved on the console, 
	confusing both human readers and our grading scripts.
*/
int
write (int fd, const void *buffer, unsigned size) {
	if (fd < 0)
		return NULL;

	if (fd == STD_OUTPUT) {
		putbuf(buffer, size);
		return size;
	}

	else {
		struct thread *curr = thread_current();
		struct file *file_obj = curr->fd_table[fd];

		// 파일에 대한 락을 획득
		lock_acquire();

		// 파일의 현재 위치로 이동, 쓰기
		file_seek(file_obj, file_tell(file_obj));
		off_t bytes_written = file_write(file_obj, buffer, size);

		// 파일의 현재 위치를 업데이트
		file_seek(file_obj, file_tell(file_obj) + bytes_written);

		// 파일에 대한 락을 해제
		lock_release();

		// 파일을 닫기
		file_close(file_obj);

		return bytes_written;
	}

	return 0;
}

/*
	Changes the next byte to be read or written in open file fd to position, 
	expressed in bytes from the beginning of the file (Thus, a position of 0 is the file's start).
	A seek past the current end of a file is not an error. 
	A later read obtains 0 bytes, indicating end of file. 
	A later write extends the file, filling any unwritten gap with zeros.
	(However, in Pintos files have a fixed length until project 4 is complete, 
	so writes past end of file will return an error.) 
	These semantics are implemented in the file system and
	do not require any special effort in system call implementation.
*/
// void
// seek (int fd, unsigned position) {
// 	syscall2 (SYS_SEEK, fd, position);
// 	printf("in seek\n");
// }


/*
	Returns the position of the next byte to be read or written in open file fd, 
	expressed in bytes from the beginning of the file.
*/
// unsigned
// tell (int fd) {
// 	return syscall1 (SYS_TELL, fd);
// }

/*
	Closes file descriptor fd. Exiting or terminating a process 
	implicitly closes all its open file descriptors, 
	as if by calling this function for each one.
*/
void
close (int fd) {
	// Error - invalid fd
	if (fd < 0)
		return NULL;

	struct thread *curr = thread_current();
	struct file *file_obj = curr->fd_table[fd];

	if (file_obj == NULL) {
		return;
	}
	if (fd <= 1) {
		return;
	}

	file_obj = NULL;
	file_close(file_obj);
}

/*
*/
// int
// dup2 (int oldfd, int newfd){
// 	return syscall2 (SYS_DUP2, oldfd, newfd);
// }

// void *
// mmap (void *addr, size_t length, int writable, int fd, off_t offset) {
// 	return (void *) syscall5 (SYS_MMAP, addr, length, writable, fd, offset);
// }

// void
// munmap (void *addr) {
// 	syscall1 (SYS_MUNMAP, addr);
// }

// bool
// chdir (const char *dir) {
// 	return syscall1 (SYS_CHDIR, dir);
// }

// bool
// mkdir (const char *dir) {
// 	return syscall1 (SYS_MKDIR, dir);
// }

// bool
// readdir (int fd, char name[READDIR_MAX_LEN + 1]) {
// 	return syscall2 (SYS_READDIR, fd, name);
// }

// bool
// isdir (int fd) {
// 	return syscall1 (SYS_ISDIR, fd);
// }

// int
// inumber (int fd) {
// 	return syscall1 (SYS_INUMBER, fd);
// }

// int
// symlink (const char* target, const char* linkpath) {
// 	return syscall2 (SYS_SYMLINK, target, linkpath);
// }

// int
// mount (const char *path, int chan_no, int dev_no) {
// 	return syscall3 (SYS_MOUNT, path, chan_no, dev_no);
// }

// int
// umount (const char *path) {
// 	return syscall1 (SYS_UMOUNT, path);
// }
