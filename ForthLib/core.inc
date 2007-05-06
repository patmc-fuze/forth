
option expr32
option casemap:none

; Begin of file core.h
kVarFetch		EQU		0t
kVarRef		EQU		1t
kVarStore		EQU		2t
kVarPlusStore		EQU		3t
kVarMinusStore		EQU		4t

kVocabSetCurrent		EQU	0t
kVocabNewestEntry		EQU	1t
kVocabFindEntry			EQU	2t
kVocabFindEntryValue	EQU	3t
kVocabAddEntry			EQU	4t
kVocabRemoveEntry		EQU	5t
kVocabEntryLength		EQU	6t
kVocabNumEntries		EQU	7t

kResultOk		EQU		0t
kResultDone		EQU		1t
kResultExitShell		EQU		2t
kResultError		EQU		3t
kResultFatalError		EQU		4t
kResultException		EQU		5t


kForthErrorNone						EQU		0t
kForthErrorBadOpcode				EQU		1t
kForthErrorBadOpcodeType			EQU		2t
kForthErrorBadParameter				EQU		3t
kForthErrorParamStackUnderflow		EQU		4t
kForthErrorParamStackOverflow		EQU		5t
kForthErrorReturnStackUnderflow		EQU		6t
kForthErrorReturnStackOverflow		EQU		7t
kForthErrorUnknownSymbol			EQU		8t
kForthErrorFileOpen					EQU		9t
kForthErrorAbort					EQU		10t
kForthErrorForgetBuiltin			EQU		11t
kForthErrorBadMethod				EQU		12t
kForthErrorException				EQU		13t
kForthErrorMissingSize				EQU		14t
kForthErrorStruct					EQU		15t
kForthErrorUserDefined				EQU		16t
kForthErrorBadSyntax				EQU		17t
kForthNumErrors						EQU		18t


kPrintSignedDecimal		EQU		0t
kPrintAllSigned		EQU		1t
kPrintAllUnsigned		EQU		2t


kOpBuiltIn		EQU		0t
kOpUserDef		EQU		1t
kOpBranch		EQU		2t
kOpBranchNZ		EQU		3t
kOpBranchZ		EQU		4t
kOpCaseBranch		EQU		5t
kOpConstant		EQU		6t
kOpOffset		EQU		7t
kOpArrayOffset		EQU		8t
kOpConstantString		EQU		9t
kOpAllocLocals		EQU		10t
kOpInitLocalString		EQU		11t
kOpLocalInt		EQU		12t
kOpLocalFloat		EQU		13t
kOpLocalDouble		EQU		14t
kOpLocalString		EQU		15t
kOpFieldInt		EQU		16t
kOpFieldFloat		EQU		17t
kOpFieldDouble		EQU		18t
kOpFieldString		EQU		19t
kOpMethodWithThis		EQU		20t
kOpMemberInt		EQU		21t
kOpMemberFloat		EQU		22t
kOpMemberDouble		EQU		23t
kOpMemberString		EQU		24t
kOpLocalUserDefined		EQU		25t
kOpMaxLocalUserDefined		EQU		127t
kOpUserMethods		EQU		128t


ForthCoreState		STRUCT 2t
optypeAction		DWORD		256t DUP (?)
builtinOps		DWORD		?
numBuiltinOps		DWORD		?
userOps		DWORD		?
numUserOps		DWORD		?
maxUserOps		DWORD		?
pEngine		DWORD		?
pThread		DWORD		?
IPtr		DWORD		?
SPtr		DWORD		?
RPtr		DWORD		?
FPtr		DWORD		?
TPtr		DWORD		?
varMode		DWORD		?
state		DWORD		?
error		DWORD		?
STPtr		DWORD		?
SLen		DWORD		?
RTPtr		DWORD		?
RLen		DWORD		?
DP			DWORD		?
DBase		DWORD		?
DLen		DWORD		?
ForthCoreState		ENDS

; End of file core.h
