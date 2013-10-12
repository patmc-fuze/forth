        .text
        .align  2
@-----------------------------------------------
@
@ inner interpreter C entry point
@
@ extern eForthResult InnerInterpreterFast( ForthCoreState *pCore )
@

       
@
@ Register usage:
@
@	R0 - R3		scratch		(R0 is pCore at start)
@	R4			core ptr
@	R5			IP
@	R6			SP (forth param stack)
@	R7			RP
@	R8			FP
@	R9			ops table
@	R10			number of ops
@	R11			actionType table
@	R12			0xFFFFFF (opvalue mask)

@	R13			system stack pointer
@	R14			LR - system link register - we use this as inner interp reentry point
@	R15			PC

@
@	core (R4) offsets:
@
c_optypes		=	0   	 	@	table of C opType action routines
n_builtin_ops	=	4			@	number of builtin ops
user_ops		=	8			@	table of user defined ops
n_user_ops		=	12			@	number of user defined ops
s_user_ops		=	16			@	current size of user defined op table
engine			=	20			@	ForthEngine pointer
ipsave			=	24			@	IP - interpreter pointer (r5)
spsave			=	28			@	SP - forth stack pointer (r6)
rpsave			=	32			@	RP - forth return stack pointer
fpsave			=	36			@	FP - frame pointer
tpm				=	40			@	TPM - this pointer (methods)
tpd				=	44			@	TPD - this pointer (data)
varmode			=	48			@	varMode - fetch/store/plusStore/minusStore
istate			=	52			@	state - ok/done/error
errorcode		=	56			@	error code
sp0				=	60			@	empty parameter stack pointer
ssize			=	64			@	size of param stack in longwords
rp0				=	68			@	empty return stack pointer
rsize			=	72			@	size of return stack in longwords

@	end of stuff which is per thread

cur_thread		=	76			@	current ForthThread pointer
dict_mem_sect	=	80			@	dictionary memory section pointer
file_funcs		=	84			@
inner_loop		=	88			@	inner interpreter asm reentry point


kVarDefaultOp			=		0
kVarFetch				=		1
kVarRef					=		2
kVarStore				=		3
kVarPlusStore			=		4
kVarMinusStore			=		5

kVocabSetCurrent		=	0
kVocabNewestEntry		=	1
kVocabRef				=	2
kVocabFindEntry			=	3
kVocabFindEntryValue	=	4
kVocabAddEntry			=	5
kVocabRemoveEntry		=	6
kVocabEntryLength		=	7
kVocabNumEntries		=	8
kVocabGetClass			=	9

kResultOk				=		0
kResultDone				=		1
kResultExitShell		=		2
kResultError			=		3
kResultFatalError		=		4
kResultException		=		5
kResultShutdown			=		6

kForthErrorNone						=		0
kForthErrorBadOpcode				=		1
kForthErrorBadOpcodeType			=		2
kForthErrorBadParameter				=		3
kForthErrorBadVarOperation			=		4
kForthErrorParamStackUnderflow		=		5
kForthErrorParamStackOverflow		=		6
kForthErrorReturnStackUnderflow		=		7
kForthErrorReturnStackOverflow		=		8
kForthErrorUnknownSymbol			=		9
kForthErrorFileOpen					=		10
kForthErrorAbort					=		11
kForthErrorForgetBuiltin			=		12
kForthErrorBadMethod				=		13
kForthErrorException				=		14
kForthErrorMissingSize				=		15
kForthErrorStruct					=		16
kForthErrorUserDefined				=		17
kForthErrorBadSyntax				=		18
kForthErrorBadPreprocessorDirective	=		19
kForthErrorUnimplementedMethod		=		20
kForthNumErrors						=		21

kPrintSignedDecimal		=		0
kPrintAllSigned			=		1
kPrintAllUnsigned		=		2


	.global	break_me
	.type	   break_me,function
break_me:
	swi	#0xFD0000
	bx	lr
	
@-----------------------------------------------
@
@ extOp is used by "builtin" ops which are only defined in C++
@
@	r1 holds the opcode value field
@
extOp:
	@ r1 is low 24-bits of opcode
	ldr	r3, [r4, #n_c_ops]
	cmp	r1, r3
	bge	badOpcode

	bl	.extOp2
	@ we come here when C op does its return
	b	InnerInterpreter
		
.extOp2:
	add	r0, r4, #ipsave			@ r0 -> IP
	stmia	r0!, {r5-r7}			@ save IP, SP, RP in core
	mov	r0, r4					@ C ops take pCore ptr as first param
	ldr	r3, [r4, #c_ops]		@ r3 = table of C builtin ops
	ldr	r2, [r3, r1, lsl #2]
	bx	r2


@-----------------------------------------------
@
@ extOpType is used to handle optypes which are only defined in C++
@
@	r1 holds the opcode value field
@	r2 holds the opcode type field
@
extOpType:
	add	r0, r4, #ipsave			@ r0 -> IP
	stmia	r0!, {r5-r7}			@ save IP, SP, RP in core
	ldr	r3, [r4]				@ r3 = table of C optype action routines
	ldr	r2, [r3, r2, lsl #2]	@ r2 = action routine for this optype
	mov	r0, r4					@ C++ optype routines take core ptr as 1st argument
	bx	r2
	
@-----------------------------------------------
@
@ inner interpreter C entry point
@
@ extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
	.global	InnerInterpreterFast
	.type	InnerInterpreterFast, %function
	
InnerInterpreterFast:
	push	{r4-r12, lr}
	mov	r4, r0				@ r4 = pCore

	@
	@ re-enter here if you stomped on rip/rsp/r7	
	@
InnerInterpreter:
	add	r0, r4, #ipsave
	ldmia	r0!, {r5-r8}		@ load IP, SP, RP, FP from core
	ldr	r9, [r4, #user_ops]		@ r9 = table of ops
	ldr	r10, [r4, #n_user_ops]	@ r10 = number of ops implemented in assembler
	ldr	r11, .opTypesTable		@ r11 = table of opType handlers
	ldr	r12, =0x00FFFFFF		@ r12 = op value mask (0x00FFFFFF)
	
	mov	r3, #kResultOk			@ r3 = kResultOk
	str	r3, [r4, #istate]		@ SET_STATE( kResultOk )

	@ uncomment next 2 lines to display IP each instruction	
	@mov	r0, r5
	@bl	ShowIP
	@bl	break_me	
	
	bl	.II2					@ don't know how to load lr directly
	@
	@ we come back here whenever an op does a "bx lr"
	@
.II1:
	@swi	#0xFD
	ldrb	r0, [r4, #istate]	@ get state from core
	cmp	r0, #kResultOk			@ keep looping if state is still ok (zero)
	bne	.IIExit
.II2:
	ldmia	r5!, {r0}			@ r0 = next opcode, advance IP
@ interpLoopExecuteEntry is entry for executeBop - expects opcode in r0
interpLoopExecuteEntry:
	cmp	r0, r10					@ is opcode native op?
	bge	.II3
	@ handle native ops
	ldr	r1, [r9, r0, lsl #2]
	bx	r1

.II3:
	@
	@ opcode is not native
	@
	lsr	r2, r1, #24				@ r2 = opType (hibyte of opcode)
	and	r1, r12					@ r1 = low 24 bits of opcode
	ldr	r0, [r11, r1, lsl #2]	@ r0 = action routine for this opType
	@
	@ optype action routines expect r1=opValue, r2=opType, r4=pCore
	@
	bx	r0						@ dispatch to action routine

	
	@
	@ exit inner interpreter
	@	
.IIExit:
	add	r1, r4, #ipsave			@ r1 -> IP
	stmia	r1!, {r5-r8}			@ save IP, SP, RP, FP in core
	pop	{r4-r12, pc}
	
interpLoopErrorExit:
	str	r0, [r4, #errorcode]
	mov	r0, #kResultError		@ status = error
	str	r0, [r4, #istate]
	bx	lr

interpLoopFatalErrorExit:
	str	r0, [r4, #errorcode]
	mov	r0, #kResultFatalError		@ status = error
	str	r0, [r4, #istate]
	bx	lr

	.align	2
.opsTable:
	.word	opsTable
.opTypesTable:
	.word	opTypesTable
	
@-----------------------------------------------
@
@ inner interpreter entry point for ops defined in assembler
@
@ extern void NativeAction( ForthCoreState *pCore, ulong opVal );

	.global	NativeAction
NativeAction:
	@	r0	=	core
	@	r1	=	opVal
	push	{r4, r5, r6, r7, r8, lr}
	add	r2, r0, #ipsave
	ldmia	r2!, {r5-r8}			@ load IP, SP, RP, FP from core
	
	ldr	r2, [r4, #user_ops]
	ldr	r3, [r2, r1, lsl #2]
	bl	.NativeAction1
	@	user op will return here
	add	r1, r4, #ipsave			@ r1 -> IP
	stmia	r1!, {r5-r8}			@ save IP, SP, RP, FP in core
	pop	{r4, r5, r6, r7, r8, pc}
		
.NativeAction1:
	bx	r3

	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		NATIVE DATATYPE ACTION ROUTINES              @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     byte
@

@-----------------------------------------------
@
@ local byte ops
@
localByteType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the byte field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> byte
@
byteEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localByte1

@
@ these routines can rely on:
@	r0 -> byte
@	r2 = 0
@

@ fetch local byte
localByteFetch:
	ldrb	r2, [r0]
	stmdb	r6!, {r2}				@ push byte on TOS
	bx	lr

@ push address of byte on TOS
localByteRef:
	stmdb	r6!, {r0}				@ push address of byte on TOS
	bx	lr

@ store byte on TOS into byte @ r0
localByteStore:
	ldmia	r6!, {r2}				@ pop TOS byte value into r2
	strb	r2, [r0]				@ store byte
	bx	lr

@ add byte on TOS into byte @ r0
localBytePlusStore:
	ldrb	r2, [r0]
	ldmia	r6!, {r1}				@ pop TOS byte value into r1
	add	r2, r1
	strb	r2, [r0]
	bx	lr
	
@ subtract byte on TOS from byte @ r0
localByteMinusStore:
	ldrb	r2, [r0]
	ldmia	r6!, {r1}
	sub	r2, r1
	strb	r2, [r0]
	bx	lr

.localByte1:
	@ r0 points to the byte field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #4
	bgt	badVarOperation
	ldr	r3, .localByte3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2
.localByte3:
	.word	localByteActionTable
	
localByteActionTable:
	.word	localByteFetch
	.word	localByteRef
	.word	localByteStore
	.word	localBytePlusStore
	.word	localByteMinusStore

fieldByteType:
	@ get ptr to byte var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	byteEntry	

memberByteType:
	@ get ptr to byte var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	byteEntry	
	
localByteArrayType:
	@ get ptr to byte var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	sub	r0, r8, r1				@ r0 points to the byte field
	b	byteEntry

fieldByteArrayType:
	@ get ptr to byte var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	byteEntry

memberByteArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	byteEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     short
@

@-----------------------------------------------
@
@ local short ops
@
localShortType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the byte field
	@ see if it is a fetch

@	
@ entry point for short variable action routines
@	r0 -> short
@
shortEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localShort1

@
@ these routines can rely on:
@	r0 -> short
@	r2 = 0
@

@ fetch local byte
localShortFetch:
	ldrh	r2, [r0]
	stmdb	r6!, {r2}				@ push short on TOS
	bx	lr

localShortRef:
	stmdb	r6!, {r0}				@ push address of short on TOS
	bx	lr
	
localShortStore:
	ldmia	r6!, {r2}				@ pop TOS short value into r2
	strh	r2, [r0]				@ store short
	bx	lr
	
localShortPlusStore:
	ldrh	r2, [r0]
	ldmia	r6!, {r1}				@ pop TOS short value into r1
	add	r2, r1
	strh	r2, [r0]
	bx	lr
	
localShortMinusStore:
	ldrh	r2, [r0]
	ldmia	r6!, {r1}
	sub	r2, r1
	strh	r2, [r0]
	bx	lr

.localShort1:
	@ r0 points to the short field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #4
	bgt	badVarOperation
	ldr	r3, .localByte3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1

	.align	2
.localShort3:
	.word	localShortActionTable
	
localShortActionTable:
	.word	localShortFetch
	.word	localShortRef
	.word	localShortStore
	.word	localShortPlusStore
	.word	localShortMinusStore

fieldShortType:
	@ get ptr to short var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	shortEntry	

memberShortType:
	@ get ptr to short var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	shortEntry	
	
localShortArrayType:
	@ get ptr to short var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	lsl	r1, #2
	lsl	r0, #1					@ convert short index to byte offset
	sub	r1, r0					@ add in array index
	sub	r0, r8, r1				@ r0 points to the byte field
	b	shortEntry

fieldShortArrayType:
	@ get ptr to short var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #1
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	shortEntry

memberShortArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #1
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	shortEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     int
@

@-----------------------------------------------
@
@ local int ops
@
localIntType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> int
@
intEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localInt1

@
@ these routines can rely on:
@	r0 -> int
@	r2 = 0
@

	@ fetch local int
localIntFetch:
	ldr	r2, [r0]
	stmdb	r6!, {r2}				@ push int on TOS
	bx	lr

localIntRef:
	stmdb	r6!, {r0}				@ push address of int on TOS
	bx	lr
	
localIntStore:
	ldmia	r6!, {r2}				@ pop TOS int value into r2
	str	r2, [r0]					@ store int
	bx	lr
	
localIntPlusStore:
	ldr	r2, [r0]
	ldmia	r6!, {r1}				@ pop TOS int value into r1
	add	r2, r1
	str	r2, [r0]
	bx	lr
	
localIntMinusStore:
	ldr	r2, [r0]
	ldmia	r6!, {r1}				@ pop TOS int value into r1
	sub	r2, r1
	str	r2, [r0]
	bx	lr

.localInt1:
	@ r0 points to the int field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #4
	bgt	badVarOperation
	ldr	r3, .localInt3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
badVarOperation:
	mov	r0, #kForthErrorBadVarOperation
	b	interpLoopErrorExit
	
	.align	2	
.localInt3:
	.word	localIntActionTable

localIntActionTable:
	.word	localIntFetch
	.word	localIntRef
	.word	localIntStore
	.word	localIntPlusStore
	.word	localIntMinusStore

fieldIntType:
	@ get ptr to int var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	intEntry	

memberIntType:
	@ get ptr to int var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	intEntry	
	
localIntArrayType:
	@ get ptr to int var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, r8, r1				@ r0 points to the int field
	b	intEntry

fieldIntArrayType:
	@ get ptr to int var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	intEntry

memberIntArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	intEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     float
@

@-----------------------------------------------
@
@ local float ops
@
localFloatType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> float
@
floatEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localFloat1
	
@
@ these routines can rely on:
@	r0 -> int
@	r2 = 0
@

	@ fetch local float
localFloatFetch:
	ldr	r2, [r0]
	stmdb	r6!, {r2}				@ push float on TOS
	bx	lr

localFloatRef:
	stmdb	r6!, {r0}				@ push address of float on TOS
	bx	lr
	
localFloatStore:
	ldmia	r6!, {r2}				@ pop TOS float value into r2
	str	r2, [r0]					@ store float
	bx	lr
	
localFloatPlusStore:
	mov	r3, r0
	ldr	r0, [r0]
	ldmia	r6!, {r1}
	push {lr}
	bl	__aeabi_fadd
	str	r0, [r3]
	pop	{pc}
	
localFloatMinusStore:
	mov	r3, r0
	ldr	r0, [r0]
	ldmia	r6!, {r1}
	push {lr}
	bl	__aeabi_fsub
	str	r0, [r3]
	pop	{pc}

.localFloat1:
	@ r0 points to the float field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #4
	bgt	badVarOperation
	ldr	r3, .localFloat3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localFloat3:
	.word	localFloatActionTable

localFloatActionTable:
	.word	localFloatFetch
	.word	localFloatRef
	.word	localFloatStore
	.word	localFloatPlusStore
	.word	localFloatMinusStore

fieldFloatType:
	@ get ptr to float var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	floatEntry	

memberFloatType:
	@ get ptr to float var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	floatEntry	
	
localFloatArrayType:
	@ get ptr to float var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, r8, r1				@ r0 points to the float field
	b	floatEntry

fieldFloatArrayType:
	@ get ptr to float var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	floatEntry

memberFloatArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	floatEntry


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		           OPS                               @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  abort  @@@@@@@@@@@@@@@@@

abortBop:
	mov	r0,	#kForthErrorAbort
	b	interpLoopFatalErrorExit
	
@@@@@@@@@@@@@@@@@  drop  @@@@@@@@@@@@@@@@@

dropBop:
	add	r6, #4
	bx	lr
	
@@@@@@@@@@@@@@@@@  _doDoes  @@@@@@@@@@@@@@@@@

@========================================

@ doDoes is executed while executing the user word
@ it puts the parameter address of the user word on TOS
@ top of rstack is parameter address
@
@ : plusser builds , does @ + ;
@ 5 plusser plus5
@
@ the above 2 lines generates 3 new ops:
@	plusser
@	unnamed op
@	plus5
@
@ code generated for above:
@
@ plusser userOp(100) starts here
@	0	op(builds)
@	4	op(comma)
@	8	op(endBuilds)		compiled by "does"
@	12	101					compiled by "does"
@ unnamed userOp(101) starts here
@	16	op(doDoes)			compiled by "does"
@	20	op(fetch)
@	24	op(plus)
@	28	op(doExit)
@
@ plus5 userOp(102) starts here
@	32	userOp(101)
@	36	5
@
@ ...
@	68	intCons(7)
@	72	userOp(102)		"plus5"
@	76	op(%d)
@
@ we are executing some userOp when we hit the plus5 at 72
@	IP		next op			PS		RS
@--------------------------------------------
@	68		intCons(7)		()
@	72		userOp(102)		(7)		()
@	32		userOp(101)		(7)		(76)
@	16		op(doDoes)		(7)		(36,76)
@	20		op(fetch)		(36,7)	(76)
@	24		op(plus)		(5,7)	(76)
@	28		op(doExit)		(12)	(76)
@	76		op(%d)			(12)	()
@
doDoesBop:
	ldmia	r7!, {r0}		@ pop top of return stack into R0
	stmdb	r6!, {r0}		@ push r0 onto param stack
	bx	lr
	
@@@@@@@@@@@@@@@@@  lit  @@@@@@@@@@@@@@@@@

litBop:
	ldmia	r5!, {r0}
	ldmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dlit  @@@@@@@@@@@@@@@@@

dlitBop:
	ldmia	r5!, {r0, r1}
	mov	r2, r0
	ldmdb	r6!, {r1, r2}
	bx	lr
	
@@@@@@@@@@@@@@@@@  _doVariable  @@@@@@@@@@@@@@@@@

doVariableBop:
	@ IP points to immediate data field
	ldmdb	r6!,{r5}		@ push addr of data field
							@    pointed to by IP
	ldmia	r7!, {r5}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doConstant  @@@@@@@@@@@@@@@@@

doConstantBop:
	@ IP points to immediate data field
	ldr	r0, [r5]			@ fetch data in immedate field pointed to by IP
	ldmdb	r6!, {r0}
	ldmia	r7!, {r5}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doDConstant  @@@@@@@@@@@@@@@@@

doDConstantBop:
	@ IP points to immediate data field
	ldmia	r5!, {r0, r1}
	mov	r2, r0
	ldmdb	r6!, {r1, r2}
	ldmia	r7!, {r5}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  done  @@@@@@@@@@@@@@@@@

doneBop:
	mov	r0, #kResultDone
	str	r0, [r4, #istate]
	bx	lr

@@@@@@@@@@@@@@@@@  _doByte  @@@@@@@@@@@@@@@@@

@ doByteOp is compiled as the first op in global byte vars
@ the byte data field is immediately after this op

doByteBop:
	@ get ptr to byte var into r0
	@ IP points to byte var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	byteEntry

@@@@@@@@@@@@@@@@@  _doByteArray  @@@@@@@@@@@@@@@@@

@ doByteArrayOp is compiled as the first op in global byte arrays
@ the data array is immediately after this op

doByteArrayBop:
	@ get ptr to byte var into r0
	@ IP points to base of byte array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	byteEntry

@@@@@@@@@@@@@@@@@  _doShort  @@@@@@@@@@@@@@@@@

@ doShortOp is compiled as the first op in global short vars
@ the short data field is immediately after this op

doShortBop:
	@ get ptr to short var into r0
	@ IP points to short var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	shortEntry

@@@@@@@@@@@@@@@@@  _doShortArray  @@@@@@@@@@@@@@@@@

@ doShortArrayOp is compiled as the first op in global short arrays
@ the data array is immediately after this op

doShortArrayBop:
	@ get ptr to short var into r0
	@ IP points to base of short array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #1
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	shortEntry

@@@@@@@@@@@@@@@@@  _doInt  @@@@@@@@@@@@@@@@@

@ doIntOp is compiled as the first op in global int vars
@ the int data field is immediately after this op

doIntBop:
	@ get ptr to int var into r0
	@ IP points to int var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	intEntry

@@@@@@@@@@@@@@@@@  _doIntArray  @@@@@@@@@@@@@@@@@

@ doIntArrayOp is compiled as the first op in global int arrays
@ the data array is immediately after this op

doIntArrayBop:
	@ get ptr to int var into r0
	@ IP points to base of int array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	intEntry

@@@@@@@@@@@@@@@@@  _doFloat  @@@@@@@@@@@@@@@@@

@ doFloatOp is compiled as the first op in global float vars
@ the float data field is immediately after this op

doFloatBop:
	@ get ptr to float var floato r0
	@ IP pofloats to float var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doFloatArray  @@@@@@@@@@@@@@@@@

@ doFloatArrayOp is compiled as the first op in global float arrays
@ the data array is immediately after this op

doFloatArrayBop:
	@ get ptr to float var floato r0
	@ IP points to base of float array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doDouble  @@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  _doString  @@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  _doOp  @@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  _doObject  @@@@@@@@@@@@@@@@@

	
@@@@@@@@@@@@@@@@@  addressOf  @@@@@@@@@@@@@@@@@

addressOfBop:
	mov	r0,	#kVarRef
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  ->  @@@@@@@@@@@@@@@@@

intoBop:
	mov	r0, #kVarStore
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@ ->+   @@@@@@@@@@@@@@@@@

addToBop:
	mov	r0, #kVarPlusStore
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  ->-  @@@@@@@@@@@@@@@@@

subtractFromBop:
	mov	r0, #kVarMinusStore
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  _doVocab  @@@@@@@@@@@@@@@@@

doVocabBop:
	@ push longword @ IP
	ldr	r0, [r5]
	stmdb	r6!, {r0}
	@ pop IP off rstack
	ldmia	r7!, {r5}
	bx	lr
	
@@@@@@@@@@@@@@@@@  _exit  @@@@@@@@@@@@@@@@@

doExitBop:
	ldr	r0, [r4, #rp0]			@ check for rstack underflow
	cmp	r7, r0
	bge	.doExitBop1
	ldmia	r7!, {r5}			@ pop IP off rstack
	bx	lr
	
.doExitBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitL  @@@@@@@@@@@@@@@@@

doExitLBop:
    @ rstack: local_var_storage oldFP oldIP
    @ FP points to oldFP
	ldr	r0, [r4, #rp0]			@ check for rstack underflow
	add	r1, r8, #4				@ take oldFP into account
	cmp	r1, r0
	bge	.doExitLBop1
	mov	r7, r8					@ deallocate local vars
	ldmia	r7!, {r8}			@ pop oldFP off rstack
	ldmia	r7!, {r5}			@ pop IP off rstack
	bx	lr
	
.doExitLBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitM  @@@@@@@@@@@@@@@@@

doExitMBop:
    @ rstack: oldIP oldTPM oldTPD
	ldr	r0, [r4, #rp0]			@ check for rstack underflow
	add	r1, r7, #8				@ take oldTPM and oldTPD into account
	cmp	r1, r0
	bge	.doExitMBop1
	ldmia	r7!, {r0, r1, r2}	@ pop oldIP, oldTPM, oldTPD off rstack
	mov	r5, r0
	str	r1, [r4, #tpm]
	str	r2, [r4, #tpd]
	bx	lr
	
.doExitMBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitML  @@@@@@@@@@@@@@@@@

doExitMLBop:
    @ rstack: local_var_storage oldFP oldIP oldTPM oldTPD
    @ FP points to oldFP
	ldr	r0, [r4, #rp0]			@ check for rstack underflow
	add	r1, r8, #12				@ take oldFP/oldTPM/oldTPD into account
	cmp	r1, r0
	bge	.doExitMLBop1
	mov	r7, r8					@ deallocate local vars
	ldmia	r7!, {r8}			@ pop oldFP off rstack
	ldmia	r7!, {r0, r1, r2}	@ pop oldIP, oldTPM, oldTPD off rstack
	mov	r5, r0
	str	r1, [r4, #tpm]
	str	r2, [r4, #tpd]
	bx	lr
	
.doExitMLBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit

	@ TBD	doDoubleArrayBop
	@ TBD	doStringArrayBop
	@ TBD	doOpArrayBop
	@ TBD	doObjectArrayBop
	@ TBD	initStringBop
	
@@@@@@@@@@@@@@@@@  +  @@@@@@@@@@@@@@@@@

// A B + ... (A+B)
// forth SP points to B
//
plusBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	add	r1, r0
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  @  @@@@@@@@@@@@@@@@@

fetchBop:
	ldr	r0, [r6]
	ldr	r1, [r0]
	str	r1, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  _doStruct  @@@@@@@@@@@@@@@@@

doStructBop:
	@ IP points to global struct immediately following this instruction
	stmdb	r6!, {r5}		@ push IP (address of struct) onto pstack
	ldmia	r7!, {r5}		@ pop IP off rstack
	bx	lr

@@@@@@@@@@@@@@@@@  _doStructArray  @@@@@@@@@@@@@@@@@

doStructArrayBop:
	@ TOS is array index
	@ IP -> bytes per element, followed by element 0
	ldr	r0, [r6]			@ pop index off pstack
	ldmia	r5!, {r1}		@ r1 = bytes per element, IP -> first element
	mul	r1, r0
	add	r1, r5				@ r1 -> Nth element
	str	r1, [r6]			@ replace index on TOS with element address
	ldmia	r7!, {r5}		@ pop IP off rstack
	bx	lr
	

@@@@@@@@@@@@@@@@@  _do  @@@@@@@@@@@@@@@@@

@
@ TOS is start-index
@ TOS+4 is end-index
@ the op right after this one should be a branch
@
doDoBop:
	ldmia	r6!, {r0, r1}	@ r0 = start index, r1 = end index
	@ rstack[2] holds top-of-loop IP
	add	r5, #4				@ skip over loop exit branch right after this op
	stmdb	r7!, {r5}		@ rpush start-of-loop IP
	stmdb	r7!, {r1}		@ rpush end index
	stmdb	r7!, {r0}		@ rpush start index
	bx	lr
	
@@@@@@@@@@@@@@@@@  _loop  @@@@@@@@@@@@@@@@@

doLoopBop:
	ldmia	r7, {r0, r1}	@ r0 = current loop index, r1 = end index
	add	r0, #1
	cmp	r0, r1
	bge	.doLoopBop1			@ branch if done
	ldr	r5, [r7, #8]		@ go back to top of loop
	bx	lr

.doLoopBop1:
	add	r7, #12				@ deallocate start index, end index, start loop IP
	bx	lr
	
	
@@@@@@@@@@@@@@@@@  _+loop  @@@@@@@@@@@@@@@@@


doLoopNBop:
	ldmia	r7, {r0, r1}	@ r0 = current loop index, r1 = end index
	ldmia	r6!, {r2}		@ r2 = increment
	add	r0, r2				@ add increment to current index
	cmp	r2, #0
	blt	.doLoopNBop1		@ branch if increment is negative

	@ r2 is positive increment
	cmp	r0, r1
	bge	.doLoopNBop2			@ branch if done
	ldr	r5, [r7, #8]		@ go back to top of loop
	bx	lr

.doLoopNBop1:
	@ r2 is negative increment
	cmp	r0, r1
	bl	.doLoopNBop2			@ branch if done
	ldr	r5, [r7, #8]		@ go back to top of loop
	bx	lr
	
.doLoopNBop2:
	add	r7, #12				@ deallocate start index, end index, start loop IP
	bx	lr
	
	
@@@@@@@@@@@@@@@@@  d@  @@@@@@@@@@@@@@@@@


dfetchBop:
	ldr	r0, [r6]
	ldmia	r0!, {r1, r2}
	str	r2, [r6]
	stmdb	r6!, {r1}
	bx	lr

@@@@@@@@@@@@@@@@@  this  @@@@@@@@@@@@@@@@@

thisBop:
	ldr	r1, [r4, #tpd]
	ldr	r0, [r4, #tpm]
	stmdb	r6!, {r0, r1}
	bx	lr
	
@@@@@@@@@@@@@@@@@  thisData  @@@@@@@@@@@@@@@@@

thisDataBop:
	ldr	r0, [r4, #tpd]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  thisMethods  @@@@@@@@@@@@@@@@@

thisMethodsBop:
	ldr	r0, [r4, #tpm]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  execute  @@@@@@@@@@@@@@@@@

executeBop:
	ldmia	r6!, {r0}
	b	interpLoopExecuteEntry


@#############################################@
@                                             @
@         runtime control flow stuff          @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  call  @@@@@@@@@@@@@@@@@

callBop:
	stmdb	r7!, {r5}
	ldmdb	r6!, {r5}
	bx	lr

@@@@@@@@@@@@@@@@@  goto  @@@@@@@@@@@@@@@@@

gotoBop:
	ldmdb	r6!, {r5}
	bx	lr

@@@@@@@@@@@@@@@@@  i  @@@@@@@@@@@@@@@@@

iBop:
	ldr	r0, [r7]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  j  @@@@@@@@@@@@@@@@@

jBop:
	ldr	r0, [r7, #12]
	stmdb	r6!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  unloop  @@@@@@@@@@@@@@@@@

unloopBop:
	add	r7, #12
	bx	lr

@@@@@@@@@@@@@@@@@  leave  @@@@@@@@@@@@@@@@@

leaveBop:
	ldr	r5, [r7, #8]
	sub	r5, #4
	add	r7, #12
	bx	lr

@@@@@@@@@@@@@@@@@  here  @@@@@@@@@@@@@@@@@

hereBop:
	ldr	r0, [r4, #dp]
	stmdb	r6!, {r0}
	bx	lr
			
@@@@@@@@@@@@@@@@@  removeEntry  @@@@@@@@@@@@@@@@@

removeEntryBop:
	mov	r0, #kVocabRemoveEntry
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  entryLength  @@@@@@@@@@@@@@@@@

entryLengthBop:
	mov	r0, #kVocabEntryLength
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  numEntries  @@@@@@@@@@@@@@@@@

numEntriesBop:
	mov	r0, #kVocabNumEntries
	str	r0, [r4, #varmode]
	bx	lr


@#############################################@
@                                             @
@                integer math                 @
@                                             @
@#############################################@

@@@@@@@@@@@@@@@@@  *2  @@@@@@@@@@@@@@@@@

times2Bop:
	ldr	r0, [r6]
	lsl	r0, r0, #1
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  *4  @@@@@@@@@@@@@@@@@

times4Bop:
	ldr	r0, [r6]
	lsl	r0, r0, #2
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /  @@@@@@@@@@@@@@@@@

	.global	__aeabi_idiv
divideBop:
	push	{lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	__aeabi_idiv
	str	r0, [r6]
	pop	{pc}
	
@@@@@@@@@@@@@@@@@  /2  @@@@@@@@@@@@@@@@@

divide2Bop:
	ldr	r0, [r6]
	asr	r0, r0, #1
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /4  @@@@@@@@@@@@@@@@@

divide4Bop:
	ldr	r0, [r6]
	asr	r0, r0, #2
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /mod  @@@@@@@@@@@@@@@@@

divmodBop:
	push	{lr}
	ldr	r2, [r6]				@ r2 = denominator
	ldr	r1, [r6, #4]			@ r1 = numerator
	sub	sp, #12					@ ? scratch space for div routine ?
	mov	r0, sp
	bl	div
	ldr	r1, [sp, #4]
	str	r1, [r6, #4]
	ldr	r2, [sp]
	str	r2, [r6]
	add	sp, #12
	pop	{pc}
	
@@@@@@@@@@@@@@@@@  mod  @@@@@@@@@@@@@@@@@

modBop:
	.global	__aeabi_idivmod
	push	{lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	__aeabi_idivmod
	str	r1, [r6]
	pop	{pc}
	
@@@@@@@@@@@@@@@@@  negate  @@@@@@@@@@@@@@@@@

negateBop:
	ldr	r0, [r6]
	neg	r0, r0
	str	r0, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  -  @@@@@@@@@@@@@@@@@

minusBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	sub	r2, r1, r2
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  *  @@@@@@@@@@@@@@@@@

timesBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	mul	r1, r0
	str	r1, [r6]
	bx	lr
	
@#############################################@
@                                             @
@         single precision fp math            @
@                                             @
@#############################################@

		.global __aeabi_fadd, __aeabi_fsub, __aeabi_fmul, __aeabi_fdiv
fplusBop:
	push {lr}
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	bl	__aeabi_fadd
	str	r0, [r6]
	pop	{pc}

fminusBop:
	push {lr}
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	bl	__aeabi_fsub
	str	r0, [r6]
	pop	{pc}

ftimesBop:
	push {lr}
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	bl	__aeabi_fmul
	str	r0, [r6]
	pop	{pc}

fdivideBop:
	push {lr}
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	bl	__aeabi_fdiv
	str	r0, [r6]
	pop	{pc}
	
@#############################################@
@                                             @
@         double precision fp math            @
@                                             @
@#############################################@
		.global __aeabi_dadd, __aeabi_dsub, __aeabi_dmul, __aeabi_ddiv
	
dplusBop:
	push {lr}
	ldmia	r6!, {r0, r1, r2, r3}
	bl	__aeabi_dadd
	stmdb	r6!, {r0, r1}
	pop	{pc}

dminusBop:
	push {lr}
	ldmia	r6!, {r0, r1, r2, r3}
	bl	__aeabi_dsub
	stmdb	r6!, {r0, r1}
	pop	{pc}

dtimesBop:
	push {lr}
	ldmia	r6!, {r0, r1, r2, r3}
	bl	__aeabi_dmul
	stmdb	r6!, {r0, r1}
	pop	{pc}

ddivideBop:
	push {lr}
	ldmia	r6!, {r0, r1, r2, r3}
	bl	__aeabi_ddiv
	stmdb	r6!, {r0, r1}
	pop	{pc}

	
@#############################################@
@                                             @
@       double precision fp functions         @
@                                             @
@#############################################@

	@ TBD	dsinBop
	@ TBD	dasinBop
	@ TBD	dcosBop
	@ TBD	dacosBop
	@ TBD	dtanBop
	@ TBD	datanBop
	@ TBD	datan2Bop
	@ TBD	dexpBop
	@ TBD	dlnBop
	@ TBD	dlog10Bop
	@ TBD	dpowBop
	@ TBD	dsqrtBop
	@ TBD	dceilBop
	@ TBD	dfloorBop
	@ TBD	dabsBop
	@ TBD	dldexpBop
	@ TBD	dfrexpBop
	@ TBD	dmodfBop
	@ TBD	dfmodBop
	
@#############################################@
@                                             @
@       int/float/double conversions          @
@                                             @
@#############################################@

	@ TBD	i2fBop
	@ TBD	i2dBop
	@ TBD	f2iBop
	@ TBD	f2dBop
	@ TBD	d2iBop
	@ TBD	d2fBop
	
@#############################################@
@                                             @
@             bit-vector logic                @
@                                             @
@#############################################@


orBop:
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	orr	r0, r1
	str	r0, [r6]
	bx	lr
	
andBop:
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	and	r0, r1
	str	r0, [r6]
	bx	lr
	
xorBop:
	ldmia	r6!, {r0}
	ldr	r1, [r6]
	eor	r0, r1
	str	r0, [r6]
	bx	lr

invertBop:
	ldr	r0, [r6]
	eor	r1, r1
	sub	r1, #1
	eor	r0, r1
	str	r0, [r6]
	bx	lr
	
lshiftBop:
	ldmia	r6!, {r0}		@ r0 = shift count
	ldr	r1, [r6]
	lsl	r1, r0
	str	r1, [r6]
	bx	lr

rshiftBop:
	ldmia	r6!, {r0}		@ r0 = shift count
	ldr	r1, [r6]
	lsr	r1, r0
	str	r1, [r6]
	bx	lr

	@ boolean logic
	@ TBD	notBop
	
@@@@@@@@@@@@@@@@@  not  @@@@@@@@@@@@@@@@@

notBop:
	eor	r1, r1
	ldr	r0, [r6]
	cmp	r0, r1
	subne	r1, #1
	stmdb	r6!, {r1}
	bx	lr
	

@@@@@@@@@@@@@@@@@  true  @@@@@@@@@@@@@@@@@

trueBop:
	eor	r0, r0
	sub	r0, #1
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  false  @@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@  null  @@@@@@@@@@@@@@@@@

falseBop:
nullBop:
	eor	r0, r0
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dnull  @@@@@@@@@@@@@@@@@

dnullBop:
	eor	r0, r0
	mov	r1, r0
	stmdb	r6!, {r0, r1}
	bx	lr

@#############################################@
@                                             @
@          integer comparisons                @
@                                             @
@#############################################@

@@@@@@@@@@@@@@@@@  ==  @@@@@@@@@@@@@@@@@

equalsBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	subeq	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  !=  @@@@@@@@@@@@@@@@@

notEqualsBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	subne	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  >  @@@@@@@@@@@@@@@@@

greaterThanBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	subgt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  >=  @@@@@@@@@@@@@@@@@

greaterEqualsBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	subge	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  <  @@@@@@@@@@@@@@@@@

lessThanBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	sublt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  <=  @@@@@@@@@@@@@@@@@

lessEqualsBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	subeq	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0==  @@@@@@@@@@@@@@@@@

equalsZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subeq	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0!=  @@@@@@@@@@@@@@@@@

notEqualsZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subne	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0>  @@@@@@@@@@@@@@@@@

greaterThanZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subgt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0>=  @@@@@@@@@@@@@@@@@

greaterEqualsZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subge	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0<  @@@@@@@@@@@@@@@@@

lessThanZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	sublt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0<=  @@@@@@@@@@@@@@@@@

lessEqualsZeroBop:
	ldmia	r6!, {r0}
	eor	r2, r2
	cmp	r0, r2
	suble	r2, #1
	stmdb	r6!, {r2}
	bx	lr

	@ TBD	unsignedGreaterThanBop
	@ TBD	unsignedLessThanBop
	
@#############################################@
@                                             @
@            stack manipulation               @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  >r  @@@@@@@@@@@@@@@@@

rpushBop:
	ldmia	r6!, {r0}
	ldmdb	r7!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  r>  @@@@@@@@@@@@@@@@@

rpopBop:
	ldmia	r7!, {r0}
	ldmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  rdrop  @@@@@@@@@@@@@@@@@

rdropBop:
	add	r7, #4
	bx	lr
	
@@@@@@@@@@@@@@@@@  rp  @@@@@@@@@@@@@@@@@

rpBop:
	ldmia	r6!, {r7}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r0  @@@@@@@@@@@@@@@@@

rzeroBop:
	ldr	r0, [r4, #rp0]
	ldmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dup  @@@@@@@@@@@@@@@@@

dupBop:
	ldr	r0, [r6]
	ldmdb	r6!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  swap  @@@@@@@@@@@@@@@@@

swapBop:
	ldr	r0, [r6]
	ldr	r1, [r6, #4]
	str	r1, [r6]
	str	r0, [r6, #4]
	bx	lr
		
@@@@@@@@@@@@@@@@@  over  @@@@@@@@@@@@@@@@@

overBop:
	ldr	r0, [r6, #4]
	ldmdb	r6!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  rot  @@@@@@@@@@@@@@@@@

rotBop:
	ldmia	r6, {r1, r2, r3}
	mov	r0, r3
	stmia	r6, {r0, r1, r2}
	bx	lr
	
@@@@@@@@@@@@@@@@@  tuck  @@@@@@@@@@@@@@@@@

tuckBop:
	ldmia	r6, {r0, r1}
	mov	r2, r0
	sub	r6, #4
	stmia	r6, {r0, r1, r2}
	bx 	lr
	
@@@@@@@@@@@@@@@@@  pick  @@@@@@@@@@@@@@@@@

pickBop:
	ldr	r0, [r6]
	add	r0, #1
	ldr	r1, [r6, r1, lsl #2]
	str	r1, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sp  @@@@@@@@@@@@@@@@@

spBop:
	mov	r0, r6
	ldmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  s0  @@@@@@@@@@@@@@@@@

szeroBop:
	ldr	r0, [r4, #sp0]
	ldmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  fp  @@@@@@@@@@@@@@@@@

fpBop:
	ldmdb	r8!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  ddup  @@@@@@@@@@@@@@@@@

ddupBop:
	ldmia	r6, {r0, r1}
	stmdb	r6!, {r0, r1}
	bx	lr

@@@@@@@@@@@@@@@@@  dswap  @@@@@@@@@@@@@@@@@

dswapBop:
	ldmia	r6, {r0, r1, r2, r3}
	stmia	r6, {r2, r3}
	str	r0, [r6, #8]
	str	r1, [r6, #12]
	bx	lr
		
@@@@@@@@@@@@@@@@@  ddrop  @@@@@@@@@@@@@@@@@

ddropBop:
	add	r6, #8
	bx	lr
	
@@@@@@@@@@@@@@@@@  dover  @@@@@@@@@@@@@@@@@

doverBop:
	ldr	r0, [r6, #8]
	ldr	r1, [r6, #12]
	stmdb	r6!, {r0, r1}
	bx	lr

@@@@@@@@@@@@@@@@@  drot  @@@@@@@@@@@@@@@@@

drotBop:
	ldmia	r6, {r0, r1}	@ r01 = p[0]
	ldr	r2, [r6, #16]		@ r23 = p[2]
	ldr	r3, [r6, #20]
	stmia	r6, {r2, r3}	@ p[0] = r23		(old p[2])
	ldr	r2, [r6, #8]		@ r23 = p[1]
	ldr	r3, [r6, #12]
	str	r0, [r6, #8]		@ p[1] = r01		(old p[0])
	str	r1, [r6, #12]
	str	r2, [r6, #16]		@ p[2] = r23		(old p[1])
	str	r3, [r6, #20]
	bx	lr

	
@#############################################@
@                                             @
@          memory store/fetch                 @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  !  @@@@@@@@@@@@@@@@@

storeBop:
	ldmia	r6!, {r0, r1}	@ r0 = int ptr, r1 = value
	str	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  c!  @@@@@@@@@@@@@@@@@

cstoreBop:
	ldmia	r6!, {r0, r1}	@ r0 = byte ptr, r1 = value
	strb	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  c@  @@@@@@@@@@@@@@@@@

cfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrb	r1, [r0]
	str	r1, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  sc@  @@@@@@@@@@@@@@@@@

scfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrsb	r1, [r0]
	str	r1, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  c2l  @@@@@@@@@@@@@@@@@

c2lBop:
	ldrsb	r0, [r6]
	str	r0, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  w!  @@@@@@@@@@@@@@@@@

wstoreBop:
	ldmia	r6!, {r0, r1}	@ r0 = short ptr, r1 = value
	strh	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  w@  @@@@@@@@@@@@@@@@@

wfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrh	r1, [r0]
	str	r1, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  sw@  @@@@@@@@@@@@@@@@@

swfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrsh	r1, [r0]
	str	r1, [r6]
	bx	lr

	
@@@@@@@@@@@@@@@@@  w2l  @@@@@@@@@@@@@@@@@

w2lBop:
	ldrsh	r0, [r6]
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  d!  @@@@@@@@@@@@@@@@@


dstoreBop:
	ldmia	r6!, {r0, r1, r2}
	str	r1, [r0]
	str	r2, [r0, #4]
	bx	lr
	
@@@@@@@@@@@@@@@@@  memcpy  @@@@@@@@@@@@@@@@@

memcpyBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	memcpy
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  memset  @@@@@@@@@@@@@@@@@

memsetBop:
	@	TOS is #bytes, TOS-1 is fillValue, TOS-2 is dst
	push	{r4, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = fillValue r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	memset
	pop	{r4, pc}
	
	
@@@@@@@@@@@@@@@@@  varAction!  @@@@@@@@@@@@@@@@@

setVarActionBop:
	ldmia	r6!, {r0}
	str	r0, [r4, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  varAction@  @@@@@@@@@@@@@@@@@

getVarActionBop:
	ldr	r0, [r4, #varmode]
	stmdb	r6!, {r0}
	bx	lr

@#############################################@
@                                             @
@             string manipulation             @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  strcpy  @@@@@@@@@@@@@@@@@

strcpyBop:
	@	TOS is src, TOS-1 is dst
	push	{r4, lr}
	ldmia	r6!, {r1, r2}
	mov	r0, r2
	bl	strcpy
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strncpy  @@@@@@@@@@@@@@@@@

strncpyBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncpy
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strlen  @@@@@@@@@@@@@@@@@

strlenBop:
	push	{r4, lr}
	ldr	r0, [r6]
	bl	strcpy
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strcat  @@@@@@@@@@@@@@@@@

strcatBop:
	@	TOS is src, TOS-1 is dst
	push	{r4, lr}
	ldmia	r6!, {r1, r2}
	mov	r0, r2
	bl	strcat
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strncat  @@@@@@@@@@@@@@@@@

strncatBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncat
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strchrBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strchr
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strrchrBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strrchr
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strcmp  @@@@@@@@@@@@@@@@@

strcmpBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strcasecmp
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  stricmp  @@@@@@@@@@@@@@@@@

stricmpBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strcasecmp
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strstr  @@@@@@@@@@@@@@@@@

strstrBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strstr
	str	r0, [r6]
	pop	{r4, pc}
	
@@@@@@@@@@@@@@@@@  strtok  @@@@@@@@@@@@@@@@@

strtokBop:
	push	{r4, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strtok
	str	r0, [r6]
	pop	{r4, pc}

	
	
@#############################################@
@                                             @
@           file manipulation                 @   
@                                             @
@#############################################@

	@ TBD	fopenBop
	@ TBD	fcloseBop
	@ TBD	fseekBop
	@ TBD	freadBop
	@ TBD	fwriteBop
	@ TBD	fgetcBop
	@ TBD	fputcBop
	@ TBD	feofBop
	@ TBD	ftellBop
	@ TBD	flenBop
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		      OP TYPE ACTION ROUTINES                @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@ opType action routines expect the following:
@	r1		op value (low 24 bits, already masked)
@	r2		op type (8 bits)
@	r4	core pointer
@	r5		IP
@	r6		SP
@	r7		RP
@	r8		FP
@	r0...r3 can be stomped on at will

builtInOp:
	@ we get here when an opcode has the builtin optype but there is no assembler version
	b	extOp
	
builtInImmediate:
	cmp	r1, r10
	blt	.builtInImmediate1
	b	extOp

.builtInImmediate1:
	ldr	r0, [r9, r1, lsl #2]
	bx	r0

	
userCodeType:
	ldr	r3, [r4, #n_user_ops]
	cmp	r1, r3
	bge	badOpcode
	ldr	r3, [r4, #user_ops]
	ldr	r0, [r3, r1, lsl #2]
	bx	r0


badOpcode:
	mov	r0, #kForthErrorBadOpcode
	b	interpLoopErrorExit
	
userDefType:
	ldr	r3, [r4, #n_user_ops]
	cmp	r1, r3
	bge	badOpcode
	@ rpush IP
	stmdb	r7!, {r5}
	@ fetch new IP
	ldr	r0, [r4, #user_ops]
	ldr	r5, [r0, r1, lsl #2]
	bx	lr
	
branchType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	and	r3, r1
	orrne	r1,r2
	lsl	r1, #2
	add	r5, r1			@ add branchVal to IP
	bx	lr

branchNZType:
	ldmia	r6!, {r0}
	cmp	r0, #0
	bne	branchType
	bx	lr
	
branchZType:
	ldmia	r6!, {r0}
	cmp	r0, #0
	beq	branchType
	bx	lr

caseBranchType:
	@ TOS is current case value to match against, TOS-1 is value to check
	ldr	r0, [r6]
	ldr	r1, [r6, #4]
	cmp	r0, r1
	bne	.caseBranch1
	@ case matched, drop top 2 TOS items, continue executing this branch
	add	r6, #8
	bx	lr
	
.caseBranch1:
	@ case did not match, drop current case value & skip to next case
	add	r6, #4
	lsl	r1, #2
	add	r5, r1
	bx	lr
	
constantType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	and	r3, r1
	orrne	r1,r2
	stmdb	r6!, {r1}
	bx	lr
	
offsetType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	and	r3, r1
	orrne	r1,r2
	ldr	r0, [r6]
	add	r0, r1
	str	r0, [r6]
	bx	lr
	
offsetFetchType:
	lsl	r3, r1, #8		@ see if bit 23 is set, if so branchVal is negative
	bpl	.oft1
	ldr	r3, .oft2
	orr	r1, r3
.oft1:
	ldr	r0, [r6]
	add	r1, r0
	ldr	r0, [r1]
	str	r0, [r6]
	bx	lr
	.align	2
.oft2:
	.word	0xFF000000

@-----------------------------------------------
@
@ InitAsmTables - initializes FCore.numAsmBuiltinOps
@
@ extern void InitAsmTables( ForthCoreState *pCore );

	.global	InitAsmTables
InitAsmTables:
	@
	@	count number of entries in opsTable, save count in core.n_asm_ops
	@
	ldr	r1, =opsTable

	eor	r3, r3	
.InitAsm2:
	add	r3, #1
	ldmia	r1!, {r2}
	cmp	r2, #0
	bne	.InitAsm2

	sub	r3, #1	
	str	r3, [r0, #n_asm_ops]
	bx	lr

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@                OP TYPES TABLE                      @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	.section .rodata
	.align	4
opTypesTable:
@	00 - 09
	.word	builtInOp		@ kOpBuiltIn = 0,
	.word	builtInImmediate		@ kOpBuiltInImmediate,
	.word	userDefType				@ kOpUserDef,
	.word	userDefType				@ kOpUserDefImmediate,
	.word	userCodeType			@ kOpUserCode,         
	.word	userCodeType			@ kOpUserCodeImmediate,
	.word	extOpType				@ kOpDLLEntryPoint,
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
@	10 - 19
	.word	branchType				@ kOpBranch = 10,
	.word	branchNZType			@ kOpBranchNZ,
	.word	branchZType				@ kOpBranchZ,
	.word	caseBranchType			@ kOpCaseBranch,
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
@	20 - 29
	.word	constantType			@ kOpConstant = 20,   
	.word	extOpType @ constantStringType		@ kOpConstantString,	
	.word	offsetType				@ kOpOffset,          
	.word	extOpType @ arrayOffsetType		@ kOpArrayOffset,     
	.word	extOpType @ allocLocalsType		@ kOpAllocLocals,     
	.word	extOpType @ localRefType			@ kOpLocalRef,
	.word	extOpType @ initLocalStringType	@ kOpInitLocalString, 
	.word	extOpType @ localStructArrayType	@ kOpLocalStructArray,
	.word	offsetFetchType		@ kOpOffsetFetch,          
	.word	extOpType @ memberRefType			@ kOpMemberRef,	

@	30 - 39
	.word	localByteType
	.word	localShortType
	.word	localIntType
	.word	localFloatType
	.word	extOpType @ localDoubleType
	.word	extOpType @ localStringType
	.word	extOpType @ localOpType
	.word	extOpType @ localObjectType
	.word	extOpType	
	.word	extOpType	
@	40 - 49
	.word	fieldByteType
	.word	fieldShortType
	.word	fieldIntType
	.word	fieldFloatType
	.word	extOpType @ fieldDoubleType
	.word	extOpType @ fieldStringType
	.word	extOpType @ fieldOpType
	.word	extOpType @ fieldObjectType
	.word	extOpType	
	.word	extOpType	
@	50 - 59
	.word	localByteArrayType
	.word	localShortArrayType
	.word	localIntArrayType
	.word	localFloatArrayType
	.word	extOpType @ localDoubleArrayType
	.word	extOpType @ localStringArrayType
	.word	extOpType @ localOpArrayType
	.word	extOpType @ localObjectArrayType
	.word	extOpType	
	.word	extOpType	
@	60 - 69
	.word	fieldByteArrayType
	.word	fieldShortArrayType
	.word	fieldIntArrayType
	.word	fieldFloatArrayType
	.word	extOpType @ fieldDoubleArrayType
	.word	extOpType @ fieldStringArrayType
	.word	extOpType @ fieldOpArrayType
	.word	extOpType @ fieldObjectArrayType
	.word	extOpType	
	.word	extOpType	
@	70 - 79
	.word	memberByteType
	.word	memberShortType
	.word	memberIntType
	.word	memberFloatType
	.word	extOpType @ memberDoubleType
	.word	extOpType @ memberStringType
	.word	extOpType @ memberOpType
	.word	extOpType @ memberObjectType
	.word	extOpType	
	.word	extOpType	
@	80 - 89
	.word	memberByteArrayType
	.word	memberShortArrayType
	.word	memberIntArrayType
	.word	memberFloatArrayType
	.word	extOpType @ memberDoubleArrayType
	.word	extOpType @ memberStringArrayType
	.word	extOpType @ memberOpArrayType
	.word	extOpType @ memberObjectArrayType
	.word	extOpType	
	.word	extOpType	
@	90 - 99
	.word	extOpType @ methodWithThisType
	.word	extOpType @ methodWithTOSType
	.word	extOpType @ initMemberStringType
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
	.word	extOpType	
@	100 - 149
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
@	150 - 199
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
@	200 - 249
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
@	250 - 255
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
	
endOpTypesTable:
	.word	0

	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		            OPS TABLE                        @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	.section .rodata
	.align	4
opsTable:
	.word	abortBop
	.word	dropBop
	.word	doDoesBop
	.word	litBop
	.word	litBop
	.word	dlitBop
	.word	doVariableBop
	.word	doConstantBop
	.word	doDConstantBop
	.word	extOp		@ endBuildsBop
	.word	doneBop
	.word	doByteBop
	.word	doShortBop
	.word	doIntBop
	.word	doFloatBop
	.word	extOp		@ doDoubleBop
	.word	extOp		@ doStringBop
	.word	extOp		@ doOpBop
	.word	extOp		@ doObjectBop
	.word	addressOfBop
	.word	intoBop
	.word	addToBop
	.word	subtractFromBop
	.word	doExitBop
	.word	doExitLBop
	.word	doExitMBop
	.word	doExitMLBop
	.word	doVocabBop
	.word	doByteArrayBop
	.word	doShortArrayBop
	.word	doIntArrayBop
	.word	doFloatArrayBop
	.word	extOp		@ doDoubleArrayBop
	.word	extOp		@ doStringArrayBop
	.word	extOp		@ doOpArrayBop
	.word	extOp		@ doObjectArrayBop
	.word	extOp		@ initStringBop
	.word	extOp		@ initStringArrayBop
	.word	plusBop
	.word	fetchBop
	.word	extOp		@ badOpBop
	.word	doStructBop
	.word	doStructArrayBop
	.word	extOp		@ doStructTypeBop
	.word	extOp		@ doClassTypeBop
	.word	extOp		@ doEnumBop
	.word	doDoBop
	.word	doLoopBop
	.word	doLoopNBop
	.word	extOp		@ doNewBop
	.word	dfetchBop

	@	
    @ stuff below this line can be rearranged
    @
	.word	thisBop
	.word	thisDataBop
	.word	thisMethodsBop
	.word	executeBop
	@ runtime control flow stuff
	.word	callBop
	.word	gotoBop
	.word	iBop
	.word	jBop
	.word	unloopBop
	.word	leaveBop
	.word	hereBop
	.word	addressOfBop
	.word	intoBop
	.word	addToBop
	.word	subtractFromBop
	.word	removeEntryBop
	.word	entryLengthBop
	.word	numEntriesBop
	
	
	@ integer math
	.word	minusBop
	.word	timesBop
	.word	times2Bop
	.word	times4Bop
	.word	divideBop
	.word	divide2Bop
	.word	divide4Bop
	.word	divmodBop
	.word	modBop
	.word	negateBop
	
	@ single precision fp math
	.word	fplusBop
	.word	fminusBop
	.word	ftimesBop
	.word	fdivideBop
	
	@ double precision fp math
	.word	dplusBop
	.word	dminusBop
	.word	dtimesBop
	.word	ddivideBop

	@ double precision fp functions	
	.word	extOp		@ dsinBop
	.word	extOp		@ dasinBop
	.word	extOp		@ dcosBop
	.word	extOp		@ dacosBop
	.word	extOp		@ dtanBop
	.word	extOp		@ datanBop
	.word	extOp		@ datan2Bop
	.word	extOp		@ dexpBop
	.word	extOp		@ dlnBop
	.word	extOp		@ dlog10Bop
	.word	extOp		@ dpowBop
	.word	extOp		@ dsqrtBop
	.word	extOp		@ dceilBop
	.word	extOp		@ dfloorBop
	.word	extOp		@ dabsBop
	.word	extOp		@ dldexpBop
	.word	extOp		@ dfrexpBop
	.word	extOp		@ dmodfBop
	.word	extOp		@ dfmodBop
	
	@ int/float/double conversions
	.word	extOp		@ i2fBop
	.word	extOp		@ i2dBop
	.word	extOp		@ f2iBop
	.word	extOp		@ f2dBop
	.word	extOp		@ d2iBop
	.word	extOp		@ d2fBop
	
	@ bit-vector logic
	.word	orBop
	.word	andBop
	.word	xorBop
	.word	invertBop
	.word	lshiftBop
	.word	rshiftBop
	
	@ boolean logic
	.word	notBop
	.word	trueBop
	.word	falseBop
	.word	nullBop
	.word	dnullBop
	
	@ integer comparisons
	.word	equalsBop
	.word	notEqualsBop
	.word	greaterThanBop
	.word	greaterEqualsBop
	.word	lessThanBop
	.word	lessEqualsBop
	.word	equalsZeroBop
	.word	notEqualsZeroBop
	.word	greaterThanZeroBop
	.word	greaterEqualsZeroBop
	.word	lessThanZeroBop
	.word	lessEqualsZeroBop
	.word	extOp		@ unsignedGreaterThanBop
	.word	extOp		@ unsignedLessThanBop
	
	@ stack manipulation
	.word	rpushBop
	.word	rpopBop
	.word	rdropBop
	.word	rpBop
	.word	rzeroBop
	.word	dupBop
	.word	swapBop
	.word	overBop
	.word	rotBop
	.word	tuckBop
	.word	pickBop
	.word	extOp		@ rollBop
	.word	spBop
	.word	szeroBop
	.word	fpBop
	.word	ddupBop
	.word	dswapBop
	.word	ddropBop
	.word	doverBop
	.word	drotBop
	
	@ memory store/fetch
	.word	storeBop
	.word	cstoreBop
	.word	cfetchBop
	.word	scfetchBop
	.word	c2lBop
	.word	wstoreBop
	.word	wfetchBop
	.word	swfetchBop
	.word	w2lBop
	.word	dstoreBop
	.word	memcpyBop
	.word	memsetBop
	.word	setVarActionBop
	.word	getVarActionBop
	
	@ string manipulation
	.word	strcpyBop
	.word	strncpyBop
	.word	strlenBop
	.word	strcatBop
	.word	strncatBop
	.word	strchrBop
	.word	strrchrBop
	.word	strcmpBop
	.word	stricmpBop
	.word	strstrBop
	.word	strtokBop
	
	@ file manipulation
	.word	extOp		@ fopenBop
	.word	extOp		@ fcloseBop
	.word	extOp		@ fseekBop
	.word	extOp		@ freadBop
	.word	extOp		@ fwriteBop
	.word	extOp		@ fgetcBop
	.word	extOp		@ fputcBop
	.word	extOp		@ feofBop
	.word	extOp		@ ftellBop
	.word	extOp		@ flenBop
	
endOpsTable:
	.word	0
	