	TITLE	InnerInterp.asm
	.486
include listing.inc
include core.inc

.model FLAT, C

PUBLIC	HelloString		; `string'

;EXTRN	_iob:BYTE
EXTRN	_filbuf:NEAR
EXTRN	printf:NEAR
EXTRN	_chkesp:NEAR
EXTRN	fprintf:NEAR, sprintf:NEAR, fscanf:NEAR, sscanf:NEAR

EXTRN	atan2:NEAR, pow:NEAR, ldexp:NEAR, frexp:NEAR, modf:NEAR, fmod:NEAR, _ftol:NEAR
EXTRN	atan2f:NEAR, powf:NEAR, ldexpf:NEAR, frexpf:NEAR, modff:NEAR, fmodf:NEAR
EXTRN	strcpy:NEAR, strncpy:NEAR, strstr:NEAR, strcmp:NEAR, stricmp:NEAR, strchr:NEAR, strrchr:NEAR, strcat:NEAR, strncat:NEAR, strtok:NEAR
EXTRN	memcpy:NEAR, memset:NEAR, strlen:NEAR
EXTRN	fopen:NEAR, fclose:NEAR, fseek:NEAR, fread:NEAR, fwrite:NEAR, fgetc:NEAR, fputc:NEAR, ftell:NEAR, feof:NEAR

FCore		TYPEDEF		ForthCoreState
FileFunc	TYPEDEF		ForthFileInterface

CONST	SEGMENT
HelloString DB 'Hello Cruelish World!', 0aH, 00H ; `string'
CONST	ENDS

;	COMDAT _main
_TEXT	SEGMENT
_c$ = -4

_TEXT	SEGMENT

; register usage in a forthOp:
;
;	EAX		free
;	EBX		free
;	ECX		IP
;	EDX		SP
;	ESI		free
;	EDI		inner interp PC (constant)
;	EBP		core ptr (constant)

; when in a opType routine:
;	AL		8-bit opType
;	EBX		full 32-bit opcode (need to mask off top 8 bits)

; remember when calling extern cdecl functions:
; 1) they are free to stomp EAX, EBX, ECX and EDX
; 2) they are free to modify their input params on stack

; if you need more than EAX and EBX in a routine, save ECX/IP & EDX/SP in FCore at start with these instructions:
;	mov	[ebp].FCore.IPtr, ecx
;	mov	[ebp].FCore.SPtr, edx
; jump to interpFunc at end - interpFunc will restore ECX, EDX, and EDI and go back to inner loop

;-----------------------------------------------
;
; the entry macro declares a label and makes it public
;
entry	MACRO	opLabel
PUBLIC opLabel;
opLabel:
	ENDM
	
;-----------------------------------------------
;
; unaryDoubleFunc is used for dsin, dsqrt, dceil, ...
;
unaryDoubleFunc	MACRO	opLabel, func
PUBLIC opLabel;
EXTRN func:NEAR
opLabel:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	func
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	ENDM
	
;-----------------------------------------------
;
; unaryFloatFunc is used for fsin, fsqrt, fceil, ...
;
unaryFloatFunc	MACRO	opLabel, func
PUBLIC opLabel;
EXTRN func:NEAR
opLabel:
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	call	func
	add	esp, 4
	pop	ecx
	pop	edx
	fstp	DWORD PTR [edx]
	jmp	edi
	ENDM
	
;========================================

; extern void CallDLLRoutine( DLLRoutine function, long argCount, void *core, ulong flags );

CallDLLRoutine PROC near C public uses ebx ecx edx esi edi ebp,
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
	and	ecx, 0001h
	jnz CallDLL4
			
	mov	ebx, [ebp].FCore.SPtr
	sub	ebx, 4
	
	; push high part of result if 64-bit return flag set
	mov	ecx, esi
	and	ecx, 0002h
	jz CallDLL3
	mov	[ebx], edx		; return high part of result on parameter stack
	sub	ebx, 4
	
CallDLL3:
	; push low part of result
	mov	[ebx], eax
	mov	[ebp].FCore.SPtr, ebx
	
CallDLL4:
	; cleanup PC stack
	mov	ecx, esi
	and	ecx, 0004h	; stdcall calling convention flag
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
NativeAction PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR,
	opVal:DWORD
	mov	eax, opVal
	mov	ebp, DWORD PTR core
	call	native1
	ret
NativeAction ENDP

entry native1
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	esi, [ebp].FCore.ops
	mov	edi, nativeActionExit
	mov	eax, [esi+eax*4]
	jmp	eax
nativeActionExit:
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	ret
	
	
ifdef	ASM_INNER_INTERPRETER
;-----------------------------------------------
;
; extOp is used by "builtin" ops which are only defined in C++
;
;	ebx holds the opcode
;
entry extOp
	; ebx holds the opcode which was just dispatched, use its low 24-bits as index into builtinOps table of ops defined in C/C++
	mov	eax, ebx
	and	eax, 00FFFFFFh
	mov	ebx, [ebp].FCore.ops
	mov	eax, [ebx+eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	; we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
	push	ebp		; push core ptr (our save)
	push	ebp		; push core ptr (input to C routine)
	call	eax
	add	esp, 4		; discard inputs to C routine
	pop	ebp
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	eax, [ebp].FCore.state
	or	eax, eax
	jnz	extOp1		; if something went wrong
	jmp	edi			; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	ecx, [ebp].FCore.IPtr
	;mov	edx, [ebp].FCore.SPtr
	;jmp	interpLoopExit	; if something went wrong
	
extOp1:
	ret

;-----------------------------------------------
;
; extOpType is used to handle optypes which are only defined in C++
;
;	ebx holds the opcode
;
entry extOpType
	; get the C routine to handle this optype from optypeAction table in FCore
	mov	eax, ebx
	shr	eax, 24							; eax is 8-bit optype
	mov	esi, [ebp].FCore.optypeAction
	mov	eax, [esi+eax*4]				; eax is C routine to dispatch to
	; save current IP and SP	
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	; we need to push ebp twice - the C compiler feels free to overwrite its input parameters,
	; so the top copy of EBP may be trashed on return from C land
	push	ebp		; push core ptr (our save)
	and	ebx, 00FFFFFFh
	push	ebx		; push 24-bit opVal (input to C routine)
	push	ebp		; push core ptr (input to C routine)
	call	eax
	add	esp, 8		; discard inputs to C routine 
	pop	ebp
	
	; load IP and SP from core, in case C routine modified them
	; NOTE: we can't just jump to interpFunc, since that will replace edi & break single stepping
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	eax, [ebp].FCore.state
	or	eax, eax
	jnz	extOpType1	; if something went wrong
	jmp	edi			; if everything is ok
	
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	ecx, [ebp].FCore.IPtr
	;mov	edx, [ebp].FCore.SPtr
	;jmp	interpLoopExit	; if something went wrong
	
extOpType1:
	ret

;-----------------------------------------------
;
; InitAsmTables - initializes first part of optable, where op positions are referenced by constants
;
PUBLIC InitAsmTables
InitAsmTables PROC near C public uses ebx ecx edx edi ebp esi,
	core:PTR
	mov	ebp, DWORD PTR core

	; setup normal (non-debug) inner interpreter re-entry point
	;mov	ebx, interpLoop
	mov	ebx, interpLoopDebug
	mov	[ebp].FCore.innerLoop, ebx
	
	ret
InitAsmTables ENDP

;-----------------------------------------------
;
; single step a thread
;
; extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
PUBLIC InterpretOneOpFast
InterpretOneOpFast PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR,
	op:DWORD
	mov	ebx, op
	mov	ebp, DWORD PTR core
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	edi, InterpretOneOpFastExit
	; TODO: should we reset state to OK before every step?
	mov	eax, kResultOk
	mov	[ebp].FCore.state, eax
	; instead of jumping directly to the inner loop, do a call so that
	; error exits which do a return instead of branching to inner loop will work
	call	interpLoopExecuteEntry
	jmp	InterpretOneOpFastExit2

InterpretOneOpFastExit:		; this is exit for state == OK - discard the unused return address from call above
	add	esp, 4
InterpretOneOpFastExit2:	; this is exit for state != OK
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	mov	eax, [ebp].FCore.state
	ret
InterpretOneOpFast ENDP

;-----------------------------------------------
;
; inner interpreter C entry point
;
; extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
PUBLIC InnerInterpreterFast
InnerInterpreterFast PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	eax, kResultOk
	mov	[ebp].FCore.state, eax
	call	interpFunc
	ret
InnerInterpreterFast ENDP

;-----------------------------------------------
;
; inner interpreter
;	jump to interpFunc if you need to reload IP, SP, interpLoop
entry interpFunc
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	;mov	edi, interpLoopDebug
	mov	edi, [ebp].FCore.innerLoop
	jmp	edi

entry interpLoopDebug
	; while debugging, store IP,SP in corestate shadow copies after every instruction
	;   so crash stacktrace will be more accurate (off by only one instruction)
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
entry interpLoop
	mov	ebx, [ecx]		; ebx is opcode
	add	ecx, 4			; advance IP
	; interpLoopExecuteEntry is entry for executeBop - expects opcode in ebx
PUBLIC	interpLoopExecuteEntry
interpLoopExecuteEntry:
	cmp	ebx, [ebp].FCore.numOps
	jae	notNative
	mov eax, [ebp].FCore.ops
	mov	esi, [eax+ebx*4]
	jmp	esi

PUBLIC	interpLoopExit
interpLoopExit:
	mov	[ebp].FCore.state, eax
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	ret

PUBLIC	badOptype
badOptype:
	mov	eax, kForthErrorBadOpcodeType
	jmp	interpLoopErrorExit

PUBLIC	badVarOperation
badVarOperation:
	mov	eax, kForthErrorBadVarOperation
	jmp	interpLoopErrorExit
	
PUBLIC	badOpcode
badOpcode:
	mov	eax, kForthErrorBadOpcode

	
PUBLIC	interpLoopErrorExit
interpLoopErrorExit:
	; error exit point
	; eax is error code
	mov	[ebp].FCore.error, eax
	mov	eax, kResultError
	jmp	interpLoopExit
	
interpLoopFatalErrorExit:
	; fatal error exit point
	; eax is error code
	mov	[ebp].FCore.error, eax
	mov	eax, kResultFatalError
	jmp	interpLoopExit
	
; op (in ebx) is not defined in assembler, dispatch through optype table
PUBLIC notNative
notNative:
	mov	eax, ebx			; leave full opcode in ebx
	shr	eax, 24			; eax is 8-bit optype
	mov	eax, opTypesTable[eax*4]
	jmp	eax

PUBLIC nativeImmediate
nativeImmediate:
	and	ebx, 00FFFFFFh
	cmp	ebx, [ebp].FCore.numOps
	jae	badOpcode
	mov eax, [ebp].FCore.ops
	mov	esi, [eax+ebx*4]
	jmp	esi

; externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
PUBLIC externalBuiltin
externalBuiltin:
	; it should be impossible to get here now
	jmp	badOpcode
	
PUBLIC cCodeType
cCodeType:
	and	ebx, 00FFFFFFh
	; dispatch to C version if valid
	cmp	ebx, [ebp].FCore.numOps
	jae	badOpcode
	jmp	extOp

;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry userDefType
	; get low-24 bits of opcode & check validity
	and	ebx, 00FFFFFFh
	cmp	ebx, [ebp].FCore.numOps
	jge	badUserDef
	; push IP on rstack
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[eax], ecx
	mov	[ebp].FCore.RPtr, eax
	; get new IP
	mov	eax, [ebp].FCore.ops
	mov	ecx, [eax+ebx*4]
	jmp	edi

badUserDef:
	mov	eax, kForthErrorBadOpcode
	jmp	interpLoopErrorExit

;-----------------------------------------------
;
; unconditional branch ops
;
entry branchType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	branchBack
	; branch forward
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ecx, ebx
	jmp	edi

branchBack:
	or	ebx,0FF000000h
	sal	ebx, 2
	add	ecx, ebx
	jmp	edi

;-----------------------------------------------
;
; branch-on-zero ops
;
entry branchZType
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; branch-on-notzero ops
;
entry branchNZType
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jnz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; case branch ops
;
entry caseBranchType
    ; TOS: this_case_value case_selector
	mov	eax, [edx]		; eax is this_case_value
	add	edx, 4
	cmp	eax, [edx]
	jz	caseMatched
	; case didn't match - branch to next case
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ecx, ebx
	jmp	edi

caseMatched:
	add	edx, 4
	jmp	edi
	

;-----------------------------------------------
;
; branch around block ops
;
entry pushBranchType
	sub	edx, 4			; push IP (pointer to block)
	mov	[edx], ecx
	and	ebx, 00FFFFFFh	; branch around block
	sal	ebx, 2
	add	ecx, ebx
	jmp	edi

;-----------------------------------------------
;
; 24-bit constant ops
;
entry constantType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	edx, 4
	and	eax,00800000h
	jnz	constantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[edx], ebx
	jmp	edi

constantNegative:
	or	ebx, 0FF000000h
	mov	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; 24-bit offset ops
;
entry offsetType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	offsetNegative
	; positive constant
	and	ebx, 00FFFFFFh
	add	[edx], ebx
	jmp	edi

offsetNegative:
	or	ebx, 0FF000000h
	add	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; 24-bit offset fetch ops
;
entry offsetFetchType
	; get low-24 bits of opcode
	mov	eax, ebx
	and	eax, 00800000h
	jnz	offsetFetchNegative
	; positive constant
	and	ebx, 00FFFFFFh
	add	ebx, [edx]
	mov	eax, [ebx]
	mov	[edx], eax
	jmp	edi

offsetFetchNegative:
	or	ebx, 0FF000000h
	add	ebx, [edx]
	mov	eax, [ebx]
	mov	[edx], eax
	jmp	edi

;-----------------------------------------------
;
; array offset ops
;
entry arrayOffsetType
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh		; ebx is size of one element
	; TOS is array base, tos-1 is index
	imul	ebx, [edx+4]	; multiply index by element size
	add	ebx, [edx]			; add array base addr
	add	edx, 4
	mov	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; local struct array ops
;
entry localStructArrayType
   ; bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
   ; multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	mov	eax, 00000FFFh
	and	eax, ebx                ; eax is padded struct length in bytes
	imul	eax, [edx]              ; multiply index * length
	add	eax, [ebp].FCore.FPtr
	and	ebx, 00FFF000h
	sar	ebx, 10							; ebx = frame offset in bytes of struct[0]
	sub	eax, ebx						; eax -> struct[N]
	mov	[edx], eax
	jmp	edi

;-----------------------------------------------
;
; string constant ops
;
entry constantStringType
	; IP points to beginning of string
	; low 24-bits of ebx is string len in longs
	sub	edx, 4
	mov	[edx], ecx		; push string ptr
	; get low-24 bits of opcode
	and	ebx, 00FFFFFFh
	shl	ebx, 2
	; advance IP past string
	add	ecx, ebx
	jmp	edi

;-----------------------------------------------
;
; local stack frame allocation ops
;
entry allocLocalsType
	; rpush old FP
	mov	esi, [ebp].FCore.FPtr
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[eax], esi
	; set FP = RP, points at old FP
	mov	[ebp].FCore.FPtr, eax
	; allocate amount of storage specified by low 24-bits of op on rstack
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	[ebp].FCore.RPtr, eax
	; clear out allocated storage
	mov esi, eax
	xor eax, eax
alt1:
	mov [esi], eax
	add esi, 4
	sub ebx, 4
	jnz alt1
	jmp	edi

;-----------------------------------------------
;
; local string init ops
;
entry initLocalStringType
   ; bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
   ; init the current & max length fields of a local string
	mov	eax, 00FFF000h
	and	eax, ebx
	sar	eax, 10							; eax = frame offset in bytes
	mov	esi, [ebp].FCore.FPtr
	sub	esi, eax						; esi -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[esi], ebx						; set max length
	xor	eax, eax
	mov	[esi+4], eax					; set current length to 0
	mov	[esi+5], al						; add terminating null
	jmp	edi

;-----------------------------------------------
;
; local reference ops
;
entry localRefType
	; push local reference - ebx is frame offset in longs
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;-----------------------------------------------
;
; member reference ops
;
entry memberRefType
	; push member reference - ebx is member offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;-----------------------------------------------
;
; local byte ops
;
entry localByteType
	; get ptr to byte var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
byteEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localByte1
	; fetch local byte
localByteFetch:
	sub	edx, 4
	movsx	ebx, BYTE PTR [eax]
	mov	[edx], ebx
	jmp	edi

entry localUByteType
	; get ptr to byte var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ubyteEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localByte1
	; fetch local unsigned byte
localUByteFetch:
	sub	edx, 4
	xor	ebx, ebx
	mov	bl, [eax]
	mov	[edx], ebx
	jmp	edi

localByteRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localByteStore:
	mov	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localBytePlusStore:
	xor	ebx, ebx
	mov	bl, [eax]
	add	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localByteMinusStore:
	xor	ebx, ebx
	mov	bl, [eax]
	sub	ebx, [edx]
	mov	[eax], bl
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localByteActionTable:
	DD	FLAT:localByteFetch
	DD	FLAT:localByteFetch
	DD	FLAT:localByteRef
	DD	FLAT:localByteStore
	DD	FLAT:localBytePlusStore
	DD	FLAT:localByteMinusStore

localUByteActionTable:
	DD	FLAT:localUByteFetch
	DD	FLAT:localUByteFetch
	DD	FLAT:localByteRef
	DD	FLAT:localByteStore
	DD	FLAT:localBytePlusStore
	DD	FLAT:localByteMinusStore

localByte1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localByteActionTable[ebx*4]
	jmp	ebx

entry fieldByteType
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteType
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteType
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry memberUByteType
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry localByteArrayType
	; get ptr to byte var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [edx]		; add in array index on TOS
	add	edx, 4
	jmp	byteEntry

entry localUByteArrayType
	; get ptr to byte var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [edx]		; add in array index on TOS
	add	edx, 4
	jmp	ubyteEntry

entry fieldByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]
	add	eax, [edx+4]
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry fieldUByteArrayType
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]
	add	eax, [edx+4]
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

entry memberByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	add	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

entry memberUByteArrayType
	; get ptr to byte var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	add	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; local short ops
;
entry localShortType
	; get ptr to short var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
shortEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localShort1
	; fetch local short
localShortFetch:
	sub	edx, 4
	movsx	ebx, WORD PTR [eax]
	mov	[edx], ebx
	jmp	edi

entry localUShortType
	; get ptr to short var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
ushortEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localShort1
	; fetch local unsigned short
localUShortFetch:
	sub	edx, 4
	movsx	ebx, WORD PTR [eax]
	xor	ebx, ebx
	mov	bx, [eax]
	mov	[edx], ebx
	jmp	edi

localShortRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localShortStore:
	mov	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localShortPlusStore:
	movsx	ebx, WORD PTR [eax]
	add	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localShortMinusStore:
	movsx	ebx, WORD PTR [eax]
	sub	ebx, [edx]
	mov	[eax], bx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localShortActionTable:
	DD	FLAT:localShortFetch
	DD	FLAT:localShortFetch
	DD	FLAT:localShortRef
	DD	FLAT:localShortStore
	DD	FLAT:localShortPlusStore
	DD	FLAT:localShortMinusStore

localUShortActionTable:
	DD	FLAT:localUShortFetch
	DD	FLAT:localUShortFetch
	DD	FLAT:localShortRef
	DD	FLAT:localShortStore
	DD	FLAT:localShortPlusStore
	DD	FLAT:localShortMinusStore

localShort1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localShortActionTable[ebx*4]
	jmp	ebx

entry fieldShortType
	; get ptr to short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	shortEntry

entry fieldUShortType
	; get ptr to unsigned short var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ushortEntry

entry memberShortType
	; get ptr to short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	shortEntry

entry memberUShortType
	; get ptr to unsigned short var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	ushortEntry

entry localShortArrayType
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	shortEntry

entry localUShortArrayType
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh	; ebx is frame offset in longs
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx
	jmp	ushortEntry

entry fieldShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 1
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry fieldUShortArrayType
	; get ptr to short var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 1
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry

entry memberShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 1
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	shortEntry

entry memberUShortArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 1
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	ushortEntry

;-----------------------------------------------
;
; local int ops
;
entry localIntType
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
intEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localInt1
	; fetch local int
localIntFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi

localIntRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localIntStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localIntPlusStore:
	mov	ebx, [eax]
	add	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localIntMinusStore:
	mov	ebx, [eax]
	sub	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localIntActionTable:
	DD	FLAT:localIntFetch
	DD	FLAT:localIntFetch
	DD	FLAT:localIntRef
	DD	FLAT:localIntStore
	DD	FLAT:localIntPlusStore
	DD	FLAT:localIntMinusStore

localInt1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localIntActionTable[ebx*4]
	jmp	ebx

entry fieldIntType
	; get ptr to int var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	intEntry

entry memberIntType
	; get ptr to int var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	intEntry

entry localIntArrayType
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	intEntry

entry fieldIntArrayType
	; get ptr to int var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

entry memberIntArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	intEntry

;-----------------------------------------------
;
; local float ops
;
entry localFloatType
	; get ptr to float var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
floatEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localFloat1
	; fetch local float
localFloatFetch:
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi

localFloatRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localFloatStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	add	edx, 4
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localFloatPlusStore:
	fld	DWORD PTR [eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fadd	DWORD PTR [edx]
	add	edx, 4
	fstp	DWORD PTR [eax]
	jmp	edi

localFloatMinusStore:
	fld	DWORD PTR [eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fsub	DWORD PTR [edx]
	add	edx, 4
	fstp	DWORD PTR [eax]
	jmp	edi

localFloatActionTable:
	DD	FLAT:localFloatFetch
	DD	FLAT:localFloatFetch
	DD	FLAT:localFloatRef
	DD	FLAT:localFloatStore
	DD	FLAT:localFloatPlusStore
	DD	FLAT:localFloatMinusStore

localFloat1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localFloatActionTable[ebx*4]
	jmp	ebx

entry fieldFloatType
	; get ptr to float var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	floatEntry

entry memberFloatType
	; get ptr to float var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	floatEntry

entry localFloatArrayType
	; get ptr to float var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	floatEntry

entry fieldFloatArrayType
	; get ptr to float var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry

entry memberFloatArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	floatEntry
	
;-----------------------------------------------
;
; local double ops
;
entry localDoubleType
	; get ptr to double var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
doubleEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localDouble1
	; fetch local double
localDoubleFetch:
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi

localDoubleRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localDoubleStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [edx+4]
	mov	[eax+4], ebx
	add	edx, 8
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localDoublePlusStore:
	fld	QWORD PTR [eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fadd	QWORD PTR [edx]
	add	edx, 8
	fstp	QWORD PTR [eax]
	jmp	edi

localDoubleMinusStore:
	fld	QWORD PTR [eax]
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fsub	QWORD PTR [edx]
	add	edx, 8
	fstp	QWORD PTR [eax]
	jmp	edi

localDoubleActionTable:
	DD	FLAT:localDoubleFetch
	DD	FLAT:localDoubleFetch
	DD	FLAT:localDoubleRef
	DD	FLAT:localDoubleStore
	DD	FLAT:localDoublePlusStore
	DD	FLAT:localDoubleMinusStore

localDouble1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localDoubleActionTable[ebx*4]
	jmp	ebx

entry fieldDoubleType
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	doubleEntry

entry memberDoubleType
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	doubleEntry

entry localDoubleArrayType
	; get ptr to double var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp doubleEntry

entry fieldDoubleArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry

entry memberDoubleArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	doubleEntry
	
;-----------------------------------------------
;
; local string ops
;
PUBLIC localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend
entry localStringType
	; get ptr to string var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
stringEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localString1
	; fetch local string
localStringFetch:
	sub	edx, 4
	add	eax, 8		; skip maxLen & currentLen fields
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
; ref on a string returns the address of maxLen field, not the string chars
localStringRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localStringStore:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[ebp].FCore.IPtr, ecx	; IP will get stomped
	mov	esi, [edx]			; esi -> chars of src string
	add	edx, 4
	mov	[ebp].FCore.SPtr, edx
	lea	edi, [eax + 8]		; edi -> chars of dst string
	push	esi
	call	strlen
	add	esp, 4
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [edi - 8]		; ebx = max string length
	cmp	eax, ebx
	jle lsStore1
	mov	eax, ebx
lsStore1:
	; set current length field
	mov	[edi - 4], eax
	
	; do the copy
	push	eax		; push numBytes
	push	eax		; and save a copy in case strncpy modifies its stack inputs
	push	esi		; srcPtr
	push	edi		; dstPtr
	call	strncpy
	add	esp, 12
	pop	ecx			; ecx = numBytes

	; add the terminating null
	xor	eax, eax
	mov	[edi + ecx], al
		
	; set var operation back to fetch
	mov	[ebp].FCore.varMode, eax
	jmp	interpFunc

localStringAppend:
	; eax -> dest string maxLen field
	; TOS is src string addr
	mov	[ebp].FCore.IPtr, ecx	; IP will get stomped
	mov	esi, [edx]			; esi -> chars of src string
	add	edx, 4
	mov	[ebp].FCore.SPtr, edx
	lea	edi, [eax + 8]		; edi -> chars of dst string
	push	esi
	call	strlen
	add	esp, 4
	; eax is src string length

	; figure how many chars to copy
	mov	ebx, [edi - 8]		; ebx = max string length
	mov	ecx, [edi - 4]		; ecx = cur string length
	add	ecx, eax
	cmp	ecx, ebx
	jle lsAppend1
	mov	eax, ebx
	sub	eax, [edi - 4]		; #bytes to copy = maxLen - curLen
	mov	ecx, ebx			; new curLen = maxLen
lsAppend1:
	; set current length field
	mov	[edi - 4], ecx
	
	; do the append
	push	eax		; push numBytes
	push	esi		; srcPtr
	push	edi		; dstPtr
	; don't need to worry about stncat stomping registers since we jump to interpFunc
	call	strncat
	add	esp, 12

	; add the terminating null
	xor	eax, eax
	mov	ecx, [edi - 4]
	mov	[edi + ecx], al
		
	; set var operation back to fetch
	mov	[ebp].FCore.varMode, eax
	jmp	interpFunc

localStringActionTable:
	DD	FLAT:localStringFetch
	DD	FLAT:localStringFetch
	DD	FLAT:localStringRef
	DD	FLAT:localStringStore
	DD	FLAT:localStringAppend

localString1:
	cmp	ebx, kVarPlusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localStringActionTable[ebx*4]
	jmp	ebx

entry fieldStringType
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	stringEntry

entry memberStringType
	; get ptr to byte var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	stringEntry

entry localStringArrayType
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx		; eax -> maxLen field of string[0]
	mov	ebx, [eax]
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [edx]	; mult index * element length
	add	edx, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	jmp stringEntry

entry fieldStringArrayType
	; get ptr to string var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [edx]		; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [edx+4]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	edx, 8
	jmp	opEntry

entry memberStringArrayType
	; get ptr to string var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	and	ebx, 00FFFFFFh
	add	ebx, [ebp].FCore.TDPtr	; ebx -> maxLen field of string[0]
	mov	eax, [ebx]		; eax = maxLen
	sar	eax, 2
	add	eax, 3			; eax is element length in longs
	imul	eax, [edx]	; mult index * element length
	sal	eax, 2
	add	eax, ebx		; eax -> maxLen field of string[N]
	add	edx, 4
	jmp	opEntry

;-----------------------------------------------
;
; local op ops
;
entry localOpType
	; get ptr to op var into ebx
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch (execute for ops)
opEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localOp1
	; execute local op
localOpExecute:
	mov	ebx, [eax]
	jmp	interpLoopExecuteEntry


localOpActionTable:
	DD	FLAT:localOpExecute
	DD	FLAT:localIntFetch
	DD	FLAT:localIntRef
	DD	FLAT:localIntStore

localOp1:
	cmp	ebx, kVarStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localOpActionTable[ebx*4]
	jmp	ebx

entry fieldOpType
	; get ptr to op var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	opEntry

entry memberOpType
	; get ptr to op var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	opEntry

entry localOpArrayType
	; get ptr to op var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	opEntry

entry fieldOpArrayType
	; get ptr to op var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 2
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry

entry memberOpArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 2
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	opEntry
	
;-----------------------------------------------
;
; local long (int64) ops
;
entry localLongType
	; get ptr to long var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
longEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localLong1
	; fetch local double
localLongFetch:
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi

localLongRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localLongStore:
	mov	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [edx+4]
	mov	[eax+4], ebx
	add	edx, 8
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi

localLongPlusStore:
	mov	ebx, [eax]
	add	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	adc	ebx, [edx+4]
	mov	[eax+4], ebx
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	add	edx, 8
	jmp	edi

localLongMinusStore:
	mov	ebx, [eax]
	sub	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [eax+4]
	sbb	ebx, [edx+4]
	mov	[eax+4], ebx
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	add	edx, 8
	jmp	edi

localLongActionTable:
	DD	FLAT:localLongFetch
	DD	FLAT:localLongFetch
	DD	FLAT:localLongRef
	DD	FLAT:localLongStore
	DD	FLAT:localLongPlusStore
	DD	FLAT:localLongMinusStore

localLong1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localLongActionTable[ebx*4]
	jmp	ebx

entry fieldLongType
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	longEntry

entry memberLongType
	; get ptr to double var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	longEntry

entry localLongArrayType
	; get ptr to double var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp longEntry

entry fieldLongArrayType
	; get ptr to double var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry

entry memberLongArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	longEntry
	
;-----------------------------------------------
;
; local object ops
;
entry localObjectType
	; get ptr to Object var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	; see if it is a fetch
objectEntry:
	mov	ebx, [ebp].FCore.varMode
	or	ebx, ebx
	jnz	localObject1
	; fetch local Object
localObjectFetch:
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi

localObjectRef:
	sub	edx, 4
	mov	[edx], eax
	; set var operation back to fetch
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
localObjectStore:
	; TOS is new object, eax points to destination/old object
	xor	ebx, ebx			; set var operation back to default/fetch
	mov	[ebp].FCore.varMode, ebx
	mov esi, eax		; esi -> destination
	mov eax, [edx+4]	; eax = newObj data
	mov ebx, [esi+4]	; ebx = olbObj data
	cmp eax, ebx
	jz losx				; objects have same data ptr, don't change refcount
	; handle newObj refcount
	or eax,eax
	jz los1				; if newObj data ptr is null, don't try to increment refcount
	inc dword ptr[eax]	; increment newObj refcount
	; handle oldObj refcount
los1:
	or ebx,ebx
	jz los2				; if oldObj data ptr is null, don't try to decrement refcount
	dec dword ptr[ebx]
	jz los3
los2:
	mov	[esi+4], eax	; oldObj.data = newObj.data
losx:
	mov	ebx, [edx]		; ebx = newObj methods
	mov	[esi], ebx		; var.methods = newObj.methods
	add	edx, 8
	jmp	edi

	; object var held last reference to oldObj, invoke olbObj.delete method
	; eax = newObj.data
	; ebx = oldObj.data
	; [esi] = var.methods
los3:
	push edi
	push eax
	; TOS is object vtable, NOS is object data ptr
	; ebx is method number

	; push this ptr pair on return stack
	mov	edi, [ebp].FCore.RPtr
	sub	edi, 8
	mov	[ebp].FCore.RPtr, edi
	mov	eax, [ebp].FCore.TDPtr
	mov	[edi+4], eax
	mov	eax, [ebp].FCore.TMPtr
	mov	[edi], eax
	
	mov	[ebp].FCore.TDPtr, ebx
	mov	ebx, [esi]
	mov	[ebp].FCore.TMPtr, ebx
	
	mov	ebx, [ebx]	; ebx = method 0 (delete) opcode

	pop eax
	pop edi
	
	mov	[esi+4], eax	; var.data = newObj.data
	mov	eax, [edx]		; ebx = newObj methods
	mov	[esi], eax		; var.methods = newObj.methods
	add	edx, 8

	; execute the delete method opcode which is in ebx
	jmp	interpLoopExecuteEntry

; clear object reference, leave on TOS
localObjectUnref:
	; leave object on TOS
	sub	edx, 8
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]	; ebx -> object refcount
	mov	[edx+4], ebx
	; if object var is already null, do nothing else
	or	ebx, ebx
	jz	lou2
	; clear object var
	mov	esi, eax		; esi -> object var
	xor	eax, eax
	mov	[esi], eax
	mov	[esi+4], eax
	; set var operation back to fetch
	mov	[ebp].FCore.varMode, eax
	; get object refcount, see if it is already 0
	mov	eax, [ebx]
	or	eax, eax
	jnz	lou1
	; report refcount negative error
	mov	eax, kForthErrorBadReferenceCount
	jmp	interpLoopErrorExit
lou1:
	; decrement object refcount
	sub	eax, 1
	mov	[ebx], eax
lou2:
	jmp	edi

	
localObjectActionTable:
	DD	FLAT:localObjectFetch
	DD	FLAT:localObjectFetch
	DD	FLAT:localObjectRef
	DD	FLAT:localObjectStore
	DD	FLAT:badVarOperation		; '->+' not defined for object
	DD	FLAT:localObjectUnref

localObject1:
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localObjectActionTable[ebx*4]
	jmp	ebx

entry fieldObjectType
	; get ptr to Object var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	objectEntry

entry memberObjectType
	; get ptr to Object var into eax
	; this data ptr is base ptr, ebx is field offset in bytes
	mov	eax, [ebp].FCore.TDPtr
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	objectEntry

entry localObjectArrayType
	; get ptr to Object var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	mov	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx
	jmp objectEntry

entry fieldObjectArrayType
	; get ptr to Object var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx+4]	; eax = index
	sal	eax, 3
	add	eax, [edx]		; add in struct base ptr
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	objectEntry

entry memberObjectArrayType
	; get ptr to short var into eax
	; this data ptr is base ptr, TOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]	; eax = index
	sal	eax, 3
	add	eax, [ebp].FCore.TDPtr
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx		; add in field offset
	jmp	objectEntry
	
;-----------------------------------------------
;
; method invocation ops
;

; invoke a method on object currently referenced by this ptr pair
entry methodWithThisType
	; ebx is method number
	; push this ptr pair on return stack
	mov	esi, [ebp].FCore.RPtr
	sub	esi, 8
	mov	[ebp].FCore.RPtr, esi
	mov	eax, [ebp].FCore.TDPtr
	mov	[esi+4], eax
	mov	eax, [ebp].FCore.TMPtr
	mov	[esi], eax
	
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, eax
	mov	ebx, [ebx]	; ebx = method opcode
	jmp	interpLoopExecuteEntry
	
; invoke a method on an object referenced by ptr pair on TOS
entry methodWithTOSType
	; TOS is object vtable, NOS is object data ptr
	; ebx is method number
	; push this ptr pair on return stack
	mov	esi, [ebp].FCore.RPtr
	sub	esi, 8
	mov	[ebp].FCore.RPtr, esi
	mov	eax, [ebp].FCore.TDPtr
	mov	[esi+4], eax
	mov	eax, [ebp].FCore.TMPtr
	mov	[esi], eax

	; set data ptr from TOS	
	mov	eax, [edx+4]
	mov	[ebp].FCore.TDPtr, eax
	; set vtable ptr from TOS
	mov	eax, [edx]
	mov	[ebp].FCore.TMPtr, eax
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	add	ebx, eax
	mov	ebx, [ebx]	; ebx = method opcode
	add	edx, 8
	jmp	interpLoopExecuteEntry
	
;-----------------------------------------------
;
; member string init ops
;
entry memberStringInitType
   ; bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
   ; init the current & max length fields of a member string
	mov	eax, 00FFF000h
	and	eax, ebx
	sar	eax, 10							; eax = member offset in bytes
	mov	esi, [ebp].FCore.TDPtr
	add	esi, eax						; esi -> max length field
	and	ebx, 00000FFFh					; ebx = max length
	mov	[esi], ebx						; set max length
	xor	eax, eax
	mov	[esi+4], eax					; set current length to 0
	mov	[esi+8], al						; add terminating null
	jmp	edi

;-----------------------------------------------
;
; builtinOps code
;

;-----------------------------------------------
;
; doByteOp is compiled as the first op in global byte vars
; the byte data field is immediately after this op
;
entry doByteBop
	; get ptr to byte var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	byteEntry

;-----------------------------------------------
;
; doUByteOp is compiled as the first op in global unsigned byte vars
; the byte data field is immediately after this op
;
entry doUByteBop
	; get ptr to byte var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doByteArrayOp is compiled as the first op in global byte arrays
; the data array is immediately after this op
;
entry doByteArrayBop
	; get ptr to byte var into eax
	mov	eax, ecx
	add	eax, [edx]
	add	edx, 4
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	byteEntry

entry doUByteArrayBop
	; get ptr to byte var into eax
	mov	eax, ecx
	add	eax, [edx]
	add	edx, 4
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	ubyteEntry

;-----------------------------------------------
;
; doShortOp is compiled as the first op in global short vars
; the short data field is immediately after this op
;
entry doShortBop
	; get ptr to short var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	shortEntry

;-----------------------------------------------
;
; doUShortOp is compiled as the first op in global unsigned short vars
; the short data field is immediately after this op
;
entry doUShortBop
	; get ptr to short var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doShortArrayOp is compiled as the first op in global short arrays
; the data array is immediately after this op
;
entry doShortArrayBop
	; get ptr to short var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	shortEntry

entry doUShortArrayBop
	; get ptr to short var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 1
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	ushortEntry

;-----------------------------------------------
;
; doIntOp is compiled as the first op in global int vars
; the int data field is immediately after this op
;
entry doIntBop
	; get ptr to int var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	intEntry

;-----------------------------------------------
;
; doIntArrayOp is compiled as the first op in global int arrays
; the data array is immediately after this op
;
entry doIntArrayBop
	; get ptr to int var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	intEntry

;-----------------------------------------------
;
; doFloatOp is compiled as the first op in global float vars
; the float data field is immediately after this op
;
entry doFloatBop
	; get ptr to float var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doFloatArrayOp is compiled as the first op in global float arrays
; the data array is immediately after this op
;
entry doFloatArrayBop
	; get ptr to float var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	floatEntry

;-----------------------------------------------
;
; doDoubleOp is compiled as the first op in global double vars
; the data field is immediately after this op
;
entry doDoubleBop
	; get ptr to double var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doDoubleArrayOp is compiled as the first op in global double arrays
; the data array is immediately after this op
;
entry doDoubleArrayBop
	; get ptr to double var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	doubleEntry

;-----------------------------------------------
;
; doStringOp is compiled as the first op in global string vars
; the data field is immediately after this op
;
entry doStringBop
	; get ptr to string var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	stringEntry

;-----------------------------------------------
;
; doStringArrayOp is compiled as the first op in global string arrays
; the data array is immediately after this op
;
entry doStringArrayBop
	; get ptr to string var into eax
	mov	eax, ecx		; eax -> maxLen field of string[0]
	mov	ebx, [eax]		; ebx = maxLen
	sar	ebx, 2
	add	ebx, 3			; ebx is element length in longs
	imul	ebx, [edx]	; mult index * element length
	add	edx, 4
	sal	ebx, 2			; ebx is offset in bytes
	add	eax, ebx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp stringEntry

;-----------------------------------------------
;
; doOpOp is compiled as the first op in global op vars
; the op data field is immediately after this op
;
entry doOpBop
	; get ptr to int var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	opEntry

;-----------------------------------------------
;
; doOpArrayOp is compiled as the first op in global op arrays
; the data array is immediately after this op
;
entry doOpArrayBop
	; get ptr to op var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 2
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	opEntry

;-----------------------------------------------
;
; doLongOp is compiled as the first op in global int64 vars
; the data field is immediately after this op
;
entry doLongBop
	; get ptr to double var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	longEntry

;-----------------------------------------------
;
; doLongArrayOp is compiled as the first op in global int64 arrays
; the data array is immediately after this op
;
entry doLongArrayBop
	; get ptr to double var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	longEntry

;-----------------------------------------------
;
; doObjectOp is compiled as the first op in global Object vars
; the data field is immediately after this op
;
entry doObjectBop
	; get ptr to Object var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	objectEntry

;-----------------------------------------------
;
; doObjectArrayOp is compiled as the first op in global Object arrays
; the data array is immediately after this op
;
entry doObjectArrayBop
	; get ptr to Object var into eax
	mov	eax, ecx
	mov	ebx, [edx]		; ebx = array index
	add	edx, 4
	sal	ebx, 3
	add	eax, ebx	
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	objectEntry

;========================================

entry initStringBop
	;	TOS: strPtr len
	mov	ebx, [edx]		; ebx -> first char of string
	xor	eax, eax
	mov	[ebx-4], eax	; set current length = 0
	mov	[ebx], al		; set first char to terminating null
	mov	eax, [edx+4]	; eax == string length
	mov	[ebx-8], eax	; set maximum length field
	add	edx, 8
	jmp	edi

;========================================

entry doneBop
	mov	eax,kResultDone
	jmp	interpLoopExit

;========================================

entry abortBop
	mov	eax,kForthErrorAbort
	jmp	interpLoopFatalErrorExit

;========================================

entry plusBop
	mov	eax, [edx]
	add	edx, 4
	add	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
entry minusBop
	mov	eax, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sub	ebx, eax
	mov	[edx], ebx
	jmp	edi

;========================================

entry timesBop
	mov	eax, [edx]
	add	edx, 4
	imul	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times2Bop
	mov	eax, [edx]
	add	eax, eax
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times4Bop
	mov	eax, [edx]
	sal	eax, 2
	mov	[edx], eax
	jmp	edi

;========================================
	
entry times8Bop
	mov	eax, [edx]
	sal	eax, 3
	mov	[edx], eax
	jmp	edi

;========================================
	
entry divideBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD PTR [ebx]		; eax is quotient, edx is remainder
	add	ebx, 4
	mov	[ebx], eax
	mov	edx, ebx
	jmp	edi

;========================================

entry divide2Bop
	mov	eax, [edx]
	sar	eax, 1
	mov	[edx], eax
	jmp	edi
	
;========================================

entry divide4Bop
	mov	eax, [edx]
	sar	eax, 2
	mov	[edx], eax
	jmp	edi
	
;========================================

entry divide8Bop
	mov	eax, [edx]
	sar	eax, 3
	mov	[edx], eax
	jmp	edi
	
;========================================
	
entry divmodBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD PTR [ebx]		; eax is quotient, edx is remainder
	mov	[ebx+4], edx
	mov	[ebx], eax
	mov	edx, ebx
	jmp	edi
	
;========================================
	
entry modBop
	; idiv takes 64-bit numerator in edx:eax
	mov	ebx, edx
	mov	eax, [edx+4]	; get numerator
	cdq					; convert into 64-bit in edx:eax
	idiv	DWORD PTR [ebx]		; eax is quotient, edx is remainder
	add	ebx, 4
	mov	[ebx], edx
	mov	edx, ebx
	jmp	edi
	
;========================================
	
entry negateBop
	mov	eax, [edx]
	neg	eax
	mov	[edx], eax
	jmp	edi
	
;========================================
	
entry fplusBop
	fld	DWORD PTR [edx+4]
	fadd	DWORD PTR [edx]
	add	edx,4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry fminusBop
	fld	DWORD PTR [edx+4]
	fsub	DWORD PTR [edx]
	add	edx,4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry ftimesBop
	fld	DWORD PTR [edx+4]
	fmul	DWORD PTR [edx]
	add	edx,4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry fdivideBop
	fld	DWORD PTR [edx+4]
	fdiv	DWORD PTR [edx]
	add	edx,4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry fEqualsZeroBop
	fldz
	jmp	fEqualsBop1
	
entry fEqualsBop
	fld	DWORD PTR [edx]
	add	edx, 4
fEqualsBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	fEqualsBop2
	dec	ebx
fEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fNotEqualsZeroBop
	fldz
	jmp	fNotEqualsBop1
	
entry fNotEqualsBop
	fld	DWORD PTR [edx]
	add	edx, 4
fNotEqualsBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	fNotEqualsBop2
	dec	ebx
fNotEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fGreaterThanZeroBop
	fldz
	jmp	fGreaterThanBop1
	
entry fGreaterThanBop
	fld	DWORD PTR [edx]
	add	edx, 4
fGreaterThanBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	fGreaterThanBop2
	dec	ebx
fGreaterThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fGreaterEqualsZeroBop
	fldz
	jmp	fGreaterEqualsBop1
	
entry fGreaterEqualsBop
	fld	DWORD PTR [edx]
	add	edx, 4
fGreaterEqualsBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 1
	jne	fGreaterEqualsBop2
	dec	ebx
fGreaterEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fLessThanZeroBop
	fldz
	jmp	fLessThanBop1
	
entry fLessThanBop
	fld	DWORD PTR [edx]
	add	edx, 4
fLessThanBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	fLessThanBop2
	dec	ebx
fLessThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fLessEqualsZeroBop
	fldz
	jmp	fLessEqualsBop1
	
entry fLessEqualsBop
	fld	DWORD PTR [edx]
	add	edx, 4
fLessEqualsBop1:
	fld	DWORD PTR [edx]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jp	fLessEqualsBop2
	dec	ebx
fLessEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fWithinBop
	fld	DWORD PTR [edx+4]
	fld	DWORD PTR [edx+8]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	fWithinBop2
	fld	DWORD PTR [edx]
	fld	DWORD PTR [edx+8]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	fWithinBop2
	dec	ebx
fWithinBop2:
	add	edx, 8
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry fMinBop
	fld	DWORD PTR [edx]
	fld	DWORD PTR [edx+4]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	fMinBop2
	mov	eax, [edx]
	mov	[edx+4], eax
fMinBop2:
	add	edx, 4
	jmp	edi
	
;========================================
	
entry fMaxBop
	fld	DWORD PTR [edx]
	fld	DWORD PTR [edx+4]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	fMaxBop2
	mov	eax, [edx]
	mov	[edx+4], eax
fMaxBop2:
	add	edx, 4
	jmp	edi
	
;========================================
	
entry dplusBop
	fld	QWORD PTR [edx+8]
	fadd	QWORD PTR [edx]
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry dminusBop
	fld	QWORD PTR [edx+8]
	fsub	QWORD PTR [edx]
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry dtimesBop
	fld	QWORD PTR [edx+8]
	fmul	QWORD PTR [edx]
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry ddivideBop
	fld	QWORD PTR [edx+8]
	fdiv	QWORD PTR [edx]
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry dEqualsZeroBop
	fldz
	jmp	dEqualsBop1
	
entry dEqualsBop
	fld	QWORD PTR [edx]
	add	edx, 8
dEqualsBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jp	dEqualsBop2
	dec	ebx
dEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dNotEqualsZeroBop
	fldz
	jmp	dNotEqualsBop1
	
entry dNotEqualsBop
	fld	QWORD PTR [edx]
	add	edx, 8
dNotEqualsBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fucompp
	fnstsw	ax
	test	ah, 44h
	jnp	dNotEqualsBop2
	dec	ebx
dNotEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dGreaterThanZeroBop
	fldz
	jmp	dGreaterThanBop1
	
entry dGreaterThanBop
	fld	QWORD PTR [edx]
	add	edx, 8
dGreaterThanBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	dGreaterThanBop2
	dec	ebx
dGreaterThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dGreaterEqualsZeroBop
	fldz
	jmp	dGreaterEqualsBop1
	
entry dGreaterEqualsBop
	fld	QWORD PTR [edx]
	add	edx, 8
dGreaterEqualsBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 1
	jne	dGreaterEqualsBop2
	dec	ebx
dGreaterEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dLessThanZeroBop
	fldz
	jmp	dLessThanBop1
	
entry dLessThanBop
	fld	QWORD PTR [edx]
	add	edx, 8
dLessThanBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	dLessThanBop2
	dec	ebx
dLessThanBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dLessEqualsZeroBop
	fldz
	jmp	dLessEqualsBop1
	
entry dLessEqualsBop
	fld	QWORD PTR [edx]
	add	edx, 8
dLessEqualsBop1:
	fld	QWORD PTR [edx]
	add	edx, 4
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jp	dLessEqualsBop2
	dec	ebx
dLessEqualsBop2:
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dWithinBop
	fld	QWORD PTR [edx+8]
	fld	QWORD PTR [edx+16]
	xor	ebx, ebx
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	dWithinBop2
	fld	QWORD PTR [edx]
	fld	QWORD PTR [edx+16]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	dWithinBop2
	dec	ebx
dWithinBop2:
	add	edx, 20
	mov	[edx], ebx
	jmp	edi
	
;========================================
	
entry dMinBop
	fld	QWORD PTR [edx]
	fld	QWORD PTR [edx+8]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 41h
	jne	dMinBop2
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx+8], eax
	mov	[edx+12], ebx
dMinBop2:
	add	edx, 8
	jmp	edi
	
;========================================
	
entry dMaxBop
	fld	QWORD PTR [edx]
	fld	QWORD PTR [edx+8]
	fcomp	ST(1)
	fnstsw	ax
	fstp	ST(0)
	test	ah, 5
	jp	dMaxBop2
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx+8], eax
	mov	[edx+12], ebx
dMaxBop2:
	add	edx, 8
	jmp	edi
	

;========================================
	
unaryDoubleFunc	dsinBop, sin
unaryDoubleFunc	dasinBop, asin
unaryDoubleFunc	dcosBop, cos
unaryDoubleFunc	dacosBop, acos
unaryDoubleFunc	dtanBop, tan
unaryDoubleFunc	datanBop, atan
unaryDoubleFunc	dsqrtBop, sqrt
unaryDoubleFunc	dexpBop, exp
unaryDoubleFunc	dlnBop, log
unaryDoubleFunc	dlog10Bop, log10
unaryDoubleFunc	dceilBop, ceil
unaryDoubleFunc	dfloorBop, floor

;========================================

unaryFloatFunc	fsinBop, sinf
unaryFloatFunc	fasinBop, asinf
unaryFloatFunc	fcosBop, cosf
unaryFloatFunc	facosBop, acosf
unaryFloatFunc	ftanBop, tanf
unaryFloatFunc	fatanBop, atanf
unaryFloatFunc	fsqrtBop, sqrtf
unaryFloatFunc	fexpBop, expf
unaryFloatFunc	flnBop, logf
unaryFloatFunc	flog10Bop, log10f
unaryFloatFunc	fceilBop, ceilf
unaryFloatFunc	ffloorBop, floorf

;========================================
	
entry datan2Bop
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	atan2
	add	esp, 16
	pop	ecx
	pop	edx
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry fatan2Bop
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	atan2f
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry dpowBop
	; a^x
	push	edx
	push	ecx
	; push x
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	; push a
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	pow
	add	esp, 16
	pop	ecx
	pop	edx
	add	edx, 8
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
entry fpowBop
	; a^x
	push	edx
	push	ecx
	; push x
	mov	eax, [edx]
	push	eax
	; push a
	mov	eax, [edx+4]
	push	eax
	call	powf
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

entry dabsBop
	fld	QWORD PTR [edx]
	fabs
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

entry fabsBop
	fld	DWORD PTR [edx]
	fabs
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================

entry dldexpBop
	; ldexp( a, n )
	push	edx
	push	ecx
	; TOS is n (int), a (double)
	; get arg n
	mov	eax, [edx]
	push	eax
	; get arg a
	mov	eax, [edx+8]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	ldexp
	add	esp, 12
	pop	ecx
	pop	edx
	add	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

entry fldexpBop
	; ldexpf( a, n )
	push	edx
	push	ecx
	; TOS is n (int), a (float)
	; get arg n
	mov	eax, [edx]
	push	eax
	; get arg a
	mov	eax, [edx+4]
	push	eax
	call	ldexpf
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================

entry dfrexpBop
	; frexp( a, ptrToIntExponentReturn )
	sub	edx, 4
	push	edx
	push	ecx
	; TOS is a (double)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	edx
	; get arg a
	mov	eax, [edx+8]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	frexp
	add	esp, 12
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx+4]
	jmp	edi
	
;========================================

entry ffrexpBop
	; frexpf( a, ptrToIntExponentReturn )
	; get arg a
	mov	eax, [edx]
	sub	edx, 4
	push	edx
	push	ecx
	; TOS is a (float)
	; we return TOS: nInt aFrac
	; alloc nInt
	push	edx
	push	eax
	call	frexpf
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	DWORD PTR [edx+4]
	jmp	edi
	
;========================================

entry dmodfBop
	; modf( a, ptrToDoubleWholeReturn )
	mov	eax, edx
	sub	edx, 8
	push	edx
	push	ecx
	; TOS is a (double)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	modf
	add	esp, 12
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

entry fmodfBop
	; modf( a, ptrToFloatWholeReturn )
	mov	eax, edx
	sub	edx, 4
	push	edx
	push	ecx
	; TOS is a (float)
	; we return TOS: bFrac aWhole
	; alloc nInt
	push	eax
	; get arg a
	mov	eax, [edx+4]
	push	eax
	call	modff
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================

entry dfmodBop
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+12]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	fmod
	add	esp, 16
	pop	ecx
	pop	edx
	add	edx, 8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

entry ffmodBop
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	fmodf
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================

entry i2fBop
	fild	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	edi	

;========================================

entry i2dBop
	fild	DWORD PTR [edx]
	sub	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi	

;========================================

entry f2iBop
	push	edx
	push	ecx
	fld	DWORD PTR [edx]
	call	_ftol
	pop	ecx
	pop	edx
	mov	[edx], eax
	jmp	edi

;========================================

entry f2dBop
	fld	DWORD PTR [edx]
	sub	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi
		
;========================================

entry d2iBop
	push	edx
	push	ecx
	fld	QWORD PTR [edx]
	call	_ftol
	pop	ecx
	pop	edx
	add	edx,4
	mov	[edx], eax
	jmp	edi

;========================================

entry d2fBop
	fld	QWORD PTR [edx]
	add	edx, 4
	fstp	DWORD PTR [edx]
	jmp	edi

;========================================

entry doExitBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [ebp].FCore.RTPtr
	cmp	ebx, eax
	jle	doExitBop1
	mov	ecx, [eax]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi

doExitBop1:
	mov	eax, kForthErrorReturnStackUnderflow
	jmp	interpLoopErrorExit
	
;========================================

entry doExitLBop
    ; rstack: local_var_storage oldFP oldIP
    ; FP points to oldFP
	mov	eax, [ebp].FCore.FPtr
	mov	ecx, [eax]
	mov	[ebp].FCore.FPtr, ecx
	add	eax, 4
	mov	ebx, [ebp].FCore.RTPtr
	cmp	ebx, eax
	jle	doExitBop1
	mov	ecx, [eax]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================


entry doExitMBop
    ; rstack: oldIP oldTPV oldTPD
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [ebp].FCore.RTPtr
	add	eax, 12
	cmp	ebx, eax
	jl	doExitBop1
	mov	[ebp].FCore.RPtr, eax
	mov	ecx, [eax-12]	; IP = oldIP
	mov	ebx, [eax-8]
	mov	[ebp].FCore.TMPtr, ebx
	mov	ebx, [eax-4]
	mov	[ebp].FCore.TDPtr, ebx
	jmp	edi

;========================================

entry doExitMLBop
    ; rstack: local_var_storage oldFP oldIP oldTPV oldTPD
    ; FP points to oldFP
	mov	eax, [ebp].FCore.FPtr
	mov	ecx, [eax]
	mov	[ebp].FCore.FPtr, ecx
	add	eax, 16
	mov	ebx, [ebp].FCore.RTPtr
	cmp	ebx, eax
	jl	doExitBop1
	mov	[ebp].FCore.RPtr, eax
	mov	ecx, [eax-12]	; IP = oldIP
	mov	ebx, [eax-8]
	mov	[ebp].FCore.TMPtr, ebx
	mov	ebx, [eax-4]
	mov	[ebp].FCore.TDPtr, ebx
	jmp	edi
	
;========================================

entry callBop
	; rpush current IP
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[eax], ecx
	mov	[ebp].FCore.RPtr, eax
	; pop new IP
	mov	ecx, [edx]
	add	edx, 4
	jmp	edi
	
;========================================

entry gotoBop
	mov	ecx, [edx]
	jmp	edi

;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doDoBop
	mov	ebx, [ebp].FCore.RPtr
	sub	ebx, 12
	mov	[ebp].FCore.RPtr, ebx
	; @RP-2 holds top-of-loop-IP
	add	ecx, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], ecx
	; @RP-1 holds end-index
	mov	eax, [edx+4]
	mov	[ebx+4], eax
	; @RP holds current-index
	mov	eax, [edx]
	mov	[ebx], eax
	add	edx, 8
	jmp	edi
	
;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch just past end of loop (used by leave)
; 
entry doCheckDoBop
	mov	eax, [edx]		; eax is start index
	mov	esi, [edx+4]	; esi is end index
	add	edx, 8
	cmp	eax,esi
	jge	doCheckDoBop1
	
	mov	ebx, [ebp].FCore.RPtr
	sub	ebx, 12
	mov	[ebp].FCore.RPtr, ebx
	; @RP-2 holds top-of-loop-IP
	add	ecx, 4    ; skip over loop exit branch right after this op
	mov	[ebx+8], ecx
	; @RP-1 holds end-index
	mov	[ebx+4], esi
	; @RP holds current-index
	mov	[ebx], eax
doCheckDoBop1:
	jmp	edi
	
;========================================

entry doLoopBop
	mov	ebx, [ebp].FCore.RPtr
	mov	eax, [ebx]
	inc	eax
	cmp	eax, [ebx+4]
	jge	doLoopBop1	; jump if done
	mov	[ebx], eax
	mov	ecx, [ebx+8]
	jmp	edi

doLoopBop1:
	add	ebx,12
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi
	
;========================================

entry doLoopNBop
	mov	ebx, [ebp].FCore.RPtr	; ebp is RP
	mov	eax, [edx]		; pop N into eax
	add	edx, 4
	or	eax, eax
	jl	doLoopNBop1		; branch if increment negative
	add	eax, [ebx]		; eax is new i
	cmp	eax, [ebx+4]
	jge	doLoopBop1		; jump if done
	mov	[ebx], eax		; update i
	mov	ecx, [ebx+8]		; branch to top of loop
	jmp	edi

doLoopNBop1:
	add	eax, [ebx]
	cmp	eax, [ebx+4]
	jl	doLoopBop1		; jump if done
	mov	[ebx], eax		; ipdate i
	mov	ecx, [ebx+8]		; branch to top of loop
	jmp	edi
	
;========================================

entry iBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry jBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax+12]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry unloopBop
	mov	eax, [ebp].FCore.RPtr
	add	eax, 12
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

entry leaveBop
	mov	eax, [ebp].FCore.RPtr
	; point IP at the branch instruction which is just before top of loop
	mov	ecx, [eax+8]
	sub	ecx, 4
	; drop current index, end index, top-of-loop-IP
	add	eax, 12
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

entry orBop
	mov	eax, [edx]
	add	edx, 4
	or	[edx], eax
	jmp	edi
	
;========================================

entry andBop
	mov	eax, [edx]
	add	edx, 4
	and	[edx], eax
	jmp	edi
	
;========================================

entry xorBop
	mov	eax, [edx]
	add	edx, 4
	xor	[edx], eax
	jmp	edi
	
;========================================

entry invertBop
	mov	eax,0FFFFFFFFh
	xor	eax, [edx]
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lshiftBop
	mov	eax, ecx
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	shl	ebx,cl
	mov	[edx], ebx
	mov	ecx, eax
	jmp	edi
	
;========================================

entry arshiftBop
	mov	eax, ecx
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sar	ebx, cl
	mov	[edx], ebx
	mov	ecx, eax
	jmp	edi
	
;========================================

entry rshiftBop
	mov	eax, ecx
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	shr	ebx, cl
	mov	[edx], ebx
	mov	ecx, eax
	jmp	edi
	
;========================================

entry notBop
	mov	eax,0FFFFFFFFh
	xor	[edx], eax
	jmp	edi
	
;========================================

entry archX86Bop
entry trueBop
	mov	eax,0FFFFFFFFh
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry archARMBop
entry falseBop
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry nullBop
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry oclearBop
	mov	eax, kVarStore
	mov	[ebp].FCore.varMode, eax
	; fall thru to dnull
		
entry dnullBop
	xor	eax, eax
	sub	edx, 8
	mov	[edx+4], eax
	mov	[edx], eax
	jmp	edi
	
;========================================

entry equalsZeroBop
	xor	ebx, ebx
	jmp	equalsBop1
	
;========================================

entry equalsBop
	mov	ebx, [edx]
	add	edx, 4
equalsBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jnz	equalsBop2
	dec	eax
equalsBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry notEqualsZeroBop
	xor	ebx, ebx
	jmp	notEqualsBop1
	
;========================================

entry notEqualsBop
	mov	ebx, [edx]
	add	edx, 4
notEqualsBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jz	notEqualsBop2
	dec	eax
notEqualsBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry greaterThanZeroBop
	xor	ebx, ebx
	jmp	gtBop1
	
;========================================

entry greaterThanBop
	mov	ebx, [edx]		; ebx = b
	add	edx, 4
gtBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jle	gtBop2
	dec	eax
gtBop2:
	mov	[edx], eax
	jmp	edi

;========================================

entry greaterEqualsZeroBop
	xor	ebx, ebx
	jmp	geBop1
	
;========================================

entry greaterEqualsBop
	mov	ebx, [edx]
	add	edx, 4
geBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jl	geBop2
	dec	eax
geBop2:
	mov	[edx], eax
	jmp	edi
	

;========================================

entry lessThanZeroBop
	xor	ebx, ebx
	jmp	ltBop1
	
;========================================

entry lessThanBop
	mov	ebx, [edx]
	add	edx, 4
ltBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jge	ltBop2
	dec	eax
ltBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry lessEqualsZeroBop
	xor	ebx, ebx
	jmp	leBop1
	
;========================================

entry lessEqualsBop
	mov	ebx, [edx]
	add	edx, 4
leBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jg	leBop2
	dec	eax
leBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry unsignedGreaterThanBop
	mov	ebx, [edx]
	add	edx, 4
ugtBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jbe	ugtBop2
	dec	eax
ugtBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry unsignedLessThanBop
	mov	ebx, [edx]
	add	edx, 4
ultBop1:
	xor	eax, eax
	cmp	[edx], ebx
	jae	ultBop2
	dec	eax
ultBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

entry withinBop
	; tos: hiLimit loLimit value
	xor	eax, eax
	mov	ebx, [edx+8]	; ebx = value
withinBop1:
	cmp	[edx], ebx
	jle	withinFail
	cmp	[edx+4], ebx
	jg	withinFail
	dec	eax
withinFail:
	add edx, 8
	mov	[edx], eax		
	jmp	edi
	
;========================================

entry minBop
	mov	ebx, [edx]
	add	edx, 4
	cmp	[edx], ebx
	jl	minBop1
	mov	[edx], ebx
minBop1:
	jmp	edi
	
;========================================

entry maxBop
	mov	ebx, [edx]
	add	edx, 4
	cmp	[edx], ebx
	jg	maxBop1
	mov	[edx], ebx
maxBop1:
	jmp	edi
	
	
;========================================

entry rpushBop
	mov	ebx, [edx]
	add	edx, 4
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[ebp].FCore.RPtr, eax
	mov	[eax], ebx
	jmp	edi
	
;========================================

entry rpopBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rpeekBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax]
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry rdropBop
	mov	eax, [ebp].FCore.RPtr
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

entry rpBop
	lea	eax, [ebp].FCore.RPtr
	jmp	intEntry
	
;========================================

entry rzeroBop
	mov	eax, [ebp].FCore.RTPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry dupBop
	mov	eax, [edx]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

entry checkDupBop
	mov	eax, [edx]
	or	eax, eax
	jz	dupNonZeroBop1
	sub	edx, 4
	mov	[edx], eax
dupNonZeroBop1:
	jmp	edi

;========================================

entry swapBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry dropBop
	add	edx, 4
	jmp	edi
	
;========================================

entry overBop
	mov	eax, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry rotBop
	mov	eax, [edx]		; tos[0], will go in tos[1]
	mov	ebx, [edx+8]	; tos[2], will go in tos[0]
	mov	[edx], ebx
	mov	ebx, [edx+4]	; tos[1], will go in tos[2]
	mov	[edx+8], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry reverseRotBop
	mov	eax, [edx]		; tos[0], will go in tos[2]
	mov	ebx, [edx+4]	; tos[1], will go in tos[0]
	mov	[edx], ebx
	mov	ebx, [edx+8]	; tos[2], will go in tos[1]
	mov	[edx+4], ebx
	mov	[edx+8], eax
	jmp	edi
	
;========================================

entry nipBop
	mov	eax, [edx]
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry tuckBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	mov	[edx+4], ebx
	mov	[edx+8], eax
	jmp	edi
	
;========================================

entry pickBop
	mov	eax, [edx]
	add   eax, 1
	mov	ebx, [edx+eax*4]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry spBop
	; this is overkill to make sp look like other vars
	mov	ebx, [ebp].FCore.varMode
	xor	eax, eax
	mov	[ebp].FCore.varMode, eax
	cmp	ebx, kVarMinusStore
	jg	badVarOperation
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR spActionTable[ebx*4]
	jmp	ebx
	
spFetch:
	mov	eax, edx
	sub	edx, 4
	mov	[edx], eax
	jmp	edi

spRef:
	; returns address of SP shadow copy
	lea	eax, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
spStore:
	mov	ebx, [edx]
	mov	edx, ebx
	jmp	edi

spPlusStore:
	mov	eax, [edx]
	add	edx, 4
	add	edx, eax
	jmp	edi

spMinusStore:
	mov	eax, [edx]
	add	edx, 4
	sub	edx, eax
	jmp	edi

spActionTable:
	DD	FLAT:spFetch
	DD	FLAT:spFetch
	DD	FLAT:spRef
	DD	FLAT:spStore
	DD	FLAT:spPlusStore
	DD	FLAT:spMinusStore

	
;========================================

entry szeroBop
	mov	eax, [ebp].FCore.STPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry fpBop
	lea	eax, [ebp].FCore.FPtr
	jmp	intEntry
	
;========================================

entry ipBop
	; let the common intVarAction code change the shadow copy of IP,
	; then jump back to ipFixup to copy the shadow copy of IP into IP register (ecx)
	push	edi
	mov	[ebp].FCore.IPtr, ecx
	lea	eax, [ebp].FCore.IPtr
	mov	edi, ipFixup
	jmp	intEntry
	
entry	ipFixup	
	mov	ecx, [ebp].FCore.IPtr
	pop	edi
	jmp	edi
	
;========================================

entry ddupBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry dswapBop
	mov	eax, [edx]
	mov	ebx, [edx+8]
	mov	[edx+8], eax
	mov	[edx], ebx
	mov	eax, [edx+4]
	mov	ebx, [edx+12]
	mov	[edx+12], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry ddropBop
	add	edx, 8
	jmp	edi
	
;========================================

entry doverBop
	mov	eax, [edx+8]
	mov	ebx, [edx+12]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry drotBop
	mov	eax, [edx+20]
	mov	ebx, [edx+12]
	mov	[edx+20], ebx
	mov	ebx, [edx+4]
	mov	[edx+12], ebx
	mov	[edx+4], eax
	mov	eax, [edx+16]
	mov	ebx, [edx+8]
	mov	[edx+16], ebx
	mov	ebx, [edx]
	mov	[edx+8], ebx
	mov	[edx], eax
	jmp	edi
	
;========================================

entry startTupleBop
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[ebp].FCore.RPtr, eax
	mov	[eax], edx
	jmp	edi
	
;========================================

entry endTupleBop
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	sub	ebx, edx
	sub	edx, 4
	sar	ebx, 2
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry hereBop
	mov	eax, [ebp].FCore.DictionaryPtr
	mov	eax, [eax].ForthMemorySection.pCurrent
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry storeBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], ebx
	jmp	edi
	
;========================================

entry fetchBop
	mov	eax, [edx]
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry storeNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	esi, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[esi], ebx
	add	esi, 4
	mov	[eax], esi
	jmp	edi
	
;========================================

entry fetchNextBop
	mov	eax, [edx]
	mov	esi, [eax]
	mov	ebx, [esi]
	mov	[edx], ebx
	add	esi, 4
	mov	[eax], esi
	jmp	edi
	
;========================================

entry cstoreBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], bl
	jmp	edi
	
;========================================

entry cfetchBop
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bl, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry cstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	esi, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[esi], bl
	add	esi, 1
	mov	[eax], esi
	jmp	edi
	
;========================================

entry cfetchNextBop
	mov	eax, [edx]
	mov	esi, [eax]
	xor	ebx, ebx
	mov	bl, [esi]
	mov	[edx], ebx
	add	esi, 1
	mov	[eax], esi
	jmp	edi
	
;========================================

entry scfetchBop
	mov	eax, [edx]
	movsx	ebx, BYTE PTR [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry c2iBop
	mov	eax, [edx]
	movsx	ebx, al
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry wstoreBop
	mov	eax, [edx]
	mov	bx, [edx+4]
	add	edx, 8
	mov	[eax], bx
	jmp	edi
	
;========================================

entry wstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	esi, [eax]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[esi], bx
	add	esi, 2
	mov	[eax], esi
	jmp	edi
	
;========================================

entry wfetchBop
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry wfetchNextBop
	mov	eax, [edx]
	mov	esi, [eax]
	xor	ebx, ebx
	mov	bx, [esi]
	mov	[edx], ebx
	add	esi, 2
	mov	[eax], esi
	jmp	edi
	
;========================================

entry swfetchBop
	mov	eax, [edx]
	movsx	ebx, WORD PTR [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry w2iBop
	mov	eax, [edx]
	movsx	ebx, ax
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry dstoreBop
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[eax], ebx
	mov	ebx, [edx+8]
	mov	[eax+4], ebx
	add	edx, 12
	jmp	edi
	
;========================================

entry dstoreNextBop
	mov	eax, [edx]		; eax -> dst ptr
	mov	esi, [eax]
	mov	ebx, [edx+4]
	mov	[esi], ebx
	mov	ebx, [edx+8]
	mov	[esi+4], ebx
	add	esi, 8
	mov	[eax], esi
	add	edx, 12
	jmp	edi
	
;========================================

entry dfetchBop
	mov	eax, [edx]
	sub	edx, 4
	mov	ebx, [eax]
	mov	[edx], ebx
	mov	ebx, [eax+4]
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

entry dfetchNextBop
	mov	eax, [edx]
	sub	edx, 4
	mov	esi, [eax]
	mov	ebx, [esi]
	mov	[edx], ebx
	mov	ebx, [esi+4]
	mov	[edx+4], ebx
	add	esi, 8
	mov	[eax], esi
	jmp	edi
	
;========================================

entry memcpyBop
	;	TOS: nBytes srcPtr dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	memcpy
	add	esp, 12
	pop	ecx
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry memsetBop
	;	TOS: nBytes byteVal dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	and	eax, 0FFh
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	memset
	add	esp, 12
	pop	ecx
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry intoBop
	mov	eax, kVarStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry addToBop
	mov	eax, kVarPlusStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry subtractFromBop
	mov	eax, kVarMinusStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry addressOfBop
	mov	eax, kVarRef
	mov	[ebp].FCore.varMode, eax
	jmp	edi

;========================================

entry removeEntryBop
	mov	eax, kVocabRemoveEntry
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry entryLengthBop
	mov	eax, kVocabEntryLength
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry numEntriesBop
	mov	eax, kVocabNumEntries
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry vocabToClassBop
	mov	eax, kVocabGetClass
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

entry setVarActionBop
	mov   eax, [edx]
	add   edx, 4
	mov	[ebp].FCore.varMode, eax
	jmp	edi

;========================================

entry getVarActionBop
	mov	eax, [ebp].FCore.varMode
	sub   edx, 4
	mov   [edx], eax
	jmp	edi

;========================================

entry byteVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	byteEntry
	
;========================================

entry ubyteVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	ubyteEntry
	
;========================================

entry shortVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	shortEntry
	
;========================================

entry ushortVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	ushortEntry
	
;========================================

entry intVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	intEntry
	
;========================================

entry floatVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	floatEntry
	
;========================================

entry doubleVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	doubleEntry
	
;========================================

entry longVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	longEntry
	
;========================================

entry opVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	opEntry
	
;========================================

entry objectVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	objectEntry
	
;========================================

entry stringVarActionBop
	mov	eax,[edx]
	add	edx, 4
	jmp	stringEntry
	
;========================================

entry strcpyBop
	;	TOS: srcPtr dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strcpy
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 8
	jmp	edi

;========================================

entry strncpyBop
	;	TOS: maxBytes srcPtr dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	strncpy
	add	esp, 12
	pop	ecx
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry strlenBop
	mov	eax, [edx]
	xor	ebx, ebx
strlenBop1:
	mov	bh, [eax]
	test	bh, 255
	jz	strlenBop2
	add	bl, 1
	add	eax, 1
	jmp	strlenBop1

strlenBop2:
	xor	bh, bh
	mov	[edx], ebx
	jmp	edi

;========================================

entry strcatBop
	;	TOS: srcPtr dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strcat
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 8
	jmp	edi

;========================================

entry strncatBop
	;	TOS: maxBytes srcPtr dstPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx+8]
	push	eax
	call	strncat
	add	esp, 12
	pop	ecx
	pop	edx
	add	edx, 12
	jmp	edi

;========================================

entry strchrBop
	;	TOS: char strPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strchr
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strrchrBop
	;	TOS: char strPtr
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strrchr
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strcmpBop
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strcmp
strcmp1:
	xor	ebx, ebx
	cmp	eax, ebx
	jz	strcmp3		; if strings equal, return 0
	jg	strcmp2
	sub	ebx, 2
strcmp2:
	inc	ebx
strcmp3:
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

entry stricmpBop
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	stricmp
	jmp	strcmp1
	
;========================================

entry strstrBop
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strstr
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry strtokBop
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strtok
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry litBop
entry flitBop
	mov	eax, [ecx]
	add	ecx, 4
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry dlitBop
	mov	eax, [ecx]
	mov	ebx, [ecx+4]
	add	ecx, 8
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

; doDoes is executed while executing the user word
; it puts the parameter address of the user word on TOS
; top of rstack is parameter address
;
; : plusser builds , does @ + ;
; 5 plusser plus5
;
; the above 2 lines generates 3 new ops:
;	plusser
;	unnamed op
;	plus5
;
; code generated for above:
;
; plusser userOp(100) starts here
;	0	op(builds)
;	4	op(comma)
;	8	op(endBuilds)		compiled by "does"
;	12	101					compiled by "does"
; unnamed userOp(101) starts here
;	16	op(doDoes)			compiled by "does"
;	20	op(fetch)
;	24	op(plus)
;	28	op(doExit)
;
; plus5 userOp(102) starts here
;	32	userOp(101)
;	36	5
;
; ...
;	68	intCons(7)
;	72	userOp(102)		"plus5"
;	76	op(%d)
;
; we are executing some userOp when we hit the plus5 at 72
;	IP		next op			PS		RS
;--------------------------------------------
;	68		intCons(7)		()
;	72		userOp(102)		(7)		()
;	32		userOp(101)		(7)		(76)
;	16		op(doDoes)		(7)		(36,76)
;	20		op(fetch)		(36,7)	(76)
;	24		op(plus)		(5,7)	(76)
;	28		op(doExit)		(12)	(76)
;	76		op(%d)			(12)	()
;
entry doDoesBop
	mov	eax, [ebp].FCore.RPtr
	sub	edx, 4
	mov	ebx, [eax]	; ebx points at param field
	mov	[edx], ebx
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

entry doVariableBop
	; push IP
	sub	edx, 4
	mov	[edx], ecx
	; rpop new ip
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi
	
;========================================

entry doConstantBop
	; push longword @ IP
	mov	eax, [ecx]
	sub	edx, 4
	mov	[edx], eax
	; rpop new ip
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi
	
;========================================

entry doDConstantBop
	; push quadword @ IP
	mov	eax, [ecx]
	sub	edx, 8
	mov	[edx], eax
	mov	eax, [ecx+4]
	mov	[edx+4], eax
	; rpop new ip
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi
	
;========================================

entry doStructBop
	; push IP
	sub	edx, 4
	mov	[edx], ecx
	; rpop new ip
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi

;========================================

entry doStructArrayBop
	; TOS is array index
	; ecx -> bytes per element, followed by element 0
	mov	eax, [ecx]		; eax = bytes per element
	add	ecx, 4			; ecx -> element 0
	imul	eax, [edx]
	add	eax, ecx		; add in array base addr
	mov	[edx], eax
	; rpop new ip
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	edi

;========================================

entry thisBop
	mov	eax, [ebp].FCore.TMPtr
	sub	edx, 8
	mov	[edx], eax
	mov	eax, [ebp].FCore.TDPtr
	mov	[edx+4], eax
	jmp	edi
	
;========================================

entry thisDataBop
	mov	eax, [ebp].FCore.TDPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry thisMethodsBop
	mov	eax, [ebp].FCore.TMPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

entry executeBop
	mov	ebx, [edx]
	add	edx, 4
	jmp	interpLoopExecuteEntry
	
;========================================

entry	fopenBop
	push	ecx
	mov	eax, [edx]	; pop access string
	add	edx, 4
	push	edx
	push	eax
	mov	eax, [edx]	; pop pathname string
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileOpen
	call	eax
	add		sp, 8
	pop	edx
	pop	ecx
	mov	[edx], eax	; push fopen result
	jmp	edi
	
;========================================

entry	fcloseBop
	push	ecx
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileClose
	call	eax
	add	sp,4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push fclose result
	jmp	edi
	
;========================================

entry	fseekBop
	push	ecx
	push	edx
	mov	eax, [edx]	; pop control
	push	eax
	mov	eax, [edx+4]	; pop offset
	push	eax
	mov	eax, [edx+8]	; pop file pointer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileSeek
	call	eax
	add		sp, 12
	pop	edx
	pop	ecx
	add	edx, 8
	mov	[edx], eax	; push fseek result
	jmp	edi
	
;========================================

entry	freadBop
	push	ecx
	push	edx
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [edx+4]	; pop numItems
	push	eax
	mov	eax, [edx+8]	; pop item size
	push	eax
	mov	eax, [edx+12]	; pop dest pointer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileRead
	call	eax
	add		sp, 16
	pop	edx
	pop	ecx
	add	edx, 8
	mov	[edx], eax	; push fread result
	jmp	edi
	
;========================================

entry	fwriteBop
	push	ecx
	push	edx
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [edx+4]	; pop numItems
	push	eax
	mov	eax, [edx+8]	; pop item size
	push	eax
	mov	eax, [edx+12]	; pop dest pointer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileWrite
	call	eax
	add		sp, 16
	pop	edx
	pop	ecx
	add	edx, 8
	mov	[edx], eax	; push fwrite result
	jmp	edi
	
;========================================

entry	fgetcBop
	push	ecx
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileGetChar
	call	eax
	add	sp, 4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push fgetc result
	jmp	edi
	
;========================================

entry	fputcBop
	push	ecx
	mov	eax, [edx]	; pop char to put
	add	edx, 4
	push	edx
	push	eax
	mov	eax, [edx]	; pop file pointer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.filePutChar
	call	eax
	add		sp, 8
	pop	edx
	pop	ecx
	mov	[edx], eax	; push fputc result
	jmp	edi
	
;========================================

entry	feofBop
	push	ecx
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileAtEnd
	call	eax
	add	sp, 4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push feof result
	jmp	edi
	
;========================================

entry	fexistsBop
	push	ecx
	mov	eax, [edx]	; pop filename pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileExists
	call	eax
	add	sp, 4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push fexists result
	jmp	edi
	
;========================================

entry	ftellBop
	push	ecx
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileTell
	call	eax
	add	sp, 4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push ftell result
	jmp	edi
	
;========================================

entry	flenBop
	push	ecx
	mov	eax, [edx]	; pop file pointer
	push	edx
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileGetLength
	call	eax
	add	sp, 4
	pop	edx
	pop	ecx
	mov	[edx], eax	; push flen result
	jmp	edi
	
;========================================

entry	fgetsBop
	push	ecx
	push	edx
	mov	eax, [edx]	; pop file
	push	eax
	mov	eax, [edx+4]	; pop maxChars
	push	eax
	mov	eax, [edx+8]	; pop buffer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.fileGetString
	call	eax
	add		sp, 12
	pop	edx
	pop	ecx
	add	edx, 8
	mov	[edx], eax	; push fgets result
	jmp	edi
	
;========================================

entry	fputsBop
	push	ecx
	push	edx
	mov	eax, [edx]	; pop file
	push	eax
	mov	eax, [edx+4]	; pop buffer
	push	eax
	mov	eax, [ebp].FCore.FileFuncs
	mov	eax, [eax].FileFunc.filePutString
	call	eax
	add		sp, 8
	pop	edx
	pop	ecx
	add	edx, 4
	mov	[edx], eax	; push fseek result
	jmp	edi
	
;========================================

;extern void sprintfSub( ForthCoreState* pCore );
;extern void fscanfSub( ForthCoreState* pCore );
;extern void sscanfSub( ForthCoreState* pCore );

;========================================

entry fprintfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	ecx, edi
fprintfSub1:
	sub	ecx, 1
	jl	fprintfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp fprintfSub1
fprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp].FCore.SPtr, edx
	call	fprintf
	mov	edx, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp].FCore.SPtr, edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern void fprintfSub( ForthCoreState* pCore );

fprintfSub PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	edx, [ebp].FCore.SPtr
	call	fprintfSubCore
	ret
fprintfSub ENDP

;========================================

entry sprintfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	ecx, edi
sprintfSub1:
	sub	ecx, 1
	jl	sprintfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp sprintfSub1
sprintfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp].FCore.SPtr, edx
	call	sprintf
	mov	edx, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp].FCore.SPtr, edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long sprintfSub( ForthCoreState* pCore );

sprintfSub PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	edx, [ebp].FCore.SPtr
	call	sprintfSubCore
	ret
sprintfSub ENDP

;========================================

entry fscanfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	ecx, edi
fscanfSub1:
	sub	ecx, 1
	jl	fscanfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp fscanfSub1
fscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp].FCore.SPtr, edx
	call	fscanf
	mov	edx, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp].FCore.SPtr, edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
	
; extern long fscanfSub( ForthCoreState* pCore );

fscanfSub PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	edx, [ebp].FCore.SPtr
	call fscanfSubCore
	ret
fscanfSub ENDP

;========================================

entry sscanfSubCore
	mov	edi, [edx]
	add	edi, 2
	add	edx, 4
	mov	ecx, edi
sscanfSub1:
	sub	ecx, 1
	jl	sscanfSub2
	mov	ebx, [edx]
	add	edx, 4
	push	ebx
	jmp sscanfSub1
sscanfSub2:
	; all args have been moved from parameter stack to PC stack
	mov	[ebp].FCore.SPtr, edx
	call	sscanf
	mov	edx, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp].FCore.SPtr, edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret

; extern long sscanfSub( ForthCoreState* pCore );

sscanfSub PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	edx, [ebp].FCore.SPtr
	call sscanfSubCore
	ret
sscanfSub ENDP

;========================================
entry dllEntryPointType
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	mov	eax, ebx
	and	eax, 0000FFFFh
	cmp	eax, [ebp].FCore.numOps
	jge	badUserDef
	; push core ptr
	push	ebp
	; push flags
	mov	ecx, ebx
	shr	ecx, 16
	and	ecx, 7
	push	ecx
	; push arg count
	mov	ecx, ebx
	shr	ecx, 19
	and	ecx, 1Fh
	push	ecx
	; push entry point address
	mov	ecx, [ebp].FCore.ops
	mov	edx, [ecx+eax*4]
	push	edx
	call	CallDLLRoutine
	add	esp, 12
	pop	ebp
	jmp	interpFunc


;-----------------------------------------------
;
; NUM VAROP OP combo ops
;  
entry nvoComboType
	; ebx: bits 0..10 are signed integer, bits 11..12 are varop-2, bit 13..23 are opcode
	mov	eax, ebx
	sub	edx, 4
	and	eax,00000400h
	jnz	nvoNegative
	; positive constant
	mov	eax, ebx
	and	eax,003FFh
	jmp	nvoCombo1

nvoNegative:
	mov	eax, ebx
	or	eax, 0FFFFF800h			; sign extend bits 11-31
nvoCombo1:
	mov	[edx], eax
	; set the varop from bits 11-12
	shr	ebx, 11
	mov	eax, ebx
	
	and	eax, 3							; eax = varop - 2
	add	eax, 2
	mov	[ebp].FCore.varMode, eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	jmp	interpLoopExecuteEntry

;-----------------------------------------------
;
; NUM VAROP combo ops
;  
entry nvComboType
	; ebx: bits 0..21 are signed integer, bits 22-23 are varop-2
	mov	eax, ebx
	sub	edx, 4
	and	eax,00200000h
	jnz	nvNegative
	; positive constant
	mov	eax, ebx
	and	eax,001FFFFFh
	jmp	nvCombo1

nvNegative:
	mov	eax, ebx
	or	ebx, 0FFE00000h			; sign extend bits 22-31
nvCombo1:
	mov	[edx], eax
	; set the varop from bits 22-23
	shr	ebx, 22							; ebx = varop - 2
	and	ebx, 3
	add	ebx, 2
	mov	[ebp].FCore.varMode, ebx

	jmp edi

;-----------------------------------------------
;
; NUM OP combo ops
;  
entry noComboType
	; ebx: bits 0..12 are signed integer, bits 13..23 are opcode
	mov	eax, ebx
	sub	edx, 4
	and	eax,000001000h
	jnz	noNegative
	; positive constant
	mov	eax, ebx
	and	eax,00FFFh
	jmp	noCombo1

noNegative:
	or	eax, 0FFFFE000h			; sign extend bits 13-31
noCombo1:
	mov	[edx], eax
	; extract the opcode
	mov	eax, ebx
	shr	ebx, 13
	and	ebx, 0000007FFh			; ebx is 11 bit opcode
	; opcode is in ebx
	jmp	interpLoopExecuteEntry
	
;-----------------------------------------------
;
; VAROP OP combo ops
;  
entry voComboType
	; ebx: bits 0-1 are varop-2, bits 2-23 are opcode
	; set the varop from bits 0-1
	mov	eax, 000000003h
	and	eax, ebx
	add	eax, 2
	mov	[ebp].FCore.varMode, eax
	
	; extract the opcode
	shr	ebx, 2
	and	ebx, 0003FFFFFh			; ebx is 22 bit opcode

	; opcode is in ebx
	jmp	interpLoopExecuteEntry

;-----------------------------------------------
;
; OP ZBRANCH combo ops
;  
entry ozbComboType
	; ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	eax, ebx
	shr	eax, 12
	and	eax, 0FFFh
	push	eax
	push	edi
	mov	edi, ozbCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	jmp	interpLoopExecuteEntry
	
ozbCombo1:
	pop	edi
	pop	eax
	mov	ebx, [edx]
	add	edx, 4
	or	ebx, ebx
	jnz	ozbCombo2			; if TOS not 0, don't branch
	mov	ebx, eax
	and	eax, 0800h
	jnz	ozbNegative
	; positive branch
	or	ebx,0FFFFC000h

ozbNegative:
	add	ecx, ebx
ozbCombo2:
	jmp	edi
	
;-----------------------------------------------
;
; OP BRANCH combo ops
;  
entry obComboType
	; ebx: bits 0..11 are opcode, bits 12-23 are signed integer branch offset in longs
	mov	eax, ebx
	shr	eax, 12
	and	eax, 0FFFh
	push	eax
	push	edi
	mov	edi, obCombo1
	and	ebx, 0FFFh
	; opcode is in ebx
	jmp	interpLoopExecuteEntry
	
obCombo1:
	pop	edi
	pop	eax
	mov	ebx, eax
	and	eax, 0800h
	jnz	obNegative
	; positive branch
	or	ebx,0FFFFC000h

obNegative:
	add	ecx, ebx
	jmp	edi
	

;-----------------------------------------------
;
; squished float literal
;  
entry squishedFloatType
	; ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a float:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (127 - 15)) << 23
	;   mantissa = (inVal & 0x3ffff) << 5
	;   outVal = sign | exponent | mantissa
	push	ecx
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	ecx, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 112
	shl	ebx, 23			; ebx is exponent
	or	eax, ebx
	
	and	ecx, 03FFFFh
	shl	ecx, 5
	or	eax, ecx
	
	sub	edx, 4
	mov	[edx], eax
	pop	ecx
	jmp	edi
	

;-----------------------------------------------
;
; squished double literal
;  
entry squishedDoubleType
	; ebx: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	; to unsquish a double:
	;   sign = (inVal & 0x800000) << 8
	;   exponent = (((inVal >> 18) & 0x1f) + (1023 - 15)) << 20
	;   mantissa = (inVal & 0x3ffff) << 2
	;   outVal = (sign | exponent | mantissa) << 32
	push	ecx
	mov	eax, ebx
	and	eax, 00800000h
	shl	eax, 8			; sign bit
	
	mov	ecx, ebx
	shr	ebx, 18
	and	ebx, 1Fh
	add	ebx, 1008
	shl	ebx, 20			; ebx is exponent
	or	eax, ebx
	
	and	ecx, 03FFFFh
	shl	ecx, 2
	or	eax, ecx
	
	sub	edx, 4
	mov	[edx], eax
	sub	edx, 4
	; loword of double is all zeros
	xor	eax, eax
	mov	[edx], eax
	pop	ecx
	jmp	edi
	

;-----------------------------------------------
;
; squished long literal
;  
entry squishedLongType
	; get low-24 bits of opcode
	mov	eax, ebx
	sub	edx, 8
	and	eax,00800000h
	jnz	longConstantNegative
	; positive constant
	and	ebx,00FFFFFFh
	mov	[edx], ebx
	xor	ebx, ebx
	mov	[edx+4], ebx
	jmp	edi

longConstantNegative:
	or	ebx, 0FF000000h
	mov	[edx], ebx
	xor	ebx, ebx
	sub	ebx, 1
	mov	[edx+4], ebx
	jmp	edi
	

;-----------------------------------------------
;
; LOCALREF OP combo ops
;
entry lroComboType
	; ebx: bits 0..11 are frame offset in longs, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	sal	ebx, 2
	mov	eax, [ebp].FCore.FPtr
	sub	eax, ebx
	sub	edx, 4
	mov	[edx], eax
	
	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	jmp	interpLoopExecuteEntry
	
;-----------------------------------------------
;
; MEMBERREF OP combo ops
;
entry mroComboType
	; ebx: bits 0..11 are member offset in bytes, bits 12-23 are op
	push	ebx
	and	ebx, 0FFFH
	mov	eax, [ebp].FCore.TDPtr
	add	eax, ebx
	sub	edx, 4
	mov	[edx], eax

	pop	ebx
	shr	ebx, 12
	and	ebx, 0FFFH			; ebx is 12 bit opcode
	; opcode is in ebx
	jmp	interpLoopExecuteEntry


;=================================================================================================
;
;                                    opType table
;  
;=================================================================================================
entry opTypesTable
; TBD: check the order of these
; TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
;	00 - 09
	DD	FLAT:externalBuiltin		; kOpNative = 0,
	DD	FLAT:nativeImmediate		; kOpNativeImmediate,
	DD	FLAT:userDefType			; kOpUserDef,
	DD	FLAT:userDefType			; kOpUserDefImmediate,
	DD	FLAT:cCodeType				; kOpCCode,         
	DD	FLAT:cCodeType				; kOpCCodeImmediate,
	DD	FLAT:dllEntryPointType		; kOpDLLEntryPoint,
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
;	10 - 19
	DD	FLAT:branchType				; kOpBranch = 10,
	DD	FLAT:branchNZType			; kOpBranchNZ,
	DD	FLAT:branchZType			; kOpBranchZ,
	DD	FLAT:caseBranchType			; kOpCaseBranch,
	DD	FLAT:pushBranchType			; kOpPushBranch,	
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
	DD	FLAT:extOpType	
;	20 - 29
	DD	FLAT:constantType			; kOpConstant = 20,   
	DD	FLAT:constantStringType		; kOpConstantString,	
	DD	FLAT:offsetType				; kOpOffset,          
	DD	FLAT:arrayOffsetType		; kOpArrayOffset,     
	DD	FLAT:allocLocalsType		; kOpAllocLocals,     
	DD	FLAT:localRefType			; kOpLocalRef,
	DD	FLAT:initLocalStringType	; kOpLocalStringInit, 
	DD	FLAT:localStructArrayType	; kOpLocalStructArray,
	DD	FLAT:offsetFetchType		; kOpOffsetFetch,          
	DD	FLAT:memberRefType			; kOpMemberRef,	

;	30 - 39
	DD	FLAT:localByteType			; 30 - 42 : local variables
	DD	FLAT:localUByteType
	DD	FLAT:localShortType
	DD	FLAT:localUShortType
	DD	FLAT:localIntType
	DD	FLAT:localIntType
	DD	FLAT:localLongType
	DD	FLAT:localLongType
	DD	FLAT:localFloatType
	DD	FLAT:localDoubleType
	
;	40 - 49
	DD	FLAT:localStringType
	DD	FLAT:localOpType
	DD	FLAT:localObjectType
	DD	FLAT:localByteArrayType		; 43 - 55 : local arrays
	DD	FLAT:localUByteArrayType
	DD	FLAT:localShortArrayType
	DD	FLAT:localUShortArrayType
	DD	FLAT:localIntArrayType
	DD	FLAT:localIntArrayType
	DD	FLAT:localLongArrayType
	
;	50 - 59
	DD	FLAT:localLongArrayType
	DD	FLAT:localFloatArrayType
	DD	FLAT:localDoubleArrayType
	DD	FLAT:localStringArrayType
	DD	FLAT:localOpArrayType
	DD	FLAT:localObjectArrayType
	DD	FLAT:fieldByteType			; 56 - 68 : field variables
	DD	FLAT:fieldUByteType
	DD	FLAT:fieldShortType
	DD	FLAT:fieldUShortType
	
;	60 - 69
	DD	FLAT:fieldIntType
	DD	FLAT:fieldIntType
	DD	FLAT:fieldLongType
	DD	FLAT:fieldLongType
	DD	FLAT:fieldFloatType
	DD	FLAT:fieldDoubleType
	DD	FLAT:fieldStringType
	DD	FLAT:fieldOpType
	DD	FLAT:fieldObjectType
	DD	FLAT:fieldByteArrayType		; 69 - 81 : field arrays
	
;	70 - 79
	DD	FLAT:fieldUByteArrayType
	DD	FLAT:fieldShortArrayType
	DD	FLAT:fieldUShortArrayType
	DD	FLAT:fieldIntArrayType
	DD	FLAT:fieldIntArrayType
	DD	FLAT:fieldLongArrayType
	DD	FLAT:fieldLongArrayType
	DD	FLAT:fieldFloatArrayType
	DD	FLAT:fieldDoubleArrayType
	DD	FLAT:fieldStringArrayType
	
;	80 - 89
	DD	FLAT:fieldOpArrayType
	DD	FLAT:fieldObjectArrayType
	DD	FLAT:memberByteType			; 82 - 94 : member variables
	DD	FLAT:memberUByteType
	DD	FLAT:memberShortType
	DD	FLAT:memberUShortType
	DD	FLAT:memberIntType
	DD	FLAT:memberIntType
	DD	FLAT:memberLongType
	DD	FLAT:memberLongType
	
;	90 - 99
	DD	FLAT:memberFloatType
	DD	FLAT:memberDoubleType
	DD	FLAT:memberStringType
	DD	FLAT:memberOpType
	DD	FLAT:memberObjectType
	DD	FLAT:memberByteArrayType	; 95 - 107 : member arrays
	DD	FLAT:memberUByteArrayType
	DD	FLAT:memberShortArrayType
	DD	FLAT:memberUShortArrayType
	DD	FLAT:memberIntArrayType
	
;	100 - 109
	DD	FLAT:memberIntArrayType
	DD	FLAT:memberLongArrayType
	DD	FLAT:memberLongArrayType
	DD	FLAT:memberFloatArrayType
	DD	FLAT:memberDoubleArrayType
	DD	FLAT:memberStringArrayType
	DD	FLAT:memberOpArrayType
	DD	FLAT:memberObjectArrayType
	DD	FLAT:methodWithThisType
	DD	FLAT:methodWithTOSType
	
;	110 - 119
	DD	FLAT:memberStringInitType
	DD	FLAT:nvoComboType
	DD	FLAT:nvComboType
	DD	FLAT:noComboType
	DD	FLAT:voComboType
	DD	FLAT:ozbComboType
	DD	FLAT:obComboType
	
	DD	FLAT:squishedFloatType
	DD	FLAT:squishedDoubleType
	DD	FLAT:squishedLongType
	
;	120 - 121
	DD	FLAT:lroComboType
	DD	FLAT:mroComboType
	
;	122 - 149
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
;	150 - 199
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
;	200 - 249
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
;	250 - 255
	DD	FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType,FLAT:extOpType
	
endOpTypesTable:
	DD	0
	
endif

_TEXT	ENDS
END
