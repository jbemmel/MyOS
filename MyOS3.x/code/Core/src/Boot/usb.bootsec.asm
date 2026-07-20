; bootf.asm  FAT12 floppy bootstrap for protected mode image
; Version 2.0, May 1, 1999
; Sample code
; by John S. Fine  johnfine@erols.com
; edited by Jeroen van Bemmel
; I do not place any restrictions on your use of this source code
; I do not provide any warranty of the correctness of this source code
;_____________________________________________________________________________
;
; Documentation:
;
;   1)  Most of the documentation is in comments at the END of this file.
;   2)  Some is in the read.me file that came with this file.
;   3)  In the comments within the source code are several uses of {}, each
;       of these indicates a section of documentation at the end of this file
;       which applies to that part of the source code.
;   4)  The major way I reduced code size was to rely on left-over values in
;       registers and/or to initialize registers at points where doing so was
;       cheap.  To make that understandable, there are several lines beginning
;       with either ";>" or ";>>".  These lines describe the register contents
;       at that point in the code.  ";>" indicates that the value is required
;       by the next section of code.  ";>>" indicates that the value passes
;       through the next section unmodified and is needed later.
;    5) I made this code by modifying my bootp example.  If something doesn't
;       make sense here, maybe comparison to the other version will help.
;_____________________________________________________________________________
;
; Limitations:
;
;    For simplicity, I made several assumptions about the environment.  Some
; of those may need to be expanded in a future version.  Except for the first,
; they are all things that could be checked by a "SYS" program that installs
; this bootstrap.  In general it makes more sense (where possible) for a SYS
; program to check compatibility issues than for a bootstrap to check them
; itself.
;
;    If these limits are violated, this code simply bombs.  It does not check
; the limits nor display error messages.
;
; 1) Runs only on a 386+
; 2) The FAT structure must be the first thing on the drive (no partitions).
;    It just uses "reserved" sectors before the FAT, not "hidden" etc.
; 3) The sector size must be 512 bytes
; 4) There are less than 256 sectors per fat (safe, since FAT12 has less than
;    4096*1.5 bytes per FAT).
; 5) LBN of root directory less than 65536 (safe in unpartitioned FAT12)
;_____________________________________________________________________________


	struc	BB		;FAT Boot block
		resb	0xD	;Things we ignore
BB_clu		resb	1	;Sectors per cluster
BB_res		resw	1	;Reserved sectors
BB_fats		resb	1	;Number of FATs
BB_root		resw	1	;Root directory entries
		resb	3	;Things we ignore
BB_fat		resw	1	;Sectors per fat
BB_sec		resw	1	;Sectors per track
BB_head		resw	1	;Heads
BB_hidd   resd  1 ; hidden sectors, added for fat16 (not 0 there!)
	endstruc

%define FLAT_CODE 8
%define FLAT_DATA 16

; Offset where this image will be loaded
%define BOOT 0x7C00

;>   cs = 0
;>>  dl = drive we were booted from

boot:	jmp short start			;Standard start of boot sector
	nop
	; resb	0x3B			      ;Skip over parameters (set by format)
	incbin "usb_bootsec.bin", 3, 0x3b

start:
	cli				       ;{0}
	xor   ax, ax
	mov   ds, ax       ; Make sure that the DS offset == 0 (some broken BIOS might do otherwise)
	lgdt	[gdt+BOOT]	 ; Load GDT
;	mov	ecx, CR0		   ; Switch to protected mode
;	inc	cx
	mov ecx, 1			   ; JvB: added, enables caching
	mov	CR0, ecx		   ;{5}
.5:
    in	al, 0x64		   ;Enable A20 {4A}
	test	al, 2
	jnz	.5
	mov	al, 0xD1
	out	0x64, al
.6:	in	al, 0x64
	and	ax, byte 2
	jnz	.6
	mov	al, 0xDF
	out	0x60, al

;>   ah = 0
;>   dl = drive we were booted from

	mov	al, FLAT_DATA	; Selector for 4Gb data seg
	mov	ds, ax			; {2} Extend limit for ds
	mov	es, ax			; Extend limit for es
	dec	cx			    ; Switch back to real mode
	mov	CR0, ecx		; {5}

	mov	[boot+BOOT], dl	; Save drive number we came from
	mov	sp, 0x800		; {1B}

	xor	eax, eax    ; Segment
	mov	ds, ax
	mov	ss, ax
	mov	es, sp			; Read directory at 800:0 {1C}

;>   eax = 00000000

	movzx	ax, byte [boot+BOOT+BB_fats]  ; Number of FATs
	mul	word [boot+BOOT+BB_fat]	          ; Times size of FAT
	add	ax, [boot+BOOT+BB_res]	          ; Plus Sectors before first FAT
					                      ; eax = LBN of Root directory
	movzx	edi,word [boot+BOOT+BB_root]  ; Root directory entries
	push	di			                  ; used again later
	dec	di			                      ; Convert to number of sectors
	shr	di, 4			                  ; 16 directory entries per sector
	inc	di
	call	read_sectors

;>  eax  = LBN of root directory
;>  edi  = length of root directory in sectors
;>  [sp] = length of root directory in entries
;>  esi  = 00000000

	lea	ebp, [eax+edi]		                 ; ebp = LBN of cluster 2
;   add ebp, dword [boot+BOOT+BB_hidd]   ; added for fat16: correct LBN for hidden sectors

	pop	bx			             ; Root directory entries
	xor	di, di			         ; Point at directory {1C}
.20:
  lea	si, [file_name+BOOT]		;Name of file we want
	xor	cx, cx
	mov	cl, 11
	rep cmpsb		    ; Found the file?
	je	found			  ; Yes
	add	cl, 21			; Offset to next directory entry (11+21=32 bytes)
	add	di, cx		; Advance to next entry
	dec	bx			    ; Loop through all entries
	jnz	.20

	;Couldn't find file in directory
boot_error:
;  mov cx, 'X'| 0x4f00         ; put red crosses on the screen
;  jmp short show_error
  
disk_error:
;  mov cx, 'D'| 0x4f00         ; put red 'D' on the screen  

show_error:
	mov	ax, 0xE07		;{3}
	int	10h
	mov   ax, 0xB800              ; load screen offset
	mov   es, ax                  ;
	mov   ax, 'X'| 0x4f00         ; put red crosses on the screen
	mov   cx, 25*80
	xor   di,di
	rep stosw
	jmp short $

;>> ecx   = 00000000
;> es     = 800
;> es:edi = Directory entry of file
;> ebp    = LBN of cluster 2
;> eax    = 0000????

found:	
	push	word [es:edi+0xF]	    ;Starting cluster of file (0xf+11=offset 26)
	mov	di, [boot+BOOT+BB_fat]	;Size of FAT (in sectors)
	mov	ax, [boot+BOOT+BB_res]	;LBN of FAT (dont correct for hidden sectors!)
;	add ax, word [boot+BOOT+BB_hidd]   ; wrong: root + FAT are before hidden sectors, data area after
	call	read_sectors          ; read in FAT for the given file, i.e. linked list of sectors (16 bit each!)
	                              ; read in @0x8000 physical

	mov	bx, 0x4000
	mov	es, bx			        ; es = 0x4000
	mov	edi, 0x100000-0x40000	; {1D}{4B} One megabyte minus ES base
.10:

;>>    ecx = 0000????
;>    [sp] = Next cluster of file
;>     esi = 0000????
;>>    edx = 0000????
;>  es:edi = Destination address
;>     ebp = LBN of cluster 2
;>      ds = 0

	xor	eax, eax
	pop	si			    ; Next cluster of file
	mov	bx, si
	cmp	si, 0xFFF8	  	; Valid cluster? (note: different between fat12(0xff8) and fat16(0xfff8) here
	jae	eof			   	; No: assume end of file
					    ; Yes: (c-bit set)
;	rcr	bx, 1			; bx = 0x8000 + cluster/2         ; not /2 for fat16
  

	mov	bx, [bx+si+0x8000]	; Get word containing FAT entry (2*index+base)
;	jnc	.11			        ; Entry is low 12 bits            ; not for fat16
;	shr	bx, 4			    ; Entry was high 12 bits          ; not for fat16 
; .11:	
; and	bh, 0xF		;Mask to just 12 bits                 ; not for fat16
	push	bx			;Save cluster after next
	push	di			;Save destination address {7}
	mov	al, [boot+BOOT+BB_clu]	;Size of each cluster (byte, in sectors)
	mov	di, ax			         ;  (in sectors)
	dec	si
	dec	si
	mul	esi			   	  ; Times cluster number minus 2
	add	eax, ebp		  ; Plus LBN of cluster 2
		
	call	read_sectors  ; Read that cluster, starting at ; start @1MB physical

;>     ecx = 0000????
;>>    edx = 0000????
;>      di = Clustersize in sectors
;>     esi = 0
;>>    ebp = LBN of cluster 2
;>    [sp] = Bottom 16-bits of destination address {7}
;>> [sp+2] = Following cluster
;>      ds = 0
;>      es = 4000

	mov	cx, di			; Cluster size in sectors
	xchg	ch, cl		; Cluster size in words
	pop	di			    ; Restore destination address {7}
	es a32 rep movsw
	jmp short .10		; Loop until end of file

;>     eax = 0
;>     edx = 0000????
;>      bx = 0FF?

eof:
	call 0xFFFF:0x10	; JvB added: execute bootphase2

	mov	dx, 0x9C00
	mov	es, dx			;es = 9C00
	xor	di, di			;{1E} Address of page tables WRT es
	mov	dh, 4096/256	;edx = 4096
.10:	mov	cx, 1024
	mov	al, 7
.20:	stosd
	add	eax, edx
	int	8			      ;{8}
	loop	.20
	shr	eax, 2			;{4C} (first time only) 4Mb / 4 = 1Mb
	neg	bl			      ;Done just one page?
	jns	.10			   ;Yes: do one more

	cli				      ;{6}

	mov	eax, 0x9C003	;First page tbl pointer in page dir
	stosd				      ;{1H}

	mov	ax, (1024-3)*2
	xchg	ax, cx
	rep stosw				; Not dwords: Would take too many bytes of instructions...
	mov	ax, 0xD003		;0FF800000 page tbl pointer

	stosd				      ;{1F}
	mov	ah, 0xE0		   ;Page directory self pointer
	stosd				      ;{1G}
	mov	al, 0
	mov	CR3, eax		   ;Set up page directory
;	mov	eax, CR0		   
;	or	eax, 0x80010001	   ; Turn on paging and protected mode and WP
;	and eax, ~0x60000000   ; Enable caching&WB by clearing bits 29,30
	mov eax, 0x80010001
	mov	CR0, eax
	mov	cl, FLAT_DATA  ;Setup ds and es
	push	cx			      ;{5}
	pop	ds
	mov	es, cx
;	mov	fs, cx			; Added: Also set up fs and gs
;	mov	gs, cx
	jmp dword FLAT_CODE:0xFF800200		;Go

read_sectors:
; Input:
;	EAX = LBN
;	DI  = sector count
;	ES = segment
; Output:
;	EBX high half cleared
;	DL = drive #
;	EDX high half cleared
;	ESI = 0
; Clobbered:
;	BX, CX, DH

	push	ax
	push	di
	push	es

.10:	
	push	ax		 ; LBN
	push	di		 ; remaining sector count
	and		di, 0x7f ; the maximum is limited to 0x7f because of Phoenix EDD

; build packet on stack:
;    uint8_t  packet_len;
;    uint8_t  reserved1;
;    uint8_t  nsects;
;    uint8_t  reserved2;
;    uint16_t buf_offset;
;    uint16_t buf_segment;
;    uint64_t lba;
	
	mov si, sp
	sub si, 0x10
	push eax
	push word 0
	push word 0	
	push es
	push word 0
	push di			; max 0x7f sectors!
	push 0x0010
		
;/*
; * BIOS call "INT 0x13 Function 0x42" to read sectors from disk into memory
; *	Call with	%ah = 0x42
; *			%dl = drive number
; *			%ds:%si = segment:offset of disk address packet
; *	Return:
; *			%al = 0x0 on success; err code on failure
; */	
	mov	ah, 0x42
	mov	dl, [boot+BOOT]	
	push	ax
	int	13h
	pop	ax
	jnc	.30

	int	13h		;If at second you don't succeed, give up
	jc near	disk_error

.30:
	add sp, 0x10 ; remove packet	
    pop	ax
    pop si		 ; restore original sector count
	add	ax, di	 ; Advance LBN

	push di
	shl	di, 5
	mov	bx, es
	add	bx, di		;Advance segment
	mov	es, bx
	pop	di

	xchg di, si
	sub	di, si
	ja	.10

	pop	es
	pop	di
	pop	ax
	xor	si, si
	ret

; SEGMENT .text USE32

file_name db 'KERNEL  BIN'

;  subField             location
;  ------------------   --------------
;  Limit[0..15]          0..15
;  Limit[16..19]         48..51
;  Minor control bits    52..55
;  Major control bits    40..47
;  Base[0..23]           16..39
;  Base[24..31]          56..63

align 4
gdt:
	dw 0x17           ; limit
	dd BOOT + gdt     ; offset
	dw 0              ; dummy

	; hand-crafted segment descriptor: OFFSET 0, LIMIT 0xFFBFF, type code
	flat_code_desc db 0xff,0xfb,0x00,0x00,0x00,0x9a,0xcf,0x00

	; hand-crafted segment descriptor: OFFSET 0, LIMIT 0xFFFFF, type data
	flat_data_desc db 0xff,0xff,0x00,0x00,0x00,0x92,0xcf,0x00

	resb 0x1FE+$$-$   ; 2 bytes left :)
	db	0x55, 0xAA		;Standard end of boot sector
;_____________________________________________________________________________
;
;  Build/Install instructions:
;
;  *)  NASM bootf.asm
;
;   I left out most of the error handling (what do you expect for 1B1 bytes).
; For errors which it does detect, it just beeps once and hangs.  Errors (and
; unsupported conditions) which it doesn't even try to detect include:
;   a)  Not running on a 386 or better
;   b)  Active partition is not a FAT16 partition
;   c)  Root directory is more than 608Kb long.
;   d)  kernel.bin more than 4Mb long
;   e)  Total RAM less than 1MB plus actual size of TEST.BIN
; Errors which it does check for include:
;   a)  C-bit set after any int 13h
;   b)  No active partition
;   c)  No "kernel.bin" in the root directory
;
;    If this were a partition boot, you probably could leave out the code of
; reading the MBR, and finding and reading partition boot.  Then you could
; add a few error checks.  As a test boot used from a floppy, it needs to
; do that extra work and you just shouldn't use it if the basic conditions
; aren't valid.  As part of a large multi-boot (another example I hope to
; document) it could be bigger than one sector.
;_____________________________________________________________________________
;
; {} Documentation connected to specific source code sections:
;
;{0}  I wasn't sure what to assume about the CPU state when the BIOS (or
; earlier boot) transfers control here.  Probably interrupts are disabled and
; DS=0;  But I didn't assume that.  Assuming that could save a couple bytes
; of code.
;
;{1}  Memory use:
;  {A} The boot itself is at 0:7C00
;  {B} The top of the stack is 0:800
;  {C} The directory and FAT are both read in and accessed starting at 800:0
;  {D} The image is read in, then copied to 1MB (physical)
;  {E} The page tables are built at 9C000 physical
;  {F} The image is mapped to FF800000 linear   			(originally it said 0xfff80000)
;  {G} The page tables are mapped to FFC00000 linear        (originally it said 0xfffc0000)
;  {H} The first 4Mb is mapped linear = physical
;
;{2}  Most of this code runs in "big real mode".  That allows the code to make
;  int 13h (and other) BIOS calls, but also to access extended memory directly.
;
;{3}  I left out most error checks, and just beep once for the errors that are
; checked.  That makes this code suitable for learning about pmode booting and
; for test use by pmode kernel developers.  To package with an OS as end-user
; boot code, you need to either take out some of this code or load from
; some larger place, and add some real error messages.
;
;{4}  One way you might want to reduce the work of the boot is to leave the job
; of enabling A20 to the kernel startup code:
;  {A}  Don't enable A20 here.
;  {B}  Load the image at 2Mb instead of at 1Mb.  (Only odd Megabytes are
;       unusable when A20 is disabled).
;  {C}  "shr eax,1" to convert 4Mb to 2Mb.
; Note that the current version has a minimum memory requirement of 1Mb plus
; size of the image.  This change would require 2Mb plus the size of the image.
;
;{5}  The first version of this code changed the value of segment registers
; directly after switching to pmode.  It worked on all but one machine that I
; tested it on (a 386).  On that machine, on the second switch to protected
; mode, the first segment register loaded got the right selector but a bad
; descriptor.  I have read that you need a JMP after to switching to pmode, but
; successfully written MANY programs without that JMP and seen many more
; written by others.  I guess there is a case in which a delay is required (the
; usual "flush the prefetch queue" theory doesn't fit the observed facts
; because I fixed the problem by changing "mov ds,cx" to "push cx", "pop ds";
; They each take only two bytes in the prefetch queue.
;   Further testing has shown that on a 386 you may need more than a short
; instruction worth of delay after switching back to real mode.  Any memory or
; I/O read is enough, so I rearranged the code to have at least one such read
; between a change of CR0 bit 0 and a write to an sreg.  The "jmp short $+2"
; that others recommend would work because it forces a true fetch access (on
; a 386 at least).  I don't use it because I don't want to waste two bytes in
; boot code.
;
;
;{6}  Most of this code doesn't care whether interrupts are enabled or not, so
; I never enable them after the initial cli.  BIOS's correctly enable
; interrupts DURING the processing of int 13h, if they need interrupts.
; Unfortunately, some BIOSs also enable interrupts on exit from int 13h.
; Interrupts must be disable for the switch to pmode, so another cli is
; required.
;
;{7}  Only the low half of edi is used by read_sectors, so the high half
; doesn't need to be saved and restored across that use.
;
;{8}  I call int 8 (IRQ 0) many times.  This is done in case the code was
; loaded from floppy.  It tricks the BIOS into turning the floppy motor off.
; I don't like to start my pmode tests with the floppy motor on.
;
;{9}  The first entry in a GDT is never used by the CPU.  A zero selector is
; defined as being safe to put into a segment register without loading a
; descriptor from the GDT (You can't access memory through it).  I generally
; use the first 6 bytes of the GDT as a self pointer.  Once you are used to
; this convention, it makes code more readable.  The macros in gdt.inc set it
; up.
;_____________________________________________________________________________
