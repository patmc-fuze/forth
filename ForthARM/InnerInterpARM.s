@-----------------------------------------------
@
@	Raspberry Pi ARM11 inner interpreter
@
@
        .text
        .align  2
        
        .arch armv6
        .fpu vfp
        
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
ops				=	8			@	table of ops
n_ops			=	12			@	number of ops
max_ops			=	16			@	current size of op table
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
spbase			=	60			@	base of parameter stack storage
sp0				=	64			@	empty parameter stack pointer
ssize			=	68			@	size of param stack in longwords
rpbase			=	72			@	base of return stack storage
rp0				=	76			@	empty return stack pointer
rsize			=	80			@	size of return stack in longwords

@	end of stuff which is per thread

cur_thread		=	84			@	current ForthThread pointer
dict_mem_sect	=	88			@	dictionary memory section pointer
file_funcs		=	92			@
inner_loop		=	96			@	inner interpreter asm reentry point


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

@ ForthMemorySection
FMSCurrent				=	0	@ current pointer into section
FMSBase					=	4	@ base of memory section
FMSLen					=	8	@ total size of memory section

@ file functions in pCore->file_funcs
fileOpen		=		0
fileClose		=		4
fileRead		=		8
fileWrite		=		12
fileGetChar		=		16
filePutChar		=		20
fileAtEnd		=		24
fileExists		=		28
fileSeek		=		32
fileTell		=		36
fileGetLength	=		40
fileGetString	=		44
filePutString	=		48

	.global	abortBop, dropBop, nipBop, doDoesBop, litBop, litBop, dlitBop, doVariableBop, doConstantBop, doDConstantBop, doneBop
	.global	doByteBop, doShortBop, doIntBop, doFloatBop, addressOfBop, intoBop, addToBop, subtractFromBop, doExitBop
	.global	doExitLBop, doExitMBop, doExitMLBop, doVocabBop, doByteArrayBop, doShortArrayBop, doIntArrayBop, doFloatArrayBop
	.global	plusBop, fetchBop, doStructBop, doStructArrayBop, doDoBop, doLoopBop, doLoopNBop, dfetchBop
	.global	thisBop, thisDataBop, thisMethodsBop, executeBop, callBop, gotoBop, iBop, jBop, unloopBop, leaveBop, hereBop
	.global	addressOfBop, intoBop, addToBop, subtractFromBop, removeEntryBop, entryLengthBop, numEntriesBop, minusBop
	.global	timesBop, times2Bop, times4Bop, times8Bop, divideBop, divide2Bop, divide4Bop, divide8Bop, divmodBop, modBop, negateBop
	.global fplusBop, fminusBop, ftimesBop, fdivideBop,	dplusBop, dminusBop, dtimesBop, ddivideBop
	.global orBop, andBop, xorBop, invertBop, lshiftBop, rshiftBop, notBop, trueBop, falseBop, nullBop, dnullBop
	.global equalsBop, notEqualsBop, greaterThanBop, greaterEqualsBop, lessThanBop, lessEqualsBop
	.global equalsZeroBop, notEqualsZeroBop, greaterThanZeroBop, greaterEqualsZeroBop, lessThanZeroBop, lessEqualsZeroBop
	.global rpushBop, rpopBop, rdropBop, rpBop, rzeroBop, dupBop, checkDupBop, swapBop, overBop
	.global rotBop, tuckBop, pickBop, spBop, szeroBop, fpBop, ddupBop, dswapBop, ddropBop, doverBop, drotBop,
	.global storeBop, cstoreBop, cfetchBop, scfetchBop, c2iBop, wstoreBop, wfetchBop, swfetchBop, w2iBop, dstoreBop
	.global	memcpyBop, memsetBop, setVarActionBop, getVarActionBop, strcpyBop, strncpyBop, strlenBop, strcatBop, strncatBop
	.global	strchrBop, strrchrBop, strcmpBop, stricmpBop, strstrBop, strtokBop, initStringBop
	.global doUByteBop, doUByteArrayBop, doUShortBop, doUShortArrayBop
	.global dsinBop, dasinBop, dcosBop, dacosBop, dtanBop, datanBop, datan2Bop, dexpBop, dlnBop, dlog10Bop, dpowBop
	.global dsqrtBop, dceilBop, dfloorBop, dabsBop, dldexpBop, dfrexpBop, dmodfBop, dfmodBop, i2fBop, i2dBop, f2iBop, f2dBop, d2iBop, d2fBop
	.global ipBop, startTupleBop, endTupleBop, rpeekBop, minBop, maxBop, storeNextBop, fetchNextBop, cstoreNextBop, cfetchNextBop
	.global wstoreNextBop, wfetchNextBop, dstoreNextBop, dfetchNextBop
	.global fEqualsBop, fNotEqualsBop, fGreaterThanBop, fGreaterEqualsBop, fLessThanBop, fLessEqualsBop
	.global fEqualsZeroBop, fNotEqualsZeroBop, fGreaterThanZeroBop, fGreaterEqualsZeroBop, fLessThanZeroBop, fLessEqualsZeroBop
	.global dEqualsBop, dNotEqualsBop, dGreaterThanBop, dGreaterEqualsBop, dLessThanBop, dLessEqualsBop
	.global dEqualsZeroBop, dNotEqualsZeroBop, dGreaterThanZeroBop, dGreaterEqualsZeroBop, dLessThanZeroBop, dLessEqualsZeroBop
	.global byteVarActionBop, ubyteVarActionBop, shortVarActionBop, ushortVarActionBop, intVarActionBop, floatVarActionBop
	.global fopenBop, fcloseBop, fseekBop, freadBop, fwriteBop, fgetcBop, fputcBop, feofBop, fexistsBop, ftellBop, flenBop, fgetsBop, fputsBop
	.global withinBop, fMinBop, fMaxBop, fWithinBop, dMinBop, dMaxBop, dWithinBop, unsignedGreaterThanBop, unsignedLessThanBop
	.global utimesBop, urshiftBop, reverseRotBop, vocabToClassBop, doCheckDoBop
	@ these are just dummies:
	.global doLongBop, doDoubleBop, doStringBop, doOpBop, doObjectBop, doLongArrayBop, doDoubleArrayBop, doObjectArrayBop, doStringArrayBop, doOpArrayBop
	.global longVarActionBop, doubleVarActionBop, stringVarActionBop, opVarActionBop, objectVarActionBop
	.global fprintfSub, fscanfSub, sprintfSub, sscanfSub
	.global InterpretOneOpFast
	
	.global	break_me
	.type	   break_me,function
break_me:
	swi	#0xFD0000
	bx	lr
	
@-----------------------------------------------
@
@ single step a thread
@
@ extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
	.global	InterpretOneOpFast
	.type	InterpretOneOpFast, %function
InterpretOneOpFast:
	push	{r4-r12, lr}
	mov	r4, r0				@ r4 = pCore
	add	r0, r4, #ipsave
	ldmia	r0!, {r5-r8}		@ load IP, SP, RP, FP from core
	ldr	r9, [r4, #ops]		@ r9 = table of ops
	ldr	r10, [r4, #n_ops]	@ r10 = number of ops implemented in assembler
	ldr	r11, .opTypesTable		@ r11 = table of opType handlers
	ldr	r12, =0x00FFFFFF		@ r12 = op value mask (0x00FFFFFF)
	
	mov	r3, #kResultOk			@ r3 = kResultOk
	str	r3, [r4, #istate]		@ SET_STATE( kResultOk )
	
	bl	.II2					@ don't know how to load lr directly
	b	.IIExit


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
	ldr	r9, [r4, #ops]		@ r9 = table of ops
	ldr	r10, [r4, #n_ops]	@ r10 = number of ops implemented in assembler
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
	lsr	r2, r0, #24				@ r2 = opType (hibyte of opcode)
	and	r1, r12, r0				@ r1 = low 24 bits of opcode
	ldr	r0, [r11, r2, lsl #2]	@ r0 = action routine for this opType
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
.opTypesTable:
	.word	opTypesTable
	
@-----------------------------------------------
@
@ cCodeType is used by "builtin" ops which are only defined in C++
@
@	r1 holds the opcode value field
@
cCodeType:
	@ r1 is low 24-bits of opcode
	ldr	r3, [r4, #n_ops]
	cmp	r1, r3
	bge	badOpcode

	bl	.cct2
	
	@ we come here when C op does its return
	ldrb	r0, [r4, #istate]	@ get state from core
	cmp	r0, #kResultOk			@ exit if state is not ok
	bne	.IIExit
	b	InnerInterpreter
		
.cct2:
	add	r0, r4, #ipsave			@ r0 -> IP
	stmia	r0!, {r5-r8}		@ save IP, SP, RP, FP in core
	mov	r0, r4					@ C ops take pCore ptr as first param
	ldr	r3, [r4, #ops]			@ r3 = table of ops
	ldr	r2, [r3, r1, lsl #2]
	bx	r2


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
	mov	r4, r0
	add	r2, r4, #ipsave
	ldmia	r2!, {r5-r8}			@ load IP, SP, RP, FP from core
	
	ldr	r2, [r4, #ops]
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
	ldrsb	r2, [r0]
	stmdb	r6!, {r2}				@ push byte on TOS
	bx	lr

@------------ unsigned byte support ----------------
localUByteType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the byte field
	@ see if it is a fetch

ubyteEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localByte1

@ fetch local unsigned byte
localUByteFetch:
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
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localByte3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2
.localByte3:
	.word	localByteActionTable
	
localByteActionTable:
	.word	localByteFetch
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

@----------------- unsigned byte ---------------------------------------
fieldUByteType:
	@ get ptr to byte var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	ubyteEntry	

memberUByteType:
	@ get ptr to byte var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	ubyteEntry	
	
localUByteArrayType:
	@ get ptr to byte var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	sub	r0, r8, r1				@ r0 points to the byte field
	b	ubyteEntry

fieldUByteArrayType:
	@ get ptr to byte var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ubyteEntry

memberUByteArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ubyteEntry


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
	ldrsh	r2, [r0]
	stmdb	r6!, {r2}				@ push short on TOS
	bx	lr

@------------ unsigned short support ----------------
localUShortType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the byte field
ushortEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localShort1
@ fetch local unsigned short
localUShortFetch:
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
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localByte3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1

	.align	2
.localShort3:
	.word	localShortActionTable
	
localShortActionTable:
	.word	localShortFetch
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

@----------------- unsigned short ---------------------------------------

fieldUShortType:
	@ get ptr to short var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	ushortEntry	

memberUShortType:
	@ get ptr to short var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	ushortEntry	
	
localUShortArrayType:
	@ get ptr to short var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	lsl	r1, #2
	lsl	r0, #1					@ convert short index to byte offset
	sub	r1, r0					@ add in array index
	sub	r0, r8, r1				@ r0 points to the byte field
	b	ushortEntry

fieldUShortArrayType:
	@ get ptr to short var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #1
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ushortEntry

memberUShortArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #1
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ushortEntry


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
	cmp	r1, #kVarMinusStore
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
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localFloat3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localFloat3:
	.word	localFloatActionTable

localFloatActionTable:
	.word	localFloatFetch
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


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     double
@

@-----------------------------------------------
@
@ local double ops
@
localDoubleType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> double
@
doubleEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localDouble1
	
@
@ these routines can rely on:
@	r0 -> double
@	r2 = 0
@

	@ fetch local double
localDoubleFetch:
	ldrd	r2, [r0]
	stmdb	r6!, {r2,r3}			@ push double on TOS
	bx	lr

localDoubleRef:
	stmdb	r6!, {r0}				@ push address of double on TOS
	bx	lr
	
localDoubleStore:
	ldmia	r6!, {r2,r3}			@ pop TOS double value into r2
	strd	r2, [r0]				@ store double
	bx	lr
	
localDoublePlusStore:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r0]
	faddd	d7, d6, d7
	fstd	d7, [r0]
	bx	lr
	
localDoubleMinusStore:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r0]
	fsubd	d7, d6, d7
	fstd	d7, [r0]
	bx	lr

.localDouble1:
	@ r0 points to the double field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localDouble3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localDouble3:
	.word	localDoubleActionTable

localDoubleActionTable:
	.word	localDoubleFetch
	.word	localDoubleFetch
	.word	localDoubleRef
	.word	localDoubleStore
	.word	localDoublePlusStore
	.word	localDoubleMinusStore

fieldDoubleType:
	@ get ptr to double var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	doubleEntry	

memberDoubleType:
	@ get ptr to double var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	doubleEntry	
	
localDoubleArrayType:
	@ get ptr to double var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, r8, r1				@ r0 points to the double field
	b	doubleEntry

fieldDoubleArrayType:
	@ get ptr to double var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	doubleEntry

memberDoubleArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	doubleEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     long
@

@-----------------------------------------------
@
@ local long ops
@
localLongType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> long
@
longEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localLong1
	
@
@ these routines can rely on:
@	r0 -> long
@	r2 = 0
@

	@ fetch local long
localLongFetch:
	ldrd	r2, [r0]
	stmdb	r6!, {r2,r3}			@ push long on TOS
	bx	lr

localLongRef:
	stmdb	r6!, {r0}				@ push address of long on TOS
	bx	lr
	
localLongStore:
	ldmia	r6!, {r2,r3}			@ pop TOS long value into r2,r3
	strd	r2, [r0]				@ store long
	bx	lr
	
localLongPlusStore:
	ldmia	r6!, {r2,r3}			@ pop TOS long value into r2,r3
	push	{r4, r5}
	ldm	r0!, {r4, r5}
	adds	r4, r2, r4
	adc	r5, r3, r5
	stm	r0!, {r4,r5}
	pop	{r4, r5}
	bx	lr
	
localLongMinusStore:
	ldmia	r6!, {r2,r3}			@ pop TOS long value into r2,r3
	push	{r4, r5}
	ldm	r0!, {r4, r5}
	subs	r4, r2, r4
	sbc	r5, r3, r5
	stm	r0!, {r4,r5}
	pop	{r4, r5}
	bx	lr

.localLong1:
	@ r0 points to the long field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localLong3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localLong3:
	.word	localLongActionTable

localLongActionTable:
	.word	localLongFetch
	.word	localLongFetch
	.word	localLongRef
	.word	localLongStore
	.word	localLongPlusStore
	.word	localLongMinusStore

fieldLongType:
	@ get ptr to long var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	longEntry	

memberLongType:
	@ get ptr to long var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	longEntry	
	
localLongArrayType:
	@ get ptr to long var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, r8, r1				@ r0 points to the long field
	b	longEntry

fieldLongArrayType:
	@ get ptr to long var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	longEntry

memberLongArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	longEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     string
@

@-----------------------------------------------
@
@ local string ops
@
localStringType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> string
@
stringEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localString1
	
@
@ these routines can rely on:
@	r0 -> string
@	r2 = 0
@

	@ fetch local string
localStringFetch:
	add	r0, #8						@ skip maxLen, curLen fields
	stmdb	r6!, {r0}				@ push string on TOS
	bx	lr

localStringRef:
	stmdb	r6!, {r0}				@ push address of string maxLen field on TOS
	bx	lr
	
localStringStore:
	push	{r4}
	ldmia	r6!, {r1}				@ r1 -> src string
	ldr	r2, [r0]					@ r2 = max chars to move
	add	r3, r0, #8					@ r3 -> dst string
.lss1:
	subs	r2, #1						@ leave space for terminating null
	beq	.lssx
	ldrb	r4, [r1]
	orrs	r4, r4
	beq	.lssx						@ hit terminating null in src
	add	r1, #1
	strb	r4, [r3]
	add	r3, #1
	b	.lss1
.lssx:
	eor	r2, r2
	strb	r2, [r3]
	sub	r3, r3, r0
	sub	r3, #8
	str	r3,	[r0, #4]				@ update curlen field
	pop	{r4}
	bx	lr
	
localStringPlusStore:
	push	{r4}
	ldmia	r6!, {r1}				@ r1 -> src string
	ldr	r2, [r0]					@ r2 = max chars to move
	add	r3, r0, #8					@ r3 -> dst string
	ldr	r4, [r0, #4]				@ r4 = current len
	add	r3, r4
	cmp	r4, r2
	blt	.lss1
	pop	{r4}
	bx	lr
	
.localString1:
	@ r0 points to the string field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarPlusStore
	bgt	badVarOperation
	ldr	r3, .localString3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localString3:
	.word	localStringActionTable

localStringActionTable:
	.word	localStringFetch
	.word	localStringFetch
	.word	localStringRef
	.word	localStringStore
	.word	localStringPlusStore

fieldStringType:
	@ get ptr to string var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	stringEntry	

memberStringType:
	@ get ptr to string var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	stringEntry	
	
localStringArrayType:
	@ get ptr to string var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, r8, r1				@ r0 points to the string field
	b	stringEntry

fieldStringArrayType:
	@ get ptr to string var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	stringEntry

memberStringArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	stringEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     op
@

@-----------------------------------------------
@
@ local op ops
@
localOpType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the op field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> op
@
opEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localOp1
	@ execute local op
localOpExecute:
	ldr	r0, [r0]
	b	interpLoopExecuteEntry

@
@ these routines can rely on:
@	r0 -> op
@	r2 = 0
@

.localOp1:
	@ r0 points to the op field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarStore
	bgt	badVarOperation
	ldr	r3, .localOp3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	
	.align	2	
.localOp3:
	.word	localOpActionTable

localOpActionTable:
	.word	localOpExecute
	.word	localIntFetch
	.word	localIntRef
	.word	localIntStore

fieldOpType:
	@ get ptr to op var opo r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	opEntry	

memberOpType:
	@ get ptr to op var opo r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	opEntry	
	
localOpArrayType:
	@ get ptr to op var opo r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, r8, r1				@ r0 points to the op field
	b	opEntry

fieldOpArrayType:
	@ get ptr to op var opo r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	opEntry

memberOpArrayType:
	@ get ptr to opo var opo r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	opEntry


@/////////////////////////////////////////////////////////////////////
@///
@//
@/                     object
@

@-----------------------------------------------
@
@ local object ops
@
localObjectType:
	lsl	r1, #2
	sub	r0, r8, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> object
@
objectEntry:
	ldr	r1, [r4, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localObject1
	
@
@ these routines can rely on:
@	r0 -> object
@	r2 = 0
@

	@ fetch local object
localObjectFetch:
	ldrd	r2, [r0]
	stmdb	r6!, {r2,r3}			@ push object on TOS
	bx	lr

localObjectRef:
	stmdb	r6!, {r0}				@ push address of object on TOS
	bx	lr
	
localObjectStore:
	push	{r4,r5}
	ldmia	r6!, {r4,r5}		@ pop TOS object value into r4,r5
	ldm	r0!, {r2,r3}			@ r2,r3 are oldObj
	cmp	r4, r2
	beq	.losx
	orr	r4, r4
	beq	.los1					@ if newObj data ptr is null, don't increment refcount
	ldr	r1, [r4]				@ increment newObj refcount
	add	r1, #1
	str	r1, [r4]
.los1:
	orr	r2, r2
	beq	.los2
	ldr	r1, [r2]				@ decrement oldObj refcount
	sub	r1, #1
	str	r1, [r2]
	beq	.los3					@ oldObj refcount went to zero, go delete it
.los2:
	str	r4, [r0]
.losx:
	str	r5, [r0, #4]
	pop	{r4,r5}
	bx	lr
	
.los3:
	@ object var held last reference to oldObj, invoke olbObj.delete method
	ldm	r0!, {r4,r5}			@ store newObj
	pop	{r4,r5}
	@ push pCore->this pair on rstack
	ldr	r0,[r4, #tpm]
	stmdb	r7!, {r0}
	ldr	r0,[r4, #tpd]
	stmdb	r7!, {r0}
	str	r2, [r4, #tpd]
	str	r3, [r4, #tpm]	
	
	ldr	r0, [r3]				@ r0 is method 0 (delete) opcode
	b		interpLoopExecuteEntry	
	
.localObject1:
	@ r0 points to the object field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [r4, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarStore
	bgt	badVarOperation
	ldr	r3, .localObject3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
	.align	2	
.localObject3:
	.word	localObjectActionTable

localObjectActionTable:
	.word	localObjectFetch
	.word	localObjectFetch
	.word	localObjectRef
	.word	localObjectStore

fieldObjectType:
	@ get ptr to object var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	r6!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	objectEntry	

memberObjectType:
	@ get ptr to object var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [r4, #tpd]
	add	r0, r1
	b	objectEntry	
	
localObjectArrayType:
	@ get ptr to object var into r0
	@ FP is base ptr, r1 is offset in objects
	ldmia	r6!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, r8, r1				@ r0 points to the object field
	b	objectEntry

fieldObjectArrayType:
	@ get ptr to object var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	r6!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	objectEntry

memberObjectArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	r6!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [r4, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	objectEntry


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
	
nipBop:
	ldmia	r6!, {r0}
	str	r0, [r6]
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
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dlit  @@@@@@@@@@@@@@@@@

dlitBop:
	ldmia	r5!, {r0, r1}
	mov	r2, r0
	stmdb	r6!, {r1, r2}
	bx	lr
	
@@@@@@@@@@@@@@@@@  _doVariable  @@@@@@@@@@@@@@@@@

doVariableBop:
	@ IP points to immediate data field
	stmdb	r6!,{r5}		@ push addr of data field
							@    pointed to by IP
	ldmia	r7!, {r5}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doConstant  @@@@@@@@@@@@@@@@@

doConstantBop:
	@ IP points to immediate data field
	ldr	r0, [r5]			@ fetch data in immedate field pointed to by IP
	stmdb	r6!, {r0}
	ldmia	r7!, {r5}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doDConstant  @@@@@@@@@@@@@@@@@

doDConstantBop:
	@ IP points to immediate data field
	ldmia	r5!, {r0, r1}
	mov	r2, r0
	stmdb	r6!, {r1, r2}
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

doUByteBop:
	@ get ptr to byte var into r0
	@ IP points to byte var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	ubyteEntry

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

doUByteArrayBop:
	@ get ptr to byte var into r0
	@ IP points to base of byte array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	ubyteEntry

@@@@@@@@@@@@@@@@@  _doShort  @@@@@@@@@@@@@@@@@

@ doShortOp is compiled as the first op in global short vars
@ the short data field is immediately after this op

doShortBop:
	@ get ptr to short var into r0
	@ IP points to short var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	shortEntry

doUShortBop:
	@ get ptr to short var into r0
	@ IP points to short var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	ushortEntry

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

doUShortArrayBop:
	@ get ptr to short var into r0
	@ IP points to base of short array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #1
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	ushortEntry

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
	@ get ptr to float var into r0
	@ IP pofloats to float var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doFloatArray  @@@@@@@@@@@@@@@@@

@ doFloatArrayOp is compiled as the first op in global float arrays
@ the data array is immediately after this op

doFloatArrayBop:
	@ get ptr to float var into r0
	@ IP points to base of float array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doDouble  @@@@@@@@@@@@@@@@@

@ doDoubleOp is compiled as the first op in global double vars
@ the double data field is immediately after this op

doDoubleBop:
	@ get ptr to double var into r0
	@ IP podoubles to double var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	doubleEntry

@@@@@@@@@@@@@@@@@  _doDoubleArray  @@@@@@@@@@@@@@@@@

@ doDoubleArrayOp is compiled as the first op in global double arrays
@ the data array is immediately after this op

doDoubleArrayBop:
	@ get ptr to double var into r0
	@ IP points to base of double array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	doubleEntry


@@@@@@@@@@@@@@@@@  _doString  @@@@@@@@@@@@@@@@@

@ doStringOp is compiled as the first op in global string vars
@ the string data field is immediately after this op

doStringBop:
	@ get ptr to string var into r0
	@ IP postrings to string var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	stringEntry

@@@@@@@@@@@@@@@@@  _doStringArray  @@@@@@@@@@@@@@@@@

@ doStringArrayOp is compiled as the first op in global string arrays
@ the data array is immediately after this op

doStringArrayBop:
	@ get ptr to string var into r0
	@ IP points to base of string array
	mov	r0, r5				@ r0 -> maxLen field of string[0]
	ldr	r1, [r0]			@ r1 = maxLen
	ldmia	r6!, {r2}		@ pop index off pstack
	asr	r1, #2
	add	r1, #3				@ r1 is element length in longs
	mul	r1, r2
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	stringEntry

@@@@@@@@@@@@@@@@@  _doLong  @@@@@@@@@@@@@@@@@
@ doLongOp is compiled as the first op in global long vars
@ the long data field is immediately after this op

doLongBop:
	@ get ptr to long var into r0
	@ IP polongs to long var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	longEntry

@@@@@@@@@@@@@@@@@  _doLongArray  @@@@@@@@@@@@@@@@@

@ doLongArrayOp is compiled as the first op in global long arrays
@ the data array is immediately after this op

doLongArrayBop:
	@ get ptr to long var into r0
	@ IP points to base of long array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	longEntry


@@@@@@@@@@@@@@@@@  _doString  @@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  _doOp  @@@@@@@@@@@@@@@@@

@ doOpOp is compiled as the first op in global op vars
@ the op data field is immediately after this op

doOpBop:
	@ get ptr to op var into r0
	@ IP points to op var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	opEntry

@@@@@@@@@@@@@@@@@  _doOpArray  @@@@@@@@@@@@@@@@@

@ doOpArrayOp is compiled as the first op in global op arrays
@ the data array is immediately after this op

doOpArrayBop:
	@ get ptr to op var into r0
	@ IP points to base of op array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	opEntry


@@@@@@@@@@@@@@@@@  _doObject  @@@@@@@@@@@@@@@@@

@ doObjectOp is compiled as the first op in global object vars
@ the object data field is immediately after this op

doObjectBop:
	@ get ptr to object var into r0
	@ IP points to object var
	mov	r0, r5
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	objectEntry

@@@@@@@@@@@@@@@@@  _doObjectArray  @@@@@@@@@@@@@@@@@

@ doObjectArrayOp is compiled as the first op in global object arrays
@ the data array is immediately after this op

doObjectArrayBop:
	@ get ptr to object var into r0
	@ IP points to base of object array
	mov	r0, r5
	ldmia	r6!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	r7!, {r5}		@ pop IP off rstack
	b	objectEntry
	
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

storeNextBop:
	ldmia	r6!, {r0, r1}	@ r0 = value, r1 = ptr to dest ptr
	ldr	r2, [r1]			@ r2 = dest ptr
	stmia	r2!, {r0}		@ store value and advance dest ptr
	str	r2, [r1]			@ update stored dest ptr
	bx	lr

fetchNextBop:
	ldr	r1, [r6]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldmia	r2!, {r0}		@ r0 = fetched value
	str	r2, [r1]			@ update stored src ptr
	str	r0, [r6]
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
	str	r0,[r7]
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
	str	r0,[r7]
	cmp	r2, #0
	blt	.doLoopNBop1		@ branch if increment is negative

	@ r2 is positive increment
	cmp	r0, r1
	bge	.doLoopNBop2		@ branch if done
	ldr	r5, [r7, #8]		@ go back to top of loop
	bx	lr

.doLoopNBop1:
	@ r2 is negative increment
	cmp	r0, r1
	bl	.doLoopNBop2		@ branch if done
	ldr	r5, [r7, #8]		@ go back to top of loop
	bx	lr
	
.doLoopNBop2:
	add	r7, #12				@ deallocate start index, end index, start loop IP
	bx	lr
	
@@@@@@@@@@@@@@@@@  _checkDo  @@@@@@@@@@@@@@@@@
	
@
@ TOS is start-index
@ TOS+4 is end-index
@ the op right after this one should be a branch
@
doCheckDoBop:
	ldmia	r6!, {r0, r1}	@ r0 = start index, r1 = end index
	cmp	r0, r1
	bge	dcdo1
	
	@ rstack[2] holds top-of-loop IP
	add	r5, #4				@ skip over loop exit branch right after this op
	stmdb	r7!, {r5}		@ rpush start-of-loop IP
	stmdb	r7!, {r1}		@ rpush end index
	stmdb	r7!, {r0}		@ rpush start index
dcdo1:
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
	ldmia	r6!, {r5}
	bx	lr

@@@@@@@@@@@@@@@@@  goto  @@@@@@@@@@@@@@@@@

gotoBop:
	ldmia	r6!, {r5}
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
	ldr	r0, [r4, #dict_mem_sect]
	ldr	r1, [r0, #FMSCurrent]
	stmdb	r6!, {r1}
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

vocabToClassBop:
	mov	r0, #kVocabGetClass
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
	
@@@@@@@@@@@@@@@@@  *8  @@@@@@@@@@@@@@@@@

times8Bop:
	ldr	r0, [r6]
	lsl	r0, r0, #3
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
	
@@@@@@@@@@@@@@@@@  /8  @@@@@@@@@@@@@@@@@

divide8Bop:
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
	
utimesBop:
	ldr	r0, [r6]		@ r0 = B
	ldr	r1, [r6, #4]	@ r1 = A
	umull	r3, r2, r1, r0
	str	r2, [r6]
	str	r3, [r6, #4]
	bx	lr
	
@#############################################@
@                                             @
@         single precision fp math            @
@                                             @
@#############################################@

fplusBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fadds	s15, s14, s15
	fsts	s15, [r6]
	bx	lr

fminusBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fsubs	s15, s14, s15
	fsts	s15, [r6]
	bx	lr

ftimesBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fmuls	s15, s14, s15
	fsts	s15, [r6]
	bx	lr

fdivideBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fdivs	s15, s14, s15
	fsts	s15, [r6]
	bx	lr
	
@#############################################@
@                                             @
@         double precision fp math            @
@                                             @
@#############################################@
	
dplusBop:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r6]
	faddd	d7, d6, d7
	fstd	d7, [r6]
	bx	lr

dminusBop:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r6]
	fsubd	d7, d6, d7
	fstd	d7, [r6]
	bx	lr

dtimesBop:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r6]
	fmuld	d7, d6, d7
	fstd	d7, [r6]
	bx	lr

ddivideBop:
	fldd	d6, [r6]
	add		r6, #8
	fldd	d7, [r6]
	fdivd	d7, d6, d7
	fstd	d7, [r6]
	bx	lr

	
@#############################################@
@                                             @
@       double precision fp functions         @
@                                             @
@#############################################@

dsinBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	sin
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dasinBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	asin
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dcosBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	cos
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dacosBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	acos
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dtanBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	tan
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

datanBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	atan
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

datan2Bop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	atan2
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dexpBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	exp
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dlnBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	log
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dlog10Bop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	log10
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dpowBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	add		r6, #8
	fldd	d1, [r6]
	bl	pow
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dsqrtBop:
	push	{r4, r12, lr}
	fldd	d7, [r6]
	fsqrtd	d7, d7
	fcmpd	d7, d7
	fmstat
	beq	.dsqrt1
	bl	sqrt
	fcpyd	d7, d0
.dsqrt1:
	fmrrd	r2, r3, d7
	strd	r2, [r6]
	pop	{r4, r12, lr}
	bx	lr

dceilBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	ceil
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dfloorBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	bl	floor
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dabsBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	fabsd	d0, d0
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dldexpBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r0}
	fldd	d0, [r6]
	bl	ldexp
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

dfrexpBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	sub	r6, #4
	mov	r0, r6
	bl	frexp
	fstd	d0, [r6, #4]
	pop	{r4, r12, lr}
	bx	lr
	
dmodfBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	sub	r6, #8
	mov	r0, r6
	bl	modf
	fstd	d0, [r6, #8]
	pop	{r4, r12, lr}
	bx	lr
	
dfmodBop:
	push	{r4, r12, lr}
	fldd	d0, [r6]
	add		r6, #8
	fldd	d1, [r6]
	bl	fmod
	fstd	d0, [r6]
	pop	{r4, r12, lr}
	bx	lr

@#############################################@
@                                             @
@       int/float/double conversions          @
@                                             @
@#############################################@

i2fBop:
	ldr	r0, [r6]
	fmsr	s14, r0 @ int
	fsitos	s15, s14
	fsts	s15, [r6]
	bx	lr

i2dBop:
	ldr	r0, [r6]
	sub	r6, #4
	fmsr	s14, r0 @ int
	fsitod	d7, s14
	fstd	d7, [r6]
	bx	lr

	
f2iBop:
	flds	s15, [r6]
	ftosizs	s15, s15
	fmrs	r0, s15 @ int
	str	r0, [r6]
	bx	lr

f2dBop:
	flds	s15, [r6]
	sub	r6, #4
	fcvtds	d7, s15
	fstd	d7, [r6]
	bx	lr

d2iBop:
	fldd	d7, [r6]
	add	r6, #4
	ftosizd	s13, d7
	fmrs	r0, s13 @ int
	str	r0, [r6]
	bx	lr
	
d2fBop:
	fldd	d7, [r6]
	add	r6, #4
	fcvtsd	s15, d7
	fsts	s15, [r6]
	bx	lr
	
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

urshiftBop:
	ldmia	r6!, {r0}		@ r0 = shift count
	ldr	r1, [r6]
	mov	r2, r1, lsr r0
	str	r2, [r6]
	bx	lr
 	
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

@@@@@@@@@@@@@@@@@  u>  @@@@@@@@@@@@@@@@@

unsignedGreaterThanBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	bls	ugt1
	sub	r2, #1
ugt1:
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

@@@@@@@@@@@@@@@@@  u<  @@@@@@@@@@@@@@@@@

unsignedLessThanBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	bcs	ult1
	sub	r2, #1
ult1:
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
@     single precision fp comparisons         @
@                                             @
@#############################################@
	
@@@@@@@@@@@@@@@@@  f=  @@@@@@@@@@@@@@@@@

fEqualsBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	subeq	r2, #1
	str	r2, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  f<>  @@@@@@@@@@@@@@@@@

fNotEqualsBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	subne	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  f>  @@@@@@@@@@@@@@@@@

fGreaterThanBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	subgt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f>=  @@@@@@@@@@@@@@@@@

fGreaterEqualsBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	subge	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f<  @@@@@@@@@@@@@@@@@

fLessThanBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	sublt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f<=  @@@@@@@@@@@@@@@@@

fLessEqualsBop:
	eor	r2, r2
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	suble	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0=  @@@@@@@@@@@@@@@@@

fEqualsZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	subeq	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<>  @@@@@@@@@@@@@@@@@

fNotEqualsZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	subne	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0>  @@@@@@@@@@@@@@@@@

fGreaterThanZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	subgt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0>=  @@@@@@@@@@@@@@@@@

fGreaterEqualsZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	subge	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<  @@@@@@@@@@@@@@@@@

fLessThanZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	sublt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<=  @@@@@@@@@@@@@@@@@

fLessEqualsZeroBop:
	eor	r2, r2
	flds	s15, [r6]
	fcmpzs	s15
	fmstat
	suble	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@ fmin  @@@@@@@@@@@@@@@@@

fMinBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	blt	fmin1
	fsts	s15, [r6]
	bx	lr
	
fmin1:
	fsts	s14, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  fmax  @@@@@@@@@@@@@@@@@

fMaxBop:
	flds	s14, [r6]
	add	r6, #4
	flds	s15, [r6]
	fcmps	s14, s15
	fmstat
	bgt	fmin1
	fsts	s15, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  fwithin  @@@@@@@@@@@@@@@@@

fWithinBop:
	eor	r3,r3
	flds	s15, [r6]			@ upper bound
	flds	s14, [r6, #4]		@ lower bound
	add	r6, #8
	flds	s13, [r6]			@ test value
	fcmps	s13, s14
	fmstat
	blt	.fwithin1
	fcmps	s13, s15
	fmstat
	subge	r3, #1
.fwithin1:
	stmdb	r6!, {r3}
	bx	lr

@#############################################@
@                                             @
@     double precision fp comparisons         @
@                                             @
@#############################################@
	
@@@@@@@@@@@@@@@@@  d=  @@@@@@@@@@@@@@@@@

dEqualsBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	subeq	r2, #1
	str	r2, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  d<>  @@@@@@@@@@@@@@@@@

dNotEqualsBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	subne	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  d>  @@@@@@@@@@@@@@@@@

dGreaterThanBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	subgt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  d>=  @@@@@@@@@@@@@@@@@

dGreaterEqualsBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	subge	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  d<  @@@@@@@@@@@@@@@@@

dLessThanBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	sublt	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  d<=  @@@@@@@@@@@@@@@@@

dLessEqualsBop:
	eor	r2, r2
	fldd	d6, [r6]
	fldd	d7, [r6, #8]
	add	r6, #12
	fcmpd	d6, d7
	fmstat
	suble	r2, #1
	str	r2, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  d0=  @@@@@@@@@@@@@@@@@

dEqualsZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	subeq	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  d0<>  @@@@@@@@@@@@@@@@@

dNotEqualsZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	subne	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  d0>  @@@@@@@@@@@@@@@@@

dGreaterThanZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	subgt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  d0>=  @@@@@@@@@@@@@@@@@

dGreaterEqualsZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	subge	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  d0<  @@@@@@@@@@@@@@@@@

dLessThanZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	sublt	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  d0<=  @@@@@@@@@@@@@@@@@

dLessEqualsZeroBop:
	eor	r2, r2
	fldd	d7, [r6]
	add	r6, #4
	fcmpzd	d7
	fmstat
	suble	r2, #1
	stmdb	r6!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@ dmin  @@@@@@@@@@@@@@@@@

dMinBop:
	fldd	d7, [r6]
	add	r6, #8
	fldd	d6, [r6]
	fcmpd	d6, d7
	fmstat
	blt	dmin1
	fstd	d7, [r6]
	bx	lr
	
dmin1:
	fstd	d6, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  dmax  @@@@@@@@@@@@@@@@@

dMaxBop:
	fldd	d7, [r6]
	add	r6, #8
	fldd	d6, [r6]
	fcmpd	d6, d7
	fmstat
	bgt	dmax1
	fstd	d7, [r6]
	bx	lr
	
dmax1:
	fstd	d6, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  dwithin  @@@@@@@@@@@@@@@@@

dWithinBop:
	eor	r3,r3
	fldd	d7, [r6]			@ upper bound
	fldd	d6, [r6, #8]		@ lower bound
	add	r6, #16
	fldd	d5, [r6]			@ test value
	fcmpd	d5, d6
	fmstat
	blt	.dwithin1
	fcmpd	d5, d7
	fmstat
	subge	r3, #1
.dwithin1:
	stmdb	r6!, {r3}
	bx	lr

@#############################################@
@                                             @
@            stack manipulation               @
@                                             @
@#############################################@

	
@@@@@@@@@@@@@@@@@  >r  @@@@@@@@@@@@@@@@@

rpushBop:
	ldmia	r6!, {r0}
	stmdb	r7!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  r>  @@@@@@@@@@@@@@@@@

rpopBop:
	ldmia	r7!, {r0}
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  rdrop  @@@@@@@@@@@@@@@@@

rdropBop:
	add	r7, #4
	bx	lr
	
@@@@@@@@@@@@@@@@@  rp  @@@@@@@@@@@@@@@@@

rpBop:
	stmdb	r6!, {r7}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r0  @@@@@@@@@@@@@@@@@

rzeroBop:
	ldr	r0, [r4, #rp0]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dup  @@@@@@@@@@@@@@@@@

dupBop:
	ldr	r0, [r6, #0]
	stmdb	r6!, {r0}
	bx	lr

checkDupBop:
	eor r2, r2
	ldr	r0, [r6, #0]
	cmp	r0, r2
	beq	cdup1
	stmdb	r6!, {r0}
cdup1:
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
	stmdb	r6!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  rot  @@@@@@@@@@@@@@@@@

rotBop:
	ldmia	r6, {r1, r2, r3}
	mov	r0, r3
	stmia	r6, {r0, r1, r2}
	bx	lr
	
reverseRotBop:
	ldmia	r6, {r1, r2, r3}
	stmia	r6, {r2, r3}
	str	r1, [r6, #8]
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
	ldr	r1, [r6, r0, lsl #2]
	str	r1, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sp  @@@@@@@@@@@@@@@@@

spBop:
	mov	r0, r6
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  s0  @@@@@@@@@@@@@@@@@

szeroBop:
	ldr	r0, [r4, #sp0]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  fp  @@@@@@@@@@@@@@@@@

fpBop:
	stmdb	r6!, {r8}
	bx	lr
	
@@@@@@@@@@@@@@@@@  ip  @@@@@@@@@@@@@@@@@

ipBop:
	stmdb	r6!, {r5}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r[ @@@@@@@@@@@@@@@@@
startTupleBop:
	stmdb	r7!, {r6}
	bx	lr
	
@@@@@@@@@@@@@@@@@  ]r @@@@@@@@@@@@@@@@@
endTupleBop:
	ldmia	r7!, {r0}
	sub	r0, r0, r6
	asr	r0, r0, #2
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r@ @@@@@@@@@@@@@@@@@
rpeekBop:
	ldr	r0, [r7]
	stmdb	r6!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  min  @@@@@@@@@@@@@@@@@

minBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	bgt	.min1
	mov	r0, r1
.min1:
	stmdb	r6!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  max  @@@@@@@@@@@@@@@@@

maxBop:
	ldmia	r6!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	blt	.max1
	mov	r1, r0
.max1:
	stmdb	r6!, {r1}
	bx	lr

@@@@@@@@@@@@@@@@@  within  @@@@@@@@@@@@@@@@@

withinBop:
	eor	r3,r3
	ldmia	r6!, {r0, r1, r2}		@ r0=test value, r1=lower bound, r2=upperBound
	cmp	r0, r1
	blt	.within1
	cmp	r0, r2
	subge	r3, #1
.within1:
	stmdb	r6!, {r3}
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

cstoreNextBop:
	ldmia	r6!, {r0, r1}	@ r0 = value, r1 = ptr to dest ptr
	ldr	r2, [r1]			@ r2 = dest ptr
	strb	r0, [r2]
	add	r2, #1
	str	r2, [r1]			@ update stored dest ptr
	bx	lr

cfetchNextBop:
	ldr	r1, [r6]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldrb	r0, [r2]
	add	r2, #1
	str	r2, [r1]			@ update stored src ptr
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sc@  @@@@@@@@@@@@@@@@@

scfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrsb	r1, [r0]
	str	r1, [r6]
	bx	lr

@@@@@@@@@@@@@@@@@  c2i  @@@@@@@@@@@@@@@@@

c2iBop:
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

wstoreNextBop:
	ldmia	r6!, {r0, r1}	@ r0 = value, r1 = ptr to dest ptr
	ldr	r2, [r1]			@ r2 = dest ptr
	strh	r0, [r2]
	add	r2, #2
	str	r2, [r1]			@ update stored dest ptr
	bx	lr

wfetchNextBop:
	ldr	r1, [r6]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldrh	r0, [r2]
	add	r2, #2
	str	r2, [r1]			@ update stored src ptr
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sw@  @@@@@@@@@@@@@@@@@

swfetchBop:
	ldr	r0, [r6]
	eor	r1, r1
	ldrsh	r1, [r0]
	str	r1, [r6]
	bx	lr

	
@@@@@@@@@@@@@@@@@  w2i  @@@@@@@@@@@@@@@@@

w2iBop:
	ldrsh	r0, [r6]
	str	r0, [r6]
	bx	lr
	
@@@@@@@@@@@@@@@@@  d!  @@@@@@@@@@@@@@@@@


dstoreBop:
	ldmia	r6!, {r0, r1, r2}
	str	r1, [r0]
	str	r2, [r0, #4]
	bx	lr
	
dstoreNextBop:
	ldmia	r6!, {r0, r1, r2}	@ r0,r1 = value, r2 = ptr to dest ptr
	ldr	r3, [r2]				@ r3 = dest ptr
	stmia	r3!, {r0, r1}		@ store value and advance dest ptr
	str	r2, [r3]				@ update stored dest ptr
	bx	lr

dfetchNextBop:
	ldmia	r6!, {r2}			@ r2 = ptr to src ptr
	ldr	r3, [r2]			@ r3 = src ptr
	ldmia	r3!, {r0, r1}	@ r0,r1 = fetched value
	str	r2, [r1]			@ update stored src ptr
	stmdb	r6!, {r0, r1}
	bx	lr
	
@@@@@@@@@@@@@@@@@  memcpy  @@@@@@@@@@@@@@@@@

memcpyBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	memcpy
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  memset  @@@@@@@@@@@@@@@@@

memsetBop:
	@	TOS is #bytes, TOS-1 is fillValue, TOS-2 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = fillValue r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	memset
	pop	{r4, r12, lr}
	bx	lr
	
	
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

byteVarActionBop:
	ldmia	r6!, {r0}
	b	byteEntry
	
ubyteVarActionBop:
	ldmia	r6!, {r0}
	b	ubyteEntry
	
shortVarActionBop:
	ldmia	r6!, {r0}
	b	shortEntry
	
ushortVarActionBop:
	ldmia	r6!, {r0}
	b	ushortEntry
	
intVarActionBop:
	ldmia	r6!, {r0}
	b	intEntry
	
floatVarActionBop:
	ldmia	r6!, {r0}
	b	floatEntry
	
stringVarActionBop:
	ldmia	r6!, {r0}
	b	stringEntry

doubleVarActionBop:
	ldmia	r6!, {r0}
	b	doubleEntry
	
longVarActionBop:
	ldmia	r6!, {r0}
	b	longEntry
	
opVarActionBop:
	ldmia	r6!, {r0}
	b	opEntry
	
objectVarActionBop:
	ldmia	r6!, {r0}
	b	objectEntry
	
@#############################################@
@                                             @
@             string manipulation             @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  strcpy  @@@@@@@@@@@@@@@@@

strcpyBop:
	@	TOS is src, TOS-1 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r1, r2}
	mov	r0, r2
	bl	strcpy
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strncpy  @@@@@@@@@@@@@@@@@

strncpyBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncpy
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strlen  @@@@@@@@@@@@@@@@@

strlenBop:
	push	{r4, r12, lr}
	ldr	r0, [r6]
	bl	strlen
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strcat  @@@@@@@@@@@@@@@@@

strcatBop:
	@	TOS is src, TOS-1 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r1, r2}
	mov	r0, r2
	bl	strcat
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strncat  @@@@@@@@@@@@@@@@@

strncatBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{r4, r12, lr}
	ldmia	r6!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncat
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strchrBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strchr
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strrchrBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strrchr
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strcmp  @@@@@@@@@@@@@@@@@

strcmpBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strcasecmp
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  stricmp  @@@@@@@@@@@@@@@@@

stricmpBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strcasecmp
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strstr  @@@@@@@@@@@@@@@@@

strstrBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strstr
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strtok  @@@@@@@@@@@@@@@@@

strtokBop:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}
	ldr	r0, [r6]
	bl	strtok
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr


initStringBop:
	eor	r2,r2
	ldmia	r6!, {r0, r1}
	str	r2,	[r1, #-4]		@ set current length = 0
	str	r0, [r1, #-8]		@ set maximum length
	strb	r2, [r1]		@ set first char to terminating null
	bx	lr
		
	
@#############################################@
@                                             @
@           file manipulation                 @   
@                                             @
@#############################################@

fopenBop:
	push	{r4, r12, lr}
	ldr	r1, [r6]
	ldr	r0, [r6, #4]
	@ r0=path, r1=access
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileOpen]
	blx	r3
	add	r0, #4
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr

fcloseBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileClose]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
fseekBop:
	push	{r4, r12, lr}
	ldr	r2, [r6]
	ldr	r1, [r6, #4]
	ldr	r0, [r6, #8]
	@ r0=pFile r1=offset r2=seekType
	ldr	r3, [r4, #file_funcs]
	ldr	r3, [r3, #fileSeek]
	blx	r3
	add	r6, #8
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
freadBop:
	push	{r4, r5, lr}
	ldr	r3, [r6]
	ldr	r2, [r6, #4]
	ldr	r1, [r6, #8]
	ldr	r0, [r6, #12]
	@ int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
	@ r0=pDst r1=itemSize r2=numItems r3=pFile
	ldr	r5, [r4, #file_funcs]
	ldr	r5, [r5, #fileRead]
	blx	r5
	add	r6, #12
	str	r0, [r6]
	pop	{r4, r5, lr}
	bx	lr
	
fwriteBop:
	push	{r4, r5, lr}
	ldr	r3, [r6]
	ldr	r2, [r6, #4]
	ldr	r1, [r6, #8]
	ldr	r0, [r6, #12]
	@ int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
	@ r0=pDst r1=itemSize r2=numItems r3=pFile
	ldr	r5, [r4, #file_funcs]
	ldr	r5, [r5, #fileWrite]
	blx	r5
	add	r6, #12
	str	r0, [r6]
	pop	{r4, r5, lr}
	bx	lr
	
fgetcBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileGetChar]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
fputcBop:
	push	{r4, r12, lr}
	ldr	r1, [r6]
	ldr	r0, [r6, #4]
	@ r1=char, r0=pFile
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #filePutChar]
	blx	r3
	add	r6, #4
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr

feofBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileAtEnd]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
ftellBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileTell]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
flenBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileGetLength]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
fexistsBop:	
	push	{r4, r12, lr}
	ldr	r0, [r6]
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #fileExists]
	blx	r3
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
fgetsBop:
	push	{r4, r12, lr}
	ldr	r2, [r6]
	ldr	r1, [r6, #4]
	ldr	r0, [r6, #8]
	@ r0=pBuffer r1=maxChars r2=pFile
	ldr	r3, [r4, #file_funcs]
	ldr	r3, [r3, #fileGetString]
	blx	r3
	add	r6, #8
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr
	
fputsBop:
	push	{r4, r12, lr}
	ldr	r1, [r6]
	ldr	r0, [r6, #4]
	@ r0=pBuffer, r1=pFile
	ldr	r2, [r4, #file_funcs]
	ldr	r3, [r2, #filePutString]
	blx	r3
	add	r6, #4
	str	r0, [r6]
	pop	{r4, r12, lr}
	bx	lr

	
@ extern void fprintfSub( ForthCoreState* pCore );
	
fprintfSub:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	r7!, {sp}		@ save old PC SP for cleanup on rstack
fprintfSub1:
	sub	r2, #1
	bl	fprintfSub2
	ldmia	r6!, {r0}
	push	{r0}
	b	fprintfSub1
fprintfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	fprintf
	stmdb	r6!, {r0}
	ldmia	r7!, {sp}		@ cleanup PC stack
	pop	{r4, r12, lr}
	bx	lr

	
fscanfSub:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	r7!, {sp}		@ save old PC SP for cleanup on rstack
fscanfSub1:
	sub	r2, #1
	bl	fscanfSub2
	ldmia	r6!, {r0}
	push	{r0}
	b	fscanfSub1
fscanfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	fscanf
	stmdb	r6!, {r0}
	ldmia	r7!, {sp}		@ cleanup PC stack
	pop	{r4, r12, lr}
	bx	lr

	
sprintfSub:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	r7!, {sp}		@ save old PC SP for cleanup on rstack
sprintfSub1:
	sub	r2, #1
	bl	sprintfSub2
	ldmia	r6!, {r0}
	push	{r0}
	b	sprintfSub1
sprintfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	sprintf
	stmdb	r6!, {r0}
	ldmia	r7!, {sp}		@ cleanup PC stack
	pop	{r4, r12, lr}
	bx	lr

	
sscanfSub:
	push	{r4, r12, lr}
	ldmia	r6!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	r7!, {sp}		@ save old PC SP for cleanup on rstack
sscanfSub1:
	sub	r2, #1
	bl	sscanfSub2
	ldmia	r6!, {r0}
	push	{r0}
	b	sscanfSub1
sscanfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	sscanf
	stmdb	r6!, {r0}
	ldmia	r7!, {sp}		@ cleanup PC stack
	pop	{r4, r12, lr}
	bx	lr

	

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
	

nativeOp:
@ is there any way we can get here?

badOpcode:
	mov	r0, #kForthErrorBadOpcode
	b	interpLoopErrorExit
	
userDefType:
	ldr	r3, [r4, #n_ops]
	cmp	r1, r3
	bge	badOpcode
	@ rpush IP
	stmdb	r7!, {r5}
	@ fetch new IP
	ldr	r0, [r4, #ops]
	ldr	r5, [r0, r1, lsl #2]
	bx	lr
	
branchType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	ands	r3, r1
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
	ands	r3, r1
	orrne	r1,r2
	stmdb	r6!, {r1}
	bx	lr
	
offsetType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	ands	r3, r1
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
@ array offset ops
@
arrayOffsetType:
	@ r1 is size of one element
	ldmia	r6!, {r2, r3}	@ r2 is array base, r3 is index
	mul	r3, r1
	add	r2, r3
	stmdb	r6!, {r2}
	bx	lr

@-----------------------------------------------
@
@ local struct array ops
@
localStructArrayType:
	@ bits 0..11 are padded struct length in bytes, bits 12..23 are frame offset in longs
	@ multiply struct length by TOS, add in (negative) frame offset, and put result on TOS
	ldr	r0, =0xFFF
	and	r0, r1
	ldr	r3, [r6]
	mul	r3, r0			@ r3 is offset of struct element from base of array
	lsr	r1, #10
	ldr	r0, =0x3FFC
	and	r1, r0			@ r1 is offset of base array from FP in bytes
	sub	r0, r8, r1	
	add	r0, r3
	stmdb	r6!, {r0}
	bx	lr

@-----------------------------------------------
@
@ string constant ops
@
constantStringType:
	@ IP points to beginning of string
	@ low 24-bits of ebx is string len in longs
	stmdb	r6!, {r5}	@ push string ptr
	@ get low-24 bits of opcode
	lsl	r1, #2
	@ advance IP past string
	add	r5, r1
	bx	lr
	
@-----------------------------------------------
@
@ local stack frame allocation ops
@
allocLocalsType:
	@ rpush old FP
	stmdb	r7!, {r8}
	@ set FP = RP, points at old FP
	mov	r8, r7
	@ allocate amount of storage specified by low 24-bits of op on rstack
	lsl	r1, #2
	sub	r7, r1
	@ clear out allocated storage
	eor	r2, r2
	mov	r0, r7
alt1:
	stmia	r0!, {r2}
	subs	r1, #4
	bne	alt1
	bx	lr
	

@-----------------------------------------------
@
@ local string init ops
@
initLocalStringType:
	@ bits 0..11 are string length in bytes, bits 12..23 are frame offset in longs
	@ init the current & max length fields of a local string
	ldr	r0, =0xFFF
	and	r0, r1			@ r0 is string length in bytes
	lsr	r1, #10
	ldr	r2, =0x3FFC
	and	r1, r2			@ r1 is offset of string from FP in bytes
	sub	r3, r8, r1
	eor	r2, r2
	stmia	r3!, {r0, r2}	
	strb	r2, [r3]	@ add terminating null
	bx	lr

@-----------------------------------------------
@
@ local reference ops
@
localRefType:
	@ push local reference - r1 is frame offset in longs
	lsl	r1, #2
	add	r1, r8
	stmdb	r6!, {r1}
	bx	lr
	
@-----------------------------------------------
@
@ member reference ops
@
memberRefType:
	@ push member reference - r1 is member offset in bytes
	ldr	r0, [r4, #tpd]
	add	r1, r0
	stmdb	r6!, {r1}
	bx	lr
	
@-----------------------------------------------
@
@ member string init ops
@
memberStringInitType:
	@ bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
	@ init the current & max length fields of a member string
	
	@ bits 0..11 are string length in bytes, bits 12..23 are member offset in longs
	@ init the current & max length fields of a member string
	ldr	r0, =0xFFF
	and	r0, r1			@ r0 is string length in bytes
	lsr	r1, #10
	ldr	r2, =0x3FFC
	and	r1, r2			@ r1 is offset of string from member base in bytes
	ldr	r2, [r4, #tpd]
	add	r3, r2, r1
	eor	r2, r2
	stmia	r3!, {r0, r2}	
	strb	r2, [r3]	@ add terminating null
	bx	lr

@-----------------------------------------------
@
@ method invocation ops
@

@ invoke a method on object currently referenced by this ptr pair
methodWithThisType:
	@ r1 is method number
	@ push this ptr pair on return stack
	ldr	r2, [r4, #tpm]
	ldr	r3, [r4, #tpd]
	stmdb	r7!, {r2, r3}
	@ get method opcode
	ldr	r0, [r2, r1, lsl #2]
	b	interpLoopExecuteEntry
		
@ invoke a method on an object referenced by ptr pair on TOS
methodWithTOSType:
	@ TOS is object vtable, NOS is object data ptr
	@ ebx is method number
	@ push this ptr pair on return stack
	ldr	r2, [r4, #tpm]
	ldr	r3, [r4, #tpd]
	stmdb	r7!, {r2, r3}
	@ set this data,methods from TOS	
	ldmia	r6!, {r2, r3}
	str	r2, [r4, #tpm]
	str	r3, [r4, #tpd]
	@ get method opcode
	ldr	r0, [r2, r1, lsl #2]
	b	interpLoopExecuteEntry
	
	
@-----------------------------------------------
@
@ InitAsmTables - initializes FCore.numAsmBuiltinOps
@
@ extern void InitAsmTables( ForthCoreState *pCore );

	.global	InitAsmTables
InitAsmTables:
	@
	@	TBD: set inner interpreter reentry point in pCore (inner_loop)
	@
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
	.word	nativeOp				@ kOpNative = 0,
	.word	nativeOp				@ kOpNativeImmediate,
	.word	userDefType				@ kOpUserDef,
	.word	userDefType				@ kOpUserDefImmediate,
	.word	cCodeType				@ kOpCCode,         
	.word	cCodeType				@ kOpCCodeImmediate,
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
	.word	constantStringType		@ kOpConstantString,	
	.word	offsetType				@ kOpOffset,          
	.word	arrayOffsetType			@ kOpArrayOffset,     
	.word	allocLocalsType			@ kOpAllocLocals,     
	.word	localRefType			@ kOpLocalRef,
	.word	initLocalStringType		@ kOpLocalStringInit, 
	.word	localStructArrayType	@ kOpLocalStructArray,
	.word	offsetFetchType			@ kOpOffsetFetch,          
	.word	memberRefType			@ kOpMemberRef,	

@	30 - 39
	.word	localByteType
	.word	localUByteType
	.word	localShortType
	.word	localUShortType
	.word	localIntType
	.word	localIntType
	.word	localLongType
	.word	localLongType
	.word	localFloatType
	.word	localDoubleType
	
@	40 - 49
	.word	localStringType
	.word	localOpType
	.word	localObjectType
	.word	localByteArrayType
	.word	localUByteArrayType
	.word	localShortArrayType
	.word	localUShortArrayType
	.word	localIntArrayType
	.word	localIntArrayType
	.word	localLongArrayType
	
@	50 - 59
	.word	localLongArrayType
	.word	localFloatArrayType
	.word	localDoubleArrayType
	.word	localStringArrayType
	.word	localOpArrayType
	.word	localObjectArrayType
	.word	fieldByteType
	.word	fieldUByteType
	.word	fieldShortType
	.word	fieldUShortType
	
@	60 - 69
	.word	fieldIntType
	.word	fieldIntType
	.word	fieldLongType
	.word	fieldLongType
	.word	fieldFloatType
	.word	fieldDoubleType
	.word	fieldStringType
	.word	fieldOpType
	.word	fieldObjectType
	.word	fieldByteArrayType
@	70 - 79
	.word	fieldUByteArrayType
	.word	fieldShortArrayType
	.word	fieldUShortArrayType
	.word	fieldIntArrayType
	.word	fieldIntArrayType
	.word	fieldLongArrayType
	.word	fieldLongArrayType
	.word	fieldFloatArrayType
	.word	fieldDoubleArrayType
	.word	fieldStringArrayType

@	80 - 89
	.word	fieldOpArrayType
	.word	fieldObjectArrayType
	.word	memberByteType
	.word	memberUByteType
	.word	memberShortType
	.word	memberUShortType
	.word	memberIntType
	.word	memberIntType
	.word	memberLongType
	.word	memberLongType
	
@	90 - 99
	.word	memberFloatType
	.word	memberDoubleType
	.word	memberStringType
	.word	memberOpType
	.word	memberObjectType
	.word	memberByteArrayType
	.word	memberUByteArrayType
	.word	memberShortArrayType
	.word	memberUShortArrayType
	.word	memberIntArrayType
	
@	100 - 109
	.word	memberIntArrayType
	.word	memberLongArrayType
	.word	memberLongArrayType
	.word	memberFloatArrayType
	.word	memberDoubleArrayType
	.word	memberStringArrayType
	.word	memberOpArrayType
	.word	memberObjectArrayType
	.word	methodWithThisType
	.word	methodWithTOSType
	
@	110 - 114
	.word	memberStringInitType
	.word	extOpType @ nvoComboType
	.word	extOpType @ nvComboType
	.word	extOpType @ noComboType
	.word	extOpType @ voComboType
	
@	115 - 149
	.word	extOpType,extOpType,extOpType,extOpType,extOpType
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
@		         THE END OF ALL THINGS               @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
