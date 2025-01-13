; Assembled with Microsoft (R) Macro Assembler Version 6.11
;
; masm stub.asm
; link /KNOWEAS stub.obj

.model small
.stack 2137h
.data
message db 'Hey! Azurre cannot be run in DOS mode. Please use Windows 10 or 11 for that!', 0h;
charBackup db 0
char db 0
row db 0
column db 0
cursorRow db 0
cursorColumn db 0
charColor db 1

.code
getCursor proc
	mov ah, 03h				; Function 03h: Get cursor position
    mov bh, 00h				; Page number 0
    int 10h					; Call BIOS video service
    mov cursorRow, dh		; Store row in cursorRow
    mov cursorColumn, dl	; Store column in cursorColumn
	ret
getCursor endp

hideCursor proc
    mov ah, 01h				; Function 01h: Set cursor shape
    mov cx, 2607h			; Set cursor to blank
    int 10h					; Call BIOS video service
	ret						; Return
hideCursor endp

showCursor proc
    mov ah, 01h				; Function 01h: Set cursor shape
    mov cx, 0607h			; Set cursor to default one
    int 10h					; Call BIOS video service
	ret						; Return
showCursor endp

setCursor proc
	mov ah, 02h				; Function 02h: Set cursor position
	mov bh, 00h				; Page number 0
	mov dh, row				; Row number 5
	mov dl, column			; Column number 10
	int 10h					; Call BIOS video service
	ret						; Return
setCursor endp

exit proc
	mov dh, cursorRow		; Set dh to default cursor row
    mov dl, cursorColumn	; Set dl to default cursor column
	mov row, dh				; Set row to dh
    mov column, dl			; Set column to dl
	call showCursor			; Call show cursor function
	call setCursor			; Call set cursor function to set old cursor position
	
    mov ah, 4Ch				; Function 4Ch: Terminate process with return code
    int 21h					; Call DOS Function Dispatcher
exit endp

sleep proc
	; unsigned cx = 0x000F; 
	; unsigned dx = 0x4240;
	; res = 1000000 // 1 second

	; unsigned cx = 0x0007; 
	; unsigned dx = 0xA120;
	; res = 500000 // 0.500 second

	; unsigned cx = 0x0003; 
	; unsigned dx = 0xD090;
	; res = 250000 // 0.250 second
	
	; unsigned cx = 0x0001; 
	; unsigned dx = 0xE848;
	; res = 125000 // 0.125 second
	
	; unsigned cx = 0x0001; 
	; unsigned dx = 0x0001;
	; res = 65537 // 0.065537 second
	
	; unsigned res = (cx << 0x10) | dx;

	mov cx, 0000h			; Set cx to 1
	mov dx, 5555d			; Set dx to 1
	mov ah, 86h				; Function 86h: Elapsed time wait (AT and after)
	int 15h					; Call System BIOS Services
	ret						; Return
sleep endp

beep proc ; Print the bell character to make a beep
    mov ah, 0Eh       ; BIOS function to write character in TTY mode
    mov al, 7		  ; Load AL with the bell character
    int 10h           ; Call BIOS interrupt
beep endp

color proc
	mov dl, charColor		; Set dl to charColor
	inc dl					; Increase dl by 1
	cmp dl, 0Fh				; Compare dl with 8Fh
	je resetColor			; If equals, call resetColor
	con:
	mov charColor, dl		; Set charColor to dl
	mov ah, 09h				; Function 09h: Write character and attribute at cursor position
    mov al, char       		; Load character to print
    mov bh, 0				; Page number (usually 0)
    mov bl, charColor		; Attribute byte
    mov cx, 1				; Number of times to write the character
    int 10h					; Call BIOS interrupt
	ret						; Return
resetColor:
	mov dl, 01h				; Set dl to 81h
	jmp con					; Jump con
	ret						; Return
color endp

; Didn't bother to comment this too

animateString proc
	loopS:	
		call setCursor
		
		mov ah, 08h; Get char at cur pos
		mov bh, 00h
		int 10h
		mov charBackup, al
		
		mov dl, char
		cmp dl, 00h
		je exit
		mov ah, 02h
		int 21h
		call setCursor
		call sleep
		inc row
		mov al, row
		mov ah, cursorRow
		cmp ah, al
		je return
		mov dl, charBackup
		mov ah, 02h
		int 21h
		jmp  loopS
		
	return:
		push ax
		push bx
		push cx
		push dx
		call color
		call beep ; this changes color palette to blue - white - pink. Don't really care, looks bettter
		pop dx
		pop cx
		pop bx
		pop ax
		ret
animateString endp

main proc
    mov ax, @data
    mov ds, ax
	
	call getCursor
	
	inc cursorRow
	call hideCursor
	mov bx, offset message
	loopMessage:
		mov row, 00
		call setCursor
		mov dl, 'v'
		mov ah, 02h
		int 21h
		mov row, 01
		mov  dl, [bx]
		mov char, dl
		cmp dl, ' '
		je skip
		call animateString
		skip:
		mov row, 00
		call setCursor
		mov dl, ' '
		mov ah, 02h
		int 21h
		inc bx
		inc column
		mov row, 01
		cmp dl, 0h
		jne loopMessage
	call exit
main endp
end main