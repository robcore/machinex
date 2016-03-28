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
	.file	"bounds.c"
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
@ -D KBUILD_STR(s)=#s -D KBUILD_BASENAME=KBUILD_STR(bounds)
@ -D KBUILD_MODNAME=KBUILD_STR(bounds)
@ -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf-linaro_4.9.4-2015.06/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/4.9.4/include
@ -include /media/root/robcore/android/machinex/include/linux/kconfig.h
@ -MD kernel/.bounds.s.d
@ /media/root/robcore/android/machinex/kernel/bounds.c -mlittle-endian
@ -marm -mapcs -mno-sched-prolog -mabi=aapcs-linux -mno-thumb-interwork
@ -mcpu=cortex-a15 -mfloat-abi=soft -munaligned-access -mfpu=neon-vfpv4
@ -mtls-dialect=gnu -auxbase-strip kernel/bounds.s -O3 -Wall -Wundef
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

	.text
	.align	2
	.global	foo
	.type	foo, %function
foo:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp	@,
	stmfd	sp!, {fp, ip, lr, pc}	@,
	sub	fp, ip, #4	@,,
#APP
@ 17 "/media/root/robcore/android/machinex/kernel/bounds.c" 1
	
->NR_PAGEFLAGS #23 __NR_PAGEFLAGS	@
@ 0 "" 2
@ 18 "/media/root/robcore/android/machinex/kernel/bounds.c" 1
	
->MAX_NR_ZONES #3 __MAX_NR_ZONES	@
@ 0 "" 2
@ 19 "/media/root/robcore/android/machinex/kernel/bounds.c" 1
	
->NR_PCG_FLAGS #3 __NR_PCG_FLAGS	@
@ 0 "" 2
	sub	sp, fp, #12	@,,
	ldmfd	sp, {fp, sp, pc}	@
	.size	foo, .-foo
	.ident	"GCC: (crosstool-NG 1.20.0 - Linaro GCC 2015.06 - Cortex-A15) 4.9.4 20150629 (prerelease)"
	.section	.note.GNU-stack,"",%progbits
