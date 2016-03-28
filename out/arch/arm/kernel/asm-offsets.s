	.cpu cortex-a15
	.fpu softvfp
	.eabi_attribute 20, 1	@ Tag_ABI_FP_denormal
	.eabi_attribute 21, 1	@ Tag_ABI_FP_exceptions
	.eabi_attribute 23, 3	@ Tag_ABI_FP_number_model
	.eabi_attribute 24, 1	@ Tag_ABI_align8_needed
	.eabi_attribute 25, 1	@ Tag_ABI_align8_preserved
	.eabi_attribute 26, 2	@ Tag_ABI_enum_size
	.eabi_attribute 30, 2	@ Tag_ABI_optimization_goals
	.eabi_attribute 34, 1	@ Tag_CPU_unaligned_access
	.eabi_attribute 18, 4	@ Tag_ABI_PCS_wchar_t
	.file	"asm-offsets.c"
@ GNU C (crosstool-NG 1.20.0 - Linaro GCC 2015.06 - Cortex-A15) version 4.9.4 20150629 (prerelease) (arm-cortex_a15-linux-gnueabihf)
@	compiled by GNU C version 4.9.2, GMP version 6.0.0, MPFR version 3.1.2, MPC version 1.0.2
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc
@ -I /media/root/robcore/android/machinex/arch/arm/include
@ -I arch/arm/include/generated -I include
@ -I /media/root/robcore/android/machinex/include
@ -I /media/root/robcore/android/machinex/. -I .
@ -I /media/root/robcore/android/machinex/arch/arm/mach-msm/include
@ -iprefix /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/
@ -isysroot /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../arm-cortex_a15-linux-gnueabihf/sysroot
@ -D __KERNEL__ -D __LINUX_ARM_ARCH__=7 -U arm -D CC_HAVE_ASM_GOTO
@ -D KBUILD_STR(s)=#s -D KBUILD_BASENAME=KBUILD_STR(asm_offsets)
@ -D KBUILD_MODNAME=KBUILD_STR(asm_offsets)
@ -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/include
@ -include /media/root/robcore/android/machinex/include/linux/kconfig.h
@ -MD arch/arm/kernel/.asm-offsets.s.d
@ /media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c
@ -mlittle-endian -marm -mapcs -mno-sched-prolog -mabi=aapcs-linux
@ -mno-thumb-interwork -mcpu=cortex-a15 -mfloat-abi=soft -munaligned-access
@ -mfpu=neon-vfpv4 -mtls-dialect=gnu
@ -auxbase-strip arch/arm/kernel/asm-offsets.s -O3 -Wall -Wundef
@ -Wstrict-prototypes -Wno-trigraphs -Wno-unused-value
@ -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations
@ -Werror=implicit-function-declaration -Wno-uninitialized
@ -Wno-format-security -Wno-array-bounds -Wno-unused-variable
@ -Wno-unused-function -Wno-maybe-uninitialized -Wframe-larger-than=1048
@ -Wno-unused-but-set-variable -Wno-pointer-sign -fno-strict-aliasing
@ -fno-common -fno-delete-null-pointer-checks -fno-dwarf2-cfi-asm
@ -fstack-protector -fno-omit-frame-pointer -fno-optimize-sibling-calls
@ -fno-var-tracking-assignments -fno-strict-overflow -fconserve-stack
@ -fverbose-asm
@ options enabled:  -faggressive-loop-optimizations -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fcombine-stack-adjustments
@ -fcompare-elim -fcprop-registers -fcrossjumping -fcse-follow-jumps
@ -fdefer-pop -fdevirtualize -fdevirtualize-speculatively -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffunction-cse -fgcse -fgcse-after-reload -fgcse-lm
@ -fgnu-runtime -fgnu-unique -fguess-branch-probability
@ -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-atomics -finline-functions
@ -finline-functions-called-once -finline-small-functions -fipa-cp
@ -fipa-cp-clone -fipa-profile -fipa-pure-const -fipa-reference -fipa-sra
@ -fira-hoist-pressure -fira-share-save-slots -fira-share-spill-slots
@ -fisolate-erroneous-paths-dereference -fivopts -fkeep-static-consts
@ -fleading-underscore -flifetime-dse -fmath-errno -fmerge-constants
@ -fmerge-debug-strings -fmove-loop-invariants -foptimize-strlen
@ -fpartial-inlining -fpeephole -fpeephole2 -fpredictive-commoning
@ -fprefetch-loop-arrays -freg-struct-return -freorder-blocks
@ -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-insns
@ -fschedule-insns2 -fsection-anchors -fshow-column -fsigned-zeros
@ -fsplit-ivs-in-unroller -fsplit-wide-types -fstack-protector
@ -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
@ -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp -ftree-builtin-call-dce
@ -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop
@ -ftree-copyrename -ftree-cselim -ftree-dce -ftree-dominator-opts
@ -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-distribute-patterns
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-loop-vectorize -ftree-parallelize-loops=
@ -ftree-partial-pre -ftree-phiprop -ftree-pre -ftree-pta -ftree-reassoc
@ -ftree-scev-cprop -ftree-sink -ftree-slp-vectorize -ftree-slsr -ftree-sra
@ -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vrp
@ -funit-at-a-time -funswitch-loops -fverbose-asm -fzero-initialized-in-bss
@ -mapcs-frame -marm -mglibc -mlittle-endian -mlra
@ -mpic-data-is-text-relative -munaligned-access -mvectorize-with-neon-quad

	.section	.text.startup,"ax",%progbits
	.align	2
	.global	main
	.type	main, %function
main:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp	@,
	stmfd	sp!, {fp, ip, lr, pc}	@,
	sub	fp, ip, #4	@,,
#APP
@ 46 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TSK_ACTIVE_MM #252 offsetof(struct task_struct, active_mm)	@
@ 0 "" 2
@ 48 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TSK_STACK_CANARY #316 offsetof(struct task_struct, stack_canary)	@
@ 0 "" 2
@ 50 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 51 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_FLAGS #0 offsetof(struct thread_info, flags)	@
@ 0 "" 2
@ 52 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_PREEMPT #4 offsetof(struct thread_info, preempt_count)	@
@ 0 "" 2
@ 53 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_ADDR_LIMIT #8 offsetof(struct thread_info, addr_limit)	@
@ 0 "" 2
@ 54 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_TASK #12 offsetof(struct thread_info, task)	@
@ 0 "" 2
@ 55 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_EXEC_DOMAIN #16 offsetof(struct thread_info, exec_domain)	@
@ 0 "" 2
@ 56 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_CPU #20 offsetof(struct thread_info, cpu)	@
@ 0 "" 2
@ 57 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_CPU_DOMAIN #24 offsetof(struct thread_info, cpu_domain)	@
@ 0 "" 2
@ 58 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_CPU_SAVE #28 offsetof(struct thread_info, cpu_context)	@
@ 0 "" 2
@ 59 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_USED_CP #80 offsetof(struct thread_info, used_cp)	@
@ 0 "" 2
@ 60 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_TP_VALUE #96 offsetof(struct thread_info, tp_value)	@
@ 0 "" 2
@ 61 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_FPSTATE #288 offsetof(struct thread_info, fpstate)	@
@ 0 "" 2
@ 62 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->TI_VFPSTATE #432 offsetof(struct thread_info, vfpstate)	@
@ 0 "" 2
@ 64 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->VFP_CPU #272 offsetof(union vfp_state, hard.cpu)	@
@ 0 "" 2
@ 75 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 76 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R0 #0 offsetof(struct pt_regs, ARM_r0)	@
@ 0 "" 2
@ 77 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R1 #4 offsetof(struct pt_regs, ARM_r1)	@
@ 0 "" 2
@ 78 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R2 #8 offsetof(struct pt_regs, ARM_r2)	@
@ 0 "" 2
@ 79 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R3 #12 offsetof(struct pt_regs, ARM_r3)	@
@ 0 "" 2
@ 80 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R4 #16 offsetof(struct pt_regs, ARM_r4)	@
@ 0 "" 2
@ 81 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R5 #20 offsetof(struct pt_regs, ARM_r5)	@
@ 0 "" 2
@ 82 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R6 #24 offsetof(struct pt_regs, ARM_r6)	@
@ 0 "" 2
@ 83 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R7 #28 offsetof(struct pt_regs, ARM_r7)	@
@ 0 "" 2
@ 84 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R8 #32 offsetof(struct pt_regs, ARM_r8)	@
@ 0 "" 2
@ 85 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R9 #36 offsetof(struct pt_regs, ARM_r9)	@
@ 0 "" 2
@ 86 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_R10 #40 offsetof(struct pt_regs, ARM_r10)	@
@ 0 "" 2
@ 87 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_FP #44 offsetof(struct pt_regs, ARM_fp)	@
@ 0 "" 2
@ 88 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_IP #48 offsetof(struct pt_regs, ARM_ip)	@
@ 0 "" 2
@ 89 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_SP #52 offsetof(struct pt_regs, ARM_sp)	@
@ 0 "" 2
@ 90 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_LR #56 offsetof(struct pt_regs, ARM_lr)	@
@ 0 "" 2
@ 91 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_PC #60 offsetof(struct pt_regs, ARM_pc)	@
@ 0 "" 2
@ 92 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_PSR #64 offsetof(struct pt_regs, ARM_cpsr)	@
@ 0 "" 2
@ 93 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_OLD_R0 #68 offsetof(struct pt_regs, ARM_ORIG_r0)	@
@ 0 "" 2
@ 94 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->S_FRAME_SIZE #72 sizeof(struct pt_regs)	@
@ 0 "" 2
@ 95 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 108 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->MM_CONTEXT_ID #384 offsetof(struct mm_struct, context.id)	@
@ 0 "" 2
@ 109 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 111 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->VMA_VM_MM #0 offsetof(struct vm_area_struct, vm_mm)	@
@ 0 "" 2
@ 112 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->VMA_VM_FLAGS #24 offsetof(struct vm_area_struct, vm_flags)	@
@ 0 "" 2
@ 113 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 114 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->VM_EXEC #4 VM_EXEC	@
@ 0 "" 2
@ 115 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 116 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->PAGE_SZ #4096 PAGE_SIZE	@
@ 0 "" 2
@ 117 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 118 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->SYS_ERROR0 #10420224 0x9f0000	@
@ 0 "" 2
@ 119 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 120 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->SIZEOF_MACHINE_DESC #72 sizeof(struct machine_desc)	@
@ 0 "" 2
@ 121 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->MACHINFO_TYPE #0 offsetof(struct machine_desc, nr)	@
@ 0 "" 2
@ 122 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->MACHINFO_NAME #4 offsetof(struct machine_desc, name)	@
@ 0 "" 2
@ 123 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 124 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->PROC_INFO_SZ #52 sizeof(struct proc_info_list)	@
@ 0 "" 2
@ 125 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->PROCINFO_INITFUNC #16 offsetof(struct proc_info_list, __cpu_flush)	@
@ 0 "" 2
@ 126 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->PROCINFO_MM_MMUFLAGS #8 offsetof(struct proc_info_list, __cpu_mm_mmu_flags)	@
@ 0 "" 2
@ 127 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->PROCINFO_IO_MMUFLAGS #12 offsetof(struct proc_info_list, __cpu_io_mmu_flags)	@
@ 0 "" 2
@ 128 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 141 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->CACHE_FLUSH_KERN_ALL #4 offsetof(struct cpu_cache_fns, flush_kern_all)	@
@ 0 "" 2
@ 143 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->
@ 0 "" 2
@ 144 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->DMA_BIDIRECTIONAL #0 DMA_BIDIRECTIONAL	@
@ 0 "" 2
@ 145 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->DMA_TO_DEVICE #1 DMA_TO_DEVICE	@
@ 0 "" 2
@ 146 "/media/root/robcore/android/machinex/arch/arm/kernel/asm-offsets.c" 1
	
->DMA_FROM_DEVICE #2 DMA_FROM_DEVICE	@
@ 0 "" 2
	mov	r0, #0	@,
	sub	sp, fp, #12	@,,
	ldmfd	sp, {fp, sp, pc}	@
	.size	main, .-main
	.ident	"GCC: (crosstool-NG 1.20.0 - Linaro GCC 2015.06 - Cortex-A15) 4.9.4 20150629 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
