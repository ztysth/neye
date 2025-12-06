/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define _GNU_SOURCE
#include "monitor.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>

void handle_events(int fd, int *wd, int wd_count) {
    (void)wd;
    (void)wd_count;
    char buffer[BUF_LEN];
    int length = read(fd, buffer, BUF_LEN);

    if (length < 0) {
        perror("read");
        return;
    }

    int i = 0;
    while (i < length) {
        struct inotify_event *event = (struct inotify_event *)&buffer[i];

        if (event->len > 0) {
            if ((event->mask & IN_CREATE) && (event->mask & IN_ISDIR)) {
                int new_wd = inotify_add_watch(fd, event->name,
                                              IN_MODIFY | IN_CREATE | IN_MOVED_TO | IN_DELETE);
                if (new_wd == -1) {
                    fprintf(stderr, "Cannot add watch for new directory: %s\n", event->name);
                }
                i += EVENT_SIZE + event->len;
                continue;
            }
        }

        i += EVENT_SIZE + event->len;
    }
}

int add_watch_recursive(int fd, const char *path, int *wd, int *wd_count, int max_wd, const WatchConfig* config) {
    DIR *dir;
    struct dirent *entry;

    if (*wd_count >= max_wd) {
        return -1;
    }

    wd[*wd_count] = inotify_add_watch(fd, path,
                                     IN_MODIFY | IN_CREATE | IN_MOVED_TO | IN_DELETE);
    if (wd[*wd_count] == -1) {
        return -1;
    }
    (*wd_count)++;

    dir = opendir(path);
    if (!dir) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (should_ignore_file(entry->d_name, config->ignore_patterns)) {
            continue;
        }

        if (entry->d_type == DT_DIR || (entry->d_type == DT_UNKNOWN)) {
            struct stat st;
            char subpath[1024];
            snprintf(subpath, sizeof(subpath), "%s/%s", path, entry->d_name);

            if (stat(subpath, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (add_watch_recursive(fd, subpath, wd, wd_count, max_wd, config) != 0) {
                    break;
                }
            }
        }
    }

    closedir(dir);
    return 0;
}

void process_file_events(int fd) {
    char buffer[BUF_LEN];
    int length = read(fd, buffer, BUF_LEN);
    if (length > 0) {
        int j = 0;
        while (j < length) {
            struct inotify_event *event = (struct inotify_event *)&buffer[j];

            if (event->len > 0 && (event->mask & IN_MODIFY ||
                                  event->mask & IN_CREATE ||
                                  event->mask & IN_MOVED_TO)) {

                char full_path[1024];
                int config_idx;
                for (config_idx = 0; config_idx < config_count; config_idx++) {
                    snprintf(full_path, sizeof(full_path), "%s/%s",
                            configs[config_idx].path, event->name);

                    if (should_watch_extension(event->name, configs[config_idx].extensions)) {
                        if (!should_ignore_file(event->name, configs[config_idx].ignore_patterns)) {
                            FileState* state = get_file_state(full_path);
                            if (state) {
                                if (should_trigger_build(full_path, configs[config_idx].line_threshold,
                                                       &state->prev_lines)) {
                                    printf("File change detected in %s (threshold met), running command: %s\n",
                                           event->name, configs[config_idx].command);
                                    run_command(configs[config_idx].command, configs[config_idx].path);
                                    break;
                                } else {
                                    printf("File change detected in %s but threshold not met, skipping\n",
                                           event->name);
                                }
                            }
                        }
                    }
                }
            }

            j += EVENT_SIZE + event->len;
        }
    }
}