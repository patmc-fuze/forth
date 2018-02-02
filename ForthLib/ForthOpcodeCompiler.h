#pragma once
//////////////////////////////////////////////////////////////////////
//
// ForthOpcodeCompiler.h: interface for the ForthOpcodeCompiler class.
//
//////////////////////////////////////////////////////////////////////


class ForthOpcodeCompiler
{
public:
                    ForthOpcodeCompiler( ForthMemorySection*	mpDictionarySection );
			        ~ForthOpcodeCompiler();
	void			Reset();
	void			CompileOpcode( forthOpType opType, long opVal );
    void			PatchOpcode(forthOpType opType, long opVal, long* pOpcode);
    void			UncompileLastOpcode();
	unsigned int	PeepholeValidCount();
	void			ClearPeephole();
    long*           GetLastCompiledOpcodePtr();
    long*           GetLastCompiledIntoPtr();
	bool			GetPreviousOpcode( forthOpType& opType, long& opVal, int index = 0 );
// MAX_PEEPHOLE_PTRS must be power of 2
#define MAX_PEEPHOLE_PTRS	8
#define PEEPHOLE_PTR_MASK   (MAX_PEEPHOLE_PTRS - 1)
private:
	ForthMemorySection*	mpDictionarySection;
	long*			mPeephole[MAX_PEEPHOLE_PTRS];
	unsigned int	mPeepholeIndex;
	unsigned int	mPeepholeValidCount;
	long*			mpLastIntoOpcode;
    long            mCompileComboOpFlags;
};

