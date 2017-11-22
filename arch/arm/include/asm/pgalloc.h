/*
 *  arch/arm/include/asm/pgalloc.h
 *
 *  Copyright (C) 2000-2001 Russell King
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _ASMARM_PGALLOC_H
#define _ASMARM_PGALLOC_H

#include <linux/pagemap.h>

#include <asm/domain.h>
#include <asm/pgtable-hwdef.h>
#include <asm/processor.h>
#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#define check_pgt_cache()		do { } while (0)

#ifdef CONFIG_MMU

#define _PAGE_USER_TABLE	(PMD_TYPE_TABLE | PMD_BIT4 | PMD_DOMAIN(DOMAIN_USER))
#define _PAGE_KERNEL_TABLE	(PMD_TYPE_TABLE | PMD_BIT4 | PMD_DOMAIN(DOMAIN_KERNEL))

#ifdef CONFIG_ARM_LPAE

static inline pmd_t *pmd_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	return (pmd_t *)get_zeroed_page(GFP_KERNEL | __GFP_REPEAT);
}

static inline void pmd_free(struct mm_struct *mm, pmd_t *pmd)
{
	BUG_ON((unsigned long)pmd & (PAGE_SIZE-1));
	free_page((unsigned long)pmd);
}

static inline void pud_populate(struct mm_struct *mm, pud_t *pud, pmd_t *pmd)
{
	set_pud(pud, __pud(__pa(pmd) | PMD_TYPE_TABLE));
}

#else	/* !CONFIG_ARM_LPAE */

/*
 * Since we have only two-level page tables, these are trivial
 */
#define pmd_alloc_one(mm,addr)		({ BUG(); ((pmd_t *)2); })
#define pmd_free(mm, pmd)		do { } while (0)
#define pud_populate(mm,pmd,pte)	BUG()

#endif	/* CONFIG_ARM_LPAE */

extern pgd_t *pgd_alloc(struct mm_struct *mm);
extern void pgd_free(struct mm_struct *mm, pgd_t *pgd);

#define PGALLOC_GFP	(GFP_KERNEL | __GFP_NOTRACK | __GFP_REPEAT | __GFP_ZERO)

static inline void clean_pte_table(pte_t *pte)
{
	clean_dcache_area(pte + PTE_HWTABLE_PTRS, PTE_HWTABLE_SIZE);
}

/*
 * Allocate one PTE table.
 *
 * This actually allocates two hardware PTE tables, but we wrap this up
 * into one table thus:
 *
 *  +------------+
 *  | Linux pt 0 |
 *  +------------+
 *  | Linux pt 1 |
 *  +------------+
 *  |  h/w pt 0  |
 *  +------------+
 *  |  h/w pt 1  |
 *  +------------+
 */
static inline pte_t *
pte_alloc_one_kernel(struct mm_struct *mm, unsigned long addr)
{
	pte_t *pte;

	pte = (pte_t *)__get_free_page(PGALLOC_GFP);
#if !defined(CONFIG_CPU_CACHE_V7) || !defined(CONFIG_SMP)
	if (pte)
		clean_pte_table(pte);
#endif

	return pte;
}

static inline pgtable_t
pte_alloc_one(struct mm_struct *mm, unsigned long addr)
{
	struct page *pte;

#ifdef CONFIG_HIGHPTE
	pte = alloc_pages(PGALLOC_GFP | __GFP_HIGHMEM, 0);
#else
	pte = alloc_pages(PGALLOC_GFP, 0);
#endif
	if (!pte)
		return NULL;
#if !defined(CONFIG_CPU_CACHE_V7) || !defined(CONFIG_SMP)
		if (!PageHighMem(pte))
			clean_pte_table(page_address(pte));
#endif
	if (!pgtable_page_ctor(pte)) {
		__free_page(pte);
		return NULL;
	}

	return pte;
}

/*
 * Free one PTE table.
 */
static inline void pte_free_kernel(struct mm_struct *mm, pte_t *pte)
{
	if (pte)
		free_page((unsigned long)pte);
}

static inline void pte_free(struct mm_struct *mm, pgtable_t pte)
{
	pgtable_page_dtor(pte);
	__free_page(pte);
}

static inline void __pmd_populate(pmd_t *pmdp, phys_addr_t pte,
				  pmdval_t prot)
{
	pmdval_t pmdval = (pte + PTE_HWTABLE_OFF) | prot;
#ifdef	TIMA_KERNEL_L1_MANAGE
	unsigned long cmd_id = 0x3f809221;
	unsigned long tima_wr_out, pmd_base;
#if __GNUC__ >= 4 && __GNUC_MINOR__ >= 6
        __asm__ __volatile__(".arch_extension sec");
#endif
	clean_dcache_area(pmdp, 8);
	__asm__ __volatile__ (
		"stmfd  sp!,{r0, r8-r11}\n"
		"mov   	r11, r0\n"
		"mov    r0, %1\n"
		"mov	r8, %2\n"
		"mov    r9, %3\n"
		"mov    r10, %4\n"
		"mcr    p15, 0, r8, c7, c14, 1\n"
		"add    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c14, 1\n"
		"dsb\n"
		"smc    #9\n"
		"sub    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c6,  1\n"
		"dsb\n"

		"mov    %0, r10\n"
		"add    r8, r8, #4\n"
		"mcr    p15, 0, r8, c7, c6,  1\n"
		"dsb\n"

		"mov    r0, #0\n"
		"mcr    p15, 0, r0, c8, c3, 0\n"
		"dsb\n"
		"isb\n"
		"pop    {r0, r8-r11}\n"
		:"=r"(tima_wr_out):"r"(cmd_id),"r"((unsigned long)pmdp),"r"(pmdval),"r"(__pmd(pmdval + 256 * sizeof(pte_t))):"r0","r8","r9","r10","r11","cc");
		
		if ((pmdp[0]|0x4)!=(__pmd(pmdval)|0x4)) {
			printk(KERN_ERR"pmdp[0] %lx != __pmd(pmdval) %lx - %lx %lx in func: %s tima_wr_out = %lx\n", 
					(unsigned long) pmdp[0], (unsigned long) __pmd(pmdval), (unsigned long) pmdp[1], (unsigned long) (__pmd(pmdval + 256 * sizeof(pte_t))),
					__func__, tima_wr_out);
			tima_send_cmd(pmdp[0], 0x3f810221);
		}
		if ((pmdp[1]|0x4)!=((__pmd(pmdval + 256 * sizeof(pte_t)))|0x4)) {
			printk(KERN_ERR"pmdp[1] %lx != (__pmd(pmdval + 256 * sizeof(pte_t))) %lx in func: %s\n",
					(unsigned long) pmdp[1], (unsigned long) (__pmd(pmdval + 256 * sizeof(pte_t))), __func__);
			tima_send_cmd(pmdp[1], 0x3f810221);
		}
#else
	pmdp[0] = __pmd(pmdval);
#ifndef CONFIG_ARM_LPAE
	pmdp[1] = __pmd(pmdval + 256 * sizeof(pte_t));
#endif
#endif
	flush_pmd_entry(pmdp);

#ifdef	TIMA_KERNEL_L1_MANAGE
        pmd_base = ((unsigned long)pmdp) & (~0x3fff);
        tima_verify_state(pmd_base, 0, 1, 2);
        tima_verify_state(pmd_base + 0x1000, 0, 1, 2);
        tima_verify_state(pmd_base + 0x2000, 0, 1, 2);
        tima_verify_state(pmd_base + 0x3000, 0, 1, 2);
#endif
}

/*
 * Populate the pmdp entry with a pointer to the pte.  This pmd is part
 * of the mm address space.
 *
 * Ensure that we always set both PMD entries.
 */
static inline void
pmd_populate_kernel(struct mm_struct *mm, pmd_t *pmdp, pte_t *ptep)
{
	/*
	 * The pmd must be loaded with the physical address of the PTE table
	 */
	__pmd_populate(pmdp, __pa(ptep), _PAGE_KERNEL_TABLE);
}

static inline void
pmd_populate(struct mm_struct *mm, pmd_t *pmdp, pgtable_t ptep)
{
	__pmd_populate(pmdp, page_to_phys(ptep), _PAGE_USER_TABLE);
}
#define pmd_pgtable(pmd) pmd_page(pmd)

#endif /* CONFIG_MMU */

#endif
