	TITLE	M:\Prj\AsmScratch\AsmScratchSub.asm
	.486
include listing.inc
include core.inc

.model FLAT, C

PUBLIC	HelloString		; `string'

PUBLIC	abortBop, dropBop, doDoesBop, litBop, flitBop, dlitBop, doVariableBop, doConstantBop, doDConstantBop;
PUBLIC	endBuildsBop, doneBop, doByteBop, doShortBop, doIntBop, doIntArrayBop, doFloatBop, doDoubleBop, doStringBop, doOpBop, intoBop, doDoBop;
PUBLIC	doLoopBop, doLoopNBop, doExitBop, doExitLBop, doExitMBop, doExitMLBop, doVocabBop, initStringBop, initStringArrayBop, plusBop;
PUBLIC	minusBop, timesBop;
PUBLIC	times2Bop, times4Bop, divideBop, divide2Bop, divide4Bop, divmodBop, modBop, negateBop;
PUBLIC	fplusBop, fminusBop, ftimesBop, fdivideBop, dplusBop, dminusBop, dtimesBop, ddivideBop;
PUBLIC	dsinBop, dasinBop, dcosBop, dacosBop, dtanBop, datanBop, datan2Bop, dexpBop;
PUBLIC	dlnBop, dlog10Bop, dpowBop, dsqrtBop, dceilBop, dfloorBop, dabsBop, dldexpBop;
PUBLIC	dfrexpBop, dmodfBop, dfmodBop, i2fBop, i2dBop, f2iBop, f2dBop, d2iBop;
PUBLIC	d2fBop, callBop, gotoBop, doBop, loopBop, loopNBop, iBop, jBop;
PUBLIC	unloopBop, leaveBop, ifBop, elseBop, endifBop, beginBop, untilBop, whileBop;
PUBLIC	repeatBop, againBop, caseBop, ofBop, endofBop, endcaseBop, orBop, andBop;
PUBLIC	xorBop, invertBop, lshiftBop, rshiftBop, notBop, trueBop, falseBop, nullBop;
PUBLIC	equalsBop, notEqualsBop, greaterThanBop, greaterEqualsBop, lessThanBop, lessEqualsBop, equalsZeroBop, notEqualsZeroBop;
PUBLIC	greaterThanZeroBop, greaterEqualsZeroBop, lessThanZeroBop, lessEqualsZeroBop, rpushBop, rpopBop, rdropBop, rpBop, rzeroBop;
PUBLIC	dupBop, swapBop, overBop, rotBop, tuckBop, pickBop, rollBop, spBop, szeroBop, fpBop, ddupBop, dswapBop, ddropBop, doverBop, drotBop;
PUBLIC	alignBop, allotBop, callotBop, commaBop, cCommaBop, hereBop, mallocBop, freeBop, storeBop;
PUBLIC	fetchBop, cstoreBop, cfetchBop, scfetchBop, c2lBop, wstoreBop, wfetchBop, swfetchBop, w2lBop, dstoreBop, dfetchBop, memcpyBop, memsetBop, addToBop;
PUBLIC	subtractFromBop, addressOfBop, setVarActionBop, getVarActionBop, strcpyBop, strncpyBop, strlenBop, strcatBop, strncatBop, strchrBop, strrchrBop;
PUBLIC	strcmpBop, stricmpBop, strstrBop, strtokBop, buildsBop, doesBop, newestSymbolBop, exitBop, semiBop, colonBop, createBop;
PUBLIC	forgetBop, autoforgetBop, definitionsBop, forthVocabBop, vocabularyBop, variableBop;
PUBLIC	constantBop, dconstantBop, byteBop, shortBop, intBop, floatBop, doubleBop, stringBop;
PUBLIC	recursiveBop, precedenceBop, loadBop, loadDoneBop, interpretBop, stateInterpretBop, stateCompileBop, stateBop, tickBop;
PUBLIC	executeBop, compileBop, bracketTickBop, printNumBop, printNumDecimalBop, printNumHexBop, printStrBop, printCharBop;
PUBLIC	printSpaceBop, printNewlineBop, printFloatBop, printDoubleBop, printFormattedBop, baseBop, decimalBop, hexBop;
PUBLIC	printDecimalSignedBop, printAllSignedBop, printAllUnsignedBop, outToFileBop, outToScreenBop, outToStringBop, outToOpBop, getConOutFileBop, fopenBop;
PUBLIC	fcloseBop, fseekBop, freadBop, fwriteBop, fgetcBop, fputcBop, feofBop, ftellBop;
PUBLIC	stdinBop, stdoutBop, stderrBop, dstackBop, drstackBop, vlistBop, systemBop, chdirBop, byeBop;
PUBLIC	argvBop, argcBop;
PUBLIC	blwordBop, wordBop;
PUBLIC	getInBufferBaseBop, getInBufferPointerBop, setInBufferPointerBop, getInBufferLengthBop, fillInBufferBop, turboBop, statsBop;

EXTRN	_iob:BYTE
EXTRN	_filbuf:NEAR
EXTRN	printf:NEAR
EXTRN	_chkesp:NEAR

EXTRN	sin:NEAR, asin:NEAR, cos:NEAR, acos:NEAR, tan:NEAR, atan:NEAR, atan2:NEAR, exp:NEAR, log:NEAR, log10:NEAR
EXTRN	pow:NEAR, sqrt:NEAR, ceil:NEAR, floor:NEAR, ldexp:NEAR, frexp:NEAR, modf:NEAR, fmod:NEAR, _ftol:NEAR
EXTRN	strcpy:NEAR, strncpy:NEAR, strstr:NEAR, strcmp:NEAR, stricmp:NEAR, strchr:NEAR, strrchr:NEAR, strcat:NEAR, strncat:NEAR, strtok:NEAR
EXTRN	memcpy:NEAR, memset:NEAR, strlen:NEAR


FCore		TYPEDEF		ForthCoreState

CONST	SEGMENT
HelloString DB 'Hello Cruelish World!', 0aH, 00H ; `string'
CONST	ENDS

;	COMDAT _main
_TEXT	SEGMENT
_c$ = -4

_TEXT	SEGMENT

;	EAX		free
;	EBX		free
;	ECX		IP
;	EDX		SP
;	ESI		builtinOp dispatch table
;	EDI		inner interp PC
;	EBP		core ptr


numBuiltins	EQU	512

;-----------------------------------------------
;
; the extOp macro allows us to call forthops written in C from assembly
;
extOp	MACRO	func
EXTRN	func:NEAR
	mov	eax, func
	jmp	callExtOp
	endm

callExtOp:
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	push	ebp		; push core ptr
	call	eax
	pop	ebp
	mov	eax, [ebp].FCore.state
	or	eax, eax
	jz	interpFunc		; if everything is ok
; NOTE: Feb. 14 '07 - doing the right thing here - restoring IP & SP and jumping to
; the interpreter loop exit point - causes an access violation exception ?why?
	;mov	ecx, [ebp].FCore.IPtr
	;mov	edx, [ebp].FCore.SPtr
	;jmp	interpLoopExit	; if something went wrong
	ret

;-----------------------------------------------
;
; inner interpreter C entry point
;
; TBD: fill unused opTypesTable slots with badOptype
; TBD: fill unused builtinOps slots with ???
; TBD: move numBuiltins to core.inc
;
InitAsmTables PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core

	; fill builtinOp table with badOpcode
	mov	eax, [ebp].FCore.builtinOps
	mov	ecx, numBuiltins
InitAsm1:
	mov	[eax], badOpcode
	add	eax, 4
	sub	ecx, 1
	jnz	InitAsm1

	; fill in builtinOps table from opsTable
	mov	eax, [ebp].FCore.builtinOps
	mov	ebx, opsTable
InitAsm2:
	mov	edx, [ebx]
	add	ebx, 4
	or	edx, edx
	jz	InitAsm3
	mov	[eax], edx
	add	eax, 4
	jmp	InitAsm2

	; fill optype table with badOptype
InitAsm3:
	lea	eax, [ebp].FCore.optypeAction
	mov	ecx, 256
InitAsm4:
	mov	[eax], badOptype
	add	eax, 4
	sub	ecx, 1
	jnz	InitAsm4
	
	; fill in optype dispatch table
	lea	eax, [ebp].FCore.optypeAction
	mov	ebx, opTypesTable
InitAsm5:
	mov	edx, [ebx]
	add	ebx, 4
	or	edx, edx
	jz	InitAsm6
	mov	[eax], edx
	add	eax, 4
	jmp	InitAsm5
InitAsm6:
	ret

InitAsmTables ENDP


;-----------------------------------------------
;
; inner interpreter C entry point
;
; extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
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
;
PUBLIC	interpFunc
interpFunc:
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	esi, [ebp].FCore.builtinOps
	mov	edi, interpLoop

PUBLIC	interpLoop
interpLoop:
	mov	eax, [ecx]		; eax is opcode
	add	ecx, 4			; advance IP
	; interpLoopExecuteEntry is entry for executeBop - expects opcode in eax
PUBLIC	interpLoopExecuteEntry
interpLoopExecuteEntry:
	cmp	eax,numBuiltins
	jge	notBuiltin
	mov	eax, [esi+eax*4]
	jmp	eax

PUBLIC	interpLoopDebug
interpLoopDebug:
	mov	eax, [ecx]		; eax is opcode
	add	ecx, 4			; advance IP
	cmp	eax, numBuiltins
	jge	notBuiltin
	cmp	eax, 256
	jl	badOpcode
; we should add checking of SP, RP etc. here
	mov	eax, [esi+eax*4]
	jmp	eax

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
	
; op is not a builtin, dispatch through optype table
notBuiltin:
	mov	ebx, eax			; leave full opcode in ebx
	shr	eax, 24			; eax is 8-bit optype
	mov	eax, [ebp+eax*4]
	jmp	eax

; extern void UserCodeAction( ForthCoreState *pCore, ulong opVal );
;-----------------------------------------------
;
; inner interpreter entry point for user ops defined in assembler
;
PUBLIC	UserCodeAction
UserCodeAction PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR,
	opVal:DWORD
; TBD!
	mov	ebp, DWORD PTR core
	mov	eax, opVal
	; TBD: fetch dispatch address from userOps
	mov	ecx, [ebp].FCore.IPtr
	mov	edx, [ebp].FCore.SPtr
	mov	esi, [ebp].FCore.userOps
	mov	edi, userCodeFuncExit
	mov	eax, [esi+eax*4]
	jmp	eax
userCodeFuncExit:
	ret
UserCodeAction ENDP

;-----------------------------------------------
;
; user-defined ops
;
userDefType:
	; get low-24 bits of opcode & check validity
	and	ebx, 00FFFFFFh
	cmp	ebx, [ebp].FCore.numUserOps
	jge	badUserDef
	; push IP on rstack
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[eax], ecx
	mov	[ebp].FCore.RPtr, eax
	; get new IP
	mov	eax, [ebp].FCore.userOps
	mov	ecx, [eax+ebx*4]
	jmp	edi

badUserDef:
	mov	eax, kForthErrorBadOpcode
	jmp	interpLoopErrorExit

;-----------------------------------------------
;
; user-defined code ops
;
userCodeType:
	; get low-24 bits of opcode & check validity
	and	ebx, 00FFFFFFh
	cmp	ebx, [ebp].FCore.numUserOps
	jge	badUserDef
	mov	eax,[ebp].FCore.userOps
	mov	eax, [eax+ebx*4]
	jmp	eax
	
;-----------------------------------------------
;
; unconditional branch ops
;
branchType:
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
branchZType:
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; branch-on-notzero ops
;
branchNZType:
	mov	eax, [edx]
	add	edx, 4
	or	eax, eax
	jnz	branchType		; branch taken
	jmp	edi	; branch not taken

;-----------------------------------------------
;
; case branch ops
;
caseBranchType:
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
; 24-bit constant ops
;
constantType:
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
offsetType:
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
; array offset ops
;
arrayOffsetType:
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
localStructArrayType:
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
constantStringType:
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
allocLocalsType:
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
	mov	esi, [ebp].FCore.builtinOps
	jmp	edi

;-----------------------------------------------
;
; local string init ops
;
initLocalStringType:
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
	mov	esi, [ebp].FCore.builtinOps
	jmp	edi

;-----------------------------------------------
;
; local reference ops
;
localRefType:
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
; local byte ops
;
localByteType:
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
	DD	FLAT:localByteRef
	DD	FLAT:localByteStore
	DD	FLAT:localBytePlusStore
	DD	FLAT:localByteMinusStore

localByte1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localByteActionTable[ebx*4]
	jmp	ebx

fieldByteType:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

localByteArrayType:
	; get ptr to byte var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sal	ebx, 2
	sub	eax, ebx
	add	eax, [edx]		; add in array index on TOS
	add	edx, 4
	jmp	byteEntry

fieldByteArrayType:
	; get ptr to byte var into eax
	; TOS is struct base ptr, NOS is index
	; ebx is field offset in bytes
	mov	eax, [edx]
	add	eax, [edx+4]
	add	edx, 8
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	byteEntry

;-----------------------------------------------
;
; local short ops
;
localShortType:
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
	DD	FLAT:localShortRef
	DD	FLAT:localShortStore
	DD	FLAT:localShortPlusStore
	DD	FLAT:localShortMinusStore

localShort1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localShortActionTable[ebx*4]
	jmp	ebx

fieldShortType:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	shortEntry

localShortArrayType:
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

fieldShortArrayType:
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

;-----------------------------------------------
;
; local int ops
;
localIntType:
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
	DD	FLAT:localIntRef
	DD	FLAT:localIntStore
	DD	FLAT:localIntPlusStore
	DD	FLAT:localIntMinusStore

localInt1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localIntActionTable[ebx*4]
	jmp	ebx

fieldIntType:
	; get ptr to int var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	intEntry

localIntArrayType:
	; get ptr to int var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	intEntry

fieldIntArrayType:
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

;-----------------------------------------------
;
; local float ops
;
localFloatType:
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
	DD	FLAT:localFloatRef
	DD	FLAT:localFloatStore
	DD	FLAT:localFloatPlusStore
	DD	FLAT:localFloatMinusStore

localFloat1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localFloatActionTable[ebx*4]
	jmp	ebx

fieldFloatType:
	; get ptr to float var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	floatEntry

localFloatArrayType:
	; get ptr to float var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	floatEntry

fieldFloatArrayType:
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

;-----------------------------------------------
;
; local double ops
;
localDoubleType:
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
	DD	FLAT:localDoubleRef
	DD	FLAT:localDoubleStore
	DD	FLAT:localDoublePlusStore
	DD	FLAT:localDoubleMinusStore

localDouble1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localDoubleActionTable[ebx*4]
	jmp	ebx

fieldDoubleType:
	; get ptr to double var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	doubleEntry

localDoubleArrayType:
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

fieldDoubleArrayType:
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

;-----------------------------------------------
;
; local string ops
;
PUBLIC localStringType, stringEntry, localStringFetch, localStringStore, localStringAppend
localStringType:
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
	push	esi		; srcPtr
	push	edi		; dstPtr
	call	strncpy
	add	esp, 8
	pop	ecx

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
	DD	FLAT:localStringRef
	DD	FLAT:localStringStore
	DD	FLAT:localStringAppend

localString1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localStringActionTable[ebx*4]
	jmp	ebx

fieldStringType:
	; get ptr to byte var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	stringEntry

localStringArrayType:
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

fieldStringArrayType:
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

;-----------------------------------------------
;
; local op ops
;
localOpType:
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
	mov	eax, [eax]
	jmp	interpLoopExecuteEntry


localOpActionTable:
	DD	FLAT:localOpExecute
	DD	FLAT:localIntRef
	DD	FLAT:localIntStore

localOp1:
	; dispatch based on value in ebx
	mov	ebx, DWORD PTR localOpActionTable[ebx*4]
	jmp	ebx

fieldOpType:
	; get ptr to op var into eax
	; TOS is base ptr, ebx is field offset in bytes
	mov	eax, [edx]
	add	edx, 4
	and	ebx, 00FFFFFFh
	add	eax, ebx
	jmp	opEntry

localOpArrayType:
	; get ptr to op var into eax
	mov	eax, [ebp].FCore.FPtr
	and	ebx, 00FFFFFFh
	sub	ebx, [edx]		; add in array index on TOS
	add	edx, 4
	sal	ebx, 2
	sub	eax, ebx
	jmp	opEntry

fieldOpArrayType:
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

;-----------------------------------------------
;
; builtinOps code
;

;-----------------------------------------------
;
; doByteOp is compiled as the first op in global byte vars
; the byte data field is immediately after this op
;
doByteBop:
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
; doByteArrayOp is compiled as the first op in global byte arrays
; the data array is immediately after this op
;
doByteArrayBop:
	; get ptr to byte var into eax
	mov	eax, ecx
	add	eax, [edx]
	add	edx, 4
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	intEntry

;-----------------------------------------------
;
; doShortOp is compiled as the first op in global short vars
; the short data field is immediately after this op
;
doShortBop:
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
; doShortArrayOp is compiled as the first op in global short arrays
; the data array is immediately after this op
;
doShortArrayBop:
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
	jmp	intEntry

;-----------------------------------------------
;
; doIntOp is compiled as the first op in global int vars
; the int data field is immediately after this op
;
doIntBop:
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
doIntArrayBop:
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
doFloatBop:
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
doFloatArrayBop:
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
doDoubleBop:
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
doDoubleArrayBop:
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
doStringBop:
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
doStringArrayBop:
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
doOpBop:
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
doOpArrayBop:
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

;========================================

initStringBop:
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

initStringArrayBop:	; TBD
	extOp	initStringArrayOp
	
;========================================

doneBop:
	mov	eax,kResultDone
	jmp	interpLoopExit

;========================================

byeBop:
	mov	eax,kResultExitShell
	jmp	interpLoopExit

;========================================

abortBop:
	mov	eax,kForthErrorAbort
	jmp	interpLoopFatalErrorExit

;========================================

badOpBop:
	extOp	badOpOp
	
;========================================

argvBop:	; TBD
	extOp	argvOp

;========================================
	
argcBop:	; TBD
	extOp	argcOp

;========================================

plusBop:
	mov	eax, [edx]
	add	edx, 4
	add	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
minusBop:
	mov	eax, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sub	ebx, eax
	mov	[edx], ebx
	jmp	edi

;========================================

timesBop:
	mov	eax, [edx]
	add	edx, 4
	imul	eax, [edx]
	mov	[edx], eax
	jmp	edi

;========================================
	
times2Bop:
	mov	eax, [edx]
	add	eax, eax
	mov	[edx], eax
	jmp	edi

;========================================
	
times4Bop:
	mov	eax, [edx]
	sal	eax, 2
	mov	[edx], eax
	jmp	edi

;========================================
	
divideBop:
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

divide2Bop:
	mov	eax, [edx]
	sar	eax, 1
	mov	[edx], eax
	jmp	edi
	
;========================================

divide4Bop:
	mov	eax, [edx]
	sar	eax, 2
	mov	[edx], eax
	jmp	edi
	
;========================================
	
divmodBop:
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
	
modBop:
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
	
negateBop:
	mov	eax, [edx]
	neg	eax
	mov	[edx], eax
	jmp	edi
	
;========================================
	
fplusBop:
	fld	DWORD PTR [edx+4]
	fadd	DWORD PTR [edx]
	add	edx,4
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
fminusBop:
	fld	DWORD PTR [edx+4]
	fsub	DWORD PTR [edx]
	add	edx,4
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
ftimesBop:
	fld	DWORD PTR [edx+4]
	fmul	DWORD PTR [edx]
	add	edx,4
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
fdivideBop:
	fld	DWORD PTR [edx+4]
	fdiv	DWORD PTR [edx]
	add	edx,4
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	DWORD PTR [edx]
	jmp	edi
	
;========================================
	
dplusBop:
	fld	QWORD PTR [edx+8]
	fadd	QWORD PTR [edx]
	add	edx,8
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dminusBop:
	fld	QWORD PTR [edx+8]
	fsub	QWORD PTR [edx]
	add	edx,8
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dtimesBop:
	fld	QWORD PTR [edx+8]
	fmul	QWORD PTR [edx]
	add	edx,8
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	QWORD PTR [edx]
	jmp	edi
	extOp	dtimesOp
	
;========================================
	
ddivideBop:
	fld	QWORD PTR [edx+8]
	fdiv	QWORD PTR [edx]
	add	edx,8
	; set var operation back to fetch
	xor	ebx, ebx
	mov	[ebp].FCore.varMode, ebx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dsinBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	sin
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dasinBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	asin
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dcosBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	cos
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

dacosBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	acos
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dtanBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	tan
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
datanBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	atan
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
datan2Bop:
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
	
dexpBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	exp
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dlnBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	log
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dlog10Bop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	log10
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================
	
dpowBop:
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
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

dsqrtBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	sqrt
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

dceilBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	ceil
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

dfloorBop:
	push	edx
	push	ecx
	mov	eax, [edx+4]
	push	eax
	mov	eax, [edx]
	push	eax
	call	floor
	add	esp, 8
	pop	ecx
	pop	edx
	fstp	QWORD PTR [edx]
	jmp	edi

;========================================

dabsBop:
	fld	QWORD PTR [edx]
	fabs
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

dldexpBop:
	; ldexp( a, n )
	push	edx
	push	ecx
	; TOS is n (long), a (double)
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

dfrexpBop:
	; frexp( a, ptrToLong )
	sub	edx, 4
	push	edx
	push	ecx
	; TOS is a (double)
	; we return TOS: nLong aFrac
	; alloc nLong
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

dmodfBop:	; TBD
	; modf( a, ptrToDouble )
	mov	eax, edx
	sub	edx, 8
	push	edx
	push	ecx
	; TOS is a (double)
	; we return TOS: bFrac aWhole
	; alloc nLong
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

dfmodBop:
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
	add	edx,8
	fstp	QWORD PTR [edx]
	jmp	edi
	
;========================================

i2fBop:
	fild	DWORD PTR [edx]
	fstp	DWORD PTR [edx]
	jmp	edi	

;========================================

i2dBop:
	fild	DWORD PTR [edx]
	sub	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi	

;========================================

f2iBop:
	push	edx
	push	ecx
	fld	DWORD PTR [edx]
	call	_ftol
	pop	ecx
	pop	edx
	mov	[edx], eax
	jmp	edi

;========================================

f2dBop:
	fld	DWORD PTR [edx]
	sub	edx, 4
	fstp	QWORD PTR [edx]
	jmp	edi
		
;========================================

d2iBop:
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

d2fBop:
	fld	QWORD PTR [edx]
	add	edx, 4
	fstp	DWORD PTR [edx]
	jmp	edi

;========================================

doExitBop:	; TBD
;	extOp	doExitOp
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [ebp].FCore.RTPtr
	cmp	ebx, eax
	jg	doExitBop1
	mov	eax, kForthErrorReturnStackUnderflow
	jmp	interpLoopErrorExit

doExitBop1:
	mov	ecx, [eax]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

doExitLBop:	; TBD
	extOp	doExitLOp
	
;========================================

doExitMBop:	; TBD
	extOp	doExitMOp
	
;========================================

doExitMLBop:	; TBD
	extOp	doExitMLOp
	
;========================================

callBop:
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

gotoBop:
	mov	ecx, [edx]
	jmp	edi

;========================================

doBop:	; TBD
	extOp	doOp
	
;========================================

loopBop:	; TBD
	extOp	loopOp
	
;========================================

loopNBop:	; TBD
	extOp	loopNOp
	
;========================================
;
; TOS is start-index
; TOS+4 is end-index
; the op right after this one should be a branch
; 
doDoBop:
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

doLoopBop:
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

doLoopNBop:
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

iBop:
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

jBop:
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [eax+12]
	sub	edx,4
	mov	[edx], ebx
	jmp	edi
	
;========================================

unloopBop:
	mov	eax, [ebp].FCore.RPtr
	add	eax, 12
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

leaveBop:
	mov	eax, [ebp].FCore.RPtr
	; point IP at the branch instruction which is just before top of loop
	mov	ecx, [eax+8]
	sub	ecx, 4
	; drop current index, end index, top-of-loop-IP
	add	eax, 12
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

ifBop:	; TBD
	extOp	ifOp
	
;========================================

elseBop:	; TBD
	extOp	elseOp
	
;========================================

endifBop:	; TBD
	extOp	endifOp
	
;========================================

beginBop:	; TBD
	extOp	beginOp
	
;========================================

untilBop:	; TBD
	extOp	untilOp
	
;========================================

whileBop:	; TBD
	extOp	whileOp
	
;========================================

repeatBop:	; TBD
	extOp	repeatOp
	
;========================================

againBop:	; TBD
	extOp	againOp
	
;========================================

caseBop:	; TBD
	extOp	caseOp
	
;========================================

ofBop:	; TBD
	extOp	ofOp
	
;========================================

endofBop:	; TBD
	extOp	endofOp
	
;========================================

endcaseBop:	; TBD
	extOp	endcaseOp
	
;========================================

orBop:
	mov	eax, [edx]
	add	edx, 4
	or	[edx], eax
	jmp	edi
	
;========================================

andBop:
	mov	eax, [edx]
	add	edx, 4
	and	[edx], eax
	jmp	edi
	
;========================================

xorBop:
	mov	eax, [edx]
	add	edx, 4
	xor	[edx], eax
	jmp	edi
	
;========================================

invertBop:
	mov	eax,0FFFFFFFFh
	xor	eax, [edx]
	mov	[edx], eax
	jmp	edi
	
;========================================

lshiftBop:
	mov	eax, ecx
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	shl	ebx,cl
	mov	[edx], ebx
	mov	ecx, eax
	jmp	edi
	
;========================================

rshiftBop:
	mov	eax, ecx
	mov	ecx, [edx]
	add	edx, 4
	mov	ebx, [edx]
	sar	ebx,cl
	mov	[edx], ebx
	mov	ecx, eax
	jmp	edi
	
;========================================

notBop:
	mov	eax,0FFFFFFFFh
	xor	[edx], eax
	jmp	edi
	
;========================================

trueBop:
	mov	eax,0FFFFFFFFh
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

falseBop:
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

nullBop:
	xor	eax, eax
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

equalsZeroBop:
	xor	ebx, ebx
	jmp	equalsBop1
	
;========================================

equalsBop:
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

notEqualsZeroBop:
	xor	ebx, ebx
	jmp	notEqualsBop1
	
;========================================

notEqualsBop:
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

greaterThanZeroBop:
	xor	ebx, ebx
	jmp	gtBop1
	
;========================================

greaterThanBop:
	mov	ebx, [edx]
	add	edx, 4
gtBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jg	gtBop2
	dec	eax
gtBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

greaterEqualsZeroBop:
	xor	ebx, ebx
	jmp	geBop1
	
;========================================

greaterEqualsBop:
	mov	ebx, [edx]
	add	edx, 4
geBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jge	geBop2
	dec	eax
geBop2:
	mov	[edx], eax
	jmp	edi
	

;========================================

lessThanZeroBop:
	xor	ebx, ebx
	jmp	ltBop1
	
;========================================

lessThanBop:
	mov	ebx, [edx]
	add	edx, 4
ltBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jl	ltBop2
	dec	eax
ltBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

lessEqualsZeroBop:
	xor	ebx, ebx
	jmp	leBop1
	
;========================================

lessEqualsBop:
	mov	ebx, [edx]
	add	edx, 4
leBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jle	leBop2
	dec	eax
leBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

unsignedGreaterThanBop:
	mov	ebx, [edx]
	add	edx, 4
ugtBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	ja	ugtBop2
	dec	eax
ugtBop2:
	mov	[edx], eax
	jmp	edi
	
;========================================

unsignedLessThanBop:
	mov	ebx, [edx]
	add	edx, 4
ultBop1:
	xor	eax, eax
	cmp	ebx, [edx]
	jb	ultBop2
	dec	eax
ultBop2:
	mov	[edx], eax
	jmp	edi
	
	
;========================================

rpushBop:
	mov	ebx, [edx]
	add	edx, 4
	mov	eax, [ebp].FCore.RPtr
	sub	eax, 4
	mov	[ebp].FCore.RPtr, eax
	mov	[eax], ebx
	jmp	edi
	
;========================================

rpopBop:
	mov	eax, [ebp].FCore.RPtr
	mov	ebx, [edx]
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	sub	edx, 4
	mov	[edx], ebx
	jmp	edi
	
;========================================

rdropBop:
	mov	eax, [ebp].FCore.RPtr
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

rpBop:
	mov	eax, [ebp].FCore.RPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

rzeroBop:
	mov	eax, [ebp].FCore.RTPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

dupBop:
	mov	eax, [edx]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi

;========================================

swapBop:
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[edx], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

dropBop:
	add	edx, 4
	jmp	edi
	
;========================================

overBop:
	mov	eax, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

rotBop:
	mov	eax, [edx]
	mov	ebx, [edx+8]
	mov	[edx], ebx
	mov	ebx, [edx+4]
	mov	[edx+8], ebx
	mov	[edx+4], eax
	jmp	edi
	
;========================================

tuckBop:	; TBD
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 4
	mov	[edx], eax
	mov	[edx+4], ebx
	mov	[edx+8], eax
	jmp	edi
	
;========================================

pickBop:
	mov	eax, [edx]
	add   eax, 1
	mov	ebx, [edx+eax*4]
	mov	[edx], ebx
	jmp	edi
	
;========================================

rollBop:	; TBD
	extOp	rollOp
	
;========================================

spBop:
	mov	eax, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

szeroBop:
	mov	eax, [ebp].FCore.STPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

fpBop:
	mov	eax, [ebp].FCore.FPtr
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

ddupBop:
	mov	eax, [edx]
	mov	ebx, [edx+4]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

dswapBop:
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

ddropBop:
	add	edx, 8
	jmp	edi
	
;========================================

doverBop:
	mov	eax, [edx+8]
	mov	ebx, [edx+12]
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

drotBop:
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

alignBop:
	extOp	alignOp
	
;========================================

allotBop:
	extOp	allotOp
	
;========================================

callotBop:
	extOp	callotOp
	
;========================================

commaBop:
	extOp	commaOp
	
;========================================

cCommaBop:
	extOp	cCommaOp
	
;========================================

hereBop:
	mov	eax, [ebp].FCore.DP
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

mallocBop:
	extOp	mallocOp
	
;========================================

freeBop:
	extOp	freeOp
	
;========================================

storeBop:
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], ebx
	jmp	edi
	
;========================================

fetchBop:
	mov	eax, [edx]
	mov	ebx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

cstoreBop:
	mov	eax, [edx]
	mov	ebx, [edx+4]
	add	edx, 8
	mov	[eax], bl
	jmp	edi
	
;========================================

cfetchBop:
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bl, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

scfetchBop:
	mov	eax, [edx]
	movsx	ebx, BYTE PTR [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

c2lBop:
	mov	eax, [edx]
	movsx	ebx, al
	mov	[edx], ebx
	jmp	edi
	
;========================================

wstoreBop:
	mov	eax, [edx]
	mov	bx, [edx+4]
	add	edx, 8
	mov	[eax], bx
	jmp	edi
	
;========================================

wfetchBop:
	mov	eax, [edx]
	xor	ebx, ebx
	mov	bx, [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

swfetchBop:
	mov	eax, [edx]
	movsx	ebx, WORD PTR [eax]
	mov	[edx], ebx
	jmp	edi
	
;========================================

w2lBop:
	mov	eax, [edx]
	movsx	ebx, ax
	mov	[edx], ebx
	jmp	edi
	
;========================================

dstoreBop:
	mov	eax, [edx]
	mov	ebx, [edx+4]
	mov	[eax], ebx
	mov	ebx, [edx+8]
	mov	[eax+4], ebx
	add	edx, 12
	jmp	edi
	
;========================================

dfetchBop:
	mov	eax, [edx]
	sub	edx, 4
	mov	ebx, [edx]
	mov	[eax], ebx
	mov	ebx, [edx+4]
	mov	[eax+4], ebx
	jmp	edi
	
;========================================

memcpyBop:
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

memsetBop:
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

intoBop:
	mov	eax, kVarStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

addToBop:
	mov	eax, kVarPlusStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

subtractFromBop:
	mov	eax, kVarMinusStore
	mov	[ebp].FCore.varMode, eax
	jmp	edi
	
;========================================

addressOfBop:
	mov	eax, kVarRef
	mov	[ebp].FCore.varMode, eax
	jmp	edi

;========================================

setVarActionBop:
   mov   eax, [edx]
   add   edx, 4
	mov	[ebp].FCore.varMode, eax
	jmp	edi

;========================================

getVarActionBop:
	mov	eax, [ebp].FCore.varMode
   sub   edx, 4
   mov   [edx], eax
	jmp	edi

;========================================

strcpyBop:
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

strncpyBop:
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

strlenBop:
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

strcatBop:
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

strncatBop:
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

strchrBop:
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

strrchrBop:
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

strcmpBop:
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	strcmp
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

stricmpBop:
	;	TOS: ptr2 ptr1
	push	edx
	push	ecx
	mov	eax, [edx]
	push	eax
	mov	eax, [edx+4]
	push	eax
	call	stricmp
	add	esp, 8
	pop	ecx
	pop	edx
	add	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

strstrBop:
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

strtokBop:
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

litBop:
flitBop:
	mov	eax, [ecx]
	add	ecx, 4
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

dlitBop:
	mov	eax, [ecx]
	mov	ebx, [ecx+4]
	add	ecx, 8
	sub	edx, 8
	mov	[edx], eax
	mov	[edx+4], ebx
	jmp	edi
	
;========================================

buildsBop:	; TBD
	extOp	buildsOp
	
;========================================

doesBop:	; TBD
	extOp	doesOp
	
;========================================

newestSymbolBop:	; TBD
	extOp	newestSymbolOp
	
;========================================

endBuildsBop:	; TBD
	extOp	endBuildsOp
	
;========================================

; doDoes is executed while executing the user word
; it puts the parameter address of the user word on TOS
; top of rstack is parameter address
;
; : plusser builds , does @ + ;
; 5 plusser plus5
;
; the above 2 lines generates 2 new ops:
;	plusser
;	unnamed op
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
;	72		userOp(102)		(7)		()
;	32		userOp(101)		(7)		(76)
;	16		op(doDoes)		(7)		(36,76)
;	20		op(fetch)		(36,7)	(76)
;	24		op(plus)		(5,7)	(76)
;	28		op(doExit)		(12)	(76)
;	76		op(%d)			(12)	()
;
doDoesBop:
	mov	eax, [ebp].FCore.RPtr
	sub	edx, 4
	mov	ebx, [eax]	; ebx points at param field
	mov	[edx], ebx
	add	eax, 4
	mov	[ebp].FCore.RPtr, eax
	jmp	edi
	
;========================================

exitBop:	; TBD
	extOp	exitOp
	
;========================================

semiBop:	; TBD
	extOp	semiOp
	
;========================================

colonBop:	; TBD
	extOp	colonOp
	
;========================================

codeBop:	; TBD
	extOp	codeOp
	
;========================================

createBop:	; TBD
	extOp	createOp
	
;========================================

forgetBop:	; TBD
	extOp	forgetOp
	
;========================================

autoforgetBop:	; TBD
	extOp	autoforgetOp
	
;========================================

definitionsBop:	; TBD
	extOp	definitionsOp
	
;========================================

forthVocabBop:	; TBD
	extOp	forthVocabOp
	
;========================================

assemblerVocabBop:	; TBD
	extOp	assemblerVocabOp
	
;========================================

alsoBop:	; TBD
	extOp	alsoOp
	
;========================================

onlyBop:	; TBD
	extOp	onlyOp
	
;========================================

vocabularyBop:	; TBD
	extOp	vocabularyOp
	
;========================================

variableBop:	; TBD
	extOp	variableOp
	
;========================================

doVariableBop:	; TBD
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

constantBop:	; TBD
	extOp	constantOp
	
;========================================

doConstantBop:
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

dconstantBop:	; TBD
	extOp	dconstantOp
	
;========================================

doDConstantBop:
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

byteBop:	; TBD
	extOp	byteOp
	
;========================================

shortBop:	; TBD
	extOp	shortOp
	
;========================================

intBop:	; TBD
	extOp	intOp
	
;========================================

floatBop:	; TBD
	extOp	floatOp
	
;========================================

doubleBop:	; TBD
	extOp	doubleOp
	
;========================================

stringBop:	; TBD
	extOp	stringOp
	
;========================================

opBop:	; TBD
	extOp	opOp
	
;========================================

voidBop:	; TBD
	extOp	voidOp
	
;========================================

arrayOfBop:	; TBD
	extOp	arrayOfOp
	
;========================================

ptrToBop:	; TBD
	extOp	ptrToOp
	
;========================================

structBop:	; TBD
	extOp	structOp
	
;========================================

endstructBop:	; TBD
	extOp	endstructOp
	
;========================================

unionBop:	; TBD
	extOp	unionOp
	
;========================================

extendsBop:	; TBD
	extOp	extendsOp
	
;========================================

doStructBop:	; TBD
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

doStructArrayBop:
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

doStructTypeBop:	; TBD
	extOp	doStructTypeOp

;========================================

doEnumBop:	; TBD
	extOp	doEnumOp

;========================================

sizeOfBop:	; TBD
	extOp	sizeOfOp
	
;========================================

offsetOfBop:	; TBD
	extOp	offsetOfOp
	
;========================================

enumBop:	; TBD
	extOp	enumOp
	
;========================================

endenumBop:	; TBD
	extOp	endenumOp
	
;========================================

doVocabBop:	; TBD
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

recursiveBop:	; TBD
	extOp	recursiveOp
	
;========================================

precedenceBop:	; TBD
	extOp	precedenceOp
	
;========================================

loadBop:	; TBD
	extOp	loadOp
	
;========================================

loadDoneBop:	; TBD
	extOp	loadDoneOp
	
;========================================

interpretBop:	; TBD
	extOp	interpretOp
	
;========================================

stateInterpretBop:	; TBD
	extOp	stateInterpretOp
	
;========================================

stateCompileBop:	; TBD
	extOp	stateCompileOp
	
;========================================

stateBop:	; TBD
	extOp	stateOp
	
;========================================

tickBop:	; TBD
	extOp	tickOp
	
;========================================

executeBop:	; TBD
	mov	eax, [edx]
	add	edx, 4
	jmp	interpLoopExecuteEntry
	
;========================================

compileBop:	; TBD
	extOp	compileOp
	
;========================================

bracketTickBop:	; TBD
	extOp	bracketTickOp
	
;========================================

printNumBop:	; TBD
	extOp	printNumOp
	
;========================================

printNumDecimalBop:	; TBD
	extOp	printNumDecimalOp
	
;========================================

printNumHexBop:	; TBD
	extOp	printNumHexOp
	
;========================================

printFloatBop:	; TBD
	extOp	printFloatOp
	
;========================================

printDoubleBop:	; TBD
	extOp	printDoubleOp
	
;========================================

printFormattedBop:	; TBD
	extOp	printFormattedOp
	
;========================================

printStrBop:	; TBD
	extOp	printStrOp
	
;========================================

printCharBop:	; TBD
	extOp	printCharOp
	
;========================================

printSpaceBop:	; TBD
	extOp	printSpaceOp
	
;========================================

printNewlineBop:	; TBD
	extOp	printNewlineOp
	
;========================================

baseBop:	; TBD
	extOp	baseOp
	
;========================================

decimalBop:	; TBD
	extOp	decimalOp
	
;========================================

hexBop:	; TBD
	extOp	hexOp
	
;========================================

printDecimalSignedBop:	; TBD
	extOp	printDecimalSignedOp
	
;========================================

printAllSignedBop:	; TBD
	extOp	printAllSignedOp
	
;========================================

printAllUnsignedBop:	; TBD
	extOp	printAllUnsignedOp
	
;========================================

outToScreenBop:	; TBD
	extOp	outToScreenOp
	
;========================================

outToFileBop:	; TBD
	extOp	outToFileOp
	
;========================================

outToStringBop:	; TBD
	extOp	outToStringOp
	
;========================================

outToOpBop:	; TBD
	extOp	outToOpOp
	
;========================================

getConOutFileBop:	; TBD
	extOp	getConOutFileOp
	
;========================================

fopenBop:	; TBD
	extOp	fopenOp
	
;========================================

fcloseBop:	; TBD
	extOp	fcloseOp
	
;========================================

fseekBop:	; TBD
	extOp	fseekOp
	
;========================================

freadBop:	; TBD
	extOp	freadOp
	
;========================================

fwriteBop:	; TBD
	extOp	fwriteOp
	
;========================================

fgetcBop:	; TBD
	extOp	fgetcOp
	
;========================================

fputcBop:	; TBD
	extOp	fputcOp
	
;========================================

feofBop:	; TBD
	extOp	feofOp
	
;========================================

ftellBop:	; TBD
	extOp	ftellOp
	
;========================================

flenBop:	; TBD
	extOp	flenOp
	
;========================================

systemBop:	; TBD
	extOp	systemOp
	
;========================================

chdirBop:	; TBD
	extOp	chdirOp
	
;========================================

stdinBop:	; TBD
	extOp	stdinOp
	
;========================================

stdoutBop:	; TBD
	extOp	stdoutOp
	
;========================================

stderrBop:	; TBD
	extOp	stderrOp
	
;========================================

dstackBop:	; TBD
	extOp	dstackOp
	
;========================================

drstackBop:	; TBD
	extOp	drstackOp

;========================================
	
vlistBop:	; TBD
	extOp	vlistOp
	
;========================================

DLLVocabularyBop:	; TBD
	extOp	DLLVocabularyOp
	
;========================================

addDLLEntryBop:	; TBD
	extOp	addDLLEntryOp
	
;========================================

blwordBop:	; TBD
	extOp	blwordOp
	
;========================================

wordBop:	; TBD
	extOp	wordOp
	
;========================================

commentBop:	; TBD
	extOp	commentOp
	
;========================================

parenCommentBop:	; TBD
	extOp	parenCommentOp
	
;========================================

parenIsCommentBop:	; TBD
	extOp	parenIsCommentOp
	
;========================================

getInBufferBaseBop:	; TBD
	extOp	getInBufferBaseOp
	
;========================================

getInBufferPointerBop:	; TBD
	extOp	getInBufferPointerOp
	
;========================================

setInBufferPointerBop:	; TBD
	extOp	setInBufferPointerOp
	
;========================================

getInBufferLengthBop:	; TBD
	extOp	getInBufferLengthOp
	
;========================================

fillInBufferBop:	; TBD
	extOp	fillInBufferOp
	
;========================================

turboBop:	; TBD
	extOp	turboOp
	

;========================================

statsBop:	; TBD
	extOp	statsOp
	

;========================================

describeBop:	; TBD
	extOp	describeOp
	
;========================================

errorBop:	; TBD
	extOp	errorOp
	
;========================================

addErrorTextBop:	; TBD
	extOp	addErrorTextOp
	

;========================================

; extern long CallDLLRoutine( DLLRoutine function, long argCount, void *core );

CallDLLRoutine PROC near C public uses ebx ecx edx esi edi ebp,
	funcAddr:PTR,
	argCount:DWORD,
	core:PTR
	mov	eax, DWORD PTR funcAddr
	mov	edi, argCount
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
	mov	edx, [ebp].FCore.SPtr
	sub	edx, 4
	mov	[edx], eax		; return result on parameter stack
	mov	[ebp].FCore.SPtr, edx
	; cleanup PC stack
	mov	ebx, edi
	sal	ebx, 2
	add	esp, ebx
	ret
CallDLLRoutine ENDP

;========================================
dllEntryPointType:
	mov	[ebp].FCore.IPtr, ecx
	mov	[ebp].FCore.SPtr, edx
	mov	eax, ebx
	and	eax, 0007FFFFh
	cmp	eax, [ebp].FCore.numUserOps
	jge	badUserDef
	; push core ptr
	push	ebp
	; push arg count
	mov	ecx, ebx
	and	ecx, 00F80000h
	sar	ecx, 19
	push	ecx
	; push entry point address
	mov	ecx, [ebp].FCore.userOps
	mov	edx, [ecx+eax*4]
	push	edx
	call	CallDLLRoutine
	add	esp, 8
	pop	ebp
	jmp	interpFunc

opTypesTable:
; TBD: check the order of these
; TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
;	00 - 09
	DD	FLAT:badOpcode				; kOpBuiltIn = 0,
	DD	FLAT:badOpcode				; kOpBuiltInImmediate,
	DD	FLAT:userDefType			; kOpUserDef,
	DD	FLAT:badOpcode				; kOpUserDefImmediate,
	DD	FLAT:userCodeType			; kOpUserCode,         
	DD	FLAT:badOpcode				; kOpUserCodeImmediate,
	DD	FLAT:dllEntryPointType		; kOpDLLEntryPoint,
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
;	10 - 19
	DD	FLAT:branchType				; kOpBranch = 10,
	DD	FLAT:branchNZType			; kOpBranchNZ,
	DD	FLAT:branchZType			; kOpBranchZ,
	DD	FLAT:caseBranchType			; kOpCaseBranch,
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
;	20 - 29
	DD	FLAT:constantType			; kOpConstant = 20,   
	DD	FLAT:constantStringType		; kOpConstantString,	
	DD	FLAT:offsetType				; kOpOffset,          
	DD	FLAT:arrayOffsetType		; kOpArrayOffset,     
	DD	FLAT:allocLocalsType		; kOpAllocLocals,     
	DD	FLAT:localRefType			; kOpLocalRef,
	DD	FLAT:initLocalStringType	; kOpInitLocalString, 
	DD	FLAT:localStructArrayType	; kOpLocalStructArray,
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	

;	30 - 39
	DD	FLAT:localByteType
	DD	FLAT:localShortType
	DD	FLAT:localIntType
	DD	FLAT:localFloatType
	DD	FLAT:localDoubleType
	DD	FLAT:localStringType
	DD	FLAT:localOpType
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
;	40 - 49
	DD	FLAT:fieldByteType
	DD	FLAT:fieldShortType
	DD	FLAT:fieldIntType
	DD	FLAT:fieldFloatType
	DD	FLAT:fieldDoubleType
	DD	FLAT:fieldStringType
	DD	FLAT:fieldOpType
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
;	50 - 59
	DD	FLAT:localByteArrayType
	DD	FLAT:localShortArrayType
	DD	FLAT:localIntArrayType
	DD	FLAT:localFloatArrayType
	DD	FLAT:localDoubleArrayType
	DD	FLAT:localStringArrayType
	DD	FLAT:localOpArrayType
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
;	60 - 69
	DD	FLAT:fieldByteArrayType
	DD	FLAT:fieldShortArrayType
	DD	FLAT:fieldIntArrayType
	DD	FLAT:fieldFloatArrayType
	DD	FLAT:fieldDoubleArrayType
	DD	FLAT:fieldStringArrayType
	DD	FLAT:fieldOpArrayType
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
	DD	FLAT:badOpcode	
endOpTypesTable:
	DD	0

opsTable:
	DD	FLAT:abortBop
	DD	FLAT:dropBop
	DD	FLAT:doDoesBop
	DD	FLAT:litBop
	DD	FLAT:litBop
	DD	FLAT:dlitBop
	DD	FLAT:doVariableBop
	DD	FLAT:doConstantBop
	DD	FLAT:doDConstantBop
	DD	FLAT:endBuildsBop
	DD	FLAT:doneBop
	DD	FLAT:doByteBop
	DD	FLAT:doShortBop
	DD	FLAT:doIntBop
	DD	FLAT:doFloatBop
	DD	FLAT:doDoubleBop
	DD	FLAT:doStringBop
	DD	FLAT:doOpBop
	DD	FLAT:intoBop
	DD	FLAT:doDoBop
	DD	FLAT:doLoopBop
	DD	FLAT:doLoopNBop
	DD	FLAT:doExitBop
	DD	FLAT:doExitLBop
	DD	FLAT:doExitMBop
	DD	FLAT:doExitMLBop
	DD	FLAT:doVocabBop
	DD	FLAT:doByteArrayBop
	DD	FLAT:doShortArrayBop
	DD	FLAT:doIntArrayBop
	DD	FLAT:doFloatArrayBop
	DD	FLAT:doDoubleArrayBop
	DD	FLAT:doStringArrayBop
	DD	FLAT:doOpArrayBop
	DD	FLAT:initStringBop
	DD	FLAT:initStringArrayBop
	DD	FLAT:plusBop
	DD	FLAT:fetchBop
	DD	FLAT:badOpBop
	DD	FLAT:doStructBop
	DD	FLAT:doStructArrayBop
	DD	FLAT:doStructTypeBop
	DD	FLAT:doEnumBop
	
	; integer math
	DD	FLAT:minusBop
	DD	FLAT:timesBop
	DD	FLAT:times2Bop
	DD	FLAT:times4Bop
	DD	FLAT:divideBop
	DD	FLAT:divide2Bop
	DD	FLAT:divide4Bop
	DD	FLAT:divmodBop
	DD	FLAT:modBop
	DD	FLAT:negateBop
	
	; single precision fp math
	DD	FLAT:fplusBop
	DD	FLAT:fminusBop
	DD	FLAT:ftimesBop
	DD	FLAT:fdivideBop
	
	; double precision fp math
	DD	FLAT:dplusBop
	DD	FLAT:dminusBop
	DD	FLAT:dtimesBop
	DD	FLAT:ddivideBop

	; double precision fp functions	
	DD	FLAT:dsinBop
	DD	FLAT:dasinBop
	DD	FLAT:dcosBop
	DD	FLAT:dacosBop
	DD	FLAT:dtanBop
	DD	FLAT:datanBop
	DD	FLAT:datan2Bop
	DD	FLAT:dexpBop
	DD	FLAT:dlnBop
	DD	FLAT:dlog10Bop
	DD	FLAT:dpowBop
	DD	FLAT:dsqrtBop
	DD	FLAT:dceilBop
	DD	FLAT:dfloorBop
	DD	FLAT:dabsBop
	DD	FLAT:dldexpBop
	DD	FLAT:dfrexpBop
	DD	FLAT:dmodfBop
	DD	FLAT:dfmodBop
	
	; int/float/double conversions
	DD	FLAT:i2fBop
	DD	FLAT:i2dBop
	DD	FLAT:f2iBop
	DD	FLAT:f2dBop
	DD	FLAT:d2iBop
	DD	FLAT:d2fBop
	
	; control flow
	DD	FLAT:callBop
	DD	FLAT:gotoBop
	DD	FLAT:doBop
	DD	FLAT:loopBop
	DD	FLAT:loopNBop
	DD	FLAT:iBop
	DD	FLAT:jBop
	DD	FLAT:unloopBop
	DD	FLAT:leaveBop
	DD	FLAT:ifBop
	DD	FLAT:elseBop
	DD	FLAT:endifBop
	DD	FLAT:beginBop
	DD	FLAT:untilBop
	DD	FLAT:whileBop
	DD	FLAT:repeatBop
	DD	FLAT:againBop
	DD	FLAT:caseBop
	DD	FLAT:ofBop
	DD	FLAT:endofBop
	DD	FLAT:endcaseBop
	
	; bit-vector logic
	DD	FLAT:orBop
	DD	FLAT:andBop
	DD	FLAT:xorBop
	DD	FLAT:invertBop
	DD	FLAT:lshiftBop
	DD	FLAT:rshiftBop
	
	; boolean logic
	DD	FLAT:notBop
	DD	FLAT:trueBop
	DD	FLAT:falseBop
	DD	FLAT:nullBop
	
	; integer comparisons
	DD	FLAT:equalsBop
	DD	FLAT:notEqualsBop
	DD	FLAT:greaterThanBop
	DD	FLAT:greaterEqualsBop
	DD	FLAT:lessThanBop
	DD	FLAT:lessEqualsBop
	DD	FLAT:equalsZeroBop
	DD	FLAT:notEqualsZeroBop
	DD	FLAT:greaterThanZeroBop
	DD	FLAT:greaterEqualsZeroBop
	DD	FLAT:lessThanZeroBop
	DD	FLAT:lessEqualsZeroBop
	DD	FLAT:unsignedGreaterThanBop
	DD	FLAT:unsignedLessThanBop
	
	; stack manipulation
	DD	FLAT:rpushBop
	DD	FLAT:rpopBop
	DD	FLAT:rdropBop
	DD	FLAT:rpBop
	DD	FLAT:rzeroBop
	DD	FLAT:dupBop
	DD	FLAT:swapBop
	DD	FLAT:overBop
	DD	FLAT:rotBop
	DD	FLAT:tuckBop
	DD	FLAT:pickBop
	DD	FLAT:rollBop
	DD	FLAT:spBop
	DD	FLAT:szeroBop
	DD	FLAT:fpBop
	DD	FLAT:ddupBop
	DD	FLAT:dswapBop
	DD	FLAT:ddropBop
	DD	FLAT:doverBop
	DD	FLAT:drotBop
	
	; data compilation/allocation
	DD	FLAT:alignBop
	DD	FLAT:allotBop
	DD	FLAT:callotBop
	DD	FLAT:commaBop
	DD	FLAT:cCommaBop
	DD	FLAT:hereBop
	DD	FLAT:mallocBop
	DD	FLAT:freeBop
	
	; memory store/fetch
	DD	FLAT:storeBop
	DD	FLAT:cstoreBop
	DD	FLAT:cfetchBop
	DD	FLAT:scfetchBop
	DD	FLAT:c2lBop
	DD	FLAT:wstoreBop
	DD	FLAT:wfetchBop
	DD	FLAT:swfetchBop
	DD	FLAT:w2lBop
	DD	FLAT:dstoreBop
	DD	FLAT:dfetchBop
	DD	FLAT:memcpyBop
	DD	FLAT:memsetBop
	DD	FLAT:addToBop
	DD	FLAT:subtractFromBop
	DD	FLAT:addressOfBop
	DD	FLAT:setVarActionBop
	DD	FLAT:getVarActionBop
	
	; string manipulation
	DD	FLAT:strcpyBop
	DD	FLAT:strncpyBop
	DD	FLAT:strlenBop
	DD	FLAT:strcatBop
	DD	FLAT:strncatBop
	DD	FLAT:strchrBop
	DD	FLAT:strrchrBop
	DD	FLAT:strcmpBop
	DD	FLAT:stricmpBop
	DD	FLAT:strstrBop
	DD	FLAT:strtokBop
	
	; op definition
	DD	FLAT:buildsBop
	DD	FLAT:doesBop
	DD	FLAT:exitBop
	DD	FLAT:semiBop
	DD	FLAT:colonBop
	DD	FLAT:codeBop
	DD	FLAT:createBop
	DD	FLAT:variableBop
	DD	FLAT:constantBop
	DD	FLAT:dconstantBop
	DD	FLAT:byteBop
	DD	FLAT:shortBop
	DD	FLAT:intBop
	DD	FLAT:floatBop
	DD	FLAT:doubleBop
	DD	FLAT:stringBop
	DD	FLAT:opBop
	DD	FLAT:voidBop
	DD	FLAT:arrayOfBop
	DD	FLAT:ptrToBop
	DD	FLAT:structBop
	DD	FLAT:endstructBop
	DD	FLAT:unionBop
	DD	FLAT:extendsBop
	DD	FLAT:sizeOfBop
	DD	FLAT:offsetOfBop
	DD	FLAT:enumBop
	DD	FLAT:endenumBop
	DD	FLAT:recursiveBop
	DD	FLAT:precedenceBop
	DD	FLAT:loadBop
	DD	FLAT:loadDoneBop
	DD	FLAT:interpretBop
	DD	FLAT:stateInterpretBop
	DD	FLAT:stateCompileBop
	DD	FLAT:stateBop
	DD	FLAT:tickBop
	DD	FLAT:executeBop
	DD	FLAT:compileBop
	DD	FLAT:bracketTickBop

	; vocabulary/symbol
	DD	FLAT:forthVocabBop
	DD	FLAT:assemblerVocabBop
	DD	FLAT:definitionsBop
	DD	FLAT:vocabularyBop
	DD	FLAT:alsoBop
	DD	FLAT:onlyBop
	DD	FLAT:newestSymbolBop
	DD	FLAT:forgetBop
	DD	FLAT:autoforgetBop
	DD	FLAT:vlistBop

	; text display	
	DD	FLAT:printNumBop
	DD	FLAT:printNumDecimalBop
	DD	FLAT:printNumHexBop
	DD	FLAT:printStrBop
	DD	FLAT:printCharBop
	DD	FLAT:printSpaceBop
	DD	FLAT:printNewlineBop
	DD	FLAT:printFloatBop
	DD	FLAT:printDoubleBop
	DD	FLAT:printFormattedBop
	DD	FLAT:baseBop
	DD	FLAT:decimalBop
	DD	FLAT:hexBop
	DD	FLAT:printDecimalSignedBop
	DD	FLAT:printAllSignedBop
	DD	FLAT:printAllUnsignedBop
	DD	FLAT:outToFileBop
	DD	FLAT:outToScreenBop
	DD	FLAT:outToStringBop
	DD	FLAT:outToOpBop
	DD	FLAT:getConOutFileBop
	
	; file manipulation
	DD	FLAT:fopenBop
	DD	FLAT:fcloseBop
	DD	FLAT:fseekBop
	DD	FLAT:freadBop
	DD	FLAT:fwriteBop
	DD	FLAT:fgetcBop
	DD	FLAT:fputcBop
	DD	FLAT:feofBop
	DD	FLAT:ftellBop
	DD	FLAT:flenBop
	DD	FLAT:stdinBop
	DD	FLAT:stdoutBop
	DD	FLAT:stderrBop
	
	; input buffer
	DD	FLAT:blwordBop
	DD	FLAT:wordBop
	DD	FLAT:commentBop
	DD	FLAT:parenCommentBop
	DD	FLAT:parenIsCommentBop
	DD	FLAT:getInBufferBaseBop
	DD	FLAT:getInBufferPointerBop
	DD	FLAT:setInBufferPointerBop
	DD	FLAT:getInBufferLengthBop
	DD	FLAT:fillInBufferBop

	; DLL support
	DD	FLAT:DLLVocabularyBop
	DD	FLAT:addDLLEntryBop
	
	; admin/debug/system
	DD	FLAT:dstackBop
	DD	FLAT:drstackBop
	DD	FLAT:systemBop
	DD	FLAT:chdirBop
	DD	FLAT:byeBop
	DD	FLAT:argvBop
	DD	FLAT:argcBop
	DD	FLAT:turboBop
	DD	FLAT:statsBop
	DD	FLAT:describeBop
	DD	FLAT:errorBop
	DD	FLAT:addErrorTextBop
endOpsTable:
	DD	0
	
_TEXT	ENDS
END
