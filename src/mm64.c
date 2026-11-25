/*
 * Copyright (C) 2026 pdnguyen of HCMC University of Technology VNU-HCM
 */

/* LamiaAtrium release
 * Source Code License Grant: The authors hereby grant to Licensee
 * personal permission to use and modify the Licensed Source Code
 * for the sole purpose of studying while attending the course CO2018.
 */

/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm64.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(MM64)

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(addr_t *pte,
             int pre,    // present
             addr_t fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             addr_t swpoff) // swap offset
{
  if (pre != 0) {
    if (swp == 0) { // Non swap ~ page online

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}


/*
 * get_pd_from_pagenum - Parse address to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_address(addr_t addr, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Extract page direactories */
	*pgd = (addr&PAGING64_ADDR_PGD_MASK)>>PAGING64_ADDR_PGD_LOBIT;
	*p4d = (addr&PAGING64_ADDR_P4D_MASK)>>PAGING64_ADDR_P4D_LOBIT;
	*pud = (addr&PAGING64_ADDR_PUD_MASK)>>PAGING64_ADDR_PUD_LOBIT;
	*pmd = (addr&PAGING64_ADDR_PMD_MASK)>>PAGING64_ADDR_PMD_LOBIT;
	*pt = (addr&PAGING64_ADDR_PT_MASK)>>PAGING64_ADDR_PT_LOBIT;

	/* TODO: implement the page direactories mapping */
  if (((addr >> (PAGING64_ADDR_PGD_HIBIT + 1)) != 0) &&
      ((addr >> (PAGING64_ADDR_PGD_HIBIT + 1)) !=
  (((addr_t)1 << (64 - PAGING64_ADDR_PGD_HIBIT - 1)) - 1)))
    return -1;
if (*pgd >= PAGING64_DIR_ENTRIES || *p4d >= PAGING64_DIR_ENTRIES || *pud >= PAGING64_DIR_ENTRIES || *pmd >= PAGING64_DIR_ENTRIES || *pt >= PAGING64_DIR_ENTRIES)
    return -1;

	return 0;
}

/*
 * get_pd_from_pagenum - Parse page number to 5 page directory level
 * @pgn   : pagenumer
 * @pgd   : page global directory
 * @p4d   : page level directory
 * @pud   : page upper directory
 * @pmd   : page middle directory
 * @pt    : page table 
 */
int get_pd_from_pagenum(addr_t pgn, addr_t* pgd, addr_t* p4d, addr_t* pud, addr_t* pmd, addr_t* pt)
{
	/* Shift the address to get page num and perform the mapping*/
	return get_pd_from_address(pgn << PAGING64_ADDR_PT_SHIFT,
                         pgd,p4d,pud,pmd,pt);
}


/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(struct pcb_t *caller, addr_t pgn, int swptyp, addr_t swpoff)
{
  if (caller == NULL || caller->mm == NULL)
    return -1;

  struct mm_struct *mm = caller->mm;

  addr_t *pte = NULL;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

#ifdef MM64	
  addr_t *p4d_table;
  addr_t *pud_table;
  addr_t *pmd_table;
  addr_t *pt_table;
  if (mm == NULL || get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt) != 0)
    return -1;
  if (mm->pgd == NULL) mm->pgd = calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  if (mm->pgd == NULL)
    return -1;
  if (mm->pgd[pgd] == 0) mm->pgd[pgd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  p4d_table = (addr_t *)mm->pgd[pgd];
  if (p4d_table == NULL) return -1;
  if (p4d_table[p4d] == 0) p4d_table[p4d] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pud_table = (addr_t *)p4d_table[p4d];
  if (pud_table == NULL) return -1;
  if (pud_table[pud] == 0) pud_table[pud] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pmd_table = (addr_t *)pud_table[pud];
  if (pmd_table == NULL) return -1;
  if (pmd_table[pmd] == 0) pmd_table[pmd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pt_table = (addr_t *)pmd_table[pmd];
  if (pt_table == NULL) return -1;
  pte = &pt_table[pt];
#else
  struct krnl_t *krnl = (caller->pid > 0 && caller->pid < 100000) ? caller->krnl : NULL;
  if (krnl == NULL || krnl->mm == NULL) return -1;
  pte = &krnl->mm->pgd[pgn];
#endif
	
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 * @caller: caller
 */
int pte_set_fpn(struct mm_struct *mm, addr_t pgn, addr_t fpn, struct pcb_t *caller)
{
  if (mm == NULL && caller != NULL)
    mm = caller->mm;
  if (mm == NULL)
    return -1;

  addr_t *pte = NULL;
  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

#ifdef MM64	
  addr_t *p4d_table;
  addr_t *pud_table;
  addr_t *pmd_table;
  addr_t *pt_table;
  if (get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt) != 0)
    return -1;
  if (mm->pgd == NULL) mm->pgd = calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  if (mm->pgd == NULL)
    return -1;
  if (mm->pgd[pgd] == 0) mm->pgd[pgd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  p4d_table = (addr_t *)mm->pgd[pgd];
  if (p4d_table == NULL) return -1;
  if (p4d_table[p4d] == 0) p4d_table[p4d] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pud_table = (addr_t *)p4d_table[p4d];
  if (pud_table == NULL) return -1;
  if (pud_table[pud] == 0) pud_table[pud] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pmd_table = (addr_t *)pud_table[pud];
  if (pmd_table == NULL) return -1;
  if (pmd_table[pmd] == 0) pmd_table[pmd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pt_table = (addr_t *)pmd_table[pmd];
  if (pt_table == NULL) return -1;
  pte = &pt_table[pt];
#else
  struct krnl_t *krnl = caller != NULL ? caller->krnl : NULL;
  if (krnl == NULL || krnl->mm == NULL) return -1;
  pte = &krnl->mm->pgd[pgn];
#endif

  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}


/* Get PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @caller: caller
 * @ret    : page table entry
 **/
addr_t pte_get_entry(struct mm_struct *mm, addr_t pgn, struct pcb_t *caller)
{
  if (mm == NULL && caller != NULL)
    mm = caller->mm;
  if (mm == NULL)
    return 0;

  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

#ifdef MM64	
  addr_t *p4d_table;
  addr_t *pud_table;
  addr_t *pmd_table;
  addr_t *pt_table;
  if (get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt) != 0 || mm->pgd == NULL)
    return 0;
  if (mm->pgd[pgd] == 0) return 0;
  p4d_table = (addr_t *)mm->pgd[pgd];
  if (p4d_table == NULL || p4d_table[p4d] == 0) return 0;
  pud_table = (addr_t *)p4d_table[p4d];
  if (pud_table == NULL || pud_table[pud] == 0) return 0;
  pmd_table = (addr_t *)pud_table[pud];
  if (pmd_table == NULL || pmd_table[pmd] == 0) return 0;
  pt_table = (addr_t *)pmd_table[pmd];
  if (pt_table == NULL) return 0;
  return pt_table[pt];
#else	
  struct krnl_t *krnl = caller != NULL ? caller->krnl : NULL;
  if (krnl == NULL || krnl->mm == NULL)
    return 0;
  return krnl->mm->pgd[pgn];
#endif
}

/* Set PTE page table entry
 * @caller : caller
 * @pgn    : page number
 * @ret    : page table entry
 **/
int pte_set_entry(struct mm_struct *mm, addr_t pgn, uint64_t pte_val, struct pcb_t *caller)
{
  if (mm == NULL && caller != NULL)
    mm = caller->mm;
  if (mm == NULL)
    return -1;

  addr_t pgd=0;
  addr_t p4d=0;
  addr_t pud=0;
  addr_t pmd=0;
  addr_t pt=0;

#ifdef MM64	
  addr_t *p4d_table;
  addr_t *pud_table;
  addr_t *pmd_table;
  addr_t *pt_table;
  if (get_pd_from_pagenum(pgn, &pgd, &p4d, &pud, &pmd, &pt) != 0)
    return -1;
  if (mm->pgd == NULL) mm->pgd = calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  if (mm->pgd == NULL)
    return -1;
  if (mm->pgd[pgd] == 0) mm->pgd[pgd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  p4d_table = (addr_t *)mm->pgd[pgd];
  if (p4d_table == NULL) return -1;
  if (p4d_table[p4d] == 0) p4d_table[p4d] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pud_table = (addr_t *)p4d_table[p4d];
  if (pud_table == NULL) return -1;
  if (pud_table[pud] == 0) pud_table[pud] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pmd_table = (addr_t *)pud_table[pud];
  if (pmd_table == NULL) return -1;
  if (pmd_table[pmd] == 0) pmd_table[pmd] = (addr_t)calloc(PAGING64_DIR_ENTRIES, sizeof(addr_t));
  pt_table = (addr_t *)pmd_table[pmd];
  if (pt_table == NULL) return -1;
  pt_table[pt] = (addr_t)pte_val;
#else	
  struct krnl_t *krnl = caller != NULL ? caller->krnl : NULL;
  if (krnl == NULL || krnl->mm == NULL)
    return -1;
  krnl->mm->pgd[pgn] = (addr_t)pte_val;
#endif

  return 0;
}

/*
 * handle_page_fault - Xử lý sự kiện thiếu trang:
 * Khi bit Present == 0 và bit Swapped == 1, nạp trang từ SWAP trở lại RAM và cập nhật lại mục bảng trang tương ứng
 * @caller: PCB của tiến trình gọi
 * @pgn: số hiệu trang ảo bị thiếu
 * Return: 0 nếu thành công, -1 nếu thất bại
 */
int handle_page_fault(struct pcb_t *caller, addr_t pgn)
{
  if (caller == NULL || caller->mm == NULL || caller->krnl == NULL ||
      caller->krnl->mram == NULL || caller->krnl->mswp == NULL)
    return -1;

  addr_t pte = pte_get_entry(caller->mm, pgn, caller);

  /* Chỉ kích hoạt khi bit Present == 0 và bit Swapped == 1 */
  if (PAGING_PAGE_PRESENT(pte) || !(pte & PAGING_PTE_SWAPPED_MASK))
    return -1;

  int swptyp = PAGING_PTE_SWPTYP(pte);
  addr_t swpoff = PAGING_PTE_SWP(pte);

  if (swptyp < 0 || swptyp >= PAGING_MAX_MMSWP ||
      caller->krnl->mswp[swptyp] == NULL ||
      caller->krnl->mswp[swptyp]->storage == NULL)
    return -1;

  addr_t ram_fpn;
  /* Xin khung trang RAM (tự động hoán trang FIFO sang SWAP nếu RAM đầy) */
  if (MEMPHY_get_freefp_with_swap(caller, &ram_fpn) != 0)
    return -1;

  /* Sao chép dữ liệu từ thiết bị SWAP vào khung RAM */
  if (__swap_cp_page(caller->krnl->mswp[swptyp], swpoff, caller->krnl->mram, ram_fpn) != 0)
  {
    MEMPHY_put_freefp(caller->krnl->mram, ram_fpn);
    return -1;
  }

  /* Giải phóng khung trang cũ trên thiết bị SWAP */
  MEMPHY_put_freefp(caller->krnl->mswp[swptyp], swpoff);

  /* Cập nhật lại mục bảng trang: Present = 1, Swapped = 0, lưu FPN */
  pte_set_fpn(caller->mm, pgn, ram_fpn, caller);

  /* Đưa trang mới nạp vào danh sách FIFO */
  enlist_pgn_node(&caller->mm->fifo_pgn, pgn);

  return 0;
}

/*
 * translate_address - Tra cứu bảng phân trang 5 cấp từ địa chỉ ảo sang địa chỉ khung trang vật lý FPN kết hợp offset
 * @caller: PCB của tiến trình gọi
 * @vaddr: địa chỉ ảo 64-bit
 * Return: địa chỉ vật lý nếu thành công, (addr_t)-1 nếu lỗi
 */
addr_t translate_address(struct pcb_t *caller, addr_t vaddr)
{
  if (caller == NULL || caller->mm == NULL)
    return (addr_t)-1;

  addr_t pgd=0, p4d=0, pud=0, pmd=0, pt=0;
  if (get_pd_from_address(vaddr, &pgd, &p4d, &pud, &pmd, &pt) != 0)
    return (addr_t)-1;

  addr_t offset = vaddr & PAGING64_ADDR_OFFST_MASK;
  addr_t pgn = vaddr >> PAGING64_ADDR_PT_SHIFT;

  addr_t pte = pte_get_entry(caller->mm, pgn, caller);

  /* Trường hợp 1: Trang đã có trong RAM (Present == 1) */
  if (PAGING_PAGE_PRESENT(pte))
  {
    addr_t fpn = PAGING_PTE_FPN(pte);
    return (fpn << PAGING64_ADDR_PT_SHIFT) | offset;
  }

  /* Trường hợp 2: Thiếu trang (Present == 0 và Swapped == 1) -> Kích hoạt nạp từ SWAP */
  if (pte & PAGING_PTE_SWAPPED_MASK)
  {
    if (handle_page_fault(caller, pgn) != 0)
      return (addr_t)-1;

    pte = pte_get_entry(caller->mm, pgn, caller);
    if (!PAGING_PAGE_PRESENT(pte))
      return (addr_t)-1;

    addr_t fpn = PAGING_PTE_FPN(pte);
    return (fpn << PAGING64_ADDR_PT_SHIFT) | offset;
  }

  /* Trường hợp 3: Trang chưa ánh xạ */
  return (addr_t)-1;
}

static void remove_pgn_node(struct pgn_t **list, addr_t pgn)
{
  if (list == NULL || *list == NULL)
    return;

  struct pgn_t *curr = *list;
  struct pgn_t *prev = NULL;

  while (curr != NULL)
  {
    if (curr->pgn == pgn)
      {
        if (prev == NULL)
          *list = curr->pg_next;
        else
          prev->pg_next = curr->pg_next;
        free(curr);
        return;
      }
    prev = curr;
    curr = curr->pg_next;
  }
}

/*
 * vunmap_page_range - Xóa các mục trong bảng phân trang và trả lại khung trang vật lý khi tiến trình gọi giải phóng bộ nhớ
 * @caller: PCB của tiến trình gọi
 * @start: địa chỉ bắt đầu của vùng nhớ
 * @end: địa chỉ kết thúc của vùng nhớ (hoặc số lượng trang pgnum nếu end < start)
 * Return: 0 nếu thành công, -1 nếu lỗi
 */
int vunmap_page_range(struct pcb_t *caller, addr_t start, addr_t end)
{
  if (caller == NULL || caller->mm == NULL || caller->krnl == NULL ||
      caller->krnl->mram == NULL || end <= start)
    return -1;

  addr_t first_pgn = start >> PAGING64_ADDR_PT_SHIFT;
  addr_t last_pgn = (end - 1) >> PAGING64_ADDR_PT_SHIFT;

  for (addr_t pgn = first_pgn; pgn <= last_pgn; pgn++)
  {
    addr_t pte = pte_get_entry(caller->mm, pgn, caller);
    if (!PAGING_PAGE_PRESENT(pte) && !(pte & PAGING_PTE_SWAPPED_MASK))
      continue;

    /* Trả lại khung trang RAM nếu trang đang trực tuyến */
    if (PAGING_PAGE_PRESENT(pte))
    {
      addr_t fpn = PAGING_PTE_FPN(pte);
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
    }
    /* Trả lại khung trang SWAP nếu trang đang nằm trên thiết bị SWAP */
    else if (pte & PAGING_PTE_SWAPPED_MASK)
    {
      int swptyp = PAGING_PTE_SWPTYP(pte);
      addr_t swpoff = PAGING_PTE_SWP(pte);
      if (caller->krnl->mswp != NULL &&
          swptyp >= 0 && swptyp < PAGING_MAX_MMSWP &&
          caller->krnl->mswp[swptyp] != NULL &&
          caller->krnl->mswp[swptyp]->storage != NULL)
      {
        MEMPHY_put_freefp(caller->krnl->mswp[swptyp], swpoff);
      }
    }

    /* Xóa mục bảng phân trang */
    pte_set_entry(caller->mm, pgn, 0, caller);

    /* Loại bỏ trang khỏi danh sách quản lý FIFO */
    remove_pgn_node(&caller->mm->fifo_pgn, pgn);
  }

  return 0;
}

/*
 * vmap_pgd_memset - map a range of page at aligned address
 */
int vmap_pgd_memset(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz
                    int pgnum)                      // num of mapping page
{
  if (caller == NULL || caller->mm == NULL || pgnum <= 0)
    return -1;

  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;
  uint64_t pattern = 0xdeadbeef;

  for (int pgit = 0; pgit < pgnum; pgit++)
  {
    if (pte_set_entry(caller->mm, pgn + pgit, pattern, caller) != 0)
      return -1;
  }

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
addr_t vmap_page_range(struct pcb_t *caller,           // process call
                    addr_t addr,                       // start address which is aligned to pagesz // start
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  struct framephy_struct *fpit = frames;
  addr_t pgn = addr >> PAGING64_ADDR_PT_SHIFT;
  int pgit = 0;

  if (caller == NULL || caller->mm == NULL || pgnum <= 0)
    return (addr_t)-1;

  if (ret_rg != NULL)
  {
    ret_rg->rg_start = addr;
    ret_rg->rg_end = addr + (addr_t)pgnum * PAGING64_PAGESZ;
    ret_rg->rg_next = NULL;
  }

  while (fpit != NULL && pgit < pgnum)
  {
    pte_set_fpn(caller->mm, pgn + pgit, fpit->fpn, caller);
    enlist_pgn_node(&caller->mm->fifo_pgn, pgn + pgit);
    fpit = fpit->fp_next;
    pgit++;
  }

  return addr;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */

addr_t alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  addr_t fpn;
  int pgit;
  struct framephy_struct *newfp_str = NULL;
  struct framephy_struct *cur = NULL;

  if (caller == NULL || caller->krnl == NULL || caller->krnl->mram == NULL || frm_lst == NULL || req_pgnum <= 0)
    return -1;

  *frm_lst = NULL;

  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    if (MEMPHY_get_freefp_with_swap(caller, &fpn) != 0)
    {
      while (*frm_lst != NULL)
      {
        struct framephy_struct *tmp = *frm_lst;
        *frm_lst = tmp->fp_next;
        MEMPHY_put_freefp(caller->krnl->mram, tmp->fpn);
        free(tmp);
      }
      return (addr_t)-3000;
    }

    newfp_str = malloc(sizeof(struct framephy_struct));
    if (newfp_str == NULL)
    {
      MEMPHY_put_freefp(caller->krnl->mram, fpn);
      while (*frm_lst != NULL)
      {
        struct framephy_struct *tmp = *frm_lst;
        *frm_lst = tmp->fp_next;
        MEMPHY_put_freefp(caller->krnl->mram, tmp->fpn);
        free(tmp);
      }
      return -1;
    }

    newfp_str->fpn = fpn;
    newfp_str->fp_next = NULL;
    newfp_str->owner = caller->mm;

    if (*frm_lst == NULL)
      *frm_lst = newfp_str;
    else
      cur->fp_next = newfp_str;

    cur = newfp_str;
  }

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
addr_t vm_map_ram(struct pcb_t *caller, addr_t astart, addr_t aend, addr_t mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  addr_t ret_alloc = 0;

  if (caller == NULL || incpgnum <= 0)
    return (addr_t)-1;

  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  if (ret_alloc == (addr_t)-1 || ret_alloc == (addr_t)-3000)
    return (addr_t)-1;

  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  while (frm_lst != NULL)
  {
    struct framephy_struct *tmp = frm_lst;
    frm_lst = frm_lst->fp_next;
    free(tmp);
  }

  return 0;
}

/*
 *Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma0 = malloc(sizeof(struct vm_area_struct));
  if (vma0 == NULL)
    return -1;

  mm->pgd = NULL;
  mm->p4d = NULL;
  mm->pud = NULL;
  mm->pmd = NULL;
  mm->pt = NULL;
  mm->fifo_pgn = NULL;
  mm->kcpooltbl = NULL;
  memset(mm->symrgtbl, 0, sizeof(mm->symrgtbl));

  /* By default the owner comes with at least one vma */
  vma0->vm_id = 0;
  vma0->vm_start = 0;
  vma0->vm_end = vma0->vm_start;
  vma0->sbrk = vma0->vm_start;
  vma0->vm_freerg_list = NULL;
  struct vm_rg_struct *first_rg = init_vm_rg(vma0->vm_start, vma0->vm_end);
  if (first_rg == NULL)
  {
    free(vma0);
    return -1;
  }
  enlist_vm_rg_node(&vma0->vm_freerg_list, first_rg);

  vma0->vm_next = NULL;
  vma0->vm_mm = mm;
  mm->mmap = vma0;

  return 0;
}

struct vm_rg_struct *init_vm_rg(addr_t rg_start, addr_t rg_end)
{
  struct vm_rg_struct *rgnode = malloc(sizeof(struct vm_rg_struct));

  rgnode->rg_start = rg_start;
  rgnode->rg_end = rg_end;
  rgnode->rg_next = NULL;

  return rgnode;
}

int enlist_vm_rg_node(struct vm_rg_struct **rglist, struct vm_rg_struct *rgnode)
{
  rgnode->rg_next = *rglist;
  *rglist = rgnode;

  return 0;
}

int enlist_pgn_node(struct pgn_t **plist, addr_t pgn)
{
  if (plist == NULL)
    return -1;

  struct pgn_t *curr = *plist;
  while (curr != NULL)
  {
    if (curr->pgn == pgn)
      return 0;
    curr = curr->pg_next;
  }

  struct pgn_t *pnode = malloc(sizeof(struct pgn_t));
  if (pnode == NULL)
    return -1;

  pnode->pgn = pgn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

int print_list_fp(struct framephy_struct *ifp)
{
  struct framephy_struct *fp = ifp;

  printf("print_list_fp: ");
  if (fp == NULL) { printf("NULL list\n"); return -1;}
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[" FORMAT_ADDR "]\n", fp->fpn);
    fp = fp->fp_next;
  }
  printf("\n");
  return 0;
}

int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[" FORMAT_ADDR "->"  FORMAT_ADDR "]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[" FORMAT_ADDR "->" FORMAT_ADDR "]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

int print_list_pgn(struct pgn_t *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL) { printf("NULL list\n"); return -1; }
  printf("\n");
  while (ip != NULL)
  {
    printf("va[" FORMAT_ADDR "]-\n", ip->pgn);
    ip = ip->pg_next;
  }
  printf("\n");
  return 0;
}

int print_pgtbl(struct pcb_t *caller, addr_t start, addr_t end)
{
  if (caller == NULL || caller->mm == NULL || caller->mm->pgd == NULL)
    return -1;

  printf("=== 5-Level Page Table Dump for PID %d ===\n", caller->pid);

  addr_t *pgd_tbl = (addr_t *)caller->mm->pgd;
  for (int i5 = 0; i5 < PAGING64_DIR_ENTRIES; i5++)
  {
    if (pgd_tbl[i5] == 0) continue;
    addr_t *p4d_tbl = (addr_t *)pgd_tbl[i5];
    for (int i4 = 0; i4 < PAGING64_DIR_ENTRIES; i4++)
    {
      if (p4d_tbl[i4] == 0) continue;
      addr_t *pud_tbl = (addr_t *)p4d_tbl[i4];
      for (int i3 = 0; i3 < PAGING64_DIR_ENTRIES; i3++)
      {
        if (pud_tbl[i3] == 0) continue;
        addr_t *pmd_tbl = (addr_t *)pud_tbl[i3];
        for (int i2 = 0; i2 < PAGING64_DIR_ENTRIES; i2++)
        {
          if (pmd_tbl[i2] == 0) continue;
          addr_t *pt_tbl = (addr_t *)pmd_tbl[i2];
          for (int i1 = 0; i1 < PAGING64_DIR_ENTRIES; i1++)
          {
            addr_t pte = pt_tbl[i1];
            if (pte == 0) continue;
            addr_t pgn = ((addr_t)i5 << (PAGING64_ADDR_PGD_LOBIT - PAGING64_ADDR_PT_SHIFT)) |
                         ((addr_t)i4 << (PAGING64_ADDR_P4D_LOBIT - PAGING64_ADDR_PT_SHIFT)) |
                         ((addr_t)i3 << (PAGING64_ADDR_PUD_LOBIT - PAGING64_ADDR_PT_SHIFT)) |
                         ((addr_t)i2 << (PAGING64_ADDR_PMD_LOBIT - PAGING64_ADDR_PT_SHIFT)) |
                         (addr_t)i1;
            addr_t va = pgn << PAGING64_ADDR_PT_SHIFT;
            if (PAGING_PAGE_PRESENT(pte))
            {
              addr_t fpn = PAGING_PTE_FPN(pte);
              printf("  VA: 0x%016lx -> PGN: 0x%lx, FPN: 0x%lx (RAM)\n", (unsigned long)va, (unsigned long)pgn, (unsigned long)fpn);
            }
            else if (pte & PAGING_PTE_SWAPPED_MASK)
            {
              int swptyp = PAGING_PTE_SWPTYP(pte);
              addr_t swpoff = PAGING_PTE_SWP(pte);
              printf("  VA: 0x%016lx -> PGN: 0x%lx, SWP: dev=%d, off=0x%lx\n", (unsigned long)va, (unsigned long)pgn, swptyp, (unsigned long)swpoff);
            }
          }
        }
      }
    }
  }
  printf("==========================================\n");
  return 0;
}

/*
 * print_pgtbl_stats - Thống kê số lượng bảng phân trang 5 cấp và chi phí RAM tiêu tốn
 * @caller: PCB của tiến trình cần thống kê
 * Return: 0 nếu thành công, -1 nếu lỗi
 */
int print_pgtbl_stats(struct pcb_t *caller)
{
  if (caller == NULL || caller->mm == NULL || caller->mm->pgd == NULL)
    return -1;

  unsigned long pgd_cnt = 1;
  unsigned long p4d_cnt = 0;
  unsigned long pud_cnt = 0;
  unsigned long pmd_cnt = 0;
  unsigned long pt_cnt = 0;
  unsigned long ram_pages = 0;
  unsigned long swp_pages = 0;

  addr_t *pgd_tbl = (addr_t *)caller->mm->pgd;
  for (int i5 = 0; i5 < PAGING64_DIR_ENTRIES; i5++)
  {
    if (pgd_tbl[i5] == 0) continue;
    p4d_cnt++;
    addr_t *p4d_tbl = (addr_t *)pgd_tbl[i5];
    for (int i4 = 0; i4 < PAGING64_DIR_ENTRIES; i4++)
    {
      if (p4d_tbl[i4] == 0) continue;
      pud_cnt++;
      addr_t *pud_tbl = (addr_t *)p4d_tbl[i4];
      for (int i3 = 0; i3 < PAGING64_DIR_ENTRIES; i3++)
      {
        if (pud_tbl[i3] == 0) continue;
        pmd_cnt++;
        addr_t *pmd_tbl = (addr_t *)pud_tbl[i3];
        for (int i2 = 0; i2 < PAGING64_DIR_ENTRIES; i2++)
        {
          if (pmd_tbl[i2] == 0) continue;
          pt_cnt++;
          addr_t *pt_tbl = (addr_t *)pmd_tbl[i2];
          for (int i1 = 0; i1 < PAGING64_DIR_ENTRIES; i1++)
          {
            addr_t pte = pt_tbl[i1];
            if (pte == 0) continue;
            if (PAGING_PAGE_PRESENT(pte))
              ram_pages++;
            else if (pte & PAGING_PTE_SWAPPED_MASK)
              swp_pages++;
          }
        }
      }
    }
  }

  unsigned long total_tables = pgd_cnt + p4d_cnt + pud_cnt + pmd_cnt + pt_cnt;
  unsigned long long single_table_bytes = (unsigned long long)PAGING64_DIR_ENTRIES * sizeof(addr_t);
  unsigned long long total_bytes = (unsigned long long)total_tables * single_table_bytes;
  unsigned long long flat_1level_bytes =
      (1ULL << (PAGING64_CPU_BUS_WIDTH - PAGING64_ADDR_PT_SHIFT)) * sizeof(addr_t);

  printf("=== 5-Level Page Table Stats for PID %d ===\n", caller->pid);
  printf("  Tables allocated: PGD=%lu, P4D=%lu, PUD=%lu, PMD=%lu, PT=%lu (Total=%lu)\n",
         pgd_cnt, p4d_cnt, pud_cnt, pmd_cnt, pt_cnt, total_tables);
  printf("  Mapped pages    : RAM=%lu, SWAP=%lu (Total=%lu)\n",
         ram_pages, swp_pages, ram_pages + swp_pages);
  printf("  5-level RAM cost: %llu bytes (%llu KB)\n",
         total_bytes, total_bytes / 1024ULL);
  printf("  Flat 1-level (57-bit VA): %llu GB\n",
         flat_1level_bytes / (1024ULL * 1024ULL * 1024ULL));
  printf("===========================================\n");
  return 0;
}

/*
 * cleanup_mm - Giải phóng toàn bộ bảng phân trang 5 cấp, danh sách FIFO và VMA của tiến trình
 * @mm: con trỏ cấu trúc quản lý bộ nhớ của tiến trình
 * Return: 0 nếu thành công, -1 nếu lỗi
 */
int cleanup_mm(struct mm_struct *mm)
{
  if (mm == NULL)
    return -1;

  if (mm->pgd != NULL)
  {
    addr_t *pgd_tbl = (addr_t *)mm->pgd;
    for (int i5 = 0; i5 < PAGING64_DIR_ENTRIES; i5++)
    {
      if (pgd_tbl[i5] == 0) continue;
      addr_t *p4d_tbl = (addr_t *)pgd_tbl[i5];
      for (int i4 = 0; i4 < PAGING64_DIR_ENTRIES; i4++)
      {
        if (p4d_tbl[i4] == 0) continue;
        addr_t *pud_tbl = (addr_t *)p4d_tbl[i4];
        for (int i3 = 0; i3 < PAGING64_DIR_ENTRIES; i3++)
        {
          if (pud_tbl[i3] == 0) continue;
          addr_t *pmd_tbl = (addr_t *)pud_tbl[i3];
          for (int i2 = 0; i2 < PAGING64_DIR_ENTRIES; i2++)
          {
            if (pmd_tbl[i2] == 0) continue;
            free((void *)pmd_tbl[i2]);
          }
          free(pmd_tbl);
        }
        free(pud_tbl);
      }
      free(p4d_tbl);
    }
    free(pgd_tbl);
    mm->pgd = NULL;
  }

  struct pgn_t *pgn = mm->fifo_pgn;
  while (pgn != NULL)
  {
    struct pgn_t *next = pgn->pg_next;
    free(pgn);
    pgn = next;
  }
  mm->fifo_pgn = NULL;

  struct vm_area_struct *vma = mm->mmap;
  while (vma != NULL)
  {
    struct vm_rg_struct *rg = vma->vm_freerg_list;
    while (rg != NULL)
    {
      struct vm_rg_struct *next_rg = rg->rg_next;
      free(rg);
      rg = next_rg;
    }
    struct vm_area_struct *next_vma = vma->vm_next;
    free(vma);
    vma = next_vma;
  }
  mm->mmap = NULL;

  return 0;
}

#endif  //def MM64
