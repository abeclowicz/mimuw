#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "common/err.h"

#define MAX_ENVIRONMENTS 1000000
#define MAX_POLICIES 1000000

#define SYS_OK(expr) ((expr) != -1)

#define MAKE_SHARED(len) mmap(NULL, (len), PROT_READ | PROT_WRITE, \
                              MAP_SHARED | MAP_ANONYMOUS, -1, 0)

char* env_path;
char* policy_path;
int extra_argc;
char** extra_argv;
int max_concurrent_policy_calls;

/* -------------------------------------------------------------------------- */

volatile sig_atomic_t interrupted = 0;

void interrupt_handler(int) {
    interrupted = 1;
}

bool set_sigint_handler(void(*handler)(int)) {
    struct sigaction sa;

    sa.sa_flags = 0;
    sa.sa_handler = handler;

    return sigemptyset(&sa.sa_mask) != -1 && sigaction(SIGINT, &sa, NULL) != -1;
}

/* -------------------------------------------------------------------------- */

char* get_fifo_name(pid_t pid, int fd) {
    char* name = (char*)malloc(sizeof(char) * 64);

    if (name) {
        (void)snprintf(name, 64, "/tmp/fifo.%d.%d", pid, fd);
    }

    return name;
}

char* fifo_replace_fd(int fd) {
    char* fifo = get_fifo_name(getpid(), fd);

    if (!fifo || !SYS_OK(mkfifo(fifo, 0755))) {
        free(fifo);
        return NULL;
    }

    int fifo_fd = open(fifo, O_RDWR);

    if (SYS_OK(fifo_fd)) {
        if (!SYS_OK(dup2(fifo_fd, fd))) {
            (void)unlink(fifo);

            free(fifo);
            fifo = NULL;
        }

        (void)close(fifo_fd);
    }

    return fifo;
}

char* fifo_replace_stdin() {
    return fifo_replace_fd(STDIN_FILENO);
}

char* fifo_replace_stdout() {
    return fifo_replace_fd(STDOUT_FILENO);
}

/* -------------------------------------------------------------------------- */

typedef struct {
    pid_t pid;
    char fifo_in[64];
    char fifo_out[64];
} exec_t;

exec_t* spawn_exec(char* path, char* args[]) {
    exec_t* shared_exec = MAKE_SHARED(sizeof(exec_t));

    if (shared_exec == MAP_FAILED) {
        return NULL;
    }

    sem_t* shared_sem = MAKE_SHARED(sizeof(sem_t));

    if (shared_sem == MAP_FAILED) {
        (void)munmap(shared_exec, sizeof(exec_t));
        return NULL;
    }

    shared_exec->pid = -1; // important

    if (SYS_OK(sem_init(shared_sem, 1, 0))) {
        if (fork() == 0) {
            char* fifo_in = fifo_replace_stdin();
            char* fifo_out = fifo_replace_stdout();

            if (!fifo_in || !fifo_out) {
                if (fifo_in) {
                    (void)unlink(fifo_in);
                }

                if (fifo_out) {
                    (void)unlink(fifo_out);
                }

                free(fifo_in);
                free(fifo_out);

                (void)sem_post(shared_sem);
                exit(1);
            }

            (void)memcpy(shared_exec->fifo_in, fifo_in, sizeof(char) * 64);
            (void)memcpy(shared_exec->fifo_out, fifo_out, sizeof(char) * 64);

            free(fifo_in);
            free(fifo_out);

            if (!set_sigint_handler(SIG_DFL)) {
                (void)unlink(shared_exec->fifo_in);
                (void)unlink(shared_exec->fifo_out);

                (void)sem_post(shared_sem);
                exit(1);
            }

            shared_exec->pid = getpid(); // important

            (void)sem_post(shared_sem);
            execv(path, args);
        }

        (void)sem_wait(shared_sem);
        (void)sem_destroy(shared_sem);
    }

    exec_t* exec = NULL;

    if (SYS_OK(shared_exec->pid)) {
        exec = (exec_t*)malloc(sizeof(exec_t));

        if (exec) {
            *exec = *shared_exec;
        }
    }

    (void)munmap(shared_exec, sizeof(exec_t));
    (void)munmap(shared_sem, sizeof(sem_t));

    return exec;
}

exec_t* spawn_policy(size_t index) {
    char** args = (char**)malloc(sizeof(char*) * (extra_argc + 3));

    if (!args) {
        return NULL;
    }

    char* index_str = (char*)malloc(sizeof(char) * 32);

    if (!index_str) {
        free(args);
        return NULL;
    }

    (void)snprintf(index_str, 32, "%ld", index);

    args[0] = "policy";
    args[1] = index_str;
    (void)memcpy(args + 2, extra_argv, sizeof(char*) * (extra_argc + 1));

    exec_t* policy = spawn_exec(policy_path, args);

    free(index_str);
    free(args);

    return policy;
}

exec_t* spawn_env(char* test_name) {
    char** args = (char**)malloc(sizeof(char*) * (extra_argc + 3));

    if (!args) {
        return NULL;
    }

    args[0] = "env";
    args[1] = test_name;
    (void)memcpy(args + 2, extra_argv, sizeof(char*) * (extra_argc + 1));

    exec_t* env = spawn_exec(env_path, args);

    free(args);

    return env;
}

void destroy_exec(exec_t* exec, bool should_free) {
    (void)unlink(exec->fifo_in);
    (void)unlink(exec->fifo_out);

    if (should_free) {
        free(exec);
    }
}

/* -------------------------------------------------------------------------- */

typedef struct {
    sem_t sem_active_environments;
    sem_t sem_concurrent_calls;
    sem_t sem_concurrent_policy_calls;

    sem_t mutex;
    atomic_size_t policy_id;
    int range_l, range_r;
    size_t available;
    exec_t policies[MAX_POLICIES];

    sem_t sem_print[MAX_ENVIRONMENTS];

} shared_data_t;

exec_t* take_policy(shared_data_t* shared_data) {
    if (!SYS_OK(sem_wait(&shared_data->mutex))) {
        return NULL;
    }

    if (shared_data->available > 0) {
        exec_t* policy = (exec_t*)malloc(sizeof(exec_t));

        if (policy) {
            *policy = shared_data->policies[shared_data->range_l];
            shared_data->range_l = (shared_data->range_l + 1) % MAX_POLICIES;
            shared_data->available--;
        }

        (void)sem_post(&shared_data->mutex);

        return policy;
    }

    (void)sem_post(&shared_data->mutex);

    return spawn_policy(atomic_fetch_add(&shared_data->policy_id, 1));
}

void return_policy(exec_t* policy, shared_data_t* shared_data) {
    (void)sem_wait(&shared_data->mutex);

    shared_data->range_r = (shared_data->range_r + 1) % MAX_POLICIES;
    shared_data->policies[shared_data->range_r] = *policy;
    shared_data->available++;

    free(policy);

    (void)sem_post(&shared_data->mutex);
}

char* evaluate(char* test_name, shared_data_t* shared_data) {
    char* action = (char*)malloc(sizeof(char) * (ACTION_SIZE + 1));
    char* state = (char*)malloc(sizeof(char) * (STATE_SIZE + 1));

    if (!action || !state ||
        !SYS_OK(sem_wait(&shared_data->sem_concurrent_calls))
    ) {
        free(action), free(state);
        return NULL;
    }

    exec_t* env = spawn_env(test_name);

    if (!env) {
        free(action), free(state);
        (void)sem_post(&shared_data->sem_concurrent_calls);
        return NULL;
    }

    int env_in = open(env->fifo_in, O_WRONLY);
    int env_out = open(env->fifo_out, O_RDONLY);

    bool error = false;

    if (env_in == -1 || env_out == -1 ||
        !SYS_OK(fcntl(env_in, F_SETFD, FD_CLOEXEC)) ||
        !SYS_OK(fcntl(env_out, F_SETFD, FD_CLOEXEC)) ||
        read(env_out, state, STATE_SIZE + 1) != STATE_SIZE + 1
    ) {
        error = true;
    }

    (void)sem_post(&shared_data->sem_concurrent_calls);

    // order dependency: error before state[0]
    while (!error && state[0] != 'T' && !interrupted) {
        if (!SYS_OK(sem_wait(&shared_data->sem_concurrent_policy_calls)) ||
            !SYS_OK(sem_wait(&shared_data->sem_concurrent_calls))
        ) {
            error = true;
            break;
        }

        exec_t* policy = take_policy(shared_data);

        if (policy) {
            int policy_in = open(policy->fifo_in, O_WRONLY);
            int policy_out = open(policy->fifo_out, O_RDONLY);

            if (policy_in == -1 || policy_out == -1 ||
                write(policy_in, state, STATE_SIZE + 1) != STATE_SIZE + 1 ||
                read(policy_out, action, ACTION_SIZE + 1) != ACTION_SIZE + 1
            ) {
                error = true;
            }

            (void)close(policy_in);
            (void)close(policy_out);

            return_policy(policy, shared_data);
        }

        (void)sem_post(&shared_data->sem_concurrent_calls);
        (void)sem_post(&shared_data->sem_concurrent_policy_calls);

        if (error || interrupted) {
            break;
        }

        if (!SYS_OK(sem_wait(&shared_data->sem_concurrent_calls))) {
            error = true;
            break;
        }

        if (write(env_in, action, ACTION_SIZE + 1) != ACTION_SIZE + 1 ||
            read(env_out, state, STATE_SIZE + 1) != STATE_SIZE + 1
        ) {
            error = true;
        }

        (void)sem_post(&shared_data->sem_concurrent_calls);
    }

    (void)close(env_in);
    (void)close(env_out);

    (void)waitpid(env->pid, NULL, 0);
    destroy_exec(env, true);

    free(action);
    if (error || interrupted) {
        free(state);
        return NULL;
    }

    state[STATE_SIZE] = '\0';
    return state;
}

/* -------------------------------------------------------------------------- */

int main(int argc, char* argv[]) {
    env_path = argv[2];
    policy_path = argv[1];
    extra_argv = &argv[6];
    extra_argc = argc - 6;
    max_concurrent_policy_calls = atoi(argv[3]);

    if (!set_sigint_handler(interrupt_handler)) {
        return 1;
    }

    shared_data_t* shared_data = MAKE_SHARED(sizeof(shared_data_t));

    if (shared_data == MAP_FAILED) {
        return 1;
    }

    if (SYS_OK(sem_init(
            &shared_data->sem_active_environments, 1, atoi(argv[5])
        ))
    ) {
        if (SYS_OK(sem_init(
                &shared_data->sem_concurrent_calls, 1, atoi(argv[4])
            ))
        ) {
            if (SYS_OK(sem_init(
                    &shared_data->sem_concurrent_policy_calls, 1, atoi(argv[3])
                ))
            ) {
                if (SYS_OK(sem_init(&shared_data->mutex, 1, 1))) {
                    goto __main_sem_init_success;
                }

                (void)sem_destroy(&shared_data->sem_concurrent_policy_calls);
            }
            
            (void)sem_destroy(&shared_data->sem_concurrent_calls);
        }
        
        (void)sem_destroy(&shared_data->sem_active_environments);
    }

    __main_sem_init_success:
    
    for (size_t i = 0; i < MAX_ENVIRONMENTS; ++i) {
        if (!SYS_OK(sem_init(&shared_data->sem_print[i], 1, (int)(i == 0)))) {
            (void)sem_destroy(&shared_data->sem_active_environments);
            (void)sem_destroy(&shared_data->sem_concurrent_calls);
            (void)sem_destroy(&shared_data->sem_concurrent_policy_calls);

            for (size_t j = 0; j < i; ++j) {
                (void)sem_destroy(&shared_data->sem_print[j]);
            }

            (void)munmap(shared_data, sizeof(shared_data));
            return 1;
        }
    }

    shared_data->policy_id = 0;
    shared_data->range_l = 0;
    shared_data->range_r = -1;
    shared_data->available = 0;

    for (size_t i = 0; i < MAX_POLICIES; ++i) {
        shared_data->policies[i].pid = -1; // important
    }

    bool error = false;
    char* test_name = (char*)malloc(sizeof(char) * (NAME_SIZE + 1));

    if (!test_name) {
        error = true;
    }

    size_t test_num = 0;

    // order dependency: error before scanf()
    while (!error && scanf("%s", test_name) == 1 && !interrupted) {
        if (!SYS_OK(sem_wait(&shared_data->sem_active_environments))) {
            error = true;
            break;
        }

        if (fork() == 0) {
            char* state = evaluate(test_name, shared_data);

            (void)sem_post(&shared_data->sem_active_environments);

            if (state) {
                size_t index = test_num % MAX_ENVIRONMENTS;
                size_t next_index = (test_num + 1) % MAX_ENVIRONMENTS;

                bool success = SYS_OK(sem_wait(&shared_data->sem_print[index]));

                if (success) {
                    (void)printf("%s %s\n", test_name, state);
                    (void)fflush(stdout);
                    
                    (void)sem_post(&shared_data->sem_print[next_index]);
                }

                free(test_name);
                free(state);

                exit((int)!success);
            }

            free(test_name);
            exit(1);
        }

        int status;
        // order dependency: waitpid() must be last
        while (!error && !interrupted && waitpid(-1, &status, WNOHANG) > 0) {
            if (status == 1) {
                error = true;
            }
        }

        test_num++;
    }
    
    free(test_name);

    int status;
    // order dependency: wait() must be last
    while (!error && !interrupted && SYS_OK(wait(&status))) {
        if (status == 1) {
            error = true;
        }
    }

    if (!error && !interrupted) {
        (void)set_sigint_handler(SIG_IGN); // still have to kill policies
    }
    
    (void)kill(0, SIGINT);

    while (SYS_OK(wait(NULL))); // wait for policies

    for (size_t i = 0; i < MAX_POLICIES; ++i) {
        if (SYS_OK(shared_data->policies[i].pid)) {
            destroy_exec(&shared_data->policies[i], false);
        }
    }

    (void)sem_destroy(&shared_data->sem_active_environments);
    (void)sem_destroy(&shared_data->sem_concurrent_calls);
    (void)sem_destroy(&shared_data->sem_concurrent_policy_calls);
    (void)sem_destroy(&shared_data->mutex);

    for (size_t i = 0; i < MAX_ENVIRONMENTS; ++i) {
        (void)sem_destroy(&shared_data->sem_print[i]);
    }

    (void)munmap(shared_data, sizeof(shared_data_t));

    return interrupted ? 2 : (int)error;
}
