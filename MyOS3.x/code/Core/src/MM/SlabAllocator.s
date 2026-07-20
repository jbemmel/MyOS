	.file	"SlabAllocator.cpp"
	.section	.text$_ZNK4MyOS4Core10IInterface13getScriptableEv,"x"
	.linkonce discard
	.align 2
	.p2align 4,,15
.globl __ZNK4MyOS4Core10IInterface13getScriptableEv
	.def	__ZNK4MyOS4Core10IInterface13getScriptableEv;	.scl	2;	.type	32;	.endef
__ZNK4MyOS4Core10IInterface13getScriptableEv:
	pushl	%ebp
	xorl	%eax, %eax
	movl	%esp, %ebp
	popl	%ebp
	ret
	.text
	.align 2
	.p2align 4,,15
.globl __ZN4MyOS2MM13SlabAllocatorC2ERNS0_14IVirtualMemoryE
	.def	__ZN4MyOS2MM13SlabAllocatorC2ERNS0_14IVirtualMemoryE;	.scl	2;	.type	32;	.endef
__ZN4MyOS2MM13SlabAllocatorC2ERNS0_14IVirtualMemoryE:
	pushl	%ebp
	movl	$32, %ecx
	movl	%esp, %ebp
	movl	12(%ebp), %eax
	pushl	%edi
	movl	8(%ebp), %edi
	movl	%eax, 128(%edi)
	xorl	%eax, %eax
/APP
 # 612 "..\..\include/string.h" 1
	cld; rep stosl
 # 0 "" 2
/NO_APP
	popl	%edi
	popl	%ebp
	ret
	.align 2
	.p2align 4,,15
.globl __ZN4MyOS2MM13SlabAllocatorC1ERNS0_14IVirtualMemoryE
	.def	__ZN4MyOS2MM13SlabAllocatorC1ERNS0_14IVirtualMemoryE;	.scl	2;	.type	32;	.endef
__ZN4MyOS2MM13SlabAllocatorC1ERNS0_14IVirtualMemoryE:
	pushl	%ebp
	movl	$32, %ecx
	movl	%esp, %ebp
	movl	12(%ebp), %eax
	pushl	%edi
	movl	8(%ebp), %edi
	movl	%eax, 128(%edi)
	xorl	%eax, %eax
/APP
 # 612 "..\..\include/string.h" 1
	cld; rep stosl
 # 0 "" 2
/NO_APP
	popl	%edi
	popl	%ebp
	ret
	.align 2
	.p2align 4,,15
.globl __ZN4MyOS2MM13SlabAllocator12newAllocatorERNS1_14BlockAllocatorE
	.def	__ZN4MyOS2MM13SlabAllocator12newAllocatorERNS1_14BlockAllocatorE;	.scl	2;	.type	32;	.endef
__ZN4MyOS2MM13SlabAllocator12newAllocatorERNS1_14BlockAllocatorE:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx
	subl	$44, %esp
	movl	8(%ebp), %edx
	movl	128(%edx), %eax
	leal	-16(%ebp), %edx
	movl	(%eax), %ecx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	*16(%ecx)
	movl	12(%ebp), %ecx
	movl	%eax, -36(%ebp)
	movl	8(%ecx), %esi
	movl	$0, (%eax)
	movl	%ecx, 4(%eax)
	movl	%esi, 8(%eax)
	movl	-36(%ebp), %eax
	addl	$16, %eax
	testl	$4, %esi
	jne	L8
	movl	$4, %edx
	.p2align 4,,7
L9:
	leal	(%eax,%edx,4), %eax
	addl	%edx, %edx
	testl	%esi, %edx
	je	L9
L8:
	movl	-36(%ebp), %edx
	movl	%eax, %ebx
	andl	$4095, %eax
	leal	0(,%esi,4), %ecx
	movl	%ecx, %edi
	negl	%edi
	movl	%eax, (%edx)
	addl	$4096, %edx
	movl	%edx, -32(%ebp)
	leal	(%ebx,%ecx), %edx
	.p2align 4,,7
L10:
	addl	%ecx, %ebx
	movl	%edx, (%ebx,%edi)
	addl	%ecx, %edx
	movl	%edx, %eax
	subl	%ecx, %eax
	cmpl	%eax, -32(%ebp)
	ja	L10
	movl	12(%ebp), %ecx
	leal	0(,%esi,4), %eax
	negl	%eax
	movl	8(%ebp), %edx
	movl	$0, (%ebx,%eax)
	movl	8(%ecx), %eax
	movl	-36(%ebp), %ecx
	andl	$-4, %eax
/APP
 # 108 "../Atomic/atomic32.h" 1
	xchg %ecx, (%edx,%eax)
 # 0 "" 2
/NO_APP
	addl	$44, %esp
	movl	%ecx, %eax
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.align 2
	.p2align 4,,15
.globl __ZN4MyOS2MM13SlabAllocator4freeEPv
	.def	__ZN4MyOS2MM13SlabAllocator4freeEPv;	.scl	2;	.type	32;	.endef
__ZN4MyOS2MM13SlabAllocator4freeEPv:
	pushl	%ebp
	movl	%esp, %ebp
	pushl	%edi
	pushl	%esi
	movl	8(%ebp), %esi
	pushl	%ebx
	movl	%esi, %edi
	movl	%esi, %ebx
	andl	$-4096, %ebx
	andl	$4095, %edi
	.p2align 4,,7
L16:
	movl	(%ebx), %ecx
	movl	%ecx, %eax
	movl	%ecx, %edx
	andl	$4095, %eax
	leal	(%ebx,%eax), %eax
	sall	$16, %edx
	movl	%eax, (%esi)
	orl	%edi, %edx
	movl	%ecx, %eax
/APP
 # 139 "../Atomic/atomic32.h" 1
	cmpxchgl %edx, (%ebx)
 # 0 "" 2
/NO_APP
	cmpl	%eax, %ecx
	jne	L16
	popl	%ebx
	popl	%esi
	popl	%edi
	popl	%ebp
	ret
	.align 2
	.p2align 4,,15
.globl __ZN4MyOS2MM13SlabAllocator8allocateEj
	.def	__ZN4MyOS2MM13SlabAllocator8allocateEj;	.scl	2;	.type	32;	.endef
__ZN4MyOS2MM13SlabAllocator8allocateEj:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$56, %esp
	movl	12(%ebp), %eax
	movl	%ebx, -12(%ebp)
	movl	%esi, -8(%ebp)
	movl	%edi, -4(%ebp)
	cmpl	$127, %eax
	ja	L28
	leal	3(%eax), %ebx
	movl	8(%ebp), %eax
	shrl	$2, %ebx
	movl	(%eax,%ebx,4), %edx
	testl	%edx, %edx
	je	L36
L28:
L27:
	movl	-12(%ebp), %ebx
	movl	-8(%ebp), %esi
	movl	-4(%ebp), %edi
	movl	%ebp, %esp
	popl	%ebp
	ret
	.p2align 4,,7
L36:
	movl	128(%eax), %eax
	leal	-16(%ebp), %edx
	movl	(%eax), %ecx
	movl	%edx, 4(%esp)
	movl	%eax, (%esp)
	call	*16(%ecx)
	testb	$4, %bl
	movl	%eax, %edi
	movl	$0, (%eax)
	movl	$0, 4(%eax)
	movl	%ebx, 8(%eax)
	leal	16(%eax), %eax
	jne	L21
	movl	$4, %edx
	.p2align 4,,7
L22:
	leal	(%eax,%edx,4), %eax
	addl	%edx, %edx
	testl	%ebx, %edx
	je	L22
L21:
	movl	%eax, %esi
	andl	$4095, %eax
	leal	0(,%ebx,4), %ecx
	movl	%eax, (%edi)
	leal	4096(%edi), %eax
	movl	%eax, -32(%ebp)
	movl	%ecx, %eax
	negl	%eax
	leal	(%esi,%ecx), %edx
	movl	%eax, -36(%ebp)
	.p2align 4,,7
L23:
	movl	-36(%ebp), %eax
	addl	%ecx, %esi
	movl	%edx, (%esi,%eax)
	addl	%ecx, %edx
	movl	%edx, %eax
	subl	%ecx, %eax
	cmpl	%eax, -32(%ebp)
	ja	L23
	leal	0(,%ebx,4), %edx
	xorl	%ebx, %ebx
	movl	8(%ebp), %ecx
	movl	%edx, %eax
	negl	%eax
	movl	$0, (%esi,%eax)
	movl	%ebx, %eax
/APP
 # 139 "../Atomic/atomic32.h" 1
	cmpxchgl %edi, (%ecx,%edx)
 # 0 "" 2
/NO_APP
	testl	%eax, %eax
	movl	%eax, %ebx
	jne	L37
	.p2align 4,,7
L34:
	movl	(%edi), %ecx
	testl	%ecx, %ecx
	je	L38
	movl	%ecx, %eax
	andl	$4095, %eax
	leal	(%edi,%eax), %ebx
	movl	%ecx, %eax
	movl	(%ebx), %edx
/APP
 # 139 "../Atomic/atomic32.h" 1
	cmpxchgl %edx, (%edi)
 # 0 "" 2
/NO_APP
	cmpl	%eax, %ecx
	jne	L34
	movl	$0, (%ebx)
	movl	%ebx, %eax
	jmp	L27
	.p2align 4,,7
L38:
	movl	4(%edi), %eax
	testl	%eax, %eax
	je	L26
	movl	%eax, %edi
	jmp	L34
L26:
	movl	8(%ebp), %eax
	movl	%edi, 4(%esp)
	movl	%eax, (%esp)
	call	__ZN4MyOS2MM13SlabAllocator12newAllocatorERNS1_14BlockAllocatorE
	movl	%eax, %edi
	jmp	L34
L37:
	movl	%edi, (%esp)
	movl	%ebx, %edi
	call	__ZdlPv
	.p2align 4,,2
	jmp	L34
.globl __ZTVN4MyOS4Core10IContainerE
	.section	.rdata$_ZTVN4MyOS4Core10IContainerE,"dr"
	.linkonce same_size
	.align 8
__ZTVN4MyOS4Core10IContainerE:
	.long	0
	.long	__ZTIN4MyOS4Core10IContainerE
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
.globl __ZTVN4MyOS4Core10IComponentE
	.section	.rdata$_ZTVN4MyOS4Core10IComponentE,"dr"
	.linkonce same_size
	.align 8
__ZTVN4MyOS4Core10IComponentE:
	.long	0
	.long	__ZTIN4MyOS4Core10IComponentE
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
	.long	___cxa_pure_virtual
.globl __ZTVN4MyOS4Core10IInterfaceE
	.section	.rdata$_ZTVN4MyOS4Core10IInterfaceE,"dr"
	.linkonce same_size
	.align 8
__ZTVN4MyOS4Core10IInterfaceE:
	.long	0
	.long	__ZTIN4MyOS4Core10IInterfaceE
	.long	__ZNK4MyOS4Core10IInterface13getScriptableEv
.globl __ZTSN4MyOS4Core10IContainerE
	.section	.rdata$_ZTSN4MyOS4Core10IContainerE,"dr"
	.linkonce same_size
__ZTSN4MyOS4Core10IContainerE:
	.ascii "N4MyOS4Core10IContainerE\0"
.globl __ZTIN4MyOS4Core10IContainerE
	.section	.rdata$_ZTIN4MyOS4Core10IContainerE,"dr"
	.linkonce same_size
	.align 4
__ZTIN4MyOS4Core10IContainerE:
	.long	__ZTVN10__cxxabiv120__si_class_type_infoE+8
	.long	__ZTSN4MyOS4Core10IContainerE
	.long	__ZTIN4MyOS4Core10IComponentE
.globl __ZTSN4MyOS4Core10IComponentE
	.section	.rdata$_ZTSN4MyOS4Core10IComponentE,"dr"
	.linkonce same_size
__ZTSN4MyOS4Core10IComponentE:
	.ascii "N4MyOS4Core10IComponentE\0"
.globl __ZTIN4MyOS4Core10IComponentE
	.section	.rdata$_ZTIN4MyOS4Core10IComponentE,"dr"
	.linkonce same_size
	.align 4
__ZTIN4MyOS4Core10IComponentE:
	.long	__ZTVN10__cxxabiv117__class_type_infoE+8
	.long	__ZTSN4MyOS4Core10IComponentE
.globl __ZTSN4MyOS4Core10IInterfaceE
	.section	.rdata$_ZTSN4MyOS4Core10IInterfaceE,"dr"
	.linkonce same_size
__ZTSN4MyOS4Core10IInterfaceE:
	.ascii "N4MyOS4Core10IInterfaceE\0"
.globl __ZTIN4MyOS4Core10IInterfaceE
	.section	.rdata$_ZTIN4MyOS4Core10IInterfaceE,"dr"
	.linkonce same_size
	.align 4
__ZTIN4MyOS4Core10IInterfaceE:
	.long	__ZTVN10__cxxabiv117__class_type_infoE+8
	.long	__ZTSN4MyOS4Core10IInterfaceE
	.def	__ZdlPv;	.scl	2;	.type	32;	.endef
	.def	___cxa_pure_virtual;	.scl	2;	.type	32;	.endef
	.section .drectve
	.ascii " -export:_ZTVN4MyOS4Core10IInterfaceE,data"
	.ascii " -export:_ZTVN4MyOS4Core10IComponentE,data"
	.ascii " -export:_ZTVN4MyOS4Core10IContainerE,data"
	.ascii " -export:_ZNK4MyOS4Core10IInterface13getScriptableEv"
