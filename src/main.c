/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/file.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <unistd.h>

#include "config.h"
#include "utils.h"
#include "monitor.h"

static volatile sig_atomic_t keep_running = 1;
static int pid_fd = -1;

int send_quit_signal() {
    const char* pid_file = expand_path("~/.config/neye/neye.pid");
    FILE* fp = fopen(pid_file, "r");
    if (!fp) {
        fprintf(stderr, "neye is not running\n");
        return 1;
    }

    char pid_str[32];
    if (fgets(pid_str, sizeof(pid_str), fp)) {
        int pid = atoi(pid_str);
        fclose(fp);
        if (pid > 0 && kill(pid, 0) == 0) {
            kill(pid, SIGTERM);
            printf("Sending termination signal to neye (PID: %d)\n", pid);
            unlink(pid_file);
            sleep(1);
            if (kill(pid, 0) == 0) {
                kill(pid, SIGKILL);
                printf("Force killing neye (PID: %d)\n", pid);
            }
            return 0;
        } else {
            fprintf(stderr, "neye is not running\n");
            unlink(pid_file);
            return 1;
        }
    } else {
        fclose(fp);
        fprintf(stderr, "Invalid PID file\n");
        return 1;
    }
}

int check_single_instance() {
    const char* pid_file = expand_path("~/.config/neye/neye.pid");

    char* pid_dir = strdup(pid_file);
    char* last_slash = strrchr(pid_dir, '/');
    if (last_slash) {
        *last_slash = '\0';
        struct stat st = {0};
        if (stat(pid_dir, &st) == -1) {
            mkdir(pid_dir, 0755);
        }
    }
    free(pid_dir);

    pid_fd = open(pid_file, O_CREAT | O_RDWR, 0644);
    if (pid_fd == -1) {
        perror("Cannot open PID file");
        return 0;
    }

    if (flock(pid_fd, LOCK_EX | LOCK_NB) == -1) {
        if (errno == EWOULDBLOCK) {
            char pid_str[32];
            ssize_t len = read(pid_fd, pid_str, sizeof(pid_str) - 1);
            if (len > 0) {
                pid_str[len] = '\0';
                int existing_pid = atoi(pid_str);
                if (existing_pid > 0 && kill(existing_pid, 0) == 0) {
                    fprintf(stderr, "neye is already running (PID: %d), exiting\n", existing_pid);
                    close(pid_fd);
                    pid_fd = -1;
                    return 0;
                }
            }
            fprintf(stderr, "neye is locked, exiting\n");
            close(pid_fd);
            pid_fd = -1;
            return 0;
        } else {
            perror("Cannot acquire file lock");
            close(pid_fd);
            pid_fd = -1;
            return 0;
        }
    }

    if (ftruncate(pid_fd, 0) == -1) {
        perror("ftruncate");
    }
    lseek(pid_fd, 0, SEEK_SET);

    char pid_str[32];
    snprintf(pid_str, sizeof(pid_str), "%d\n", getpid());
    if (write(pid_fd, pid_str, strlen(pid_str)) == -1) {
        perror("Cannot write to PID file");
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        pid_fd = -1;
        return 0;
    }

    return 1;
}

void remove_pid_file() {
    if (pid_fd != -1) {
        flock(pid_fd, LOCK_UN);
        close(pid_fd);
        pid_fd = -1;
    }
    unlink(expand_path("~/.config/neye/neye.pid"));
}

void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [WATCH_PATH]\n", program_name);
    printf("Options:\n");
    printf("  -c, --config FILE    Config file path (default: ~/.config/neye/config.ini)\n");
    printf("  -e, --extensions EXT File extensions to watch (e.g.: .c .cpp .h)\n");
    printf("  -t, --threshold NUM  Line change threshold (default: 40)\n");
    printf("  -i, --ignore PATTERN Ignore patterns (space separated)\n");
    printf("  -r, --run COMMAND    Command to run on trigger\n");
    printf("  -h, --help           Show help information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s /path/to/project\n", program_name);
    printf("  %s -e \".c .h\" -t 20 -r \"make\" /path/to/project\n", program_name);
    printf("  %s -c ~/.neye.conf\n", program_name);
}

int main(int argc, char *argv[]) {
    const char* config_file = CONFIG_FILE;
    const char* watch_path = NULL;
    const char* extensions = NULL;
    const char* ignore_patterns = NULL;
    const char* command = NULL;
    int line_threshold = -1;

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_file = argv[++i];
            }
        } else if (strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--extensions") == 0) {
            if (i + 1 < argc) {
                extensions = argv[++i];
            }
        } else if (strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threshold") == 0) {
            if (i + 1 < argc) {
                line_threshold = atoi(argv[++i]);
            }
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--ignore") == 0) {
            if (i + 1 < argc) {
                ignore_patterns = argv[++i];
            }
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--run") == 0) {
            if (i + 1 < argc) {
                command = argv[++i];
            }
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "quit") == 0 || strcmp(argv[i], "exit") == 0) {
            return send_quit_signal();
        } else if (argv[i][0] != '-') {
            watch_path = argv[i];
        }
    }

    if (argc == 2 && (strcmp(argv[1], "quit") == 0 || strcmp(argv[1], "exit") == 0)) {
        return send_quit_signal();
    }

    if (!check_single_instance()) {
        return EXIT_FAILURE;
    }

    WatchConfig default_config;
    memset(&default_config, 0, sizeof(default_config));

    if (load_config(config_file)) {
        default_config = configs[0];
    }

        if (watch_path) {
        strncpy(default_config.path, expand_path(watch_path), sizeof(default_config.path) - 1);
        default_config.path[sizeof(default_config.path) - 1] = '\0';
    }
    if (command) {
        strncpy(default_config.command, command, sizeof(default_config.command) - 1);
        default_config.command[sizeof(default_config.command) - 1] = '\0';
    }
    if (extensions) {
        strncpy(default_config.extensions, extensions, sizeof(default_config.extensions) - 1);
        default_config.extensions[sizeof(default_config.extensions) - 1] = '\0';
    }
    if (ignore_patterns) {
        strncpy(default_config.ignore_patterns, ignore_patterns, sizeof(default_config.ignore_patterns) - 1);
        default_config.ignore_patterns[sizeof(default_config.ignore_patterns) - 1] = '\0';
    }
    if (line_threshold != -1) {
        default_config.line_threshold = line_threshold;
    }

      if (strlen(default_config.path) == 0) {
        if (!watch_path) {
            fprintf(stderr, "Error: watch path not specified and not found in config file\n");
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    if (strlen(default_config.command) == 0) {
        strcpy(default_config.command, "make");
    }
    if (strlen(default_config.extensions) == 0) {
        strcpy(default_config.extensions, ".c .cpp .h .hpp .v .vh .mk .py .sh");
    }
    if (strlen(default_config.ignore_patterns) == 0) {
        strcpy(default_config.ignore_patterns, "obj_dir dump.vcd prog.txt .git *.o *.so *.d *.exe");
    }
    if (default_config.line_threshold == 0) {
        default_config.line_threshold = 2;
    }

    config_count = 1;
    configs[0] = default_config;

    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        exit(EXIT_FAILURE);
    }

    const int max_wd = 256;
    int wd[max_wd];
    int wd_count = 0;

    for (i = 0; i < config_count; i++) {
        if (add_watch_recursive(fd, configs[i].path, wd, &wd_count, max_wd, &configs[i]) != 0) {
            fprintf(stderr, "Cannot add watch path: %s\n", configs[i].path);
            continue;
        }
    }

    if (wd_count == 0) {
        fprintf(stderr, "No paths to watch\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("neye started, watching %d paths. Type 'neye quit' or 'neye exit' to exit\n", wd_count);

    fd_set rfds;
    while (keep_running) {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int max_fd = (fd > STDIN_FILENO) ? fd : STDIN_FILENO;
        int ret = select(max_fd + 1, &rfds, NULL, NULL, &tv);

        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        } else if (ret > 0) {
            if (FD_ISSET(fd, &rfds)) {
                process_file_events(fd);
            }

            if (FD_ISSET(STDIN_FILENO, &rfds)) {
                char input[256];
                if (fgets(input, sizeof(input), stdin)) {
                    input[strcspn(input, "\n")] = '\0';

                    if (strcmp(input, "neye quit") == 0 || strcmp(input, "neye exit") == 0) {
                        printf("Exiting neye...\n");
                        keep_running = 0;
                        break;
                    }
                }
            }
        }
    }

    for (i = 0; i < wd_count; i++) {
        inotify_rm_watch(fd, wd[i]);
    }

    close(fd);
    remove_pid_file();
    printf("neye stopped\n");
    return 0;
}