#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cclynx.h"
#include "hashmap.h"

#define MAX_LINE_SIZE (1024)
#define MAX_MAPS (16)

static struct hashmap maps[MAX_MAPS];
static int map_count = 0;


static void process_command(const char * line)
{
    char cmd[64] = {0};
    sscanf(line, "%63s", cmd);

    if (strcmp(cmd, "init") == 0) {
        int capacity = 0;
        sscanf(line, "init %d", &capacity);

        if (map_count >= MAX_MAPS) {
            fprintf(stderr, "ERROR: too many maps\n");
            exit(1);
        }

        hashmap_init(&maps[map_count], (size_t)capacity);
        printf("map%d = init(%d)\n", map_count, capacity);
        ++map_count;
        return;
    }

    if (strcmp(cmd, "insert") == 0) {
        int map_id = 0;
        char key[256] = {0};
        char value[256] = {0};
        sscanf(line, "insert %d %255s %255s", &map_id, key, value);

        char * stored_key = memory_blob_pool_alloc(main_pool, strlen(key) + 1);
        strcpy(stored_key, key);

        char * stored_value = memory_blob_pool_alloc(main_pool, strlen(value) + 1);
        strcpy(stored_value, value);

        hashmap_insert(&maps[map_id], stored_key, stored_value);
        printf("map%d.insert(\"%s\", \"%s\")\n", map_id, key, value);
        return;
    }

    if (strcmp(cmd, "find") == 0) {
        int map_id = 0;
        char key[256] = {0};
        sscanf(line, "find %d %255s", &map_id, key);

        char * result = hashmap_find(&maps[map_id], key);

        if (result != NULL) {
            printf("map%d.find(\"%s\") = \"%s\"\n", map_id, key, result);
        } else {
            printf("map%d.find(\"%s\") = NULL\n", map_id, key);
        }
        return;
    }

    if (strcmp(cmd, "hash") == 0) {
        char key[256] = {0};
        sscanf(line, "hash %255s", key);

        unsigned int h = hashmap_hash(key);
        printf("hash(\"%s\") = %u\n", key, h);
        return;
    }
}


int main(const int argc, const char * argv[])
{
    if (argc <= 1) {
        fprintf(stderr, "No source given!\n");
        exit(1);
    }

    FILE * file = fopen(argv[1], "r");

    if (file == NULL) {
        fprintf(stderr, "Could not open file %s\n", argv[1]);
        exit(1);
    }

    struct cclynx_context ctx;
    cclynx_init(&ctx);

    char line[MAX_LINE_SIZE];
    while (fgets(line, MAX_LINE_SIZE, file) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (line[0] == '\0') {
            continue;
        }

        process_command(line);
    }

    cclynx_free(&ctx);

    fclose(file);

    return 0;
}
