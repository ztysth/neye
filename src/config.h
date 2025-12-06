/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <ini.h>

#define MAX_CONFIG_DIRS 32
#define CONFIG_FILE "~/.config/neye/config.ini"

typedef struct {
    char path[512];
    char command[1024];
    char extensions[256];
    int line_threshold;
    char ignore_patterns[512];
} WatchConfig;

extern WatchConfig configs[MAX_CONFIG_DIRS];
extern int config_count;

const char* expand_path(const char* path);
int create_default_config(const char* config_file);
int load_config(const char* config_file);

#endif