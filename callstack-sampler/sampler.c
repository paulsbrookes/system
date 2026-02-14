#define _DEFAULT_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_STACK_DEPTH 64
#define DEFAULT_INTERVAL_MS 100

static volatile sig_atomic_t running = 1;

static void sigint_handler(int sig)
{
	(void)sig;
	running = 0;
}

static void read_and_print_stack(pid_t pid, int sample_num)
{
	struct user_regs_struct regs;

	if (ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
		fprintf(stderr, "ptrace GETREGS failed: %s\n", strerror(errno));
		return;
	}

	printf("--- sample %d ---\n", sample_num);
	printf("  0x%llx\n", (unsigned long long)regs.rip);

	uint64_t rbp = regs.rbp;
	int depth;

	for (depth = 0; depth < MAX_STACK_DEPTH; depth++) {
		if (rbp == 0)
			break;

		errno = 0;
		long ret_addr = ptrace(PTRACE_PEEKDATA, pid, rbp + 8, NULL);
		if (errno != 0)
			break;

		errno = 0;
		long saved_rbp = ptrace(PTRACE_PEEKDATA, pid, rbp, NULL);
		if (errno != 0)
			break;

		if (ret_addr == 0)
			break;

		printf("  0x%lx\n", (unsigned long)ret_addr);

		if ((uint64_t)saved_rbp <= rbp)
			break;

		rbp = (uint64_t)saved_rbp;
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <pid> [interval_ms]\n", argv[0]);
		return 1;
	}

	char *endptr;
	long pid_arg = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0' || pid_arg <= 0) {
		fprintf(stderr, "Invalid PID: %s\n", argv[1]);
		return 1;
	}
	pid_t pid = (pid_t)pid_arg;

	int interval_ms = DEFAULT_INTERVAL_MS;
	if (argc == 3) {
		long iv = strtol(argv[2], &endptr, 10);
		if (*endptr != '\0' || iv <= 0) {
			fprintf(stderr, "Invalid interval: %s\n", argv[2]);
			return 1;
		}
		interval_ms = (int)iv;
	}

	/* Check that target process exists */
	if (kill(pid, 0) == -1) {
		fprintf(stderr, "Process %d: %s\n", pid, strerror(errno));
		return 1;
	}

	/* Install SIGINT handler for clean shutdown */
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	/* Attach to target */
	if (ptrace(PTRACE_ATTACH, pid, NULL, NULL) == -1) {
		fprintf(stderr, "ptrace ATTACH to %d failed: %s\n",
			pid, strerror(errno));
		return 1;
	}

	/* Wait for initial stop after attach */
	int status;
	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		return 1;
	}

	if (!WIFSTOPPED(status)) {
		fprintf(stderr, "Target did not stop as expected\n");
		ptrace(PTRACE_DETACH, pid, NULL, NULL);
		return 1;
	}

	printf("Attached to process %d, sampling every %d ms\n", pid, interval_ms);
	printf("Press Ctrl+C to stop\n\n");

	int sample = 0;

	/* First sample: target is already stopped from PTRACE_ATTACH */
	sample++;
	read_and_print_stack(pid, sample);

	while (running) {
		/* Resume target */
		if (ptrace(PTRACE_CONT, pid, NULL, NULL) == -1) {
			fprintf(stderr, "ptrace CONT failed: %s\n",
				strerror(errno));
			break;
		}

		/* Sleep for the sampling interval */
		usleep((useconds_t)(interval_ms * 1000));

		if (!running)
			break;

		/* Stop target for next sample */
		if (kill(pid, SIGSTOP) == -1) {
			fprintf(stderr, "kill SIGSTOP failed: %s\n",
				strerror(errno));
			break;
		}

		if (waitpid(pid, &status, 0) == -1) {
			if (errno == EINTR) {
				/* SIGINT during waitpid — break cleanly */
				break;
			}
			perror("waitpid");
			break;
		}

		if (WIFEXITED(status) || WIFSIGNALED(status)) {
			printf("Target process exited\n");
			return 0;
		}

		if (!WIFSTOPPED(status))
			break;

		sample++;
		read_and_print_stack(pid, sample);
	}

	/* Detach — target resumes normally */
	printf("\nDetaching from process %d\n", pid);
	ptrace(PTRACE_CONT, pid, NULL, NULL);
	ptrace(PTRACE_DETACH, pid, NULL, NULL);

	return 0;
}
