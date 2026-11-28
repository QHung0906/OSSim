/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

#include "mm.h"
#include "mm64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

pthread_mutex_t mem_lock = PTHREAD_MUTEX_INITIALIZER;

unsigned long mem_read_count = 0;
unsigned long mem_write_count = 0;
unsigned long swap_out_count = 0;

int MEMPHY_mv_csr(struct memphy_struct *mp, addr_t offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }

   return 0;
}

int MEMPHY_seq_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg)
      return -1;

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];

   return 0;
}

int MEMPHY_read(struct memphy_struct *mp, addr_t addr, BYTE *value)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);
   mem_read_count++;

   int ret;
   if (mp->rdmflg)
   {
      *value = mp->storage[addr];
      ret = 0;
   }
   else
   {
      ret = MEMPHY_seq_read(mp, addr, value);
   }

   pthread_mutex_unlock(&mem_lock);
   return ret;
}

int MEMPHY_seq_write(struct memphy_struct *mp, addr_t addr, BYTE value)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg)
      return -1;

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

int MEMPHY_write(struct memphy_struct *mp, addr_t addr, BYTE data)
{
   if (mp == NULL || mp->storage == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);
   mem_write_count++;

   int ret;
   if (mp->rdmflg)
   {
      mp->storage[addr] = data;
      ret = 0;
   }
   else
   {
      ret = MEMPHY_seq_write(mp, addr, data);
   }

   pthread_mutex_unlock(&mem_lock);
   return ret;
}

int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;

   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;
   fst->fp_next = NULL;
   fst->owner = NULL;
   mp->free_fp_list = fst;
   mp->used_fp_list = NULL;

   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      newfst->owner = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }

   return 0;
}

int MEMPHY_get_freefp(struct memphy_struct *mp, addr_t *retfpn)
{
   if (mp == NULL || retfpn == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   struct framephy_struct *fp = mp->free_fp_list;
   if (fp == NULL)
   {
      pthread_mutex_unlock(&mem_lock);
      return -1;
   }

   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;

   fp->fp_next = mp->used_fp_list;
   mp->used_fp_list = fp;

   pthread_mutex_unlock(&mem_lock);
   return 0;
}

int MEMPHY_put_freefp(struct memphy_struct *mp, addr_t fpn)
{
   if (mp == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   struct framephy_struct *curr = mp->used_fp_list;
   struct framephy_struct *prev = NULL;

   while (curr != NULL && curr->fpn != fpn)
   {
      prev = curr;
      curr = curr->fp_next;
   }

   if (curr != NULL)
   {
      if (prev == NULL)
         mp->used_fp_list = curr->fp_next;
      else
         prev->fp_next = curr->fp_next;

      curr->fp_next = mp->free_fp_list;
      mp->free_fp_list = curr;
   }
   else
   {
      struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));
      if (newnode != NULL)
      {
         newnode->fpn = fpn;
         newnode->fp_next = mp->free_fp_list;
         newnode->owner = NULL;
         mp->free_fp_list = newnode;
      }
   }

   pthread_mutex_unlock(&mem_lock);
   return 0;
}

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

int init_memphy(struct memphy_struct *mp, addr_t max_size, int randomflg)
{
   if (mp == NULL)
      return -1;

   mp->free_fp_list = NULL;
   mp->used_fp_list = NULL;
   mp->maxsz = max_size;
   mp->rdmflg = (randomflg != 0) ? 1 : 0;
   mp->cursor = 0;

   if (max_size == 0)
   {
      mp->storage = NULL;
      return 0;
   }

   mp->storage = (BYTE *)malloc(max_size * sizeof(BYTE));
   if (mp->storage == NULL)
      return -1;
   memset(mp->storage, 0, max_size * sizeof(BYTE));

#ifdef MM64
   MEMPHY_format(mp, PAGING64_PAGESZ);
#else
   MEMPHY_format(mp, PAGING_PAGESZ);
#endif

   return 0;
}

int find_victim_page(struct mm_struct *mm, addr_t *pgn)
{
   if (mm == NULL || pgn == NULL || mm->fifo_pgn == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   if (mm->fifo_pgn->pg_next == NULL)
   {
      *pgn = mm->fifo_pgn->pgn;
      free(mm->fifo_pgn);
      mm->fifo_pgn = NULL;
      pthread_mutex_unlock(&mem_lock);
      return 0;
   }

   struct pgn_t *prev = mm->fifo_pgn;
   struct pgn_t *curr = prev->pg_next;
   while (curr->pg_next != NULL)
   {
      prev = curr;
      curr = curr->pg_next;
   }
   *pgn = curr->pgn;
   prev->pg_next = NULL;
   free(curr);

   pthread_mutex_unlock(&mem_lock);
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

int MEMPHY_get_freefp_with_swap(struct pcb_t *caller, addr_t *retfpn)
{
   if (caller == NULL || retfpn == NULL || caller->krnl == NULL)
      return -1;

   struct krnl_t *krnl = caller->krnl;
   struct memphy_struct *mram = krnl->mram;
   if (mram == NULL)
      return -1;

   if (MEMPHY_get_freefp(mram, retfpn) == 0)
      return 0;

   if (caller->mm == NULL || krnl->mswp == NULL)
      return -1;

   addr_t victim_pgn = 0;
   addr_t victim_pte = 0;
   int found_victim = 0;
   while (find_victim_page(caller->mm, &victim_pgn) == 0)
   {
      victim_pte = pte_get_entry(caller->mm, victim_pgn, caller);
      if (PAGING_PAGE_PRESENT(victim_pte))
      {
         found_victim = 1;
         break;
      }
   }
   if (!found_victim)
      return -1;

   addr_t victim_fpn = PAGING_PTE_FPN(victim_pte);

   int swp_id = -1;
   addr_t swp_fpn = 0;
   for (int i = 0; i < PAGING_MAX_MMSWP; i++)
   {
      if (krnl->mswp[i] != NULL && krnl->mswp[i]->storage != NULL &&
          MEMPHY_get_freefp(krnl->mswp[i], &swp_fpn) == 0)
      {
         swp_id = i;
         break;
      }
   }

   if (swp_id == -1)
   {
      enlist_pgn_node(&caller->mm->fifo_pgn, victim_pgn);
      return -1;
   }

   if (__swap_cp_page(mram, victim_fpn, krnl->mswp[swp_id], swp_fpn) != 0)
   {
      MEMPHY_put_freefp(krnl->mswp[swp_id], swp_fpn);
      enlist_pgn_node(&caller->mm->fifo_pgn, victim_pgn);
      return -1;
   }

   if (pte_set_swap(caller, victim_pgn, swp_id, swp_fpn) != 0)
   {
      MEMPHY_put_freefp(krnl->mswp[swp_id], swp_fpn);
      enlist_pgn_node(&caller->mm->fifo_pgn, victim_pgn);
      return -1;
   }

   pthread_mutex_lock(&mem_lock);
   swap_out_count++;
   pthread_mutex_unlock(&mem_lock);

   *retfpn = victim_fpn;
   return 0;
}

void MEMPHY_print_stats(void)
{
   pthread_mutex_lock(&mem_lock);

   unsigned long total = mem_read_count + mem_write_count;
   double rate = total ? ((double)swap_out_count / total) * 100.0 : 0.0;

   printf("[MEMPHY] reads=%lu writes=%lu swap_outs=%lu fault_rate=%.2f%%\n",
          mem_read_count, mem_write_count, swap_out_count, rate);

   pthread_mutex_unlock(&mem_lock);
}

int MEMPHY_cleanup(struct memphy_struct *mp)
{
   if (mp == NULL)
      return -1;

   pthread_mutex_lock(&mem_lock);

   struct framephy_struct *curr = mp->free_fp_list;
   while (curr != NULL)
   {
      struct framephy_struct *next = curr->fp_next;
      free(curr);
      curr = next;
   }
   mp->free_fp_list = NULL;

   curr = mp->used_fp_list;
   while (curr != NULL)
   {
      struct framephy_struct *next = curr->fp_next;
      free(curr);
      curr = next;
   }
   mp->used_fp_list = NULL;

   if (mp->storage != NULL)
   {
      free(mp->storage);
      mp->storage = NULL;
   }

   pthread_mutex_unlock(&mem_lock);
   return 0;
}