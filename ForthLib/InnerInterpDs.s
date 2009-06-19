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
@	R6			SP
@	R7			0xFFFFFF (opvalue mask)
@
@	core (R4) offsets:
@
c_optypes	=	0    	@	table of C opType action routines
c_ops		=	4		@	table of C builtin ops
n_c_ops		=	8		@	number of C builtin ops
n_asm_ops	=	12		@	number of assembler builtin ops
user_ops	=	16		@	table of user defined ops
n_user_ops	=	20		@	number of user defined ops
s_user_ops	=	24		@	current size of user defined op table
engine		=	28		@	ForthEngine pointer
cur_thread	=	32		@	current ForthThread pointer
ipsave		=	36		@	IP - interpreter pointer (r5)
spsave		=	40		@	SP - forth stack pointer (r6)
rpsave		=	44		@	RP - forth return stack pointer
fpsave		=	48		@	FP - frame pointer
tpm			=	52		@	TPM - this pointer (methods)
tpd			=	56		@	TPD - this pointer (data)
varmode		=	60		@	varMode - fetch/store/plusStore/minusStore
istate		=	64		@	state - ok/done/error
errorcode	=	68		@	error code
sp0			=	72		@	empty parameter stack pointer
ssize		=	76		@	size of param stack in longwords
rp0			=	80		@	empty return stack pointer
rsize		=	84		@	size of return stack in longwords
dp			=	88		@	dictionary pointer
dp0			=	92		@	dictionary base
dsize		=	96		@	size of dictionary

core	=	r4
rip		=	r5
rsp		=	r6

	.global	break_me
	.type	   break_me,function
break_me:
	swi	#0xFD0000
	bx	lr
	
@-----------------------------------------------
@
@ extOp is used by "builtin" ops which are only defined in C++
@
	.thumb_func
extOp:
	ldr	r3, [r4, #n_c_ops]
	cmp	r1, r3
	blt	.extOp1
	b	badOpcode

.extOp1:
	bl	.extOp2
	@ we come here when C op does its return
	b	InnerInterpreter
		
.extOp2:
	mov	r0, r4
	@ C ops take pCore ptr as first param
	str	r5, [r4, #ipsave]
	str	r6, [r4, #spsave]
	ldr	r3, [r4, #c_ops]		@ r3 = table of C builtin ops
	lsl	r1, #2
	ldr	r2, [r3, r1]
	bx	r2


@-----------------------------------------------
@
@ extOpType is used to handle optypes which are only defined in C++
@
@	r1 holds the opcode value field
@	r2 holds the opcode type field
@
	.thumb_func
extOpType:
	ldr	r3, [r4]		@ r3 = table of C optype action routines
	lsl	r2, #2
	ldr	r2, [r3, r2]	@ r2 = action routine for this optype
	mov	r0, r4			@ C++ optype routines take core ptr as 1st argument
	str	r5, [r4, #ipsave]
	str	r6, [r4, #spsave]
	bx	r2
	
@-----------------------------------------------
@
@ inner interpreter C entry point
@
@ extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
	.global	InnerInterpreterFast
	.thumb_func
	.type	InnerInterpreterFast, %function
	
InnerInterpreterFast:
	push	{r4, r5, r6, r7, lr}
	mov	r4, r0				@ r4 = pCore

	@
	@ re-enter here if you stomped on rip/rsp/r7	
	@
InnerInterpreter:
	ldr	r5, [r4, #ipsave]		@ r5 = IP
	ldr	r6, [r4, #spsave]		@ r6 = SP
	ldr	r7, .opValueMask				@ r7 = op value mask (0x00FFFFFF)
	
	mov	r3, #0					@ r3 = kResultOk
	str	r3, [r4, #istate]		@ SET_STATE( kResultOk )
	
	mov	r0, r5
	bl	ShowIP
	@bl	break_me	
	
	bl	.II2					@ don't know how to load lr directly
	@
	@ we come back here whenever an op does a "bx lr"
	@
.II1:
	@swi	#0xFD
	ldr	r0, [r4, #istate]		@ get state from core
	lsl	r0, r0, #24				@ mask off top 24 bits
	lsr	r0, r0, #24
	cmp	r0, #0					@ keep looping if state is still ok (zero)
	bne	.IIExit
.II2:
	ldmia	r5!, {r1}			@ r1 = next opcode, advance IP
	ldr	r3, [r4, #n_asm_ops]	@ r3 = number of assembler builtin ops
	cmp	r1, r3
	bge	.II3
	@ handle builtin ops
	ldr	r3, .opsTable
	lsl	r1, #2
	ldr	r0, [r3, r1]
	bx	r0
	

.II3:
	@
	@ opcode is not assembler builtin
	@
	lsr	r2, r1, #24				@ r2 = opType (hibyte of opcode)
	ldr	r3, .opTypesTable		@ r3 = opType action routine table address
	and	r1, r7					@ r1 = low 24 bits of opcode
	mov	r0, r1
	lsl	r0, #2
	ldr	r0, [r3, r0]			@ r0 = action routine for this opType
	@
	@ optype action routines expect r1=opValue, r2=opType, r4=pCore
	@
	bx	r3						@ dispatch to action routine

	
	@
	@ exit inner interpreter
	@	
.IIExit:
	str	r5, [r4, #ipsave]		@ save IP in core
	str	r6, [r4, #spsave]		@ save SP in core
	pop	{r4, r5, r6, r7, pc}
	
	.align	2
.opValueMask:
	.word	0x00FFFFFF
.opsTable:
	.word	opsTable
.opTypesTable:
	.word	opTypesTable
	
@-----------------------------------------------
@
@ inner interpreter entry point for user ops defined in assembler
@
@ extern void UserCodeAction( ForthCoreState *pCore, ulong opVal );

	.global	UserCodeAction
	.thumb_func
UserCodeAction:
	@	r0	=	core
	@	r1	=	opVal
	push	{r4, r5, r6, lr}
	mov	r4, r0				@ r4 = pCore
	ldr	r5, [r4, #ipsave]		@ r5 = IP
	ldr	r6, [r4, #spsave]		@ r6 = SP
	
	ldr	r2, [r4, #user_ops]
	lsl	r1, #2
	add	r2, r1
	ldr	r1, [r2]
	bl	.UserCodeAction1
	@	user op will return here
	str	r5, [r4, #ipsave]		@ save IP in core
	str	r6, [r4, #spsave]		@ save SP in core
	pop	{r4, r5, r6, pc}
		
.UserCodeAction1:
	bx	r1
	
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
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	sub	r0, r1					@ r0 points to the byte field
	@ see if it is a fetch
byteEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	cmp	r1, #0
	bne	.localByte1
	@ fetch local byte
localByteFetch:
	sub	r6, #4
	eor	r2, r2
	ldrb	r2, [r0]
	str	r2, [r6]
	bx	lr

localByteRef:
	sub	r6, #4
	str	r0, [r6]			@ TOS = byte var address
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localByteStore:
	ldr	r2, [r6]
	add	r6, #4
	strb	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localBytePlusStore:
	ldrb	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	add	r2, r1
	strb	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localByteMinusStore:
	ldrb	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	sub	r2, r1
	strb	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr

.localByte1:
	@ r0 points to the byte field
	@ r1 is varMode
	cmp	r1, #4
	ble	.localByte2
	b	badVarOperation
.localByte2:
	lsl	r1, #2
	ldr	r2, .localByte3
	ldr	r2, [r2, r1]
	bx	r2
	
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
	
	ldr	r0, [r6]		@ pop offset off TOS
	add	r6, #4
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
	ldr	r0, [r6]				@ r0 = array index
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	add	r6, #4
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	sub	r0, r1
	b	byteEntry

fieldByteArrayType:
	@ get ptr to byte var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6, #4]			@ r0 = index
	ldr	r2, [r6]				@ r2 = struct base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #8
	b	byteEntry

memberByteArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6]				@ r0 = index
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #4
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
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	lsl	r1, #1
	sub	r0, r1						@ r0 points to the short field
	@ see if it is a fetch
shortEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	cmp	r1, #0
	bne	.localShort1
	@ fetch local short
localShortFetch:
	sub	r6, #4
	eor	r2, r2
	ldrh	r2, [r0]
	str	r2, [r6]
	bx	lr

localShortRef:
	sub	r6, #4
	str	r0, [r6]				@ TOS = short var address
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localShortStore:
	ldr	r2, [r6]
	add	r6, #4
	strh	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localShortPlusStore:
	ldrh	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	add	r2, r1
	strh	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localShortMinusStore:
	ldrh	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	sub	r2, r1
	strh	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr

.localShort1:
	@ r0 points to the short field
	@ r1 is varMode
	cmp	r1, #4
	ble	.localShort2
	b	badVarOperation
.localShort2:
	lsl	r1, #2
	ldr	r2, .localShort3
	ldr	r2, [r2, r1]
	bx	r2

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
	
	ldr	r0, [r6]		@ pop offset off TOS
	add	r6, #4
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
	ldr	r0, [r6]				@ r0 = array index
	lsl	r0, #1
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	add	r6, #4
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	sub	r0, r1
	b	shortEntry

fieldShortArrayType:
	@ get ptr to short var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6, #4]			@ r0 = index
	lsl	r0, #1
	ldr	r2, [r6]				@ r2 = struct base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #8
	b	shortEntry

memberShortArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6]				@ r0 = index
	lsl	r0, #1
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #4
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
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	lsl	r1, #2
	sub	r0, r1						@ r0 points to the int field
	@ see if it is a fetch
intEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	cmp	r1, #0
	bne	.localInt1
	@ fetch local int
localIntFetch:
	sub	r6, #4
	ldr	r2, [r0]
	str	r2, [r6]
	bx	lr

localIntRef:
	sub	r6, #4
	str	r0, [r6]				@ TOS = int var address
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localIntStore:
	ldr	r2, [r6]
	add	r6, #4
	str	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localIntPlusStore:
	ldr	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	add	r2, r1
	str	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localIntMinusStore:
	ldr	r2, [r0]
	ldr	r1, [r6]
	add	r6, #4
	sub	r2, r1
	str	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr

.localInt1:
	@ r0 points to the int field
	@ r1 is varMode
	cmp	r1, #4
	bgt	badVarOperation
	lsl	r1, #2
	ldr	r2, .localInt2
	ldr	r2, [r2, r1]
	bx	r2
	
badVarOperation:
	mov	r0, #4		@ bad var operation error
	str	r0, [r4, #errorcode]
	mov	r0, #3		@ status = error
	str	r0, [r4, #istate]
	bx	lr

	.align	2	
.localInt2:
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
	
	ldr	r0, [r6]		@ pop offset off TOS
	add	r6, #4
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
	ldr	r0, [r6]				@ r0 = array index
	sub	r1, r0					@ add in array index
	add	r6, #4
	lsl	r1, #2
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	sub	r0, r1
	b	intEntry

fieldIntArrayType:
	@ get ptr to int var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6, #4]			@ r0 = index
	lsl	r0, #2
	ldr	r2, [r6]				@ r2 = struct base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #8
	b	intEntry

memberIntArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6]				@ r0 = index
	lsl	r0, #2
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #4
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
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	lsl	r1, #2
	sub	r0, r1						@ r0 points to the float field
	@ see if it is a fetch
floatEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	cmp	r1, #0
	bne	.localFloat1
	@ fetch local float
localFloatFetch:
	sub	r6, #4
	ldr	r2, [r0]
	str	r2, [r6]
	bx	lr

localFloatRef:
	sub	r6, #4
	str	r0, [r6]				@ TOS = float var address
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localFloatStore:
	ldr	r2, [r6]
	add	r6, #4
	str	r2, [r0]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	bx	lr
	
localFloatPlusStore:
	mov	r3, r0
	ldr	r0, [r0]
	ldr	r1, [r6]
	add	r6, #4
	push {lr}
	bl	__aeabi_fadd
	str	r0, [r3]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	pop	{pc}
	
localFloatMinusStore:
	mov	r3, r0
	ldr	r0, [r0]
	ldr	r1, [r6]
	add	r6, #4
	push {lr}
	bl	__aeabi_fsub
	str	r0, [r3]
	eor	r0, r0
	str	r0, [r4, #varmode]		@ set varMode back to fetch
	pop	{pc}

.localFloat1:
	@ r0 points to the float field
	@ r1 is varMode
	cmp	r1, #4
	bgt	badVarOperation
	lsl	r1, #2
	ldr	r2, .localFloat2
	ldr	r2, [r2, r1]
	bx	r2
	
	.align	2	
.localFloat2:
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
	
	ldr	r0, [r6]		@ pop offset off TOS
	add	r6, #4
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
	ldr	r0, [r6]				@ r0 = array index
	sub	r1, r0					@ add in array index
	add	r6, #4
	lsl	r1, #2
	ldr	r0, [r4, #fpsave]			@ r0 = FP
	sub	r0, r1
	b	floatEntry

fieldFloatArrayType:
	@ get ptr to float var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6, #4]			@ r0 = index
	lsl	r0, #2
	ldr	r2, [r6]				@ r2 = struct base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #8
	b	floatEntry

memberFloatArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldr	r0, [r6]				@ r0 = index
	lsl	r0, #2
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	add	r6, #4
	b	floatEntry


@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		           OPS                               @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	@ TBD	abortBop
	
dropBop:
	add	r6, #4
	bx	lr
	
	@ TBD	doDoesBop
	
litBop:
	sub	r6, r6, #4
	ldmia	r5!, {r0}
	str	r0, [r6]
	bx	lr
	
dlitBop:
	ldmia	r5!, {r0}
	ldmia	r5!, {r1}
	sub	r6, #8
	str	r0, [r6]
	str	r1, [r6, #4]
	bx	lr
	
doVariableBop:
	@ IP points to immediate data field
	sub	r6, r6, #4		@ push addr of data field
	str	r5, [r6]		@    pointed to by IP
	ldr	r0, [r4, #rpsave]	@ r0 = RP
	ldmia	r0!, {r5}	@ pop return stack into IP
	str	r0, [r4, #rpsave]
	bx	lr

doConstantBop:
	@ IP points to immediate data field
	sub	r6, r6, #4		@ push data in immediate field
	ldr	r0, [r5]		@    pointed to by IP
	str	r0, [r6]
	ldr	r0, [r4, #rpsave]	@ r0 = RP
	ldmia	r0!, {r5}	@ pop return stack into IP
	str	r0, [r4, #rpsave]
	bx	lr

doDConstantBop:
	@ IP points to immediate data field
	sub	r6, r6, #8		@ push data in immediate field
	ldr	r0, [r5]		@    pointed to by IP
	ldr	r1, [r5, #4]
	str	r0, [r6]
	str	r1, [r6, #4]
	ldr	r0, [r4, #rpsave]	@ r0 = RP
	ldmia	r0!, {r5}	@ pop return stack into IP
	str	r0, [r4, #rpsave]
	bx	lr

doneBop:
	mov	r0, #1
	str	r0, [r4, #istate]
	bx	lr

	@ TBD	doByteBop
	@ TBD	doShortBop
	@ TBD	doIntBop
	@ TBD	doFloatBop
	@ TBD	doDoubleBop
	@ TBD	doStringBop
	@ TBD	doOpBop
	@ TBD	doObjectBop
	
	@ TBD	addressOfBop
	@ TBD	intoBop
	@ TBD	addToBop
	@ TBD	subtractFromBop
	@ TBD	doExitBop
	@ TBD	doExitLBop
	@ TBD	doExitMBop
	@ TBD	doExitMLBop

	@ TBD	doByteArrayBop
	@ TBD	doShortArrayBop
	@ TBD	doIntArrayBop
	@ TBD	doFloatArrayBop
	@ TBD	doDoubleArrayBop
	@ TBD	doStringArrayBop
	@ TBD	doOpArrayBop
	@ TBD	doObjectArrayBop
	@ TBD	initStringBop
	
// A B + ... (A+B)
// forth SP points to B
//
	.thumb_func
.plusBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	add	r2, r1, r2
	str	r2, [r6]
	bx	lr
	
fetchBop:
	ldr	r0, [r6]
	ldr	r1, [r0]
	str	r1, [r6]
	bx	lr

	@ TBD	doStructBop
	@ TBD	doStructArrayBop
	@ TBD	doDoBop
	@ TBD	doLoopBop
	@ TBD	doLoopNBop
	@ TBD	dfetchBop

	@ TBD	thisBop
	@ TBD	thisDataBop
	@ TBD	thisMethodsBop
	@ TBD	executeBop
	
	@ runtime control flow stuff
	@ TBD	callBop
	@ TBD	gotoBop
	@ TBD	iBop
	@ TBD	jBop
	@ TBD	unloopBop
	@ TBD	leaveBop
	@ TBD	hereBop
	@ TBD	addressOfBop
	@ TBD	intoBop
	@ TBD	addToBop
	@ TBD	subtractFromBop
	@ TBD	removeEntryBop
	@ TBD	entryLengthBop
	@ TBD	numEntriesBop
	
	
	@ integer math

times2Bop:
	ldr	r0, [r6]
	lsl	r0, r0, #1
	str	r0, [r6]
	bx	lr
	
times4Bop:
	ldr	r0, [r6]
	lsl	r0, r0, #2
	str	r0, [r6]
	bx	lr
	
	.global	__aeabi_idiv
divideBop:
	push	{lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	__aeabi_idiv
	str	r0, [r6]
	pop	{pc}
	
divide2Bop:
	ldr	r0, [r6]
	asr	r0, r0, #1
	str	r0, [r6]
	bx	lr
	
divide4Bop:
	ldr	r0, [r6]
	asr	r0, r0, #2
	str	r0, [r6]
	bx	lr
	
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
	
modBop:
	.global	__aeabi_idivmod
	push	{lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	__aeabi_idivmod
	str	r1, [r6]
	pop	{pc}
	
negateBop:
	ldr	r0, [r6]
	neg	r0, r0
	str	r0, [r6]
	bx	lr
	
// A B - ... (A-B)
//
minusBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	sub	r2, r1, r2
	str	r2, [r6]
	bx	lr

timesBop:
	ldmia	r6!, {r0}	@ r0 = B
	ldr	r1, [r6]		@ r1 = A
	mul	r1, r0
	str	r1, [r6]
	bx	lr
	
	@ single precision fp math
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
	
	@ double precision fp math
		.global __aeabi_dadd, __aeabi_dsub, __aeabi_dmul, __aeabi_ddiv
	@ TBD	dplusBop
	@ TBD	dminusBop
	@ TBD	dtimesBop
	@ TBD	ddivideBop
	
dplusBop:
	push {lr}
	ldmia	r6!, {r0, r1}
	ldr	r2, [r6]
	ldr	r3, [r6]
	bl	__aeabi_dadd
	str	r0, [r6]
	str	r1, [r6, #4]
	pop	{pc}

dminusBop:
	push {lr}
	ldmia	r6!, {r0, r1}
	ldr	r2, [r6]
	ldr	r3, [r6]
	bl	__aeabi_dsub
	str	r0, [r6]
	str	r1, [r6, #4]
	pop	{pc}

dtimesBop:
	push {lr}
	ldmia	r6!, {r0, r1}
	ldr	r2, [r6]
	ldr	r3, [r6]
	bl	__aeabi_dmul
	str	r0, [r6]
	str	r1, [r6, #4]
	pop	{pc}

ddivideBop:
	push {lr}
	ldmia	r6!, {r0, r1}
	ldr	r2, [r6]
	ldr	r3, [r6]
	bl	__aeabi_ddiv
	str	r0, [r6]
	str	r1, [r6, #4]
	pop	{pc}

	
	@ double precision fp functions	
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
	
	@ int/float/double conversions
	@ TBD	i2fBop
	@ TBD	i2dBop
	@ TBD	f2iBop
	@ TBD	f2dBop
	@ TBD	d2iBop
	@ TBD	d2fBop
	
	@ bit-vector logic
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
	
trueBop:
	eor	r0, r0
	sub	r0, #1
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
falseBop:
	mov	r0, #0
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
nullBop:
	mov	r0, #0
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
dnullBop:
	mov	r0, #0
	sub	r6, #8
	str	r0, [r6]
	str	r0, [r6, #4]
	bx	lr
	
	@ integer comparisons
	@ TBD	equalsBop
	@ TBD	notEqualsBop
	@ TBD	greaterThanBop
	@ TBD	greaterEqualsBop
	@ TBD	lessThanBop
	@ TBD	lessEqualsBop
	@ TBD	equalsZeroBop
	@ TBD	notEqualsZeroBop
	@ TBD	greaterThanZeroBop
	@ TBD	greaterEqualsZeroBop
	@ TBD	lessThanZeroBop
	@ TBD	lessEqualsZeroBop
	@ TBD	unsignedGreaterThanBop
	@ TBD	unsignedLessThanBop
	
	@ stack manipulation
rpushBop:
	ldr	r0, [r6]
	add	r6, #4
	ldr	r1, [r4, #rpsave]
	sub	r1, #4
	str	r0, [r1]
	str	r1, [r4, #rpsave]
	bx	lr

rpopBop:
	ldr	r1, [r4, #rpsave]
	ldr	r0, [r1]
	add	r1, #4
	str	r1, [r4, #rpsave]
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
rdropBop:
	ldr	r1, [r4, #rpsave]
	add	r1, #4
	str	r1, [r4, #rpsave]
	bx	lr
	
rpBop:
	ldr	r0, [r4, #rpsave]
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
rzeroBop:
	ldr	r0, [r4, #rp0]
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
dupBop:
	ldr	r0, [r6]
	sub	r6, #4
	str	r0, [r6]
	bx	lr

swapBop:
	ldr	r0, [r6]
	ldr	r1, [r6, #4]
	ldr	r1, [r6]
	ldr	r0, [r6, #4]
	bx	lr
		
overBop:
	sub	r6, #4
	ldr	r0, [r6, #8]
	str	r0, [r6]
	bx	lr

	@ TBD	rotBop
	@ TBD	tuckBop
	@ TBD	pickBop
	
spBop:
	mov	r0, r6
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
szeroBop:
	ldr	r0, [r4, #sp0]
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
fpBop:
	ldr	r0, [r4, #fpsave]
	sub	r6, #4
	str	r0, [r6]
	bx	lr
	
	@ TBD	ddupBop
	@ TBD	dswapBop
	@ TBD	ddropBop
	@ TBD	doverBop
	@ TBD	drotBop
	
	@ memory store/fetch
storeBop:
	ldr	r0, [r6]			@ r0 = int ptr
	ldr	r1, [r6, #4]		@ r1 = value
	add	r6,#8
	str	r1, [r0]
	bx	lr
		
cstoreBop:
	ldr	r0, [r6]			@ r0 = byte ptr
	ldr	r1, [r6, #4]		@ r1 = value
	add	r6,#8
	strb	r1, [r0]
	bx	lr

cfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrb	r1, [r0]
	str	r1, [r6]
	bx	lr
			
	@ TBD	scfetchBop
	@ TBD	c2lBop
	
wstoreBop:
	ldr	r0, [r6]			@ r0 = byte ptr
	ldr	r1, [r6, #4]		@ r1 = value
	add	r6,#8
	strh	r1, [r0]
	bx	lr

wfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrh	r1, [r0]
	str	r1, [r6]
	bx	lr
			
	@ TBD	swfetchBop
	@ TBD	w2lBop
	@ TBD	dstoreBop
	@ TBD	memcpyBop
	@ TBD	memsetBop
	@ TBD	setVarActionBop
	@ TBD	getVarActionBop
	
	@ string manipulation
	@ TBD	strcpyBop
	@ TBD	strncpyBop
	@ TBD	strlenBop
	@ TBD	strcatBop
	@ TBD	strncatBop
	@ TBD	strchrBop
	@ TBD	strrchrBop
	@ TBD	strcmpBop
	@ TBD	stricmpBop
	@ TBD	strstrBop
	@ TBD	strtokBop
	
	@ file manipulation
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
@	core	core pointer
@	rip		IP
@	rsp		SP
@	r0...r3 can be stomped on at will

builtInOp:
	@ we get here when an opcode has the builtin optype but there is no assembler version
	b	extOp
	
builtInImmediate:
	ldr	r3, [r4, #n_asm_ops]
	cmp	r1, r3
	blt	.builtInImmediate1
	b	extOp

.builtInImmediate1:
	ldr	r3, =opsTable
	lsl	r1, #2
	ldr	r0, [r3, r1]
	bx	r0

	
userCodeType:
	ldr	r3, [r4, #n_user_ops]
	cmp	r1, r3
	bge	badOpcode
	ldr	r3, [r4, #user_ops]
	lsl	r1, #2
	ldr	r0, [r3, r1]
	bx	r0


badOpcode:
	mov	r0, #1		@ bad opcode error
	str	r0, [r4, #errorcode]
	mov	r0, #3		@ status = error
	str	r0, [r4, #istate]
	bx	lr
	
userDefType:
	ldr	r3, [r4, #n_user_ops]
	cmp	r1, r3
	bge	badOpcode
	@ rpush IP
	ldr	r0, [r4, #rpsave]
	sub	r0, #4
	str	r5, [r0]
	str	r0, [r4, #rpsave]
	@ fetch new IP
	ldr	r0, [r4, #user_ops]
	lsl	r1, #2
	add	r0, r1
	ldr	r5, [r0]
	bx	lr
	
branchType:
	lsl	r3, r1, #8		@ see if bit 23 is set, if so branchVal is negative
	bpl	.branchType1
	ldr	r3, .branchType2
	orr	r1, r3
.branchType1:
	lsl	r1, #2
	add	r5, r1			@ add branchVal to IP
	bx	lr
	.align 2
.branchType2:
	.word	0xFF000000

branchNZType:
	ldr	r0, [r6]		@ r0 = TOS
	add	r6, #4
	cmp	r0, #0
	bne	branchType
	bx	lr
	
branchZType:
	ldr	r0, [r6]		@ r0 = TOS
	add	r6, #4
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
	lsl	r3, r1, #8		@ see if bit 23 is set, if so branchVal is negative
	bpl	.constantType1
	ldr	r3, .constantType2
	orr	r1, r3
.constantType1:
	sub	r6, #4
	str	r1, [r6]
	bx	lr
	.align	2
.constantType2:
	.word	0xFF000000
	
offsetType:
	lsl	r3, r1, #8		@ see if bit 23 is set, if so branchVal is negative
	bpl	.offsetType1
	ldr	r3, .offsetType2
	orr	r1, r3
.offsetType1:
	ldr	r0, [r6]
	add	r1, r0
	str	r1, [r6]
	bx	lr
	.align	2
.offsetType2:
	.word	0xFF000000
	
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
	.thumb_func
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
	.word	extOp		@ abortBop
	.word	dropBop
	.word	extOp		@ doDoesBop
	.word	litBop
	.word	litBop
	.word	dlitBop
	.word	doVariableBop
	.word	doConstantBop
	.word	doDConstantBop
	.word	extOp		@ endBuildsBop
	.word	doneBop
	.word	extOp		@ doByteBop
	.word	extOp		@ doShortBop
	.word	extOp		@ doIntBop
	.word	extOp		@ doFloatBop
	.word	extOp		@ doDoubleBop
	.word	extOp		@ doStringBop
	.word	extOp		@ doOpBop
	.word	extOp		@ doObjectBop
	.word	extOp		@ addressOfBop
	.word	extOp		@ intoBop
	.word	extOp		@ addToBop
	.word	extOp		@ subtractFromBop
	.word	extOp		@ doExitBop
	.word	extOp		@ doExitLBop
	.word	extOp		@ doExitMBop
	.word	extOp		@ doExitMLBop
	.word	extOp		@ doVocabBop
	.word	extOp		@ doByteArrayBop
	.word	extOp		@ doShortArrayBop
	.word	extOp		@ doIntArrayBop
	.word	extOp		@ doFloatArrayBop
	.word	extOp		@ doDoubleArrayBop
	.word	extOp		@ doStringArrayBop
	.word	extOp		@ doOpArrayBop
	.word	extOp		@ doObjectArrayBop
	.word	extOp		@ initStringBop
	.word	extOp		@ initStringArrayBop
	.word	plusBop
	.word	fetchBop
	.word	extOp		@ badOpBop
	.word	extOp		@ doStructBop
	.word	extOp		@ doStructArrayBop
	.word	extOp		@ doStructTypeBop
	.word	extOp		@ doClassTypeBop
	.word	extOp		@ doEnumBop
	.word	extOp		@ doDoBop
	.word	extOp		@ doLoopBop
	.word	extOp		@ doLoopNBop
	.word	extOp		@ doNewBop
	.word	extOp		@ dfetchBop

	@	
    @ stuff below this line can be rearranged
    @
	.word	extOp		@ thisBop
	.word	extOp		@ thisDataBop
	.word	extOp		@ thisMethodsBop
	.word	extOp		@ executeBop
	@ runtime control flow stuff
	.word	extOp		@ callBop
	.word	extOp		@ gotoBop
	.word	extOp		@ iBop
	.word	extOp		@ jBop
	.word	extOp		@ unloopBop
	.word	extOp		@ leaveBop
	.word	extOp		@ hereBop
	.word	extOp		@ addressOfBop
	.word	extOp		@ intoBop
	.word	extOp		@ addToBop
	.word	extOp		@ subtractFromBop
	.word	extOp		@ removeEntryBop
	.word	extOp		@ entryLengthBop
	.word	extOp		@ numEntriesBop
	
	
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
	.word	extOp		@ orBop
	.word	extOp		@ andBop
	.word	extOp		@ xorBop
	.word	extOp		@ invertBop
	.word	extOp		@ lshiftBop
	.word	extOp		@ rshiftBop
	
	@ boolean logic
	.word	extOp		@ notBop
	.word	extOp		@ trueBop
	.word	extOp		@ falseBop
	.word	extOp		@ nullBop
	.word	extOp		@ dnullBop
	
	@ integer comparisons
	.word	extOp		@ equalsBop
	.word	extOp		@ notEqualsBop
	.word	extOp		@ greaterThanBop
	.word	extOp		@ greaterEqualsBop
	.word	extOp		@ lessThanBop
	.word	extOp		@ lessEqualsBop
	.word	extOp		@ equalsZeroBop
	.word	extOp		@ notEqualsZeroBop
	.word	extOp		@ greaterThanZeroBop
	.word	extOp		@ greaterEqualsZeroBop
	.word	extOp		@ lessThanZeroBop
	.word	extOp		@ lessEqualsZeroBop
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
	.word	extOp		@ rotBop
	.word	extOp		@ tuckBop
	.word	extOp		@ pickBop
	.word	extOp		@ rollBop
	.word	spBop
	.word	szeroBop
	.word	fpBop
	.word	extOp		@ ddupBop
	.word	extOp		@ dswapBop
	.word	extOp		@ ddropBop
	.word	extOp		@ doverBop
	.word	extOp		@ drotBop
	
	@ memory store/fetch
	.word	storeBop
	.word	cstoreBop
	.word	cfetchBop
	.word	extOp		@ scfetchBop
	.word	extOp		@ c2lBop
	.word	wstoreBop
	.word	wfetchBop
	.word	extOp		@ swfetchBop
	.word	extOp		@ w2lBop
	.word	extOp		@ dstoreBop
	.word	extOp		@ memcpyBop
	.word	extOp		@ memsetBop
	.word	extOp		@ setVarActionBop
	.word	extOp		@ getVarActionBop
	
	@ string manipulation
	.word	extOp		@ strcpyBop
	.word	extOp		@ strncpyBop
	.word	extOp		@ strlenBop
	.word	extOp		@ strcatBop
	.word	extOp		@ strncatBop
	.word	extOp		@ strchrBop
	.word	extOp		@ strrchrBop
	.word	extOp		@ strcmpBop
	.word	extOp		@ stricmpBop
	.word	extOp		@ strstrBop
	.word	extOp		@ strtokBop
	
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
	