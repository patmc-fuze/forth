	TITLE	AsmCore.asm
	.486
include listing.inc
include core.inc

.model FLAT, C


FCore		TYPEDEF		ForthCoreState
FileFunc	TYPEDEF		ForthFileInterface

;	COMDAT _main
_TEXT	SEGMENT
_c$ = -4

_TEXT	SEGMENT

;========================================

; extern void CallDLLRoutine( DLLRoutine function, long argCount, ulong flags, void *core );

PUBLIC CallDLLRoutine
CallDLLRoutine PROC near C public uses ebx esi edx ecx edi ebp,
	funcAddr:PTR,
	argCount:DWORD,
	flags:DWORD,
	core:PTR
	mov	eax, DWORD PTR funcAddr
	mov	edi, argCount
	mov esi, flags
	mov	ebp, DWORD PTR core
	mov	edx, [ebp].FCore.SPtr
	mov	ecx, edi
CallDLL1:
	sub	ecx, 1
	jl	CallDLL2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp CallDLL1
CallDLL2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp].FCore.SPtr, edx
	
	call	eax
	
	; handle void return flag
	mov	ecx, esi
	and	esi, 0001h
	jnz CallDLL4
			
	mov	ebx, [ebp].FCore.SPtr
	sub	ebx, 4
	
	; push high part of result if 64-bit return flag set
	mov	esi, ecx
	and	esi, 0002h
	jz CallDLL3
	mov	[ebx], edx		; return high part of result on parameter stack
	sub	ebx, 4
	
CallDLL3:
	; push low part of result
	mov	[ebx], eax
	mov	[ebp].FCore.SPtr, ebx
	
CallDLL4:
	; cleanup PC stack
	mov	esi, ecx
	and	esi, 0004h	; stdcall calling convention flag
	jnz CallDLL5
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
CallDLL5:
	ret
CallDLLRoutine ENDP


; extern void NativeAction( ForthCoreState *pCore, ulong opVal );
;-----------------------------------------------
;
; inner interpreter entry point for ops defined in assembler
;
PUBLIC	NativeAction
NativeAction PROC near C public uses ebx esi edx ecx edi ebp,
	core:PTR,
	opVal:DWORD
	mov	eax, opVal
	mov	ebp, DWORD PTR core
	call	native1
	ret
NativeAction ENDP

PUBLIC native1
native1:
	mov	esi, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	ecx, [ebp].FCore.ops
	mov	edi, nativeActionExit
	mov	eax, [ecx+eax*4]
	jmp	eax
nativeActionExit:
	mov	[ebp].FCore.IPtr, esi
	mov	[ebp].FCore.SPtr, edx
	ret
	
_TEXT	ENDS
END
