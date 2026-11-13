/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 *
 * demacOprx release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 *
 * DMA C Testbench 
 */

/**
 * Build modes:
 *   DMA_MAIN=1 - Build as standalone executable with main()
 *   DMA_MAIN=0 - Build as library module with do_dma()
 */
   
#ifndef DMA_MAIN
#define DMA_MAIN 0
#endif

#include "sysbus.h"
#include "dpi_rtl.h"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cstdbool>
#include <thread>
#include <atomic>

// Generated DPI functions from RTL
#include <verilated.h>
#include <verilated_vcd_c.h>
#include "Vdma_top.h"
#include "Vdma_top__Dpi.h"  

#if DMA_MAIN == 1
extern "C" {
#include "common.h"
#include "os-mm.h"
}
#endif

// RTL Direct Test - Drive Verilator RTL directly
extern "C" int dpi_rtl_dma_copy(uint64_t src_offset, uint64_t dst_offset, uint64_t length);
extern "C" int dpi_rtl_dma_is_available(void);

/*
 * dpi_dma_copy - perform DMA copy via DPI
 * src_offset: source memory offset
 * dst_offset: destination memory offset
 * length: number of bytes to copy
 */
int dpi_dma_copy(struct pcb_t *caller, uint64_t src_offset, uint64_t dst_offset, uint64_t length) {
    void *src = NULL;
    void *dst = NULL;
    //struct krnl_t *krnl = caller->krnl;

    if (memory_bank_is_available()) {
        printf("dma_testbench modeo with dpi_dma_copy from memory bank\n");
        src = memory_bank_get_ptr(src_offset);
        dst = memory_bank_get_ptr(dst_offset);
    } else {
        struct krnl_t *krnl = caller ? caller->krnl : NULL;
        if (krnl && krnl->mram && krnl->mram->storage) {
            printf("dma_testbench mode with OS memory address\n");
            sysbus_set_ram(krnl->mram->storage, krnl->mram->maxsz);
            src = (void*)(krnl->mram->storage + src_offset);
            dst = (void*)(krnl->mram->storage + dst_offset);
        }
    }

    if (!src || !dst) {
        printf("dpi_dma_copy: invalid source or destination address\n");
        return -1;
    }

    printf("dpi_dma_copy: src=%llu, dst=%llu, len=%llu\n", 
		    (unsigned long long) src_offset, 
		    (unsigned long long) dst_offset,
		    (unsigned long long) length);

    uint8_t* psrc = (uint8_t*)src;
    uint64_t pattern_length = (length <= 255) ? length : 255;
    
    bool is_empty = true;
    for (uint64_t i = 0; i < length && i < 8; i++) {
        if (psrc[i] != 0) {
            is_empty = false;
            break;
        }
    }
    if (is_empty) {
        for (uint64_t i = 0; i < length; i++) {
            // Pattern cycles through 0x01 to min(length, 255)
            psrc[i] = (uint8_t)((i % pattern_length) + 1);
        }
    }

    printf("Before copy:\n");
    printf("  src[0] = 0x%02x, src[1] = 0x%02x\n"
           "  src[2] = 0x%02x, src[3] = 0x%02x\n"
           "  src[4] = 0x%02x, src[5] = 0x%02x\n"
           "  src[6] = 0x%02x, src[7] = 0x%02x\n",
           ((uint8_t*)src)[0], ((uint8_t*)src)[1],
           ((uint8_t*)src)[2], ((uint8_t*)src)[3],
           ((uint8_t*)src)[4], ((uint8_t*)src)[5],
           ((uint8_t*)src)[6], ((uint8_t*)src)[7]);
    printf("  dst[0] = 0x%02x, dst[1] = 0x%02x\n"
           "  dst[2] = 0x%02x, dst[3] = 0x%02x\n"
           "  dst[4] = 0x%02x, dst[5] = 0x%02x\n"
           "  dst[6] = 0x%02x, dst[7] = 0x%02x\n",
           ((uint8_t*)dst)[0], ((uint8_t*)dst)[1],
           ((uint8_t*)dst)[2], ((uint8_t*)dst)[3],
           ((uint8_t*)dst)[4], ((uint8_t*)dst)[5],
           ((uint8_t*)dst)[6], ((uint8_t*)dst)[7]);

    printf("\nCalling RTL DMA...\n");
    
    // Call the RTL DMA function directly with provided parameters
    int result = dpi_rtl_dma_copy(src_offset, dst_offset, length);
    
    printf("\nAfter copy:\n");
    printf("  src[0] = 0x%02x, src[1] = 0x%02x\n"
           "  src[2] = 0x%02x, src[3] = 0x%02x\n"
           "  src[4] = 0x%02x, src[5] = 0x%02x\n"
           "  src[6] = 0x%02x, src[7] = 0x%02x\n",
           ((uint8_t*)src)[0], ((uint8_t*)src)[1],
           ((uint8_t*)src)[2], ((uint8_t*)src)[3],
           ((uint8_t*)src)[4], ((uint8_t*)src)[5],
           ((uint8_t*)src)[6], ((uint8_t*)src)[7]);
    printf("  dst[0] = 0x%02x, dst[1] = 0x%02x\n"
           "  dst[2] = 0x%02x, dst[3] = 0x%02x\n"
           "  dst[4] = 0x%02x, dst[5] = 0x%02x\n"
           "  dst[6] = 0x%02x, dst[7] = 0x%02x\n",
           ((uint8_t*)dst)[0], ((uint8_t*)dst)[1],
           ((uint8_t*)dst)[2], ((uint8_t*)dst)[3],
           ((uint8_t*)dst)[4], ((uint8_t*)dst)[5],
           ((uint8_t*)dst)[6], ((uint8_t*)dst)[7]);
    
    return result;
}

static std::atomic<bool> dma_busy(false);
extern "C" pthread_mutex_t dma_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * do_dma - perform DMA function
 * source: source memory
 * destination: destination memory
 * length: number of bytes to copy
 */
extern "C" int do_dma(struct pcb_t *caller, uint64_t source, uint64_t destination, uint64_t length) {
    pthread_mutex_lock(&dma_lock);
    if (dma_busy) {
        pthread_mutex_unlock(&dma_lock);
        return -1;
    }

    dma_busy = true;

    std::thread dma_thread([caller, source, destination, length]() {
        dpi_dma_copy(caller, source, destination, length);
        dma_busy = false;
        pthread_mutex_unlock(&dma_lock);
    });
    dma_thread.detach();

    return 0;
}

#if DMA_MAIN == 1
/* main function for independent standalone dma_testbench build
 * [Usage]: ./dma_testbench [source_offset] [destination_offset] [length]
 * source: source memory offset
 * destination: destination memory offset
 * length: number of bytes to copy
 */
int main(int argc, char** argv) {
    // Default test parameters
    uint64_t source = 0x1000;
    uint64_t destination = 0x2000;
    uint64_t length = 512;

    /*
     *  Only accept command line arguments if provided
     *  [Usage]: ./dma_testbench [source_offset] [destination_offset] [length]
     */
    if (argc >= 4) {
        source = strtoull(argv[1], NULL, 0);
        destination = strtoull(argv[2], NULL, 0);
        length = strtoull(argv[3], NULL, 0);
    }
    
    printf("Running dma_testbench source=0x%llx, destination=0x%llx, length=%llu\n",
           (unsigned long long)source, (unsigned long long)destination, (unsigned long long)length);

    /*
     * Initialize the system memory bank
     * This is necessary to independent standalone dma_testbench build .
     */
    memory_bank_init(1024 * 1024);

    /* Initialize the internal Verilator RTL simulation
     * This is necessary to ensure that the RTL simulation is ready for DPI calls.
     */
    if (rtl_dma_init_simulation() != 0) {
        printf("Cannot initialize RTL simulation\n");
        return 1;
    }

    /*
    * Create process control block for the caller
    * Properly initialize all required fields
    */
    struct pcb_t *caller = (struct pcb_t *)calloc(1, sizeof(struct pcb_t));
    caller->krnl = (struct krnl_t *)calloc(1, sizeof(struct krnl_t));
    caller->krnl->mm = (struct mm_struct *)calloc(1, sizeof(struct mm_struct));
    caller->krnl->mram = (struct memphy_struct *)calloc(1, sizeof(struct memphy_struct));
    caller->krnl->active_mswp = (struct memphy_struct *)calloc(1, sizeof(struct memphy_struct));
    caller->mm = caller->krnl->mm;
    caller->pid = 0;  // Set a default PID

    int result = do_dma(caller, source, destination, length);

    while (dma_busy) {
        std::this_thread::yield();
    }

    // Cleanup
    free(caller->krnl->active_mswp);
    free(caller->krnl->mram);
    free(caller->krnl->mm);
    free(caller->krnl);
    free(caller);

    return result;
}
#endif
