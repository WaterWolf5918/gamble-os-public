#include <gosh.h>
#include "types.h"
#include "memory.h"
#include "math.h"
#include "str.h"

typedef struct _Allocation {
    u16 start;
    u16 length;
} Allocation;

static u8 blocks[MAX_BLOCKS/8];

static Allocation allocs[MAX_ALLOCS];

void init_mem() {
    memset(blocks, 0, sizeof(blocks));
    memset(allocs, 0, sizeof(allocs));
}

void set_block_used(u32 block, bool used) {
    if(used) blocks[block/8] |= (1 << (block % 8));
    else blocks[block/8] &= ~(1 << (block % 8));
}

bool is_block_used(u32 block) {
    if(blocks[block/8] & (1 << (block % 8))) return TRUE;
    return FALSE;
}

void push_alloc(u16 start, u16 length) {
    for(int i=0;i<MAX_ALLOCS;i++) {
        if(allocs[i].length == 0) {
            allocs[i].start = start;
            allocs[i].length = length;
            break;
        }
    }
}

// BEHOLD: The first fit, mostly inefficient malloc function!!
void *k_malloc(size_t size) {
    u32 blocks_req = ceil((f32)size/(f32)BLOCK_SIZE);

    u16 p   = 0;
    u16 st  = 0;

    for(u32 i=0;i<MAX_BLOCKS;i++) {
        if(is_block_used(i)) {
            st = i+1;
            p = 0;
        } else {
            p++;
        }
        
        if(p == blocks_req) {
            for(int x=0;x<p;x++) {
                set_block_used(st+x, TRUE);
            }

            // char *ret = itoa(MEM_BASE+BLOCK_SIZE*st, 16);
            // putc_dbg('[');
            // for(int i=0;i<strlen(ret);i++) putc_dbg(ret[i]);
            // putc_dbg(']');

            return (void*)(MEM_BASE+BLOCK_SIZE*st);
        }
    }

    // putc_dbg('/');
    
    return NULL;
}

void k_free(void *mem) {
    if(mem == NULL) return;

    u16 start = ((size_t)mem - MEM_BASE)/BLOCK_SIZE;

    for(int i=0;i<MAX_ALLOCS;i++) {
        if(allocs[i].start == start && allocs[i].length != 0) {
            for(int x=0;x<allocs[i].length;x++) {
                set_block_used(x+allocs[i].start, FALSE);
            }
            allocs[i].start = 0;
            allocs[i].length = 0;
            break;
        }
    }
}

int k_get_used() {
    int used = 0;
    for(int i=0;i<sizeof(blocks)*8;i++) {
        if(is_block_used(i)) used+=BLOCK_SIZE;
    }
    return used;
}

void memcpy(void*dest, void*src, size_t num) {
    for(size_t i=0;i<num;i++)
        *(u8*)(dest+i) = *(u8*)(src+i);
}