;
; This code is executed after the kernel has been read in from floppy, but
; before PM is set up
; 
; The intention is to be able to make calls to real mode BIOS functions
; such as 15h, 0xE820 for determining the amount of available memory
;_____________________________________________________________________________


; Offset where this image will be loaded
%define BOOT 0x100000

bootphase2:
	pushad				; keep 32-bit registers as bootsec.asm expects them
	push	ES
	push	DS			; Save these, and keep stack 32-bit aligned

	; see http://www.uruk.org/orig-grub/mem64mb.html
	xor		EBX, EBX	  	; first call
	mov		ES, BX			; Clear ES
	mov		EDI, 0x7C00		; ES:DI buffer for result; overwrites boot
	mov		ECX, 20		  	; size, minimal 20 bytes

next:
	mov		EAX, 0xE820	; Need to set every time, BIOS returns EAX='PAMS'

	; Should be 'SMAP', but comparing to bochs/linux code this is reverse...	
	mov		EDX, 'PAMS'	; some BIOSes destroy this upon return...
	int		15h
	jc		.done		; Carry means either not supported, or last entry
	add		DI, CX		; Move buffer ptr (by size reported by BIOS)
	test	EBX, EBX	; Upon last entry, EBX is 0
	jnz		next		; Could also check EAX=='PAMS'
	
 						  ; Could store the count somewhere, or parse until 0
.done:	
	mov	dword [ES:DI], -1 	; Make sure parser can detect end of map

; For now, disable this code
	jmp		no_13h_ext 

	
; Detect available ATA devices using the BIOS - easier&safer than poking at registers
	mov		AH, 41h
	mov		BX, 0x55aa
	mov		DL, 80h			; first HDD
	int		13h				; Check presence of extended 13h BIOS functions
	mov word [0x7800], 0	; Set size to 0, in case this is not supported
	jc		no_13h_ext

; Put the HDD tables at 0x7800, 400h bytes before start of E820 map should be enough
	mov		ESI, 0x7800		; This function uses SI rather than DI
	xor		BX, BX
	mov		DS, BX
	mov		DL, 80h			; HDD drive 0
next_disc:
	mov word [DS:SI], 42h	; Size required for v3.0
	mov		AH, 48h			; Get drive parameters	
	int		13h
	jc		no_13h_ext
	add si, [DS:SI]			; read size returned, advance buffer ptr
	inc		DL
	cmp		DL, 88h			; Detect up to 8 devices
	jb		next_disc
	mov word [DS:SI], 0h	; Mark end of records

no_13h_ext:	
	pop	DS
	pop	ES
	popad					; restore
	retf					; back to bootsec.asm
	
	resb 0x200+$$-$   	; Make the binary image exactly 0x200 bytes long