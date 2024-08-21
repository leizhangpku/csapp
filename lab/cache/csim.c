#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <getopt.h>

#define BUFFER_SIZE 256 

// capacity = S * E * B


// cache行
typedef struct CacheLine
{
    int valid;
    long tag;
    int timestamp;
} CacheLine;

// cache组
typedef struct CacheSet
{
    CacheLine* lines;
} CacheSet;

typedef struct LRUCache
{
    CacheSet* sets;
} *LRUCache;

LRUCache cache;

int hit_cnt = 0;
int miss_cnt = 0;
int eviction_cnt = 0;

int S;
int E;
int s;
int b;
int v;
char trace_file[BUFFER_SIZE];

void initCache() {
    // S组
    cache = (LRUCache)malloc(sizeof(LRUCache));
    cache->sets = (CacheSet*)malloc(S * sizeof(CacheSet));
    for (int i = 0; i < S; ++i) {
        // E行
        // printf("%ld\n", &cache);
        cache->sets[i].lines = (CacheLine*)malloc(E * sizeof(CacheLine));
        for (int j = 0; j < E; ++j) {
            cache->sets[i].lines[j].valid = -1;
            cache->sets[i].lines[j].tag = -1;
            cache->sets[i].lines[j].timestamp = -1;
        }
    }
}

void freeCache() {
    for (int i = 0; i < S; ++i) {
        free(cache->sets[i].lines);
    }
    free(cache);
}

void accessData(long address) {
    long tag = (address >> b) >> s;
    int set_index = (address >> b) & ((1 << s) - 1);
    // printf(" tag = %ld, set_index = %d ", tag, set_index);
    // printf("S = %d, E = %d, b = %d\n", S, E, b);

    for (int i = 0; i < E; ++i) {
        // 命中
        if (cache->sets[set_index].lines[i].valid == 1 && cache->sets[set_index].lines[i].tag == tag) {
            hit_cnt++;
            if (v == 1) {
                printf("hit ");
            }
            cache->sets[set_index].lines[i].timestamp = 0;
            return;
        }
    }
    for (int i = 0; i < E; ++i) {
        // 未命中，有空行
        if (cache->sets[set_index].lines[i].valid == -1) {
            cache->sets[set_index].lines[i].valid = 1;
            cache->sets[set_index].lines[i].tag = tag;
            cache->sets[set_index].lines[i].timestamp = 0;
            if (v == 1) {
                printf("miss ");
            }
            miss_cnt++;
            return;
        }
    }
    int timestamp = -1;
    int index = -1;
    // cache set已满, 按LRU算法选出需要淘汰的行(即选出timestamp最大的行)
    miss_cnt++;
    eviction_cnt++;
    if (v == 1) {
        printf("miss eviction ");
    }
    for (int i = 0; i < E; ++i) {
        if (cache->sets[set_index].lines[i].timestamp > timestamp) {
            timestamp = cache->sets[set_index].lines[i].timestamp;
            index = i;
        }
    }
    cache->sets[set_index].lines[index].timestamp = 0;
    cache->sets[set_index].lines[index].tag = tag;
    return;
}

void updateTimestamp() {
    for (int i = 0; i < S; ++i) {
        for (int j = 0; j < E; ++j) {
            if (cache->sets[i].lines[j].valid == 1) {
                cache->sets[i].lines[j].timestamp++;
            }
        }
    }
    return;
}

// int parseMemAdress(MemOperation* mem_operation, char* line) {
//     unsigned long hexNum;
//     while (*line == ' ') {
//         line++;
//     } 
//     mem_operation->op = line[0];
//     // 使用sscanf尝试从每行中提取十六进制数  
//     // 假设格式总是 'X xxxxxxxx,y' 其中X是任意字符（可能包含空格），xxxxxxxx是十六进制数，y是数字  
//     if (sscanf(line, "%*c %lx,", &hexNum) == 1) {  
//         // 如果成功，打印十六进制数  
//         printf("Hex Number: 0x%lx\n", hexNum);
//         mem_operation->b = hexNum & (1 << (b - 1));
//         hexNum >>= b;
//         mem_operation->s = hexNum & (1 << (s - 1));
//         hexNum >>= s;
//         mem_operation->tag = hexNum; 
//     } else {  
//         printf("Failed to parse line: %s\n", line);  
//         return -1;
//     }
//     return 1;
// }


void replayTrace() {
    FILE *fp = fopen(trace_file, "r");
    char op;
    unsigned long address;
    int b;
    while (fscanf(fp, " %c %lx,%d\n", &op, &address, &b) > 0) {
        if (v == 1) {
            printf("%c %lx,%d ", op, address, b);
        }
        switch (op)
        {
        case 'L':
            accessData(address);
            break;
        case 'M':
            accessData(address);   // 这里后面没有break！“M” a data modify (i.e., a data load followed by a data store)
        case 'S':
            accessData(address);
            break;
        default:
            break;
        }
        if (v == 1) {
            printf("\n");
        }
        updateTimestamp();
    }
    fclose(fp);
    return;
}

void printUsage() {
    printf("Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n"
           "Options:\n"
           "-h         Print this help message.\n\n"
           "-v         Optional verbose flag.\n\n"
           "-s <num>   Number of set index bits.\n\n"
           "-E <num>   Number of lines per set.\n\n"
           "-b <num>   Number of block offset bits.\n\n"
           "-t <file>  Trace file.\n\n"
           "Examples:\n"
           "linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n"
           "linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}


int main(int argc, char* argv[])
{

    v = 0;
    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt)
        {
        case 'h':
            printUsage();
            break;
        case 'v':
            v = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strcpy(trace_file, optarg);
            break;
        default:
            printUsage();
        }
    }

    if (s <= 0 || E <= 0 || b <= 0 || trace_file == NULL) {
        printf("Input option error!\n");
        return -1;
    }
    S = 1 << s;
    initCache();

    FILE *fp = fopen(trace_file, "r");
    if (!fp) {
        printf("Open file %s error!\n", trace_file);
        return -1;
    }

    replayTrace();

    freeCache();
    printSummary(hit_cnt, miss_cnt, eviction_cnt);
    return 0;
}
