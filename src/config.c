/*
 * Copyright (c) 2025 ztysth neye is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at: http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#define _GNU_SOURCE
#include "config.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <unistd.h>

WatchConfig configs[MAX_CONFIG_DIRS];
int config_count = 0;

const char* expand_path(const char* path) {
    if (!path) return NULL;

    static char expanded[1024];
    if (path[0] == '~') {
        struct passwd *pw = getpwuid(getuid());
        if (pw) {
            snprintf(expanded, sizeof(expanded), "%s%s", pw->pw_dir, path + 1);
            return expanded;
        }
    }
    strncpy(expanded, path, sizeof(expanded) - 1);
    expanded[sizeof(expanded) - 1] = '\0';
    return expanded;
}

static int config_handler(void* user, const char* section, const char* name, const char* value) {
    WatchConfig* config = (WatchConfig*)user;

    if (strcmp(section, "watch") == 0) {
        if (strcmp(name, "path") == 0) {
            strncpy(config->path, expand_path(value), sizeof(config->path) - 1);
            config->path[sizeof(config->path) - 1] = '\0';
        } else if (strcmp(name, "command") == 0) {
            strncpy(config->command, value, sizeof(config->command) - 1);
            config->command[sizeof(config->command) - 1] = '\0';
        } else if (strcmp(name, "extensions") == 0) {
            strncpy(config->extensions, value, sizeof(config->extensions) - 1);
            config->extensions[sizeof(config->extensions) - 1] = '\0';
        } else if (strcmp(name, "line_threshold") == 0) {
            config->line_threshold = atoi(value);
        } else if (strcmp(name, "ignore_patterns") == 0) {
            strncpy(config->ignore_patterns, value, sizeof(config->ignore_patterns) - 1);
            config->ignore_patterns[sizeof(config->ignore_patterns) - 1] = '\0';
        }
    }

    return 1;
}

int create_default_config(const char* config_file) {
    char* expanded_path = strdup(expand_path(config_file));
    char* dir_path = strdup(expanded_path);
    char* last_slash = strrchr(dir_path, '/');
    if (last_slash) {
        *last_slash = '\0';
        struct stat st = {0};
        if (stat(dir_path, &st) == -1) {
            mkdir(dir_path, 0755);
        }
    }

    FILE* dest_fp = fopen(expanded_path, "w");
    if (!dest_fp) {
        free(expanded_path);
        free(dir_path);
        return 0;
    }

    FILE* src_fp = fopen("config/default.ini", "r");
    if (src_fp) {
        char buffer[1024];
        while (fgets(buffer, sizeof(buffer), src_fp)) {
            fputs(buffer, dest_fp);
        }
        fclose(src_fp);
    } else {
        fprintf(dest_fp, "[watch]\n");
        fprintf(dest_fp, "path = ~/projects\n");
        fprintf(dest_fp, "command = make\n");
        fprintf(dest_fp, "extensions = .c .cpp .h .hpp .v .vh .mk .py .sh\n");
        fprintf(dest_fp, "line_threshold = 2\n");
        fprintf(dest_fp, "ignore_patterns = obj_dir dump.vcd prog.txt .git *.o *.so *.d *.exe build\n");
    }

    fclose(dest_fp);
    free(expanded_path);
    free(dir_path);
    return 1;
}

int load_config(const char* config_file) {
    char* expanded_path = strdup(expand_path(config_file));
    FILE* fp = fopen(expanded_path, "r");

    if (!fp) {
              if (create_default_config(config_file)) {
            fp = fopen(expanded_path, "r");
            if (!fp) {
                fprintf(stderr, "Cannot open config file after creating: %s\n", expanded_path);
                free(expanded_path);
                return 0;
            }
        } else {
            fprintf(stderr, "Cannot open config file: %s\n", expanded_path);
            free(expanded_path);
            return 0;
        }
    }
    fclose(fp);
    free(expanded_path);

    config_count = 0;
    memset(configs, 0, sizeof(configs));

    WatchConfig temp_config;
    memset(&temp_config, 0, sizeof(temp_config));

    if (ini_parse(expand_path(config_file), config_handler, &temp_config) < 0) {
        fprintf(stderr, "Cannot parse config file: %s\n", config_file);
        return 0;
    }

    if (strlen(temp_config.path) > 0) {
        configs[config_count++] = temp_config;
                return 1;
    }

    return 0;
}