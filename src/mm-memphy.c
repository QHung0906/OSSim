/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* Caitoa release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

// #ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include "mm64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER;
extern struct memphy_struct mswp[PAGING_MAX_MMSWP];
/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, addr_t offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      /* Traverse sequentially */
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock); 

   if (mp->rdmflg)
   {
      *value = mp->storage[addr];
      pthread_mutex_unlock(&mem_lock); 
      return 0;
   }
   else
   {
      pthread_mutex_unlock(&mem_lock); 
      return MEMPHY_seq_read(mp, addr, value);
   }
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, addr_t addr, BYTE value)
{

   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock); 

   if (mp->rdmflg)
   {
      mp->storage[addr] = data;
      pthread_mutex_unlock(&mem_lock); 
      return 0;
   }
   else
   {
      pthread_mutex_unlock(&mem_lock);
      return MEMPHY_seq_write(mp, addr, data);
   }
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   /* This setting come with fixed constant PAGESZ */
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;

   /* Init head of free framephy list */
   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;
   mp->free_fp_list = fst;
   mp->used_fp_list = NULL;

   /* We have list with first element, fill in the rest num-1 element member*/
   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }

   return 0;
}
static void used_fp_list(struct memphy_struct *mp, struct framephy_struct *node)
{
   node->fp_next = NULL;
   if (mp->used_fp_list == NULL)
   {
      mp->used_fp_list = node;
   }
   else
   {
      struct framephy_struct *curr = mp->used_fp_list;
      while (curr->fp_next != NULL)
      {
         curr = curr->fp_next;
      }
      curr->fp_next = node;
   }
}

/* Hàm lấy khung trang rảnh / thay thế trang gốc */
int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)
{
   if (mp == NULL || retfpn == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   struct framephy_struct *fp = mp->free_fp_list;
   if (fp != NULL)
   {
      *retfpn = fp->fpn;
      mp->free_fp_list = fp->fp_next;
      used_fp_list(mp, fp);
      pthread_mutex_unlock(&mem_lock);
      return 0;
   }

   struct framephy_struct *victim = mp->used_fp_list;
   if (victim == NULL)
   {
      pthread_mutex_unlock(&mem_lock);
      return -1;
   }

   addr_t victim_fpn = victim->fpn;
   int swp_dev = -1;
   addr_t swp_fpn = 0;

   for (int i = 0; i < PAGING_MAX_MMSWP; i++)
   {
      if (mswp[i].storage != NULL && mswp[i].free_fp_list != NULL)
      {
         struct framephy_struct *swp_fp = mswp[i].free_fp_list;
         swp_fpn = swp_fp->fpn;
         mswp[i].free_fp_list = swp_fp->fp_next;
         free(swp_fp);
         swp_dev = i;
         break;
      }
   }

   if (swp_dev == -1)
   {
      pthread_mutex_unlock(&mem_lock);
      return -1; 
   }

   if (__swap_cp_page(mp, victim_fpn, &mswp[swp_dev], swp_fpn) != 0)
   {
      pthread_mutex_unlock(&mem_lock);
      return -1;
   }

   mp->used_fp_list = victim->fp_next;
   used_fp_list(mp, victim);

   *retfpn = victim_fpn;

   pthread_mutex_unlock(&mem_lock);
   return 0;
}
   /* 
    * TODO manage free frame list
    */
   //*retfpn ..

   /* 
    * TODO MEMPHY is iteratively used up until its exhausted
    * No garbage collector acting then it not been released
    */
   

int MEMPHY_dump(struct memphy_struct *mp)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   for (int i = 0; i < mp->maxsz; i++)
   {
      if (i % 16 == 0)
         printf("\n0x%08x: ", i);

      printf("%02x ", (unsigned char)mp->storage[i]);
   }
   printf("\n");

   pthread_mutex_unlock(&mem_lock);
   return 0;
}
  /*
   * TODO dump memphy contnt mp->storage
   *     for tracing the memory content
   */


int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn)
{
   if (mp == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   struct framephy_struct *curr = mp->used_fp_list;
   struct framephy_struct *prev = NULL;

   while (curr != NULL)
   {
      if (curr->fpn == fpn)
      {
         if (prev == NULL)
            mp->used_fp_list = curr->fp_next;
         else
            prev->fp_next = curr->fp_next;

         free(curr);
         break;
      }
      prev = curr;
      curr = curr->fp_next;
   }

   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
   newnode->fpn = fpn;
   newnode->fp_next = mp->free_fp_list;
   mp->free_fp_list = newnode;

   pthread_mutex_unlock(&mem_lock);
   return 0;
}

/*
 *  Init MEMPHY struct
 */
int init_memphy(struct memphy_struct *mp, addr_t max_size, int randomflg)
{
   mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
   mp->maxsz = max_size;
   memset(mp->storage, 0, max_size * sizeof(BYTE));

#ifdef MM64
   MEMPHY_format(mp, PAGING64_PAGESZ);
#else
   MEMPHY_format(mp, PAGING_PAGESZ);
#endif

   mp->rdmflg = (randomflg != 0) ? 1 : 0;

   if (!mp->rdmflg) /* Not Ramdom acess device, then it serial device*/
      mp->cursor = 0;

   return 0;
}
int find_victim_page(struct mm_struct *mm, addr_t *pgn)
{
   if (mm == NULL || pgn == NULL)
      return -1;

   if (mm->fifo_pgn == NULL)
      return -1;

   struct pgn_t *victim = mm->fifo_pgn;
   *pgn = victim->pgn;
   mm->fifo_pgn = victim->pg_next;

   free(victim);

   return 0;
}

int __swap_cp_page(struct memphy_struct *mpsrc, addr_t srcfpn,
                   struct memphy_struct *mpdst, addr_t dstfpn)
{
   if (mpsrc == NULL || mpdst == NULL)
      return -1;

   if (mpsrc->storage == NULL || mpdst->storage == NULL)
      return -1;

#ifdef MM64
   addr_t pgsz = PAGING64_PAGESZ;
#else
   addr_t pgsz = PAGING_PAGESZ;
#endif

   addr_t src_off = srcfpn * pgsz;
   addr_t dst_off = dstfpn * pgsz;

   if (src_off + pgsz > (addr_t)mpsrc->maxsz)
      return -1;

   if (dst_off + pgsz > (addr_t)mpdst->maxsz)
      return -1;

   memcpy(&mpdst->storage[dst_off], &mpsrc->storage[src_off], pgsz);

   return 0;
}
// #endif
