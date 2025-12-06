/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include <sys/inotify.h>

#define MAX_FILE_STATES 1024

typedef struct {
    char filepath[1024];
    int prev_lines;
} FileState;

extern FileState file_states[MAX_FILE_STATES];
extern int file_state_count;

int count_lines_in_file(const char* filepath);
int should_trigger_build(const char* filepath, int line_threshold, int* prev_lines);
int should_ignore_file(const char* name, const char* ignore_patterns);
int should_watch_extension(const char* name, const char* extensions);
int run_command(const char* command, const char* watch_path);
FileState* get_file_state(const char* filepath);
WatchConfig* find_config_by_path(const char* path);

#endif