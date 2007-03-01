	TITLE	M:\Prj\AsmScratch\AsmScratchSub.asm
	.486
include listing.inc
include core.inc

.model FLAT, C

PUBLIC	HelloString		; `string'

PUBLIC	abortBop, dropBop, doDoesBop, litBop, flitBop, dlitBop, doVariableBop, doConstantBop, doDConstantBop;
PUBLIC	endBuildsBop, doneBop, doIntBop, doFloatBop, doDoubleBop, doStringBop, intoBop, doDoBop;
PUBLIC	doLoopBop, doLoopNBop, doExitBop, doExitLBop, doExitMBop, doExitMLBop, doVocabBop, plusBop;
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
PUBLIC	greaterThanZeroBop, greaterEqualsZeroBop, lessThanZeroBop, lessEqualsZeroBop, rpushBop, rpopBop, rdropBop, dupBop;
PUBLIC	swapBop, overBop, rotBop, ddupBop, dswapBop, ddropBop, doverBop, drotBop;
PUBLIC	alignBop, allotBop, commaBop, cCommaBop, hereBop, mallocBop, freeBop, storeBop;
PUBLIC	fetchBop, cstoreBop, cfetchBop, scfetchBop, c2lBop, wstoreBop, wfetchBop, swfetchBop, w2lBop, dstoreBop, dfetchBop, addToBop;
PUBLIC	subtractFromBop, addressOfBop, strcpyBop, strncpyBop, strlenBop, strcatBop, strncatBop, strchrBop, strrchrBop;
PUBLIC	strcmpBop, stricmpBop, strstrBop, strtokBop, buildsBop, doesBop, newestSymbolBop, exitBop, semiBop, colonBop, createBop;
PUBLIC	forgetBop, autoforgetBop, definitionsBop, usesBop, forthVocabBop, searchVocabBop, definitionsVocabBop, vocabularyBop, variableBop;
PUBLIC	constantBop, dconstantBop, varsBop, endvarsBop, intBop, floatBop, doubleBop, stringBop;
PUBLIC	recursiveBop, precedenceBop, loadBop, loadDoneBop, interpretBop, stateInterpretBop, stateCompileBop, stateBop, tickBop;
PUBLIC	executeBop, compileBop, bracketTickBop, printNumBop, printNumDecimalBop, printNumHexBop, printStrBop, printCharBop;
PUBLIC	printSpaceBop, printNewlineBop, printFloatBop, printDoubleBop, printFormattedBop, baseBop, decimalBop, hexBop;
PUBLIC	printDecimalSignedBop, printAllSignedBop, printAllUnsignedBop, outToFileBop, outToScreenBop, outToStringBop, getConOutFileBop, fopenBop;
PUBLIC	fcloseBop, fseekBop, freadBop, fwriteBop, fgetcBop, fputcBop, feofBop, ftellBop;
PUBLIC	stdinBop, stdoutBop, stderrBop, dstackBop, drstackBop, vlistBop, systemBop, chdirBop, byeBop;
PUBLIC	argvBop, argcBop, loadLibraryBop, freeLibraryBop, getProcAddressBop, callProc0Bop, callProc1Bop, callProc2Bop;
PUBLIC	callProc3Bop, callProc4Bop, callProc5Bop, callProc6Bop, callProc7Bop, callProc8Bop, blwordBop, wordBop;
PUBLIC	getInBufferBaseBop, getInBufferPointerBop, setInBufferPointerBop, getInBufferLengthBop, fillInBufferBop, turboBop, statsBop;

EXTRN	_iob:BYTE
EXTRN	_filbuf:NEAR
EXTRN	printf:NEAR
EXTRN	_chkesp:NEAR

EXTRN	sin:NEAR, asin:NEAR, cos:NEAR, acos:NEAR, tan:NEAR, atan:NEAR, atan2:NEAR, exp:NEAR, log:NEAR, log10:NEAR
EXTRN	pow:NEAR, sqrt:NEAR, ceil:NEAR, floor:NEAR, ldexp:NEAR, frexp:NEAR, modf:NEAR, fmod:NEAR, _ftol:NEAR
EXTRN	strcpy:NEAR, strncpy:NEAR, strstr:NEAR, strcmp:NEAR, stricmp:NEAR, strchr:NEAR, strrchr:NEAR, strcat:NEAR, strncat:NEAR, strtok:NEAR


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
InnerInterpreterFast PROC near C public uses ebx ecx edx esi edi ebp,
	core:PTR
	mov	ebp, DWORD PTR core
	mov	eax, kResultOk
	mov	[ebp].FCore.state, eax
	call	interpFunc
	push	ecx
	push	edx
	pop	edx
	pop	ecx
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
	and	eax,00800000h
	jnz	offsetNegative
	; positive constant
	and	ebx,00FFFFFFh
	add	[edx], ebx
	jmp	edi

offsetNegative:
	or	ebx, 0FF000000h
	add	[edx], ebx
	jmp	edi

;-----------------------------------------------
;
; string literal ops
;
stringLitType:
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



;-----------------------------------------------
;
; builtinOps code
;

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

doDoubleBop:
	; get ptr to float var into eax
	mov	eax, ecx
	; pop rstack
	mov	ebx, [ebp].FCore.RPtr
	mov	ecx, [ebx]
	add	ebx, 4
	mov	[ebp].FCore.RPtr, ebx
	jmp	doubleEntry

doStringBop:	;TBD
	extOp	doStringOp


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

argvBop::	; TBD
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

timesBop::
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
	mov	eax, [esi]
	add	esi, 4
	sub	edx, 4
	mov	[edx], eax
	jmp	edi
	
;========================================

dlitBop:
	mov	eax, [esi]
	mov	ebx, [esi+4]
	add	esi, 8
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

usesBop:	; TBD
	extOp	usesOp
	
;========================================

forthVocabBop:	; TBD
	extOp	forthVocabOp
	
;========================================

searchVocabBop:	; TBD
	extOp	searchVocabOp
	
;========================================

definitionsVocabBop:	; TBD
	extOp	definitionsVocabOp
	
;========================================

vocabularyBop:	; TBD
	extOp	vocabularyOp
	
;========================================

variableBop:	; TBD
	extOp	variableOp
	
;========================================

varsBop:	; TBD
	extOp	varsOp
	
;========================================

endvarsBop:	; TBD
	extOp	endvarsOp
	
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

loadLibraryBop:	; TBD
	extOp	loadLibraryOp
	
;========================================

freeLibraryBop:	; TBD
	extOp	freeLibraryOp
	
;========================================

getProcAddressBop:	; TBD
	extOp	getProcAddressOp
	
;========================================

callProc0Bop:	; TBD
	extOp	callProc0Op
	
;========================================

callProc1Bop:	; TBD
	extOp	callProc1Op
	
;========================================

callProc2Bop:	; TBD
	extOp	callProc2Op
	
;========================================

callProc3Bop:	; TBD
	extOp	callProc3Op
	
;========================================

callProc4Bop:	; TBD
	extOp	callProc4Op
	
;========================================

callProc5Bop:	; TBD
	extOp	callProc5Op
	
;========================================

callProc6Bop:	; TBD
	extOp	callProc6Op
	
;========================================

callProc7Bop:	; TBD
	extOp	callProc7Op
	
;========================================

callProc8Bop:	; TBD
	extOp	callProc8Op
	
;========================================

blwordBop:	; TBD
	extOp	blwordOp
	
;========================================

wordBop:	; TBD
	extOp	wordOp
	
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
	


opTypesTable:
; TBD: check the order of these
; TBD: copy these into base of ForthCoreState, fill unused slots with badOptype
	DD	FLAT:badOpcode	
	DD	FLAT:userDefType
	DD	FLAT:branchType
	DD	FLAT:branchNZType
	DD	FLAT:branchZType
	DD	FLAT:caseBranchType
	DD	FLAT:constantType
	DD	FLAT:offsetType
	DD	FLAT:stringLitType
	DD	FLAT:allocLocalsType
	DD	FLAT:localIntType
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
	DD	FLAT:doIntBop
	DD	FLAT:doFloatBop
	DD	FLAT:doDoubleBop
	DD	FLAT:doStringBop
	DD	FLAT:intoBop
	DD	FLAT:doDoBop
	DD	FLAT:doLoopBop
	DD	FLAT:doLoopNBop
	DD	FLAT:doExitBop
	DD	FLAT:doExitLBop
	DD	FLAT:doExitMBop
	DD	FLAT:doExitMLBop
	DD	FLAT:doVocabBop
	DD	FLAT:plusBop
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
	DD	FLAT:fplusBop
	DD	FLAT:fminusBop
	DD	FLAT:ftimesBop
	DD	FLAT:fdivideBop
	DD	FLAT:dplusBop
	DD	FLAT:dminusBop
	DD	FLAT:dtimesBop
	DD	FLAT:ddivideBop
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
	DD	FLAT:i2fBop
	DD	FLAT:i2dBop
	DD	FLAT:f2iBop
	DD	FLAT:f2dBop
	DD	FLAT:d2iBop
	DD	FLAT:d2fBop
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
	DD	FLAT:orBop
	DD	FLAT:andBop
	DD	FLAT:xorBop
	DD	FLAT:invertBop
	DD	FLAT:lshiftBop
	DD	FLAT:rshiftBop
	DD	FLAT:notBop
	DD	FLAT:trueBop
	DD	FLAT:falseBop
	DD	FLAT:nullBop
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
	DD	FLAT:rpushBop
	DD	FLAT:rpopBop
	DD	FLAT:rdropBop
	DD	FLAT:dupBop
	DD	FLAT:swapBop
	DD	FLAT:overBop
	DD	FLAT:rotBop
	DD	FLAT:ddupBop
	DD	FLAT:dswapBop
	DD	FLAT:ddropBop
	DD	FLAT:doverBop
	DD	FLAT:drotBop
	DD	FLAT:alignBop
	DD	FLAT:allotBop
	DD	FLAT:commaBop
	DD	FLAT:cCommaBop
	DD	FLAT:hereBop
	DD	FLAT:mallocBop
	DD	FLAT:freeBop
	DD	FLAT:storeBop
	DD	FLAT:fetchBop
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
	DD	FLAT:addToBop
	DD	FLAT:subtractFromBop
	DD	FLAT:addressOfBop
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
	DD	FLAT:buildsBop
	DD	FLAT:doesBop
	DD	FLAT:newestSymbolBop
	DD	FLAT:exitBop
	DD	FLAT:semiBop
	DD	FLAT:colonBop
	DD	FLAT:createBop
	DD	FLAT:forgetBop
	DD	FLAT:autoforgetBop
	DD	FLAT:definitionsBop
	DD	FLAT:usesBop
	DD	FLAT:forthVocabBop
	DD	FLAT:searchVocabBop
	DD	FLAT:definitionsVocabBop
	DD	FLAT:vocabularyBop
	DD	FLAT:variableBop
	DD	FLAT:constantBop
	DD	FLAT:dconstantBop
	DD	FLAT:varsBop
	DD	FLAT:endvarsBop
	DD	FLAT:intBop
	DD	FLAT:floatBop
	DD	FLAT:doubleBop
	DD	FLAT:stringBop
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
	DD	FLAT:getConOutFileBop
	DD	FLAT:fopenBop
	DD	FLAT:fcloseBop
	DD	FLAT:fseekBop
	DD	FLAT:freadBop
	DD	FLAT:fwriteBop
	DD	FLAT:fgetcBop
	DD	FLAT:fputcBop
	DD	FLAT:feofBop
	DD	FLAT:ftellBop
	DD	FLAT:stdinBop
	DD	FLAT:stdoutBop
	DD	FLAT:stderrBop
	DD	FLAT:dstackBop
	DD	FLAT:drstackBop
	DD	FLAT:vlistBop
	DD	FLAT:systemBop
	DD	FLAT:chdirBop
	DD	FLAT:byeBop
	DD	FLAT:argvBop
	DD	FLAT:argcBop
	DD	FLAT:loadLibraryBop
	DD	FLAT:freeLibraryBop
	DD	FLAT:getProcAddressBop
	DD	FLAT:callProc0Bop
	DD	FLAT:callProc1Bop
	DD	FLAT:callProc2Bop
	DD	FLAT:callProc3Bop
	DD	FLAT:callProc4Bop
	DD	FLAT:callProc5Bop
	DD	FLAT:callProc6Bop
	DD	FLAT:callProc7Bop
	DD	FLAT:callProc8Bop
	DD	FLAT:blwordBop
	DD	FLAT:wordBop
	DD	FLAT:getInBufferBaseBop
	DD	FLAT:getInBufferPointerBop
	DD	FLAT:setInBufferPointerBop
	DD	FLAT:getInBufferLengthBop
	DD	FLAT:fillInBufferBop
	DD	FLAT:turboBop
	DD	FLAT:statsBop
endOpsTable:
	DD	0
	
_TEXT	ENDS
END
