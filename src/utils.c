/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define _GNU_SOURCE
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

FileState file_states[MAX_FILE_STATES];
int file_state_count = 0;

int count_lines_in_file(const char* filepath) {
    FILE* file = fopen(filepath, "r");
    if (!file) return -1;

    int lines = 0;
    int ch;
    int prev_ch = '\n';

    while ((ch = fgetc(file)) != EOF) {
        if (ch == '\n') {
            lines++;
        }
        prev_ch = ch;
    }

    if (prev_ch != '\n' && lines > 0) {
        lines++;
    }

    fclose(file);
    return lines;
}

int should_trigger_build(const char* filepath, int line_threshold, int* prev_lines) {
    if (line_threshold <= 0) {
        return 1;
    }

    int current_lines = count_lines_in_file(filepath);
    if (current_lines < 0) {
        return 1;
    }

    int line_diff = abs(current_lines - *prev_lines);

    if (*prev_lines == -1) {
        *prev_lines = current_lines;
        return 0;
    }

    if (line_diff >= line_threshold) {
        *prev_lines = current_lines;
        return 1;
    }

    return 0;
}

int should_ignore_file(const char* name, const char* ignore_patterns) {
    if (!ignore_patterns || !name) return 0;

    char* patterns = strdup(ignore_patterns);
    char* token = strtok(patterns, " ");

    while (token) {
        if (strstr(token, "*")) {
            const char* ext = strrchr(name, '.');
            if (ext && strcmp(token + 1, ext) == 0) {
                free(patterns);
                return 1;
            }
        } else {
            if (strcmp(name, token) == 0) {
                free(patterns);
                return 1;
            }
        }
        token = strtok(NULL, " ");
    }

    free(patterns);
    return 0;
}

int should_watch_extension(const char* name, const char* extensions) {
    if (!extensions || strlen(extensions) == 0) {
        return 1;
    }

    if (!name) return 0;

    const char* ext = strrchr(name, '.');
    if (!ext) return 0;

    char* ext_list = strdup(extensions);
    char* token = strtok(ext_list, " ");

    while (token) {
        if (strcmp(ext, token) == 0) {
            free(ext_list);
            return 1;
        }
        token = strtok(NULL, " ");
    }

    free(ext_list);
    return 0;
}

int run_command(const char* command, const char* watch_path) {
    if (!command || strlen(command) == 0) {
        return 0;
    }

    pid_t pid = fork();

    if (pid == 0) {
        if (chdir(watch_path) != 0) {
            fprintf(stderr, "Cannot change directory to: %s\n", watch_path);
            exit(EXIT_FAILURE);
        }

        execl("/bin/sh", "sh", "-c", command, NULL);
        perror("execl");
        exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
        return WEXITSTATUS(status);
    } else {
        perror("fork");
        return -1;
    }
}

FileState* get_file_state(const char* filepath) {
    int i;
    for (i = 0; i < file_state_count; i++) {
        if (strcmp(file_states[i].filepath, filepath) == 0) {
            return &file_states[i];
        }
    }

    if (file_state_count < MAX_FILE_STATES) {
        strncpy(file_states[file_state_count].filepath, filepath, sizeof(file_states[file_state_count].filepath) - 1);
        file_states[file_state_count].filepath[sizeof(file_states[file_state_count].filepath) - 1] = '\0';
        file_states[file_state_count].prev_lines = -1;
        return &file_states[file_state_count++];
    }

    return NULL;
}

WatchConfig* find_config_by_path(const char* path) {
    int i;
    for (i = 0; i < config_count; i++) {
        if (strstr(path, configs[i].path) == path) {
            return &configs[i];
        }
    }
    return NULL;
}