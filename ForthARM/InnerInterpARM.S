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

#define rcore	r4
#define rip		r5
#define rsp		r6
#define rrp		r7
#define rfp		r8
#define rops	r9
#define rnumops	r10
#define raction	r11

@	R12			unused, can be trashed by called routines
@	R13			system stack pointer
@	R14			LR - system link register - we use this as inner interp reentry point
@	R15			PC

@ NOTE: when calling external routines, the stack should be kept 64-bit aligned, clib routines assume this,
@ and if it isn't 64-bit aligned, double register loads in clib routines will fail - this is what made %f and %d fail
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
rpsave			=	32			@	RP - forth return stack pointer (r7)
fpsave			=	36			@	FP - frame pointer (r8)
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
conout_stream  = 100
base          = 104
signed_print_mode = 108

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
	.global orBop, andBop, xorBop, invertBop, lshiftBop, arshiftBop, notBop, trueBop, falseBop, nullBop, dnullBop
	.global equalsBop, notEqualsBop, greaterThanBop, greaterEqualsBop, lessThanBop, lessEqualsBop
	.global equals0Bop, notEquals0Bop, greaterThan0Bop, greaterEquals0Bop, lessThan0Bop, lessEquals0Bop
	.global rpushBop, rpopBop, rdropBop, rpBop, r0Bop, dupBop, checkDupBop, swapBop, overBop
	.global rotBop, tuckBop, pickBop, spBop, s0Bop, fpBop, ddupBop, dswapBop, ddropBop, doverBop, drotBop,
	.global storeBop, cstoreBop, cfetchBop, scfetchBop, c2iBop, wstoreBop, wfetchBop, swfetchBop, w2iBop, dstoreBop
	.global	moveBop, fillBop, setVarActionBop, getVarActionBop, strcpyBop, strncpyBop, strlenBop, strcatBop, strncatBop
	.global	strchrBop, strrchrBop, strcmpBop, stricmpBop, strstrBop, strtokBop, initStringBop
	.global doUByteBop, doUByteArrayBop, doUShortBop, doUShortArrayBop
	.global dsinBop, dasinBop, dcosBop, dacosBop, dtanBop, datanBop, datan2Bop, dexpBop, dlnBop, dlog10Bop, dpowBop
	.global dsqrtBop, dceilBop, dfloorBop, dabsBop, dldexpBop, dfrexpBop, dmodfBop, dfmodBop, i2fBop, i2dBop, f2iBop, f2dBop, d2iBop, d2fBop
	.global ipBop, startTupleBop, endTupleBop, rpeekBop, minBop, maxBop, storeNextBop, fetchNextBop, cstoreNextBop, cfetchNextBop
	.global wstoreNextBop, wfetchNextBop, dstoreNextBop, dfetchNextBop
	.global fEqualsBop, fNotEqualsBop, fGreaterThanBop, fGreaterEqualsBop, fLessThanBop, fLessEqualsBop
	.global fEquals0Bop, fNotEquals0Bop, fGreaterThan0Bop, fGreaterEquals0Bop, fLessThan0Bop, fLessEquals0Bop
	.global dEqualsBop, dNotEqualsBop, dGreaterThanBop, dGreaterEqualsBop, dLessThanBop, dLessEqualsBop
	.global dEquals0Bop, dNotEquals0Bop, dGreaterThan0Bop, dGreaterEquals0Bop, dLessThan0Bop, dLessEquals0Bop
	.global byteVarActionBop, ubyteVarActionBop, shortVarActionBop, ushortVarActionBop, intVarActionBop, floatVarActionBop
	.global fopenBop, fcloseBop, fseekBop, freadBop, fwriteBop, fgetcBop, fputcBop, feofBop, fexistsBop, ftellBop, flenBop, fgetsBop, fputsBop
	.global withinBop, fMinBop, fMaxBop, fWithinBop, dMinBop, dMaxBop, dWithinBop, unsignedGreaterThanBop, unsignedLessThanBop
	.global umtimesBop, rshiftBop, reverseRotBop, vocabToClassBop, doCheckDoBop, archARMBop, archX86Bop
	@ these are just dummies:
	.global doLongBop, doDoubleBop, doStringBop, doOpBop, doObjectBop, doLongArrayBop, doDoubleArrayBop, doObjectArrayBop, doStringArrayBop, doOpArrayBop
	.global longVarActionBop, doubleVarActionBop, stringVarActionBop, opVarActionBop, objectVarActionBop
	.global fsinBop, fasinBop, fcosBop, facosBop, ftanBop, fatanBop, fatan2Bop, fexpBop, flnBop, flog10Bop
	.global fpowBop, fsqrtBop, fceilBop, ffloorBop, fabsBop
	.global fcmpBop, dcmpBop, fldexpBop, ffrexpBop, fmodfBop, ffmodBop
	.global icmpBop, uicmpBop, oclearBop, lplusBop, lminusBop, ltimesBop, mtimesBop, umtimesBop

	.global fprintfSub, fscanfSub, sprintfSub, sscanfSub
	.global InterpretOneOpFast
	
	.global	break_me
	.type	   break_me,function
break_me:
	swi	#0xFD0000
	bx	lr
	
@-----------------------------------------------
@
@ inner interpreter entry point for ops defined in assembler
@
@ extern void NativeAction( ForthCoreState *pCore, ulong opVal );

	.global	NativeAction
NativeAction:
	@	r0	=	core
	@	r1	=	opVal
	push	{rcore, rip, rsp, rrp, rfp, lr}
	mov	rcore, r0
	add	r2, rcore, #ipsave
	ldmia	r2!, {rip-rfp}			@ load IP, SP, RP, FP from core
	
	ldr	r2, [rcore, #ops]
	ldr	r3, [r2, r1, lsl #2]
	bl	.NativeAction1
	@	user op will return here
	add	r1, rcore, #ipsave			@ r1 -> IP
	stmia	r1!, {rip-rfp}			@ save IP, SP, RP, FP in core
	pop	{rcore, rip, rsp, rrp, rfp, pc}
		
.NativeAction1:
	bx	r3

#ifdef ASM_INNER_INTERPRETER
	
@-----------------------------------------------
@
@ single step a thread
@
@ extern eForthResult InterpretOneOpFast( ForthCoreState *pCore, long op );
	.global	InterpretOneOpFast
	.type	InterpretOneOpFast, %function
InterpretOneOpFast:
	push	{rcore-raction, r12, lr}
	mov	rcore, r0					@ r4 = pCore
	add	r0, rcore, #ipsave
	ldmia	r0!, {rip-rfp}			@ load IP, SP, RP, FP from core
	ldr	rops, [rcore, #ops]			@ rops = table of ops
	ldr	rnumops, [rcore, #n_ops]	@ rnumops = number of ops implemented in assembler
	ldr	raction, .opTypesTable		@ raction = table of opType handlers
	
	mov	r3, #kResultOk				@ r3 = kResultOk
	str	r3, [rcore, #istate]		@ SET_STATE( kResultOk )
	
	bl	.II2						@ don't know how to load lr directly
	b	.IIExit


@-----------------------------------------------
@
@ inner interpreter C entry point
@
@ extern eForthResult InnerInterpreterFast( ForthCoreState *pCore );
	.global	InnerInterpreterFast
	.type	InnerInterpreterFast, %function
	
InnerInterpreterFast:
	push	{rcore-raction, r12, lr}
	mov	rcore, r0					@ r4 = pCore

	@
	@ re-enter here if you stomped on rip/rsp/rrp	
	@
InnerInterpreter:
	add	r0, rcore, #ipsave
	ldmia	r0!, {rip-rfp}			@ load IP, SP, RP, FP from core
	ldr	rops, [rcore, #ops]			@ rops = table of ops
	ldr	rnumops, [rcore, #n_ops]	@ rnumops = number of ops implemented in assembler
	ldr	raction, .opTypesTable		@ raction = table of opType handlers
	
	mov	r3, #kResultOk				@ r3 = kResultOk
	str	r3, [rcore, #istate]		@ SET_STATE( kResultOk )

	@ uncomment next 2 lines to display IP each instruction	
	@mov	r0, rip
	@bl	ShowIP
	@bl	break_me	
	
	bl	.II2					@ don't know how to load lr directly
	@
	@ we come back here whenever an op does a "bx lr"
	@
.II1:
	@swi	#0xFD
	ldrb	r0, [rcore, #istate]	@ get state from core
	cmp	r0, #kResultOk			@ keep looping if state is still ok (zero)
	bne	.IIExit
.II2:
	ldmia	rip!, {r0}			@ r0 = next opcode, advance IP
@ interpLoopExecuteEntry is entry for executeBop - expects opcode in r0
interpLoopExecuteEntry:
	cmp	r0, rnumops					@ is opcode native op?
	bge	.II3
	@ handle native ops
	ldr	r1, [rops, r0, lsl #2]
	bx	r1

.II3:
	@
	@ opcode is not native
	@
	lsr	r2, r0, #24				@ r2 = opType (hibyte of opcode)
	ldr	r1, =0x00FFFFFF				@ op value mask (0x00FFFFFF)
	and	r1, r0						@ r1 = low 24 bits of opcode
	ldr	r0, [raction, r2, lsl #2]	@ r0 = action routine for this opType
	@
	@ optype action routines expect r1=opValue, r2=opType, r4=pCore
	@
	bx	r0						@ dispatch to action routine

	
	@
	@ exit inner interpreter
	@	
.IIExit:
	add	r1, rcore, #ipsave			@ r1 -> IP
	stmia	r1!, {rip-rfp}			@ save IP, SP, RP, FP in core
	pop	{rcore-raction, r12, pc}
	
interpLoopErrorExit:
	str	r0, [rcore, #errorcode]
	mov	r0, #kResultError		@ status = error
	str	r0, [rcore, #istate]
	bx	lr

interpLoopFatalErrorExit:
	str	r0, [rcore, #errorcode]
	mov	r0, #kResultFatalError		@ status = error
	str	r0, [rcore, #istate]
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
	ldr	r3, [rcore, #n_ops]
	cmp	r1, r3
	bge	badOpcode

	bl	.cct2
	
	@ we come here when C op does its return
	ldrb	r0, [rcore, #istate]	@ get state from core
	cmp	r0, #kResultOk			@ exit if state is not ok
	bne	.IIExit
	b	InnerInterpreter
		
.cct2:
	add	r0, rcore, #ipsave			@ r0 -> IP
	stmia	r0!, {rip-rfp}		@ save IP, SP, RP, FP in core
	mov	r0, rcore					@ C ops take pCore ptr as first param
	ldr	r3, [rcore, #ops]			@ r3 = table of ops
	ldr	r2, [r3, r1, lsl #2]
	bx	r2


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
	sub	r0, rfp, r1					@ r0 points to the byte field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> byte
@
byteEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2}				@ push byte on TOS
	bx	lr

@------------ unsigned byte support ----------------
localUByteType:
	lsl	r1, #2
	sub	r0, rfp, r1					@ r0 points to the byte field
	@ see if it is a fetch

ubyteEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localByte1

@ fetch local unsigned byte
localUByteFetch:
	ldrb	r2, [r0]
	stmdb	rsp!, {r2}				@ push byte on TOS
	bx	lr

	
@ push address of byte on TOS
localByteRef:
	stmdb	rsp!, {r0}				@ push address of byte on TOS
	bx	lr

@ store byte on TOS into byte @ r0
localByteStore:
	ldmia	rsp!, {r2}				@ pop TOS byte value into r2
	strb	r2, [r0]				@ store byte
	bx	lr

@ add byte on TOS into byte @ r0
localBytePlusStore:
	ldrb	r2, [r0]
	ldmia	rsp!, {r1}				@ pop TOS byte value into r1
	add	r2, r1
	strb	r2, [r0]
	bx	lr
	
@ subtract byte on TOS from byte @ r0
localByteMinusStore:
	ldrb	r2, [r0]
	ldmia	rsp!, {r1}
	sub	r2, r1
	strb	r2, [r0]
	bx	lr

.localByte1:
	@ r0 points to the byte field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	byteEntry	

memberByteType:
	@ get ptr to byte var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	byteEntry	
	
localByteArrayType:
	@ get ptr to byte var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	sub	r0, rfp, r1				@ r0 points to the byte field
	b	byteEntry

fieldByteArrayType:
	@ get ptr to byte var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	byteEntry

memberByteArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	byteEntry

@----------------- unsigned byte ---------------------------------------
fieldUByteType:
	@ get ptr to byte var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	ubyteEntry	

memberUByteType:
	@ get ptr to byte var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	ubyteEntry	
	
localUByteArrayType:
	@ get ptr to byte var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	lsl	r1, #2
	sub	r1, r0					@ add in array index
	sub	r0, rfp, r1				@ r0 points to the byte field
	b	ubyteEntry

fieldUByteArrayType:
	@ get ptr to byte var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ubyteEntry

memberUByteArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the byte field
	@ see if it is a fetch

@	
@ entry point for short variable action routines
@	r0 -> short
@
shortEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2}				@ push short on TOS
	bx	lr

@------------ unsigned short support ----------------
localUShortType:
	lsl	r1, #2
	sub	r0, rfp, r1					@ r0 points to the byte field
ushortEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
	eor	r2, r2						@ r2 = 0
	cmp	r1, r2
	bne	.localShort1
@ fetch local unsigned short
localUShortFetch:
	ldrh	r2, [r0]
	stmdb	rsp!, {r2}				@ push short on TOS
	bx	lr


localShortRef:
	stmdb	rsp!, {r0}				@ push address of short on TOS
	bx	lr
	
localShortStore:
	ldmia	rsp!, {r2}				@ pop TOS short value into r2
	strh	r2, [r0]				@ store short
	bx	lr
	
localShortPlusStore:
	ldrh	r2, [r0]
	ldmia	rsp!, {r1}				@ pop TOS short value into r1
	add	r2, r1
	strh	r2, [r0]
	bx	lr
	
localShortMinusStore:
	ldrh	r2, [r0]
	ldmia	rsp!, {r1}
	sub	r2, r1
	strh	r2, [r0]
	bx	lr

.localShort1:
	@ r0 points to the short field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	shortEntry	

memberShortType:
	@ get ptr to short var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	shortEntry	
	
localShortArrayType:
	@ get ptr to short var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	lsl	r1, #2
	lsl	r0, #1					@ convert short index to byte offset
	sub	r1, r0					@ add in array index
	sub	r0, rfp, r1				@ r0 points to the byte field
	b	shortEntry

fieldShortArrayType:
	@ get ptr to short var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #1
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	shortEntry

memberShortArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #1
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	shortEntry

@----------------- unsigned short ---------------------------------------

fieldUShortType:
	@ get ptr to short var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	ushortEntry	

memberUShortType:
	@ get ptr to short var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	ushortEntry	
	
localUShortArrayType:
	@ get ptr to short var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	lsl	r1, #2
	lsl	r0, #1					@ convert short index to byte offset
	sub	r1, r0					@ add in array index
	sub	r0, rfp, r1				@ r0 points to the byte field
	b	ushortEntry

fieldUShortArrayType:
	@ get ptr to short var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #1
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	ushortEntry

memberUShortArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #1
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> int
@
intEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2}				@ push int on TOS
	bx	lr

localIntRef:
	stmdb	rsp!, {r0}				@ push address of int on TOS
	bx	lr
	
localIntStore:
	ldmia	rsp!, {r2}				@ pop TOS int value into r2
	str	r2, [r0]					@ store int
	bx	lr
	
localIntPlusStore:
	ldr	r2, [r0]
	ldmia	rsp!, {r1}				@ pop TOS int value into r1
	add	r2, r1
	str	r2, [r0]
	bx	lr
	
localIntMinusStore:
	ldr	r2, [r0]
	ldmia	rsp!, {r1}				@ pop TOS int value into r1
	sub	r2, r1
	str	r2, [r0]
	bx	lr

.localInt1:
	@ r0 points to the int field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	intEntry	

memberIntType:
	@ get ptr to int var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	intEntry	
	
localIntArrayType:
	@ get ptr to int var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, rfp, r1				@ r0 points to the int field
	b	intEntry

fieldIntArrayType:
	@ get ptr to int var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	intEntry

memberIntArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> float
@
floatEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2}				@ push float on TOS
	bx	lr

localFloatRef:
	stmdb	rsp!, {r0}				@ push address of float on TOS
	bx	lr
	
localFloatStore:
	ldmia	rsp!, {r2}				@ pop TOS float value into r2
	str	r2, [r0]					@ store float
	bx	lr
	
localFloatPlusStore:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [r0]
	fadds	s15, s15, s14
	fsts	s15, [r0]
	bx	lr
	
	
localFloatMinusStore:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [r0]
	fadds	s15, s15, s14
	fsts	s15, [r0]
	bx	lr

.localFloat1:
	@ r0 points to the float field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	floatEntry	

memberFloatType:
	@ get ptr to float var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	floatEntry	
	
localFloatArrayType:
	@ get ptr to float var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, rfp, r1				@ r0 points to the float field
	b	floatEntry

fieldFloatArrayType:
	@ get ptr to float var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	floatEntry

memberFloatArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> double
@
doubleEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2,r3}			@ push double on TOS
	bx	lr

localDoubleRef:
	stmdb	rsp!, {r0}				@ push address of double on TOS
	bx	lr
	
localDoubleStore:
	ldmia	rsp!, {r2,r3}			@ pop TOS double value into r2
	strd	r2, [r0]				@ store double
	bx	lr
	
localDoublePlusStore:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [r0]
	faddd	d7, d7, d6
	fstd	d7, [r0]
	bx	lr
	
localDoubleMinusStore:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [r0]
	fsubd	d7, d7, d6
	fstd	d7, [r0]
	bx	lr

.localDouble1:
	@ r0 points to the double field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	doubleEntry	

memberDoubleType:
	@ get ptr to double var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	doubleEntry	
	
localDoubleArrayType:
	@ get ptr to double var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, rfp, r1				@ r0 points to the double field
	b	doubleEntry

fieldDoubleArrayType:
	@ get ptr to double var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	doubleEntry

memberDoubleArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> long
@
longEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	ldr	r1, r3
	stmdb	rsp!, {r1,r2}			@ push long on TOS
	bx	lr

localLongRef:
	stmdb	rsp!, {r0}				@ push address of long on TOS
	bx	lr
	
localLongStore:
	ldmia	rsp!, {r2,r3}			@ pop TOS long value into r2,r3
	ldr	r1, r3
	strd	r1, [r0]				@ store long
	bx	lr
	
localLongPlusStore:
	ldmia	rsp!, {r2,r3}			@ pop TOS long value into r2,r3
	push	{rcore, rip}
	ldm	r0!, {rcore, rip}
	adds rip, r2, rip
	adc	rcore, r3, rcore
	stm	r0!, {rcore,rip}
	pop	{rcore, rip}
	bx	lr
	
localLongMinusStore:
	ldmia	rsp!, {r2,r3}			@ pop TOS long value into r2,r3
	push	{rcore, rip}
	ldm	r0!, {rcore, rip}
	subs	rip, r2, rip
	sbc	rcore, r3, rcore
	stm	r0!, {rcore,rip}
	pop	{rcore, rip}
	bx	lr

.localLong1:
	@ r0 points to the long field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	longEntry	

memberLongType:
	@ get ptr to long var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	longEntry	
	
localLongArrayType:
	@ get ptr to long var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, rfp, r1				@ r0 points to the long field
	b	longEntry

fieldLongArrayType:
	@ get ptr to long var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	longEntry

memberLongArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> string
@
stringEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r0}				@ push string on TOS
	bx	lr

localStringRef:
	stmdb	rsp!, {r0}				@ push address of string maxLen field on TOS
	bx	lr
	
localStringStore:
	push	{rcore}
	ldmia	rsp!, {r1}				@ r1 -> src string
	ldr	r2, [r0]					@ r2 = max chars to move
	add	r3, r0, #8					@ r3 -> dst string
.lss1:
	subs	r2, #1						@ leave space for terminating null
	beq	.lssx
	ldrb	rcore, [r1]
	orrs	rcore, rcore
	beq	.lssx						@ hit terminating null in src
	add	r1, #1
	strb	rcore, [r3]
	add	r3, #1
	b	.lss1
.lssx:
	eor	r2, r2
	strb	r2, [r3]
	sub	r3, r3, r0
	sub	r3, #8
	str	r3,	[r0, #4]				@ update curlen field
	pop	{rcore}
	bx	lr
	
localStringPlusStore:
	push	{rcore}
	ldmia	rsp!, {r1}				@ r1 -> src string
	ldr	r2, [r0]					@ r2 = max chars to move
	add	r3, r0, #8					@ r3 -> dst string
	ldr	rcore, [r0, #4]				@ rcore = current len
	add	r3, rcore
	cmp	rcore, r2
	blt	.lss1
	pop	{rcore}
	bx	lr
	
.localString1:
	@ r0 points to the string field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	stringEntry	

memberStringType:
	@ get ptr to string var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	stringEntry	
	
localStringArrayType:
	@ get ptr to string var into r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, rfp, r1				@ r0 points to the string field
	b	stringEntry

fieldStringArrayType:
	@ get ptr to string var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	stringEntry

memberStringArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the op field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> op
@
opEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
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
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	opEntry	

memberOpType:
	@ get ptr to op var opo r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	opEntry	
	
localOpArrayType:
	@ get ptr to op var opo r0
	@ FP is base ptr, r1 is offset in longs
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #2
	sub	r0, rfp, r1				@ r0 points to the op field
	b	opEntry

fieldOpArrayType:
	@ get ptr to op var opo r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #2
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	opEntry

memberOpArrayType:
	@ get ptr to opo var opo r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #2
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	sub	r0, rfp, r1					@ r0 points to the int field
	@ see if it is a fetch

@	
@ entry point for byte variable action routines
@	r0 -> object
@
objectEntry:
	ldr	r1, [rcore, #varmode]			@ r1 = varMode
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
	stmdb	rsp!, {r2,r3}			@ push object on TOS
	bx	lr

localObjectRef:
	stmdb	rsp!, {r0}				@ push address of object on TOS
	bx	lr
	
localObjectStore:
	push	{rcore,rip}
	ldmia	rsp!, {rcore,rip}	@ pop TOS object value into rcore,rip
	ldmia	r0, {r2,r3}			@ r2,r3 are oldObj
	cmp	rcore, r2
	beq	.losx
	orrs	rip, rip
	beq	.los1					@ if newObj data ptr is null, don't increment refcount
	ldr	r1, [rip]				@ increment newObj refcount
	add	r1, #1
	str	r1, [rip]
.los1:
	orrs	r3, r3
	beq	.los2
	ldr	r1, [r3]				@ decrement oldObj refcount
	subs	r1, #1
	str	r1, [r3]
	beq	.los3					@ oldObj refcount went to zero, go delete it
.los2:
	str	rcore, [r0]
.losx:
	str	rip, [r0, #4]
	pop	{rcore,rip}
	bx	lr
	
.los3:
	@ object var held last reference to oldObj, invoke olbObj.delete method
	stmia	r0, {rcore,rip}			@ store newObj
	pop	{rcore,rip}
	@ push pCore->this pair on rstack
	ldr	r0,[rcore, #tpm]
	stmdb	rrp!, {r0}
	ldr	r0,[rcore, #tpd]
	stmdb	rrp!, {r0}
	str	r2, [rcore, #tpm]
	str	r3, [rcore, #tpd]	
	
	ldr	r0, [r2]				@ r0 is method 0 (delete) opcode
	b		interpLoopExecuteEntry	
	
.localObject1:
	@ r0 points to the object field
	@ r1 is varMode
	@ r2 is 0
	str	r2, [rcore, #varmode]		@ set varMode back to fetch
	cmp	r1, #kVarMinusStore
	bgt	badVarOperation
	ldr	r3, .localObject3
	ldr	r1, [r3, r1, lsl #2]
	bx	r1
	
@ store object on TOS in variable pointed to by r0
@ do not adjust reference counts of old object or new object
localObjectStoreNoRef:
	@ TOS is new object, r0 points to destination/old object
	ldmia	rsp!, {r1}			@ r1 = hi, r2 = lo
	ldr	r3, r1
	strd	r2, [r0]
	bx	lr

@ clear object reference, leave on TOS
localObjectUnref:
	@ leave object on TOS
	ldrd	r1, [r0]	@ r1 = methods, r2 = data
	stmdb rsp!, {r1, r2}
	@ if object var is already null, do nothing else
	orrs	r3, r2, r1
	beq	lou2
	@ clear object var
	eorr	r3, r3
	str	r3, [r0]
	str	r3, [r0, #4]
	@ get object refcount, see if it is already 0
	ldr r3, [r2]
	orr r3, r3
	bne lou1
	@ report refcount negative error
	mov	r0, kForthErrorBadReferenceCount
	b	interpLoopErrorExit
lou1:
	@ decrement object refcount
	sub	r3, #1
	str	r3, [r2]
lou2:
	bx	lr

	.align	2	
.localObject3:
	.word	localObjectActionTable

localObjectActionTable:
	.word	localObjectFetch
	.word	localObjectFetch
	.word	localObjectRef
	.word	localObjectStore
	.word	localObjectStoreNoRef
	.word	localObjectUnref

	
fieldObjectType:
	@ get ptr to object var into r0
	@ TOS is base ptr, r1 is field offset in bytes
	
	ldmia	rsp!, {r0}	@ r0 = field offset from TOS
	add	r0, r1
	b	objectEntry	

memberObjectType:
	@ get ptr to object var into r0
	@ this data ptr is base ptr, r1 is field offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r0, r1
	b	objectEntry	
	
localObjectArrayType:
	@ get ptr to object var into r0
	@ FP is base ptr, r1 is offset in objects
	ldmia	rsp!, {r0}			@ r0 = array index
	sub	r1, r0					@ add in array index
	lsl	r1, #3
	sub	r0, rfp, r1				@ r0 points to the object field
	b	objectEntry

fieldObjectArrayType:
	@ get ptr to object var into r0
	@ TOS is struct base ptr, NOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!, {r0, r2}		@ r0 = struct base ptr, r2 = index
	lsl	r2, #3
	add	r0, r2
	add	r0, r1					@ add in offset to base of array
	b	objectEntry

memberObjectArrayType:
	@ get ptr to into var into r0
	@ this data ptr is base ptr, TOS is index
	@ r1 is field offset in bytes
	ldmia	rsp!,{r0}			@ r0 = index
	lsl	r0, #3
	ldr	r2, [rcore, #tpd]			@ r2 = object base ptr
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
	add	rsp, #4
	bx	lr
	
nipBop:
	ldmia	rsp!, {r0}
	str	r0, [rsp]
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
	ldmia	rrp!, {r0}		@ pop top of return stack into R0
	stmdb	rsp!, {r0}		@ push r0 onto param stack
	bx	lr
	
@@@@@@@@@@@@@@@@@  lit  @@@@@@@@@@@@@@@@@

litBop:
	ldmia	rip!, {r0}
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dlit  @@@@@@@@@@@@@@@@@

dlitBop:
	ldmia	rip!, {r0, r1}
	mov	r2, r0
	stmdb	rsp!, {r1, r2}
	bx	lr
	
@@@@@@@@@@@@@@@@@  _doVariable  @@@@@@@@@@@@@@@@@

doVariableBop:
	@ IP points to immediate data field
	stmdb	rsp!,{rip}		@ push addr of data field
							@    pointed to by IP
	ldmia	rrp!, {rip}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doConstant  @@@@@@@@@@@@@@@@@

doConstantBop:
	@ IP points to immediate data field
	ldr	r0, [rip]			@ fetch data in immedate field pointed to by IP
	stmdb	rsp!, {r0}
	ldmia	rrp!, {rip}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  _doDConstant  @@@@@@@@@@@@@@@@@

doDConstantBop:
	@ IP points to immediate data field
	ldmia	rip!, {r0, r1}
	stmdb	rsp!, {r0, r1}
	ldmia	rrp!, {rip}		@ pop return stack into IP
	bx	lr

@@@@@@@@@@@@@@@@@  done  @@@@@@@@@@@@@@@@@

doneBop:
	mov	r0, #kResultDone
	str	r0, [rcore, #istate]
	bx	lr

@@@@@@@@@@@@@@@@@  _doByte  @@@@@@@@@@@@@@@@@

@ doByteOp is compiled as the first op in global byte vars
@ the byte data field is immediately after this op

doByteBop:
	@ get ptr to byte var into r0
	@ IP points to byte var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	byteEntry

doUByteBop:
	@ get ptr to byte var into r0
	@ IP points to byte var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	ubyteEntry

@@@@@@@@@@@@@@@@@  _doByteArray  @@@@@@@@@@@@@@@@@

@ doByteArrayOp is compiled as the first op in global byte arrays
@ the data array is immediately after this op

doByteArrayBop:
	@ get ptr to byte var into r0
	@ IP points to base of byte array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	byteEntry

doUByteArrayBop:
	@ get ptr to byte var into r0
	@ IP points to base of byte array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	ubyteEntry

@@@@@@@@@@@@@@@@@  _doShort  @@@@@@@@@@@@@@@@@

@ doShortOp is compiled as the first op in global short vars
@ the short data field is immediately after this op

doShortBop:
	@ get ptr to short var into r0
	@ IP points to short var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	shortEntry

doUShortBop:
	@ get ptr to short var into r0
	@ IP points to short var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	ushortEntry

@@@@@@@@@@@@@@@@@  _doShortArray  @@@@@@@@@@@@@@@@@

@ doShortArrayOp is compiled as the first op in global short arrays
@ the data array is immediately after this op

doShortArrayBop:
	@ get ptr to short var into r0
	@ IP points to base of short array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #1
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	shortEntry

doUShortArrayBop:
	@ get ptr to short var into r0
	@ IP points to base of short array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #1
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	ushortEntry

@@@@@@@@@@@@@@@@@  _doInt  @@@@@@@@@@@@@@@@@

@ doIntOp is compiled as the first op in global int vars
@ the int data field is immediately after this op

doIntBop:
	@ get ptr to int var into r0
	@ IP points to int var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	intEntry

@@@@@@@@@@@@@@@@@  _doIntArray  @@@@@@@@@@@@@@@@@

@ doIntArrayOp is compiled as the first op in global int arrays
@ the data array is immediately after this op

doIntArrayBop:
	@ get ptr to int var into r0
	@ IP points to base of int array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	intEntry

@@@@@@@@@@@@@@@@@  _doFloat  @@@@@@@@@@@@@@@@@

@ doFloatOp is compiled as the first op in global float vars
@ the float data field is immediately after this op

doFloatBop:
	@ get ptr to float var into r0
	@ IP pofloats to float var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doFloatArray  @@@@@@@@@@@@@@@@@

@ doFloatArrayOp is compiled as the first op in global float arrays
@ the data array is immediately after this op

doFloatArrayBop:
	@ get ptr to float var into r0
	@ IP points to base of float array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	floatEntry

@@@@@@@@@@@@@@@@@  _doDouble  @@@@@@@@@@@@@@@@@

@ doDoubleOp is compiled as the first op in global double vars
@ the double data field is immediately after this op

doDoubleBop:
	@ get ptr to double var into r0
	@ IP podoubles to double var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	doubleEntry

@@@@@@@@@@@@@@@@@  _doDoubleArray  @@@@@@@@@@@@@@@@@

@ doDoubleArrayOp is compiled as the first op in global double arrays
@ the data array is immediately after this op

doDoubleArrayBop:
	@ get ptr to double var into r0
	@ IP points to base of double array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	doubleEntry


@@@@@@@@@@@@@@@@@  _doString  @@@@@@@@@@@@@@@@@

@ doStringOp is compiled as the first op in global string vars
@ the string data field is immediately after this op

doStringBop:
	@ get ptr to string var into r0
	@ IP postrings to string var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	stringEntry

@@@@@@@@@@@@@@@@@  _doStringArray  @@@@@@@@@@@@@@@@@

@ doStringArrayOp is compiled as the first op in global string arrays
@ the data array is immediately after this op

doStringArrayBop:
	@ get ptr to string var into r0
	@ IP points to base of string array
	mov	r0, rip				@ r0 -> maxLen field of string[0]
	ldr	r1, [r0]			@ r1 = maxLen
	ldmia	rsp!, {r2}		@ pop index off pstack
	asr	r1, #2
	add	r1, #3				@ r1 is element length in longs
	mul	r1, r2
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	stringEntry

@@@@@@@@@@@@@@@@@  _doLong  @@@@@@@@@@@@@@@@@
@ doLongOp is compiled as the first op in global long vars
@ the long data field is immediately after this op

doLongBop:
	@ get ptr to long var into r0
	@ IP polongs to long var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	longEntry

@@@@@@@@@@@@@@@@@  _doLongArray  @@@@@@@@@@@@@@@@@

@ doLongArrayOp is compiled as the first op in global long arrays
@ the data array is immediately after this op

doLongArrayBop:
	@ get ptr to long var into r0
	@ IP points to base of long array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	longEntry


@@@@@@@@@@@@@@@@@  _doString  @@@@@@@@@@@@@@@@@

@@@@@@@@@@@@@@@@@  _doOp  @@@@@@@@@@@@@@@@@

@ doOpOp is compiled as the first op in global op vars
@ the op data field is immediately after this op

doOpBop:
	@ get ptr to op var into r0
	@ IP points to op var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	opEntry

@@@@@@@@@@@@@@@@@  _doOpArray  @@@@@@@@@@@@@@@@@

@ doOpArrayOp is compiled as the first op in global op arrays
@ the data array is immediately after this op

doOpArrayBop:
	@ get ptr to op var into r0
	@ IP points to base of op array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #2
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	opEntry


@@@@@@@@@@@@@@@@@  _doObject  @@@@@@@@@@@@@@@@@

@ doObjectOp is compiled as the first op in global object vars
@ the object data field is immediately after this op

doObjectBop:
	@ get ptr to object var into r0
	@ IP points to object var
	mov	r0, rip
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	objectEntry

@@@@@@@@@@@@@@@@@  _doObjectArray  @@@@@@@@@@@@@@@@@

@ doObjectArrayOp is compiled as the first op in global object arrays
@ the data array is immediately after this op

doObjectArrayBop:
	@ get ptr to object var into r0
	@ IP points to base of object array
	mov	r0, rip
	ldmia	rsp!, {r1}		@ pop index off pstack
	lsl	r1, #3
	add	r0, r1				@ add index to array base
	ldmia	rrp!, {rip}		@ pop IP off rstack
	b	objectEntry
	
@@@@@@@@@@@@@@@@@  addressOf  @@@@@@@@@@@@@@@@@

addressOfBop:
	mov	r0,	#kVarRef
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  ->  @@@@@@@@@@@@@@@@@

intoBop:
	mov	r0, #kVarStore
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@ ->+   @@@@@@@@@@@@@@@@@

addToBop:
	mov	r0, #kVarPlusStore
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  ->-  @@@@@@@@@@@@@@@@@

subtractFromBop:
	mov	r0, #kVarMinusStore
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  _doVocab  @@@@@@@@@@@@@@@@@

doVocabBop:
	@ push longword @ IP
	ldr	r0, [rip]
	stmdb	rsp!, {r0}
	@ pop IP off rstack
	ldmia	rrp!, {rip}
	bx	lr
	
@@@@@@@@@@@@@@@@@  _exit  @@@@@@@@@@@@@@@@@

doExitBop:
	ldr	r0, [rcore, #rp0]			@ check for rstack underflow
	cmp	rrp, r0
	bge	.doExitBop1
	ldmia	rrp!, {rip}			@ pop IP off rstack
	bx	lr
	
.doExitBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitL  @@@@@@@@@@@@@@@@@

doExitLBop:
    @ rstack: local_var_storage oldFP oldIP
    @ FP points to oldFP
	ldr	r0, [rcore, #rp0]			@ check for rstack underflow
	add	r1, rfp, #4				@ take oldFP into account
	cmp	r1, r0
	bge	.doExitLBop1
	mov	rrp, rfp					@ deallocate local vars
	ldmia	rrp!, {rfp}			@ pop oldFP off rstack
	ldmia	rrp!, {rip}			@ pop IP off rstack
	bx	lr
	
.doExitLBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitM  @@@@@@@@@@@@@@@@@

doExitMBop:
    @ rstack: oldIP oldTPM oldTPD
	ldr	r0, [rcore, #rp0]			@ check for rstack underflow
	add	r1, rrp, #8				@ take oldTPM and oldTPD into account
	cmp	r1, r0
	bge	.doExitMBop1
	ldmia	rrp!, {r0, r1, r2}	@ pop oldIP, oldTPM, oldTPD off rstack
	mov	rip, r0
	str	r1, [rcore, #tpm]
	str	r2, [rcore, #tpd]
	bx	lr
	
.doExitMBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit
	
@@@@@@@@@@@@@@@@@  _exitML  @@@@@@@@@@@@@@@@@

doExitMLBop:
    @ rstack: local_var_storage oldFP oldIP oldTPM oldTPD
    @ FP points to oldFP
	ldr	r0, [rcore, #rp0]			@ check for rstack underflow
	add	r1, rfp, #12				@ take oldFP/oldTPM/oldTPD into account
	cmp	r1, r0
	bge	.doExitMLBop1
	mov	rrp, rfp					@ deallocate local vars
	ldmia	rrp!, {rfp}			@ pop oldFP off rstack
	ldmia	rrp!, {r0, r1, r2}	@ pop oldIP, oldTPM, oldTPD off rstack
	mov	rip, r0
	str	r1, [rcore, #tpm]
	str	r2, [rcore, #tpd]
	bx	lr
	
.doExitMLBop1:
	mov	r0, #kForthErrorReturnStackUnderflow
	b	interpLoopErrorExit

	@ TBD	doDoubleArrayBop
	@ TBD	doStringArrayBop
	@ TBD	doOpArrayBop
	@ TBD	doObjectArrayBop
	
@@@@@@@@@@@@@@@@@  +  @@@@@@@@@@@@@@@@@

plusBop:
	ldmia	rsp!, {r0}	@ r0 = B
	ldr	r1, [rsp]		@ r1 = A
	add	r1, r0
	str	r1, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  @  @@@@@@@@@@@@@@@@@

fetchBop:
	ldr	r0, [rsp]
	ldr	r1, [r0]
	str	r1, [rsp]
	bx	lr

storeNextBop:
	ldmia	rsp!, {r0, r1}	@ r1 = value, r0 = ptr to dest ptr
	ldr	r2, [r0]			@ r2 = dest ptr
	stmia	r2!, {r1}		@ store value and advance dest ptr
	str	r2, [r0]			@ update stored dest ptr
	bx	lr

fetchNextBop:
	ldr	r1, [rsp]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldmia	r2!, {r0}		@ r0 = fetched value
	str	r2, [r1]			@ update stored src ptr
	str	r0, [rsp]
	bx	lr
	
		
@@@@@@@@@@@@@@@@@  _doStruct  @@@@@@@@@@@@@@@@@

doStructBop:
	@ IP points to global struct immediately following this instruction
	stmdb	rsp!, {rip}		@ push IP (address of struct) onto pstack
	ldmia	rrp!, {rip}		@ pop IP off rstack
	bx	lr

@@@@@@@@@@@@@@@@@  _doStructArray  @@@@@@@@@@@@@@@@@

doStructArrayBop:
	@ TOS is array index
	@ IP -> bytes per element, followed by element 0
	ldr	r0, [rsp]			@ pop index off pstack
	ldmia	rip!, {r1}		@ r1 = bytes per element, IP -> first element
	mul	r1, r0
	add	r1, rip				@ r1 -> Nth element
	str	r1, [rsp]			@ replace index on TOS with element address
	ldmia	rrp!, {rip}		@ pop IP off rstack
	bx	lr
	

@@@@@@@@@@@@@@@@@  _do  @@@@@@@@@@@@@@@@@

@
@ TOS is start-index
@ TOS+4 is end-index
@ the op right after this one should be a branch
@
doDoBop:
	ldmia	rsp!, {r0, r1}	@ r0 = start index, r1 = end index
	@ rstack[2] holds top-of-loop IP
	add	rip, #4				@ skip over loop exit branch right after this op
	stmdb	rrp!, {rip}		@ rpush start-of-loop IP
	stmdb	rrp!, {r1}		@ rpush end index
	stmdb	rrp!, {r0}		@ rpush start index
	bx	lr
	
@@@@@@@@@@@@@@@@@  _loop  @@@@@@@@@@@@@@@@@

doLoopBop:
	ldmia	rrp, {r0, r1}	@ r0 = current loop index, r1 = end index
	add	r0, #1
	cmp	r0, r1
	bge	.doLoopBop1			@ branch if done
	str	r0,[rrp]
	ldr	rip, [rrp, #8]		@ go back to top of loop
	bx	lr

.doLoopBop1:
	add	rrp, #12				@ deallocate start index, end index, start loop IP
	bx	lr
	
	
@@@@@@@@@@@@@@@@@  _+loop  @@@@@@@@@@@@@@@@@


doLoopNBop:
	ldmia	rrp, {r0, r1}	@ r0 = current loop index, r1 = end index
	ldmia	rsp!, {r2}		@ r2 = increment
	add	r0, r2				@ add increment to current index
	str	r0,[rrp]
	cmp	r2, #0
	blt	.doLoopNBop1		@ branch if increment is negative

	@ r2 is positive increment
	cmp	r0, r1
	bge	.doLoopNBop2		@ branch if done
	ldr	rip, [rrp, #8]		@ go back to top of loop
	bx	lr

.doLoopNBop1:
	@ r2 is negative increment
	cmp	r0, r1
	blt	.doLoopNBop2		@ branch if done
	ldr	rip, [rrp, #8]		@ go back to top of loop
	bx	lr
	
.doLoopNBop2:
	add	rrp, #12				@ deallocate start index, end index, start loop IP
	bx	lr
	
@@@@@@@@@@@@@@@@@  _checkDo  @@@@@@@@@@@@@@@@@
	
@
@ TOS is start-index
@ TOS+4 is end-index
@ the op right after this one should be a branch
@
doCheckDoBop:
	ldmia	rsp!, {r0, r1}	@ r0 = start index, r1 = end index
	cmp	r0, r1
	bge	dcdo1
	
	@ rstack[2] holds top-of-loop IP
	add	rip, #4				@ skip over loop exit branch right after this op
	stmdb	rrp!, {rip}		@ rpush start-of-loop IP
	stmdb	rrp!, {r1}		@ rpush end index
	stmdb	rrp!, {r0}		@ rpush start index
dcdo1:
	bx	lr
	
@@@@@@@@@@@@@@@@@  d@  @@@@@@@@@@@@@@@@@


dfetchBop:
	ldr	r0, [rsp]
	ldmia	r0!, {r1, r2}
	str	r2, [rsp]
	stmdb	rsp!, {r1}
	bx	lr

@@@@@@@@@@@@@@@@@  this  @@@@@@@@@@@@@@@@@

thisBop:
	ldr	r1, [rcore, #tpd]
	ldr	r0, [rcore, #tpm]
	stmdb	rsp!, {r0, r1}
	bx	lr
	
@@@@@@@@@@@@@@@@@  thisData  @@@@@@@@@@@@@@@@@

thisDataBop:
	ldr	r0, [rcore, #tpd]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  thisMethods  @@@@@@@@@@@@@@@@@

thisMethodsBop:
	ldr	r0, [rcore, #tpm]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  execute  @@@@@@@@@@@@@@@@@

executeBop:
	ldmia	rsp!, {r0}
	b	interpLoopExecuteEntry


@#############################################@
@                                             @
@         runtime control flow stuff          @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  call  @@@@@@@@@@@@@@@@@

callBop:
	stmdb	rrp!, {rip}
	ldmia	rsp!, {rip}
	bx	lr

@@@@@@@@@@@@@@@@@  goto  @@@@@@@@@@@@@@@@@

gotoBop:
	ldmia	rsp!, {rip}
	bx	lr

@@@@@@@@@@@@@@@@@  i  @@@@@@@@@@@@@@@@@

iBop:
	ldr	r0, [rrp]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  j  @@@@@@@@@@@@@@@@@

jBop:
	ldr	r0, [rrp, #12]
	stmdb	rsp!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  unloop  @@@@@@@@@@@@@@@@@

unloopBop:
	add	rrp, #12
	bx	lr

@@@@@@@@@@@@@@@@@  leave  @@@@@@@@@@@@@@@@@

leaveBop:
	ldr	rip, [rrp, #8]
	sub	rip, #4
	add	rrp, #12
	bx	lr

@@@@@@@@@@@@@@@@@  here  @@@@@@@@@@@@@@@@@

hereBop:
	ldr	r0, [rcore, #dict_mem_sect]
	ldr	r1, [r0, #FMSCurrent]
	stmdb	rsp!, {r1}
	bx	lr
			
@@@@@@@@@@@@@@@@@  removeEntry  @@@@@@@@@@@@@@@@@

removeEntryBop:
	mov	r0, #kVocabRemoveEntry
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  entryLength  @@@@@@@@@@@@@@@@@

entryLengthBop:
	mov	r0, #kVocabEntryLength
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  numEntries  @@@@@@@@@@@@@@@@@

numEntriesBop:
	mov	r0, #kVocabNumEntries
	str	r0, [rcore, #varmode]
	bx	lr

vocabToClassBop:
	mov	r0, #kVocabGetClass
	str	r0, [rcore, #varmode]
	bx	lr


@#############################################@
@                                             @
@                integer math                 @
@                                             @
@#############################################@

@@@@@@@@@@@@@@@@@  *2  @@@@@@@@@@@@@@@@@

times2Bop:
	ldr	r0, [rsp]
	lsl	r0, r0, #1
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  *4  @@@@@@@@@@@@@@@@@

times4Bop:
	ldr	r0, [rsp]
	lsl	r0, r0, #2
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  *8  @@@@@@@@@@@@@@@@@

times8Bop:
	ldr	r0, [rsp]
	lsl	r0, r0, #3
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /  @@@@@@@@@@@@@@@@@

	.global	__aeabi_idiv
divideBop:
	push	{r12, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	__aeabi_idiv
	str	r0, [rsp]
	pop	{r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  /2  @@@@@@@@@@@@@@@@@

divide2Bop:
	ldr	r0, [rsp]
	asr	r0, r0, #1
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /4  @@@@@@@@@@@@@@@@@

divide4Bop:
	ldr	r0, [rsp]
	asr	r0, r0, #2
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /8  @@@@@@@@@@@@@@@@@

divide8Bop:
	ldr	r0, [rsp]
	asr	r0, r0, #3
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  /mod  @@@@@@@@@@@@@@@@@

divmodBop:
	push	{lr}
	ldr	r2, [rsp]				@ r2 = denominator
	ldr	r1, [rsp, #4]			@ r1 = numerator
	sub	sp, #12					@ ? scratch space for div routine ?
	mov	r0, sp
	bl	div
	ldr	r1, [sp, #4]
	str	r1, [rsp, #4]
	ldr	r2, [sp]
	str	r2, [rsp]
	add	sp, #12
	pop	{lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  mod  @@@@@@@@@@@@@@@@@

modBop:
	.global	__aeabi_idivmod
	push	{r12, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	__aeabi_idivmod
	str	r1, [rsp]
	pop	{r12, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  negate  @@@@@@@@@@@@@@@@@

negateBop:
	ldr	r0, [rsp]
	neg	r0, r0
	str	r0, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  -  @@@@@@@@@@@@@@@@@

minusBop:
	ldmia	rsp!, {r0}	@ r0 = B
	ldr	r1, [rsp]		@ r1 = A
	sub	r2, r1, r0
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  *  @@@@@@@@@@@@@@@@@

timesBop:
	ldmia	rsp!, {r0}	@ r0 = B
	ldr	r1, [rsp]		@ r1 = A
	mul	r1, r0
	str	r1, [rsp]
	bx	lr
	
@#############################################@
@                                             @
@         single precision fp math            @
@                                             @
@#############################################@

fplusBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fadds	s15, s15, s14
	fsts	s15, [rsp]
	bx	lr

fminusBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fsubs	s15, s15, s14
	fsts	s15, [rsp]
	bx	lr

ftimesBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fmuls	s15, s15, s14
	fsts	s15, [rsp]
	bx	lr

fdivideBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fdivs	s15, s15, s14
	fsts	s15, [rsp]
	bx	lr
	
@#############################################@
@                                             @
@         double precision fp math            @
@                                             @
@#############################################@
	
dplusBop:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [rsp]
	faddd	d7, d7, d6
	fstd	d7, [rsp]
	bx	lr

dminusBop:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [rsp]
	fsubd	d7, d7, d6
	fstd	d7, [rsp]
	bx	lr

dtimesBop:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [rsp]
	fmuld	d7, d7, d6
	fstd	d7, [rsp]
	bx	lr

ddivideBop:
	fldd	d6, [rsp]
	add		rsp, #8
	fldd	d7, [rsp]
	fdivd	d7, d7, d6
	fstd	d7, [rsp]
	bx	lr

	
@#############################################@
@                                             @
@       single precision fp functions         @
@                                             @
@#############################################@

fsinBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	sinf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fasinBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	asinf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fcosBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	cosf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

facosBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	acosf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

ftanBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	tanf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fatanBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	atanf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fatan2Bop:
	push	{rcore, lr}
	flds	s0, [rsp]
	add		rsp, #4
	flds	s1, [rsp]
	bl	atan2f
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fexpBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	expf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

flnBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	logf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

flog10Bop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	log10f
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fpowBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	add		rsp, #4
	flds	s1, [rsp]
	bl	powf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fsqrtBop:
	push	{rcore, lr}
	flds	s7, [rsp]
	fsqrts	s7, s7
	fcmps	s7, s7
	fmstat
	beq	.fsqrt1
	bl	sqrtf
	fcpys	s7, s0
.fsqrt1:
	fsts	s7, [rsp]
	pop	{rcore, lr}
	bx	lr

fceilBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	ceilf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

ffloorBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	bl	floorf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fabsBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	fabss	s0, s0
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

fldexpBop:
	push	{rcore, lr}
	ldmia	rsp!, {r0}
	flds	s0, [rsp]
	bl	ldexpf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

ffrexpBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	sub	rsp, #4
	mov	r0, rsp
	bl	frexpf
	fsts	s0, [rsp, #4]
	pop	{rcore, lr}
	bx	lr
	
fmodfBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	sub	rsp, #8
	mov	r0, rsp
	bl	modff
	fsts	s0, [rsp, #4]
	pop	{rcore, lr}
	bx	lr
	
ffmodBop:
	push	{rcore, lr}
	flds	s0, [rsp]
	add		rsp, #4
	flds	s1, [rsp]
	bl	fmodf
	fsts	s0, [rsp]
	pop	{rcore, lr}
	bx	lr

@#############################################@
@                                             @
@       double precision fp functions         @
@                                             @
@#############################################@

dsinBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	sin
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dasinBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	asin
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dcosBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	cos
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dacosBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	acos
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dtanBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	tan
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

datanBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	atan
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

datan2Bop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	add		rsp, #8
	fldd	d1, [rsp]
	bl	atan2
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dexpBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	exp
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dlnBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	log
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dlog10Bop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	log10
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dpowBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	add		rsp, #8
	fldd	d1, [rsp]
	bl	pow
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dsqrtBop:
	push	{rcore, lr}
	fldd	d7, [rsp]
	fsqrtd	d7, d7
	fcmpd	d7, d7
	fmstat
	beq	.dsqrt1
	bl	sqrt
	fcpyd	d7, d0
.dsqrt1:
	fmrrd	r2, r3, d7
	strd	r2, [rsp]
	pop	{rcore, lr}
	bx	lr

dceilBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	ceil
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dfloorBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	bl	floor
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dabsBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	fabsd	d0, d0
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dldexpBop:
	push	{rcore, lr}
	ldmia	rsp!, {r0}
	fldd	d0, [rsp]
	bl	ldexp
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

dfrexpBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	sub	rsp, #4
	mov	r0, rsp
	bl	frexp
	fstd	d0, [rsp, #4]
	pop	{rcore, lr}
	bx	lr
	
dmodfBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	sub	rsp, #8
	mov	r0, rsp
	bl	modf
	fstd	d0, [rsp, #8]
	pop	{rcore, lr}
	bx	lr
	
dfmodBop:
	push	{rcore, lr}
	fldd	d0, [rsp]
	add		rsp, #8
	fldd	d1, [rsp]
	bl	fmod
	fstd	d0, [rsp]
	pop	{rcore, lr}
	bx	lr

lplusBop:
	ldmia	rsp!, {r2, r3}			@ pop TOS long value into r3(lo),r2(hi)
	ldm	rsp!, {r0, r1}			@ pop TOS long value into r1(lo),r0(hi)
	adds	r1, r3, r1
	addc	r0, r2, r0
	stm	!rsp, {r0, r1}
	bx	lr

lminusBop:
	ldmia	rsp!, {r2, r3}			@ pop TOS long value into r3(lo),r2(hi)
	ldm	rsp!, {r0, r1}			@ pop TOS long value into r1(lo),r0(hi)
	subs	r1, r1, r3
	sbc	r0, r0, r2
	stm	!rsp, {r0, r1}
	bx	lr

ltimesBop:
	@ based on http://stackoverflow.com/questions/1131833/how-do-you-multiply-two-64-bit-numbers-in-x86-assembler
	@ TOS: bhi blo ahi alo
	@ umull RdLo, RdHi, Rn, Rm
	add	r2, rcore, #ipsave			@ r1 -> IP
	stmia	r2!, {rip, rsp, rfp}				@ save IP, RP, FP (r5-r8) in core
	@ rip will be the sign flag
	eor	rip, rip
	eor	rfp, rfp
	@ negate a and/or b and stuff absolute values back in stack
	ldmia	rsp!, {r0, r1}			@ r0=bhi r1=blo
	ldm	rsp!, {r2, r3}				@ r2=ahi r3=alo
	@ negate b if necessary
	orrs	r0, r0
	bge	ltimes1
	@ negate b
	add	rip, #1
	subs	r1, rfp, r1
	sbc	r0, rfp, r0
ltimes1:	
	@ negate a if necessary
	orrs	r2, r2
	bge	ltimes2
	@ negate a
	add	rip, #1
	subs	r3, rfp, r3
	sbc	r2, rfp, r2
ltimes2:
	umull	rrp, rfp, r1, r3		@ rrp = resultLo, rfp = resultHi
	umull	r0, r3, r0, r3			@ r0 = low 32 bits of (bhi * alo)
	add	rfp, r0
	umull	r0, r3, r2, r1			@ r0 = low 32 bits of (ahi * blo)
	add	rfp, r0
	@ rrp = resultLo, rfp = resultHi
	tst	rip, #1
	beq	ltimes3
	@ invert result
	eor rip, rip
	subs	rrp, rip, rrp
	sbc	rfp, rip, rfp
ltimes3:
	str	rfp, [rsp]
	str	rrp, [rsp, #4]
	bx	lr

umtimesBop:
	ldr	r0, [rsp]		@ r0 = B
	ldr	r1, [rsp, #4]	@ r1 = A
	umull	r3, r2, r1, r0
	stmia	rsp, {r2, r3}
	bx	lr

mtimesBop:
	ldr	r0, [rsp]		@ r0 = B
	ldr	r1, [rsp, #4]	@ r1 = A
	smull	r3, r2, r1, r0
	stmia	rsp, {r2, r3}
	bx	lr

@#############################################@
@                                             @
@       int/float/double conversions          @
@                                             @
@#############################################@

i2fBop:
	ldr	r0, [rsp]
	fmsr	s14, r0 @ int
	fsitos	s15, s14
	fsts	s15, [rsp]
	bx	lr

i2dBop:
	ldr	r0, [rsp]
	sub	rsp, #4
	fmsr	s14, r0 @ int
	fsitod	d7, s14
	fstd	d7, [rsp]
	bx	lr

	
f2iBop:
	flds	s15, [rsp]
	ftosizs	s15, s15
	fmrs	r0, s15 @ int
	str	r0, [rsp]
	bx	lr

f2dBop:
	flds	s15, [rsp]
	sub	rsp, #4
	fcvtds	d7, s15
	fstd	d7, [rsp]
	bx	lr

d2iBop:
	fldd	d7, [rsp]
	add	rsp, #4
	ftosizd	s13, d7
	fmrs	r0, s13 @ int
	str	r0, [rsp]
	bx	lr
	
d2fBop:
	fldd	d7, [rsp]
	add	rsp, #4
	fcvtsd	s15, d7
	fsts	s15, [rsp]
	bx	lr
	
@#############################################@
@                                             @
@             bit-vector logic                @
@                                             @
@#############################################@

orBop:
	ldmia	rsp!, {r0}
	ldr	r1, [rsp]
	orr	r0, r1
	str	r0, [rsp]
	bx	lr
	
andBop:
	ldmia	rsp!, {r0}
	ldr	r1, [rsp]
	and	r0, r1
	str	r0, [rsp]
	bx	lr
	
xorBop:
	ldmia	rsp!, {r0}
	ldr	r1, [rsp]
	eor	r0, r1
	str	r0, [rsp]
	bx	lr

invertBop:
	ldr	r0, [rsp]
	eor	r1, r1
	sub	r1, #1
	eor	r0, r1
	str	r0, [rsp]
	bx	lr
	
lshiftBop:
	ldmia	rsp!, {r0}		@ r0 = shift count
	ldr	r1, [rsp]
	lsl	r1, r0
	str	r1, [rsp]
	bx	lr

arshiftBop:
	ldmia	rsp!, {r0}		@ r0 = shift count
	ldr	r1, [rsp]
	asr	r1, r0
	str	r1, [rsp]
	bx	lr

rshiftBop:
	ldmia	rsp!, {r0}		@ r0 = shift count
	ldr	r1, [rsp]
	mov	r2, r1, lsr r0
	str	r2, [rsp]
	bx	lr
 	
@@@@@@@@@@@@@@@@@  not  @@@@@@@@@@@@@@@@@

notBop:
	eor	r1, r1
	ldr	r0, [rsp]
	cmp	r0, r1
	subeq	r1, #1
	str	r1, [rsp]
	bx	lr
	

@@@@@@@@@@@@@@@@@  true  @@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@  archARM  @@@@@@@@@@@@@@@@@

archARMBop:
trueBop:
	eor	r0, r0
	sub	r0, #1
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  false  @@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@  null  @@@@@@@@@@@@@@@@@
@@@@@@@@@@@@@@@@@  archX86  @@@@@@@@@@@@@@@@@

archX86Bop:
falseBop:
nullBop:
	eor	r0, r0
	stmdb	rsp!, {r0}
	bx	lr
	

oclearBop
	mov	r0, #kVarStore
	str	r0, [rcore, #varmode]
	@ fall thru to dnull
	
@@@@@@@@@@@@@@@@@  dnull  @@@@@@@@@@@@@@@@@

dnullBop:
	eor	r0, r0
	mov	r1, r0
	stmdb	rsp!, {r0, r1}
	bx	lr

@#############################################@
@                                             @
@          integer comparisons                @
@                                             @
@#############################################@
	
@@@@@@@@@@@@@@@@@  =  @@@@@@@@@@@@@@@@@

equalsBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subeq	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  <>  @@@@@@@@@@@@@@@@@

notEqualsBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subne	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  >  @@@@@@@@@@@@@@@@@

greaterThanBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subgt	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  u>  @@@@@@@@@@@@@@@@@

unsignedGreaterThanBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subhi	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  >=  @@@@@@@@@@@@@@@@@

greaterEqualsBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subge	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  <  @@@@@@@@@@@@@@@@@

lessThanBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	sublt	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  u<  @@@@@@@@@@@@@@@@@

unsignedLessThanBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subcc	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  <=  @@@@@@@@@@@@@@@@@

lessEqualsBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	suble	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0=  @@@@@@@@@@@@@@@@@

equals0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r2, r0
	subeq	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0<>  @@@@@@@@@@@@@@@@@

notEquals0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r2, r0
	subne	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0>  @@@@@@@@@@@@@@@@@

greaterThan0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subgt	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0>=  @@@@@@@@@@@@@@@@@

greaterEquals0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r0, r2
	subge	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0<  @@@@@@@@@@@@@@@@@

lessThan0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r0, r2
	sublt	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  0<=  @@@@@@@@@@@@@@@@@

lessEquals0Bop:
	ldmia	rsp!, {r0}
	eor	r2, r2
	cmp	r0, r2
	suble	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

	
@@@@@@@@@@@@@@@@@  icmp  @@@@@@@@@@@@@@@@@
	
icmpBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	sublt	r2, #1
	addgt	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  uicmp  @@@@@@@@@@@@@@@@@
	
uicmpBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r1, r0
	subhi	r2, #1
	addcc	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@#############################################@
@                                             @
@     single precision fp comparisons         @
@                                             @
@#############################################@
	
@@@@@@@@@@@@@@@@@  f=  @@@@@@@@@@@@@@@@@

fEqualsBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	subeq	r2, #1
	str	r2, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  f<>  @@@@@@@@@@@@@@@@@

fNotEqualsBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	subne	r2, #1
	stmdb	rsp!, {r2}
	bx	lr

@@@@@@@@@@@@@@@@@  f>  @@@@@@@@@@@@@@@@@

fGreaterThanBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	subgt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f>=  @@@@@@@@@@@@@@@@@

fGreaterEqualsBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	subge	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f<  @@@@@@@@@@@@@@@@@

fLessThanBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	sublt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f<=  @@@@@@@@@@@@@@@@@

fLessEqualsBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	suble	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0=  @@@@@@@@@@@@@@@@@

fEquals0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	subeq	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<>  @@@@@@@@@@@@@@@@@

fNotEquals0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	subne	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0>  @@@@@@@@@@@@@@@@@

fGreaterThan0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	subgt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0>=  @@@@@@@@@@@@@@@@@

fGreaterEquals0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	subge	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<  @@@@@@@@@@@@@@@@@

fLessThan0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	sublt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  f0<=  @@@@@@@@@@@@@@@@@

fLessEquals0Bop:
	eor	r2, r2
	flds	s15, [rsp]
	fcmpzs	s15
	fmstat
	suble	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@ fmin  @@@@@@@@@@@@@@@@@

fMinBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s14, s15
	fmstat
	blt	fmin1
	fsts	s15, [rsp]
	bx	lr
	
fmin1:
	fsts	s14, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  fmax  @@@@@@@@@@@@@@@@@

fMaxBop:
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s14, s15
	fmstat
	bgt	fmin1
	fsts	s15, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  fwithin  @@@@@@@@@@@@@@@@@

fWithinBop:
	eor	r3, r3
	flds	s15, [rsp]			@ upper bound
	flds	s14, [rsp, #4]		@ lower bound
	add	rsp, #8
	flds	s13, [rsp]			@ test value
	fcmps	s13, s14
	fmstat
	blt	.fwithin1
	fcmps	s13, s15
	fmstat
	sublt	r3, #1
.fwithin1:
	str	r3, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  fcmp  @@@@@@@@@@@@@@@@@
	
fcmpBop:
	eor	r2, r2
	flds	s14, [rsp]
	add	rsp, #4
	flds	s15, [rsp]
	fcmps	s15, s14
	fmstat
	subgt	r2, #1
	addlt	r2, #1
	str	r2, [rsp]
	bx	lr

@#############################################@
@                                             @
@     double precision fp comparisons         @
@                                             @
@#############################################@
	
@@@@@@@@@@@@@@@@@  d=  @@@@@@@@@@@@@@@@@

dEqualsBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	subeq	r2, #1
	str	r2, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  d<>  @@@@@@@@@@@@@@@@@

dNotEqualsBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	subne	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d>  @@@@@@@@@@@@@@@@@

dGreaterThanBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	subgt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d>=  @@@@@@@@@@@@@@@@@

dGreaterEqualsBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	subge	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d<  @@@@@@@@@@@@@@@@@

dLessThanBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	sublt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d<=  @@@@@@@@@@@@@@@@@

dLessEqualsBop:
	eor	r2, r2
	fldd	d6, [rsp]
	fldd	d7, [rsp, #8]
	add	rsp, #12
	fcmpd	d7, d6
	fmstat
	suble	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0=  @@@@@@@@@@@@@@@@@

dEquals0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	subeq	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0<>  @@@@@@@@@@@@@@@@@

dNotEquals0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	subne	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0>  @@@@@@@@@@@@@@@@@

dGreaterThan0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	subgt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0>=  @@@@@@@@@@@@@@@@@

dGreaterEquals0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	subge	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0<  @@@@@@@@@@@@@@@@@

dLessThan0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	sublt	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  d0<=  @@@@@@@@@@@@@@@@@

dLessEquals0Bop:
	eor	r2, r2
	fldd	d7, [rsp]
	add	rsp, #4
	fcmpzd	d7
	fmstat
	suble	r2, #1
	str	r2, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@ dmin  @@@@@@@@@@@@@@@@@

dMinBop:
	fldd	d7, [rsp]
	add	rsp, #8
	fldd	d6, [rsp]
	fcmpd	d6, d7
	fmstat
	blt	dmin1
	fstd	d7, [rsp]
	bx	lr
	
dmin1:
	fstd	d6, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  dmax  @@@@@@@@@@@@@@@@@

dMaxBop:
	fldd	d7, [rsp]
	add	rsp, #8
	fldd	d6, [rsp]
	fcmpd	d6, d7
	fmstat
	bgt	dmax1
	fstd	d7, [rsp]
	bx	lr
	
dmax1:
	fstd	d6, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  dwithin  @@@@@@@@@@@@@@@@@

dWithinBop:
	eor	r3,r3
	fldd	d7, [rsp]			@ upper bound
	fldd	d6, [rsp, #8]		@ lower bound
	add	rsp, #16
	fldd	d5, [rsp]			@ test value
	fcmpd	d5, d6
	fmstat
	blt	.dwithin1
	fcmpd	d5, d7
	fmstat
	sublt	r3, #1
.dwithin1:
	add	rsp, #4
	str	r3, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  dcmp  @@@@@@@@@@@@@@@@@
	
dcmpBop:
	eor	r2, r2
	fldd	d6, [rsp]
	add	rsp, #8
	fldd	d7, [rsp]
	add rsp, #4
	fcmpd	d7, d6
	fmstat
	subgt	r2, #1
	addlt	r2, #1
	str	r2, [rsp]
	bx	lr

@#############################################@
@                                             @
@            stack manipulation               @
@                                             @
@#############################################@

	
@@@@@@@@@@@@@@@@@  >r  @@@@@@@@@@@@@@@@@

rpushBop:
	ldmia	rsp!, {r0}
	stmdb	rrp!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  r>  @@@@@@@@@@@@@@@@@

rpopBop:
	ldmia	rrp!, {r0}
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  rdrop  @@@@@@@@@@@@@@@@@

rdropBop:
	add	rrp, #4
	bx	lr
	
@@@@@@@@@@@@@@@@@  rp  @@@@@@@@@@@@@@@@@

rpBop:
	stmdb	rsp!, {rrp}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r0  @@@@@@@@@@@@@@@@@

r0Bop:
	ldr	r0, [rcore, #rp0]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  dup  @@@@@@@@@@@@@@@@@

dupBop:
	ldr	r0, [rsp, #0]
	stmdb	rsp!, {r0}
	bx	lr

checkDupBop:
	eor r2, r2
	ldr	r0, [rsp, #0]
	cmp	r0, r2
	beq	cdup1
	stmdb	rsp!, {r0}
cdup1:
	bx	lr

@@@@@@@@@@@@@@@@@  swap  @@@@@@@@@@@@@@@@@

swapBop:
	ldr	r0, [rsp]
	ldr	r1, [rsp, #4]
	str	r1, [rsp]
	str	r0, [rsp, #4]
	bx	lr
		
@@@@@@@@@@@@@@@@@  over  @@@@@@@@@@@@@@@@@

overBop:
	ldr	r0, [rsp, #4]
	stmdb	rsp!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  rot  @@@@@@@@@@@@@@@@@

rotBop:
	ldmia	rsp, {r1, r2, r3}
	mov	r0, r3
	stmia	rsp, {r0, r1, r2}
	bx	lr
	
reverseRotBop:
	ldmia	rsp, {r1, r2, r3}
	stmia	rsp, {r2, r3}
	str	r1, [rsp, #8]
	bx	lr
	
@@@@@@@@@@@@@@@@@  tuck  @@@@@@@@@@@@@@@@@

tuckBop:
	ldmia	rsp, {r0, r1}
	mov	r2, r0
	sub	rsp, #4
	stmia	rsp, {r0, r1, r2}
	bx 	lr
	
@@@@@@@@@@@@@@@@@  pick  @@@@@@@@@@@@@@@@@

pickBop:
	ldr	r0, [rsp]
	add	r0, #1
	ldr	r1, [rsp, r0, lsl #2]
	str	r1, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sp  @@@@@@@@@@@@@@@@@

spBop:
	mov	r0, rsp
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  s0  @@@@@@@@@@@@@@@@@

s0Bop:
	ldr	r0, [rcore, #sp0]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  fp  @@@@@@@@@@@@@@@@@

fpBop:
	stmdb	rsp!, {rfp}
	bx	lr
	
@@@@@@@@@@@@@@@@@  ip  @@@@@@@@@@@@@@@@@

ipBop:
	stmdb	rsp!, {rip}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r[ @@@@@@@@@@@@@@@@@
startTupleBop:
	stmdb	rrp!, {rsp}
	bx	lr
	
@@@@@@@@@@@@@@@@@  ]r @@@@@@@@@@@@@@@@@
endTupleBop:
	ldmia	rrp!, {r0}
	sub	r0, r0, rsp
	asr	r0, r0, #2
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  r@ @@@@@@@@@@@@@@@@@
rpeekBop:
	ldr	r0, [rrp]
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  min  @@@@@@@@@@@@@@@@@

minBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	bgt	.min1
	mov	r0, r1
.min1:
	stmdb	rsp!, {r0}
	bx	lr

@@@@@@@@@@@@@@@@@  max  @@@@@@@@@@@@@@@@@

maxBop:
	ldmia	rsp!, {r0, r1}
	eor	r2, r2
	cmp	r0, r1
	blt	.max1
	mov	r1, r0
.max1:
	stmdb	rsp!, {r1}
	bx	lr

@@@@@@@@@@@@@@@@@  within  @@@@@@@@@@@@@@@@@

withinBop:
	eor	r3,r3
	ldmia	rsp!, {r0, r1, r2}		@ r2=test value, r1=lower bound, r0=upperBound
	cmp	r2, r1
	blt	.within1
	cmp	r2, r0
	sublt	r3, #1
.within1:
	stmdb	rsp!, {r3}
	bx	lr

@@@@@@@@@@@@@@@@@  ddup  @@@@@@@@@@@@@@@@@

ddupBop:
	ldmia	rsp, {r0, r1}
	stmdb	rsp!, {r0, r1}
	bx	lr

@@@@@@@@@@@@@@@@@  dswap  @@@@@@@@@@@@@@@@@

dswapBop:
	ldmia	rsp, {r0, r1, r2, r3}
	stmia	rsp, {r2, r3}
	str	r0, [rsp, #8]
	str	r1, [rsp, #12]
	bx	lr
	
@@@@@@@@@@@@@@@@@  ddrop  @@@@@@@@@@@@@@@@@

ddropBop:
	add	rsp, #8
	bx	lr
	
@@@@@@@@@@@@@@@@@  dover  @@@@@@@@@@@@@@@@@

doverBop:
	ldr	r0, [rsp, #8]
	ldr	r1, [rsp, #12]
	stmdb	rsp!, {r0, r1}
	bx	lr

@@@@@@@@@@@@@@@@@  drot  @@@@@@@@@@@@@@@@@

drotBop:
	ldmia	rsp, {r0, r1}	@ r01 = p[0]
	ldr	r2, [rsp, #16]		@ r23 = p[2]
	ldr	r3, [rsp, #20]
	stmia	rsp, {r2, r3}	@ p[0] = r23		(old p[2])
	ldr	r2, [rsp, #8]		@ r23 = p[1]
	ldr	r3, [rsp, #12]
	str	r0, [rsp, #8]		@ p[1] = r01		(old p[0])
	str	r1, [rsp, #12]
	str	r2, [rsp, #16]		@ p[2] = r23		(old p[1])
	str	r3, [rsp, #20]
	bx	lr

	
@#############################################@
@                                             @
@          memory store/fetch                 @
@                                             @
@#############################################@

@@@@@@@@@@@@@@@@@  !  @@@@@@@@@@@@@@@@@

storeBop:
	ldmia	rsp!, {r0, r1}	@ r0 = int ptr, r1 = value
	str	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  c!  @@@@@@@@@@@@@@@@@

cstoreBop:
	ldmia	rsp!, {r0, r1}	@ r0 = byte ptr, r1 = value
	strb	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  c@  @@@@@@@@@@@@@@@@@

cfetchBop:
	ldr	r0, [rsp]
	eor	r1, r1
	ldrb	r1, [r0]
	str	r1, [rsp]
	bx	lr

cstoreNextBop:
	ldmia	rsp!, {r0, r1}	@ r1 = value, r0 = ptr to dest ptr
	ldr	r2, [r0]			@ r2 = dest ptr
	strb	r1, [r2]
	add	r2, #1
	str	r2, [r0]			@ update stored dest ptr
	bx	lr

cfetchNextBop:
	ldr	r1, [rsp]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldrb	r0, [r2]
	add	r2, #1
	str	r2, [r1]			@ update stored src ptr
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sc@  @@@@@@@@@@@@@@@@@

scfetchBop:
	ldr	r0, [rsp]
	eor	r1, r1
	ldrsb	r1, [r0]
	str	r1, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  c2i  @@@@@@@@@@@@@@@@@

c2iBop:
	ldrsb	r0, [rsp]
	str	r0, [rsp]
	bx	lr

@@@@@@@@@@@@@@@@@  w!  @@@@@@@@@@@@@@@@@

wstoreBop:
	ldmia	rsp!, {r0, r1}	@ r0 = short ptr, r1 = value
	strh	r1, [r0]
	bx	lr

@@@@@@@@@@@@@@@@@  w@  @@@@@@@@@@@@@@@@@

wfetchBop:
	ldr	r0, [rsp]
	eor	r1, r1
	ldrh	r1, [r0]
	str	r1, [rsp]
	bx	lr

wstoreNextBop:
	ldmia	rsp!, {r0, r1}	@ r1 = value, r0 = ptr to dest ptr
	ldr	r2, [r0]			@ r2 = dest ptr
	strh	r1, [r2]
	add	r2, #2
	str	r2, [r0]			@ update stored dest ptr
	bx	lr

wfetchNextBop:
	ldr	r1, [rsp]			@ r1 - ptr to src ptr
	ldr	r2, [r1]			@ r2 = src ptr
	ldrh	r0, [r2]
	add	r2, #2
	str	r2, [r1]			@ update stored src ptr
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  sw@  @@@@@@@@@@@@@@@@@

swfetchBop:
	ldr	r0, [rsp]
	eor	r1, r1
	ldrsh	r1, [r0]
	str	r1, [rsp]
	bx	lr

	
@@@@@@@@@@@@@@@@@  w2i  @@@@@@@@@@@@@@@@@

w2iBop:
	ldrsh	r0, [rsp]
	str	r0, [rsp]
	bx	lr
	
@@@@@@@@@@@@@@@@@  d!  @@@@@@@@@@@@@@@@@

dstoreBop:
	ldmia	rsp!, {r0, r1, r2}
	str	r1, [r0]
	str	r2, [r0, #4]
	bx	lr
	
dstoreNextBop:
	ldmia	rsp!, {r0, r1, r2}	@ r1,r2 = value, r0 = ptr to dest ptr
	ldr	r3, [r0]				@ r3 = dest ptr
	stmia	r3!, {r1, r2}		@ store value and advance dest ptr
	str	r3, [r0]				@ update stored dest ptr
	bx	lr

dfetchNextBop:
	ldmia	rsp!, {r2}			@ r2 = ptr to src ptr
	ldr	r3, [r2]				@ r3 = src ptr
	ldmia	r3!, {r0, r1}		@ r0,r1 = fetched value
	str	r3, [r2]				@ update stored src ptr
	stmdb	rsp!, {r0, r1}
	bx	lr
	
@@@@@@@@@@@@@@@@@  o!  @@@@@@@@@@@@@@@@@
ostoreBop:
	ldmia	rsp!, {r0, r1, r2}
	str	r2, [r0]
	str	r1, [r0, #4]
	bx	lr
	
ostoreNextBop:
	ldmia	rsp!, {r0, r1, r2}	@ r1,r2 = value, r0 = ptr to dest ptr
	ldr	r3, r1
	ldr	r1, [r0]				@ r1 = dest ptr
	stmia	r1!, {r2, r3}	@ store value and advance dest ptr
	str	r1, [r0]				@ update stored dest ptr
	bx	lr

ofetchBop:
	ldr	r0, [rsp]
	ldmia	r0!, {r1, r2}
	str	r1, [rsp]
	stmdb	rsp!, {r2}
	bx	lr

ofetchNextBop:
	ldmia	rsp!, {r2}			@ r2 = ptr to src ptr
	ldr	r3, [r2]					@ r3 = src ptr
	ldmia	r3!, {r0, r1}		@ r0,r1 = fetched value
	str	r3, [r2]					@ update stored src ptr
	stmdb	rsp!, {r1}
	stmdb	rsp!, {r0}
	bx	lr
	
@@@@@@@@@@@@@@@@@  move  @@@@@@@@@@@@@@@@@

moveBop:
	@	TOS is #bytes, TOS-1 is dst, TOS-2 is src
	push	{rcore, lr}
	ldmia	rsp!, {r0, r1, r3}		@ r0 = #bytes, r1 = dst r3 = src
	@ memcpy takes r0 = dst, r1 = src, r2 = #bytes
	mov	r2, r0
	mov	r0, r1
	mov	r1, r3
	bl	memcpy
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  fill  @@@@@@@@@@@@@@@@@

fillBop:
	@	TOS is #bytes, TOS-1 is fillValue, TOS-2 is dst
	push	{rcore, lr}
	ldmia	rsp!, {r0, r1, r3}		@ r0 = #bytes, r1 = fillValue r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	memset
	pop	{rcore, lr}
	bx	lr
	
	
@@@@@@@@@@@@@@@@@  varAction!  @@@@@@@@@@@@@@@@@

setVarActionBop:
	ldmia	rsp!, {r0}
	str	r0, [rcore, #varmode]
	bx	lr

@@@@@@@@@@@@@@@@@  varAction@  @@@@@@@@@@@@@@@@@

getVarActionBop:
	ldr	r0, [rcore, #varmode]
	stmdb	rsp!, {r0}
	bx	lr

byteVarActionBop:
	ldmia	rsp!, {r0}
	b	byteEntry
	
ubyteVarActionBop:
	ldmia	rsp!, {r0}
	b	ubyteEntry
	
shortVarActionBop:
	ldmia	rsp!, {r0}
	b	shortEntry
	
ushortVarActionBop:
	ldmia	rsp!, {r0}
	b	ushortEntry
	
intVarActionBop:
	ldmia	rsp!, {r0}
	b	intEntry
	
floatVarActionBop:
	ldmia	rsp!, {r0}
	b	floatEntry
	
stringVarActionBop:
	ldmia	rsp!, {r0}
	b	stringEntry

doubleVarActionBop:
	ldmia	rsp!, {r0}
	b	doubleEntry
	
longVarActionBop:
	ldmia	rsp!, {r0}
	b	longEntry
	
opVarActionBop:
	ldmia	rsp!, {r0}
	b	opEntry
	
objectVarActionBop:
	ldmia	rsp!, {r0}
	b	objectEntry
	
@#############################################@
@                                             @
@             string manipulation             @
@                                             @
@#############################################@


@@@@@@@@@@@@@@@@@  strcpy  @@@@@@@@@@@@@@@@@

strcpyBop:
	@	TOS is src, TOS-1 is dst
	push	{rcore, lr}
	ldmia	rsp!, {r1, r2}
	mov	r0, r2
	bl	strcpy
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strncpy  @@@@@@@@@@@@@@@@@

strncpyBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{rcore, lr}
	ldmia	rsp!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncpy
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strlen  @@@@@@@@@@@@@@@@@

strlenBop:
	push	{rcore, lr}
	ldr	r0, [rsp]
	bl	strlen
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strcat  @@@@@@@@@@@@@@@@@

strcatBop:
	@	TOS is src, TOS-1 is dst
	push	{rcore, lr}
	ldmia	rsp!, {r1, r2}
	mov	r0, r2
	bl	strcat
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strncat  @@@@@@@@@@@@@@@@@

strncatBop:
	@	TOS is #bytes, TOS-1 is src, TOS-2 is dst
	push	{rcore, lr}
	ldmia	rsp!, {r0, r1, r3}		@ r0 = #bytes, r1 = src r3 = dst
	mov	r2, r0
	mov	r0, r3
	bl	strncat
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strchrBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strchr
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strchr  @@@@@@@@@@@@@@@@@

strrchrBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strrchr
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strcmp  @@@@@@@@@@@@@@@@@

strcmpBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strcmp
strcmp1:
	eor	r1, r1
	cmp	r0, #0
	sublt	r1, #1
	addgt	r1, #1
	str	r1, [rsp]
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  stricmp  @@@@@@@@@@@@@@@@@

stricmpBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strcasecmp
	b	strcmp1
	
@@@@@@@@@@@@@@@@@  strstr  @@@@@@@@@@@@@@@@@

strstrBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strstr
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
@@@@@@@@@@@@@@@@@  strtok  @@@@@@@@@@@@@@@@@

strtokBop:
	push	{rcore, lr}
	ldmia	rsp!, {r1}
	ldr	r0, [rsp]
	bl	strtok
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr


initStringBop:
	eor	r2,r2
	ldmia	rsp!, {r0, r1}		@ r0 = len, r1 = strptr
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
	push	{rcore, lr}
	ldr	r1, [rsp]
	ldr	r0, [rsp, #4]
	@ r0=path, r1=access
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileOpen]
	blx	r3
	add	r0, #4
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr

fcloseBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileClose]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
fseekBop:
	push	{rcore, lr}
	ldr	r2, [rsp]
	ldr	r1, [rsp, #4]
	ldr	r0, [rsp, #8]
	@ r0=pFile r1=offset r2=seekType
	ldr	r3, [rcore, #file_funcs]
	ldr	r3, [r3, #fileSeek]
	blx	r3
	add	rsp, #8
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
freadBop:
	push	{rip, lr}
	ldr	r3, [rsp]
	ldr	r2, [rsp, #4]
	ldr	r1, [rsp, #8]
	ldr	r0, [rsp, #12]
	@ int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
	@ r0=pDst r1=itemSize r2=numItems r3=pFile
	ldr	rip, [rcore, #file_funcs]
	ldr	rip, [rip, #fileRead]
	blx	rip
	add	rsp, #12
	str	r0, [rsp]
	pop	{rip, lr}
	bx	lr
	
fwriteBop:
	push	{rip, lr}
	ldr	r3, [rsp]
	ldr	r2, [rsp, #4]
	ldr	r1, [rsp, #8]
	ldr	r0, [rsp, #12]
	@ int result = pCore->pFileFuncs->fileRead( pDst, itemSize, numItems, pFP );
	@ r0=pDst r1=itemSize r2=numItems r3=pFile
	ldr	rip, [rcore, #file_funcs]
	ldr	rip, [rip, #fileWrite]
	blx	rip
	add	rsp, #12
	str	r0, [rsp]
	pop	{rip, lr}
	bx	lr
	
fgetcBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileGetChar]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
fputcBop:
	push	{rcore, lr}
	ldr	r1, [rsp]
	ldr	r0, [rsp, #4]
	@ r1=char, r0=pFile
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #filePutChar]
	blx	r3
	add	rsp, #4
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr

feofBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileAtEnd]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
ftellBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileTell]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
flenBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileGetLength]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
fexistsBop:	
	push	{rcore, lr}
	ldr	r0, [rsp]
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #fileExists]
	blx	r3
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
fgetsBop:
	push	{rcore, lr}
	ldr	r2, [rsp]
	ldr	r1, [rsp, #4]
	ldr	r0, [rsp, #8]
	@ r0=pBuffer r1=maxChars r2=pFile
	ldr	r3, [rcore, #file_funcs]
	ldr	r3, [r3, #fileGetString]
	blx	r3
	add	rsp, #8
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr
	
fputsBop:
	push	{rcore, lr}
	ldr	r1, [rsp]
	ldr	r0, [rsp, #4]
	@ r0=pBuffer, r1=pFile
	ldr	r2, [rcore, #file_funcs]
	ldr	r3, [r2, #filePutString]
	blx	r3
	add	rsp, #4
	str	r0, [rsp]
	pop	{rcore, lr}
	bx	lr

	
@ extern void fprintfSub( ForthCoreState* pCore );
	
fprintfSub:
	push	{rcore, lr}
	ldmia	rsp!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	rrp!, {sp}		@ save old PC SP for cleanup on rstack
fprintfSub1:
	sub	r2, #1
	bl	fprintfSub2
	ldmia	rsp!, {r0}
	push	{r0}
	b	fprintfSub1
fprintfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	fprintf
	stmdb	rsp!, {r0}
	ldmia	rrp!, {sp}		@ cleanup PC stack
	pop	{rcore, lr}
	bx	lr

	
fscanfSub:
	push	{rcore, lr}
	ldmia	rsp!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	rrp!, {sp}		@ save old PC SP for cleanup on rstack
fscanfSub1:
	sub	r2, #1
	bl	fscanfSub2
	ldmia	rsp!, {r0}
	push	{r0}
	b	fscanfSub1
fscanfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	fscanf
	stmdb	rsp!, {r0}
	ldmia	rrp!, {sp}		@ cleanup PC stack
	pop	{rcore, lr}
	bx	lr

	
sprintfSub:
	push	{rcore, lr}
	ldmia	rsp!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	rrp!, {sp}		@ save old PC SP for cleanup on rstack
sprintfSub1:
	sub	r2, #1
	bl	sprintfSub2
	ldmia	rsp!, {r0}
	push	{r0}
	b	sprintfSub1
sprintfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	sprintf
	stmdb	rsp!, {r0}
	ldmia	rrp!, {sp}		@ cleanup PC stack
	pop	{rcore, lr}
	bx	lr

	
sscanfSub:
	push	{rcore, lr}
	ldmia	rsp!, {r1}		@ r1 is argument count
	add	r1, #2				
	mov	r2, r1
	stmdb	rrp!, {sp}		@ save old PC SP for cleanup on rstack
sscanfSub1:
	sub	r2, #1
	bl	sscanfSub2
	ldmia	rsp!, {r0}
	push	{r0}
	b	sscanfSub1
sscanfSub2:
	@ all args have been moved from parameter stack to PC stack
	bl	sscanf
	stmdb	rsp!, {r0}
	ldmia	rrp!, {sp}		@ cleanup PC stack
	pop	{rcore, lr}
	bx	lr

	

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		      OP TYPE ACTION ROUTINES                    @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

@ opType action routines expect the following:
@	r1		op value (low 24 bits, already masked)
@	r2		op type (8 bits)
@	r4		core pointer
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
	add	r0, rcore, #ipsave			@ r0 -> IP
	stmia	r0!, {rip-rrp}			@ save IP, SP, RP in core
	ldr	r3, [rcore]				@ r3 = table of C optype action routines
	ldr	r2, [r3, r2, lsl #2]	@ r2 = action routine for this optype
	mov	r0, rcore					@ C++ optype routines take core ptr as 1st argument
	bx	r2

nativeImmediate:
	ldr	r3, [rcore, #n_ops]
	cmp	r1, r3
	bge	badOpcode
	ldr	r0, [rcore, #ops]
	ldr	r2, [r0, r1, lsl #2]
  bx  r2

@ externalBuiltin is invoked when a builtin op which is outside of range of table is invoked
externalBuiltin:
	@ it should be impossible to get here now
	b	badOpcode
	

badOpcode:
	mov	r0, #kForthErrorBadOpcode
	b	interpLoopErrorExit
	
userDefType:
	ldr	r3, [rcore, #n_ops]
	cmp	r1, r3
	bge	badUserDef
	@ rpush IP
	stmdb	rrp!, {rip}
	@ fetch new IP
	ldr	r0, [rcore, #ops]
	ldr	rip, [r0, r1, lsl #2]
	bx	lr
	
badUserDef:
	mov	eax, kForthErrorBadOpcode
	jmp	interpLoopErrorExit
;-----------------------------------------------
;
; user-defined ops (forth words defined with colon)
;
entry relativeDefType
	@ r1 is offset from dictionary base of user definition
	ldr	r2,=0x00FFFFFF
  ands  r1, r2
  lsl r1, #2

  @ check that new IP is below top of dictionary
	ldr	r2, [rcore, #dict_mem_sect]
  add r1, [r2, #FMSBase]
  cmp r1, [r2, #FMSCurrent]
	bge	badUserDef

	@ rpush IP
	stmdb	rrp!, {rip}
  ldr rip, r1
	bx	lr

branchType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	ands	r3, r1
	orrne	r1,r2
	lsl	r1, #2
	add	rip, r1			@ add branchVal to IP
	bx	lr

branchNZType:
	ldmia	rsp!, {r0}
	cmp	r0, #0
	bne	branchType
	bx	lr
	
branchZType:
	ldmia	rsp!, {r0}
	cmp	r0, #0
	beq	branchType
	bx	lr

@ branch around block ops
pushBranchType:
	stmdb	rsp!, {rip}   @ push IP (pointer to block)
	ldr	r2,=0x00FFFFFF
  ands  r1, r2
  lsl r1, #2  
  add rip, r1
  bx  lr

caseBranchType:
	@ TOS is current case value to match against, TOS-1 is value to check
	@ r1 holds branch offset in longs
	ldmia	rsp, {r0, r2}
	cmp	r0, r2
	bne	.caseBranch1
	@ case matched, drop top 2 TOS items, continue executing this branch
	add	rsp, #8
	bx	lr
	
.caseBranch1:
	@ case did not match, drop current case value & skip to next case
	add	rsp, #4
	lsl	r1, #2
	add	rip, r1
	bx	lr
	
constantType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	ands	r3, r1
	orrne	r1,r2
	stmdb	rsp!, {r1}
	bx	lr
	
offsetType:
	ldr	r2,=0xFF000000
	ldr	r3,=0x00800000
	ands	r3, r1
	orrne	r1,r2
	ldr	r0, [rsp]
	add	r0, r1
	str	r0, [rsp]
	bx	lr
	
offsetFetchType:
	lsl	r3, r1, #8		@ see if bit 23 is set, if so branchVal is negative
	bpl	.oft1
	ldr	r3, .oft2
	orr	r1, r3
.oft1:
	ldr	r0, [rsp]
	add	r1, r0
	ldr	r0, [r1]
	str	r0, [rsp]
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
	ldmia	rsp!, {r2, r3}	@ r2 is array base, r3 is index
	mul	r3, r1
	add	r2, r3
	stmdb	rsp!, {r2}
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
	ldr	r3, [rsp]
	mul	r3, r0			@ r3 is offset of struct element from base of array
	lsr	r1, #10
	ldr	r0, =0x3FFC
	and	r1, r0			@ r1 is offset of base array from FP in bytes
	sub	r0, rfp, r1	
	add	r0, r3
	stmdb	rsp!, {r0}
	bx	lr

@-----------------------------------------------
@
@ string constant ops
@
constantStringType:
	@ IP points to beginning of string
	@ low 24-bits of ebx is string len in longs
	stmdb	rsp!, {rip}	@ push string ptr
	@ get low-24 bits of opcode
	lsl	r1, #2
	@ advance IP past string
	add	rip, r1
	bx	lr
	
@-----------------------------------------------
@
@ local stack frame allocation ops
@
allocLocalsType:
	@ rpush old FP
	stmdb	rrp!, {rfp}
	@ set FP = RP, points at old FP
	mov	rfp, rrp
	@ allocate amount of storage specified by low 24-bits of op on rstack
	lsl	r1, #2
	sub	rrp, r1
	@ clear out allocated storage
	eor	r2, r2
	mov	r0, rrp
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
	sub	r3, rfp, r1
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
	add	r1, rfp
	stmdb	rsp!, {r1}
	bx	lr
	
@-----------------------------------------------
@
@ member reference ops
@
memberRefType:
	@ push member reference - r1 is member offset in bytes
	ldr	r0, [rcore, #tpd]
	add	r1, r0
	stmdb	rsp!, {r1}
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
	ldr	r2, [rcore, #tpd]
	add	r3, r2, r1
	eor	r2, r2
	stmia	r3!, {r0, r2}	
	strb	r2, [r3]	@ add terminating null
	bx	lr

@-----------------------------------------------
@
@ DLL vocabulary support
@
@ extern void CallDLLRoutine( DLLRoutine function, long argCount, void *core, ulong flags );

CallDLLRoutine:
	push	{r4-r10, lr}
	@ r0	function addr
	@ r1	argCount
	@ r2	flags (bit0 is return void, bit2 is return 64-bit, bit3 is use stdcall convention
	@ r3	pCore
	mov	r4, r0				@ r4 is function addr
	mov	r5, r1				@ r5 is argCount
	mov	r6,	r2				@ r6 is flags
	mov r7, r3				@ r7 is pCore
	ldr	r8, [r7, #spsave]	@ r8 is param stack ptr
	
	orrs	r5, r5
	beq	.callDLLArgsReady			@ if no args
	mov	r10, r8				@ r10 will be used if there are more than 4 args
	add	r8, r8, r5, lsl #2	@ r8 is now param stack ptr after all args are consumed
	str	r8, [r7, #spsave]	@ save param stack ptr
	mov	r9, r8	
	sub	r9, #4				
	ldmda	r9!, {r0}
	subs	r5, #1
	beq	.callDLLArgsReady
	ldmda	r9!, {r1}
	subs	r5, #1
	beq	.callDLLArgsReady
	ldmda	r9!, {r2}
	subs	r5, #1
	beq	.callDLLArgsReady
	ldmda	r9!, {r3}
	subs	r5, #1
	beq	.callDLLArgsReady
	
	@ more than 4 args, rest are on param stack
	mov	r8, sp				@ for fixing sp if stdcall convention is used
.callDLLArgLoop:
	ldmia	r10!, {r9}
	push	{r9}
	subs	r5, #1
	bne	.callDLLArgLoop
		
.callDLLArgsReady:					@ args are in registers or on stack
	bl	.callDLLDispatch
	@ we will return here from DLL routine
	ands	r2, r6, #4		@ use stdcall convention?
	beq	.callDLL5
	mov	sp, r8
	@ TBD: cleanup stack for stdcall convention
.callDLL5:
	ldr	r8, [r7, #spsave]	@ r8 is param stack ptr
	ands	r2, r6, #1		@ void return?
	bne	.callDLL4
	ands	r2, r6, #2		@ 64-bit return?
	beq	.callDLL3
	stmdb	r8!, {r0, r1}
	b	.callDLL4
	
.callDLL3:
	stmdb	r8!, {r0}	
.callDLL4:
	str	r8, [r7, #spsave]	@ r8 is param stack ptr
	
	pop	{r4-r10, pc}

.callDLLDispatch:
	bx	r4
	
@========================================
dllEntryPointType:
	@ r1: opcode in 0..15, flags in 16..18, argCount in 19..23
	ldr	r2, =0xFFFF
	and	r3, r2, r1				@ r3 is opcode index for dll entry point
	ldr	r0, [rcore, #n_ops]
	cmp	r3, r0
	bge	badOpcode
	push	{r12, lr}
	lsr	r2, r1, #16
	and	r2, #7					@ r2 is flags
	lsr	r1, #19					@ r1 is arg count
	ldr	r0, [rops, r3, lsl #2]	@ r0 is dll function addr
	mov	r3, r4					@ r3 is pCore
	
	str	rsp, [rcore, #spsave]
	bl	CallDLLRoutine
	ldr	rsp, [rcore, #spsave]

	pop	{r12, lr}
	bx	lr
	

@-----------------------------------------------
@
@ method invocation ops
@

@ invoke a method on object currently referenced by this ptr pair
methodWithThisType:
	@ r1 is method number
	@ push this ptr pair on return stack
	ldr	r2, [rcore, #tpm]
	ldr	r3, [rcore, #tpd]
	stmdb	rrp!, {r2, r3}
	@ get method opcode
	ldr	r0, [r2, r1, lsl #2]
	b	interpLoopExecuteEntry
		
@ invoke a method on an object referenced by ptr pair on TOS
methodWithTOSType:
	@ TOS is object vtable, NOS is object data ptr
	@ ebx is method number
	@ push this ptr pair on return stack
	ldr	r2, [rcore, #tpm]
	ldr	r3, [rcore, #tpd]
	stmdb	rrp!, {r2, r3}
	@ set this data,methods from TOS	
	ldmia	rsp!, {r2, r3}
	str	r2, [rcore, #tpm]
	str	r3, [rcore, #tpd]
	@ get method opcode
	ldr	r0, [r2, r1, lsl #2]
	b	interpLoopExecuteEntry
	
@ squished float literal
entry squishedFloatType
	@ r1: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	@ to unsquish a float:
	@   sign = (inVal & 0x800000) << 8
	@   exponent = (((inVal >> 18) & 0x1f) + (127 - 15)) << 23
	@   mantissa = (inVal & 0x3ffff) << 5
	@   outVal = sign | exponent | mantissa
	ldr	r0, =0x00800000     @ get sign bit
	and	r0, r1
	lsl	r0, #8

	ldr	r3, r1
	lsr	r3, #18
	ldr	r2, =0x1F
	and	r3, r2
	lsl	r3, #23			@ r3 is exponent
	orr	r0, r3
	
	ldr	r2, =0x03FFFF
	and	r2, r1
	shl	r2, #5			@ r2 is mantissa
	or	r0, r2
	
	stmdb	rsp!, {r0}
	bx	lr

@ squished double literal
squishedDoubleType:
	@ r1: bit 23 is sign, bits 22..18 are exponent, bits 17..0 are mantissa
	@ to unsquish a double:
	@   sign = (inVal & 0x800000) << 8
	@   exponent = (((inVal >> 18) & 0x1f) + (1023 - 15)) << 20
	@   mantissa = (inVal & 0x3ffff) << 2
	@   outVal = (sign | exponent | mantissa) << 32
	ldr	r0, =0x00800000     @ get sign bit
	and	r0, r1
	lsl	r0, #8

	ldr	r3, r1
	lsr	r3, #18
	ldr	r2, =0x1F
	and	r3, r2
	ldr	r2, =1008
	add	r3, r2
	lsl	r3, #20			@ r3 is exponent
	orr	r0, r3
	
	ldr	r2, =0x03FFFF
	and	r2, r1
	shl	r2, #2			@ r2 is mantissa
	or	r0, r2
	
	stmdb	rsp!, {r0}
	eorr	r2, r2
	stmdb	rsp!, {r0}		@ loword of double is all zeros
	bx	lr
	
@ squished long literal
squishedLongType:
	ldr	r0, =0x00800000     @ get sign bit
	and	r0, r1
	cmp	r0, #0
	bne longConstantNegative
	
	@ positive constant
	ldr	r0, =0x00FFFFFF
	and	r0, r1
	stmdb	rsp!, {r0}
	eorr	r0, r0
	stmdb	rsp!, {r0}
	bx	lr
	
longConstantNegative:
	ldr	r0, =0xFF000000
	or	r0, r1
	stmdb	rsp!, {r0}
	ldr	r0, =0xFFFFFFFF
	stmdb	rsp!, {r0}
	bx	lr


@ LOCALREF OP combo ops
lroComboType:
	@ r1: bits 0..11 are frame offset in longs, bits 12-23 are op
	ldr	r0, =0x0FFF
	and	r0, r1
	lsl	r0, #2
	sub	r2, rfp, r0					@ r2 points to the local var
	stmdb	rsp!, {r2}

	lsr	r1, #12
	ldr	r0, =0x0FFF
	and	r0, r2				@ opcode is in r0
	bx	interpLoopExecuteEntry
	
@ MEMBERREF OP combo ops
mroComboType:
	@ r1: bits 0..11 are member offset in bytes, bits 12-23 are op
	@ r1: bits 0..11 are frame offset in longs, bits 12-23 are op
	ldr	r0, =0x0FFF
	and	r0, r1
	ldr	r2, [rcore, #tpd]
	add	r2, r0							@ r2 points to the member var
	stmdb	rsp!, {r2}

	lsr	r1, #12
	ldr	r0, =0x0FFF
	and	r0, r2				@ opcode is in r0
	bx	interpLoopExecuteEntry


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
	.word	externalBuiltin		@ kOpNative = 0,
	.word	nativeImmediate		@ kOpNativeImmediate,
	.word	userDefType				@ kOpUserDef,
	.word	userDefType				@ kOpUserDefImmediate,
	.word	cCodeType				  @ kOpCCode,         
	.word	cCodeType				  @ kOpCCodeImmediate,
	.word	relativeDefType	  @ kOpRelativeDef
	.word	relativeDefType	  @ kOpRelativeDefImmediate
	.word	dllEntryPointType	@ kOpDLLEntryPoint,
	.word	extOpType	
@	10 - 19
	.word	branchType				@ kOpBranch = 10,
	.word	branchNZType			@ kOpBranchNZ,
	.word	branchZType				@ kOpBranchZ,
	.word	caseBranchType		@ kOpCaseBranch,
	.word	pushBranchType    @ kOpPushBranch,	
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
	
@	110 - 119
	.word	memberStringInitType
	.word	extOpType @ nvoComboType
	.word	extOpType @ nvComboType
	.word	extOpType @ noComboType
	.word	extOpType @ voComboType
	.word	extOpType @ ozbComboType
	.word	extOpType @ obComboType

	.word	squishedFloatType 
	.word	squishedDoubleType
	.word	squishedLongType
	
@	120 - 121
	.word	lroComboType
	.word	mroComboType

@	122 - 149
	.word	extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType,extOpType
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

#endif
	
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
@                                                    @
@                                                    @
@		         THE END OF ALL THINGS               @
@                                                    @
@                                                    @
@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
