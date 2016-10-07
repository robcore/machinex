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
@ GNU C89 (crosstool-NG crosstool-ng-1.21.0-345-ga2573ff - GNU GCC 5.3 - Cortex-A15) version 5.3.0 (arm-cortex_a15-linux-gnueabihf)
@	compiled by GNU C version 5.2.1 20151010, GMP version 6.1.0, MPFR version 3.1.2, MPC version 1.0.2
@ GGC heuristics: --param ggc-min-expand=100 --param ggc-min-heapsize=131072
@ options passed:  -nostdinc -I /root/machinex/arch/arm/include
@ -I arch/arm/include/generated -I include -I /root/machinex/include
@ -I /root/machinex/. -I . -I /root/machinex/arch/arm/mach-msm/include
@ -iprefix /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/
@ -isysroot /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../arm-cortex_a15-linux-gnueabihf/sysroot
@ -D __KERNEL__ -D __LINUX_ARM_ARCH__=7 -U arm -D CC_HAVE_ASM_GOTO
@ -D KBUILD_STR(s)=#s -D KBUILD_BASENAME=KBUILD_STR(bounds)
@ -D KBUILD_MODNAME=KBUILD_STR(bounds)
@ -isystem /opt/toolchains/arm-cortex_a15-linux-gnueabihf_5.3/bin/../lib/gcc/arm-cortex_a15-linux-gnueabihf/5.3.0/include
@ -include /root/machinex/include/linux/kconfig.h -MD kernel/.bounds.s.d
@ /root/machinex/kernel/bounds.c -mlittle-endian -mtune=cortex-a15
@ -mfpu=neon-vfpv4 -marm -mabi=aapcs-linux -mno-thumb-interwork
@ -mcpu=cortex-a15 -mtune=cortex-a15 -mfloat-abi=soft -mcpu=cortex-a15
@ -mtune=cortex-a15 -mfpu=neon-vfpv4 -mvectorize-with-neon-quad
@ -munaligned-access -mtls-dialect=gnu -auxbase-strip kernel/bounds.s -O3
@ -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -Wno-unused-variable
@ -Wno-maybe-uninitialized -Wno-format-security -Wno-unused-function
@ -Wno-unused-label -Wno-array-bounds -Wno-logical-not-parentheses -Wno-cpp
@ -Wno-sizeof-pointer-memaccess -Wno-aggressive-loop-optimizations
@ -Wno-sequence-point -Wframe-larger-than=2048 -Wno-unused-but-set-variable
@ -Wdeclaration-after-statement -Wno-pointer-sign -std=gnu90
@ -fno-strict-aliasing -fno-common -fno-delete-null-pointer-checks
@ -fno-dwarf2-cfi-asm -fstack-protector -fno-ipa-sra -funwind-tables
@ -fomit-frame-pointer -fno-var-tracking-assignments -fno-strict-overflow
@ -fconserve-stack -fgraphite-identity -floop-parallelize-all
@ -floop-interchange -floop-strip-mine -floop-block -ftree-vectorize
@ -funroll-loops -fno-align-functions -fno-align-jumps -fno-align-loops
@ -fno-align-labels -fno-prefetch-loop-arrays -fverbose-asm
@ --param allow-store-data-races=0
@ options enabled:  -faggressive-loop-optimizations -fauto-inc-dec
@ -fbranch-count-reg -fcaller-saves -fchkp-check-incomplete-type
@ -fchkp-check-read -fchkp-check-write -fchkp-instrument-calls
@ -fchkp-narrow-bounds -fchkp-optimize -fchkp-store-bounds
@ -fchkp-use-static-bounds -fchkp-use-static-const-bounds
@ -fchkp-use-wrappers -fcombine-stack-adjustments -fcompare-elim
@ -fcprop-registers -fcrossjumping -fcse-follow-jumps -fdefer-pop
@ -fdevirtualize -fdevirtualize-speculatively -fearly-inlining
@ -feliminate-unused-debug-types -fexpensive-optimizations
@ -fforward-propagate -ffunction-cse -fgcse -fgcse-after-reload -fgcse-lm
@ -fgnu-runtime -fgnu-unique -fgraphite-identity -fguess-branch-probability
@ -fhoist-adjacent-loads -fident -fif-conversion -fif-conversion2
@ -findirect-inlining -finline -finline-atomics -finline-functions
@ -finline-functions-called-once -finline-small-functions -fipa-cp
@ -fipa-cp-alignment -fipa-cp-clone -fipa-icf -fipa-icf-functions
@ -fipa-icf-variables -fipa-profile -fipa-pure-const -fipa-ra
@ -fipa-reference -fira-hoist-pressure -fira-share-save-slots
@ -fira-share-spill-slots -fisolate-erroneous-paths-dereference -fivopts
@ -fkeep-static-consts -fleading-underscore -flifetime-dse -floop-block
@ -floop-interchange -floop-parallelize-all -floop-strip-mine -flra-remat
@ -flto-odr-type-merging -fmath-errno -fmerge-constants
@ -fmerge-debug-strings -fmove-loop-invariants -fomit-frame-pointer
@ -foptimize-sibling-calls -foptimize-strlen -fpartial-inlining -fpeephole
@ -fpeephole2 -fpredictive-commoning -freg-struct-return -frename-registers
@ -freorder-blocks -freorder-functions -frerun-cse-after-loop
@ -fsched-critical-path-heuristic -fsched-dep-count-heuristic
@ -fsched-group-heuristic -fsched-interblock -fsched-last-insn-heuristic
@ -fsched-pressure -fsched-rank-heuristic -fsched-spec
@ -fsched-spec-insn-heuristic -fsched-stalled-insns-dep -fschedule-fusion
@ -fschedule-insns -fschedule-insns2 -fsection-anchors
@ -fsemantic-interposition -fshow-column -fshrink-wrap -fsigned-zeros
@ -fsplit-ivs-in-unroller -fsplit-wide-types -fssa-phiopt -fstack-protector
@ -fstdarg-opt -fstrict-volatile-bitfields -fsync-libcalls -fthread-jumps
@ -ftoplevel-reorder -ftrapping-math -ftree-bit-ccp -ftree-builtin-call-dce
@ -ftree-ccp -ftree-ch -ftree-coalesce-vars -ftree-copy-prop
@ -ftree-copyrename -ftree-cselim -ftree-dce -ftree-dominator-opts
@ -ftree-dse -ftree-forwprop -ftree-fre -ftree-loop-distribute-patterns
@ -ftree-loop-if-convert -ftree-loop-im -ftree-loop-ivcanon
@ -ftree-loop-optimize -ftree-loop-vectorize -ftree-parallelize-loops=
@ -ftree-partial-pre -ftree-phiprop -ftree-pre -ftree-pta -ftree-reassoc
@ -ftree-scev-cprop -ftree-sink -ftree-slp-vectorize -ftree-slsr -ftree-sra
@ -ftree-switch-conversion -ftree-tail-merge -ftree-ter -ftree-vectorize
@ -ftree-vrp -funit-at-a-time -funroll-loops -funswitch-loops
@ -funwind-tables -fverbose-asm -fweb -fzero-initialized-in-bss -marm
@ -mglibc -mlittle-endian -mpic-data-is-text-relative -msched-prolog
@ -munaligned-access -mvectorize-with-neon-quad

	.text
	.align	2
	.global	foo
	.type	foo, %function
foo:
	.fnstart
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 0, uses_anonymous_args = 0
	@ link register save eliminated.
	.syntax divided
@ 17 "/root/machinex/kernel/bounds.c" 1
	
->NR_PAGEFLAGS #25 __NR_PAGEFLAGS	@
@ 0 "" 2
@ 18 "/root/machinex/kernel/bounds.c" 1
	
->MAX_NR_ZONES #3 __MAX_NR_ZONES	@
@ 0 "" 2
@ 19 "/root/machinex/kernel/bounds.c" 1
	
->NR_PCG_FLAGS #3 __NR_PCG_FLAGS	@
@ 0 "" 2
	.arm
	.syntax divided
	bx	lr	@
	.fnend
	.size	foo, .-foo
	.ident	"GCC: (crosstool-NG crosstool-ng-1.21.0-345-ga2573ff - GNU GCC 5.3 - Cortex-A15) 5.3.0"
	.section	.note.GNU-stack,"",%progbits
