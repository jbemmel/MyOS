
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

DBLN	equ	21		;Number of lines to display

;>   cs = 0
;>>  dl = drive we were booted from

boot:	jmp short start			;Standard start of boot sector
	nop
	; resb	0x3B			      ;Skip over parameters (set by format)
	incbin "usb_bootsec.bin", 3, 0x3b

start:
  cli
	push	ss		;Setup ds and es
	pop	ds
	push	ss
	pop	es
	mov	edi, 0xB8000	;Point at display
	mov	esi, reg_list	;List of register names
	mov	ebp, esp	;Point at stack contents
	mov	bl, DBLN	;Lines to display
	mov	bh, 0
.1:	mov	eax, ebp	;Display address
	call	hex8
	mov	eax, [ebp]	;Display contents
	call	hex8
	add	ebp, 4		;Next address
	mov	cl, [0x44A]	;Screen width
	sub	cl, 18		;Minus 18 characters used
	cmp byte [esi], al	;Past end of reg_list?
	jz	near .3		;Yes

	cmp dword [esi], 'eip'	;Is it eip?
	jnz	.2		;No
	mov	edx, [ebp+4]	;Flags
	and	edx, 0xFFFE802A
	cmp	edx, 0x20002	;V86 mode?
	je	.15		;possibly
	cmp	edx, 2		;Really flags?
	jne	.3		;No
	cmp word [ebp], FLAT_CODE ;Segment correct?
	jne	.3		;No
	cmp word [ebp-2], 0xFF80 ;eip correct?
	jne	.3		;No
	movzx	edx, bl		;words left
	lea	edx, [ebp+edx*4]
	push	edx
	mov	edx, [ebp-4]	;Remember eip
	mov byte [vregs], 0
	jmp	.19
.15:	cmp word [ebp-2], 0	;V86 eip?
	jnz	.3		;No
	cmp word [ebp+10], 0	;V86 esp?
	jnz	.3		;No
	movzx	edx, word [ebp+12]
	shl	edx, 4
	add	edx, [ebp+8]
	push	edx
	movzx	edx, word [ebp]	;cs
	shl	edx, 4
	add	edx, [ebp-4]
.19:	inc	bh
.2:	lodsb			;Display register name
	stosw
	test	al, al		;  null terminated
	loopnz	.2
	
.3:	rep stosw		;Clear to start of next line
	dec	bl		;Count lines
	jnz near .1

	dec	bh
	js	.99
	mov	edi, 0xB8000+50
	call	hexlist
	pop	edx
	mov	edi, 0xB8000+68
	call	hexlist
.99:	jmp short $		;Hang

hexlist:
	mov	eax, edx
	call	hex8	
	mov	cl, [0x44A]	;Screen width
	lea	edi, [edi+ecx*2]
	mov	bl, DBLN-2
.10:	mov	cl, [0x44A]	;Screen width
	lea	edi, [edi+ecx*2-18]
	mov	eax, [edx]
	add	edx, 4
	call	hex8
	dec	bl
	jnz	.10
	ret

hex8:
;Purpose:
;	Converts value of eax in hex into display buffer pointed to by edi
;Inputs:
;	eax value
;	edi display pointer
;Outputs:
;	edi advanced
;	ecx zero
;	ax  0x700
;_____________________________________________________________________________

	mov	ecx, 8		;Count eight digits
.1:	rol	eax, 4		;Next digit
	push	eax		;Save
	and	al, 0xF		;Convert nibble to hex ascii
	cmp	al, 10
	sbb	al, 0x69
	das
	mov	ah, 7		;Default screen attribute
	stosw			;Store it and advance edi
	pop	eax		;Restore
	loop	.1		;Eight digits
	mov	ax, 0x700	;Invisible character, default attribute
	stosw			;Store it and advance edi
	ret			;Return

SEGMENT DATA
reg_list db	"Vector",0	;Names for items in the stack dump
	db	"es",0
	db	"ds",0
	db	"eax",0
	db	"ecx",0
	db	"edx",0
	db	"ebx",0
	db	" ",0		;Useless item
	db	"ebp",0
	db	"esi",0
	db	"edi",0
	db	"eip",0
	db	"cs",0
	db	"flags",0
vregs:
	db	"esp",0
	db	"ss",0
	db	"es",0
	db	"ds",0
	db	"fs",0
	db	"gs",0
	db	0		;Extra null terminates list

	resb 0x1FE+$$-$   
	db	0x55, 0xAA		;Standard end of boot sector
