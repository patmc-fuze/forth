#pragma once

#include <MMSystem.h>

//////////////////////////////////////////////////////////////////////
//
// ForthMidiExtension.h: interface for the ForthMidiExtension class.
//
//////////////////////////////////////////////////////////////////////

#include <vector>

#include "Forth.h"
#include "ForthEngine.h"
#include "ForthExtension.h"

class ForthThread;

class ForthMidiExtension : public ForthExtension
{
public:
    ForthMidiExtension();
    virtual ~ForthMidiExtension();

    virtual void Initialize( ForthEngine* pEngine );
    virtual void Reset();
    virtual void Shutdown();
    virtual void ForgetOp( ulong opNumber );

    static ForthMidiExtension*     GetInstance( void );

    long OpenInput( long cbOp, long cbOpData, long deviceId );
    long OpenOutput( long cbOp, long cbOpData, long deviceId );
    long CloseInput( long deviceId );
    long CloseOutput( long deviceId );

    void Enable( bool enable );

    long InputDeviceCount();
    long OutputDeviceCount();

    MIDIOUTCAPS* GetOutputDeviceCapabilities( UINT_PTR deviceId );
    MIDIINCAPS* GetInputDeviceCapabilities( UINT_PTR deviceId );

protected:
    static ForthMidiExtension* mpInstance;

    static void MidiInCallback( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
                                DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
    static void MidiOutCallback( HMIDIOUT hMidiOut, UINT wMsg, DWORD_PTR dwInstance,
                                 DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    void HandleMidiIn( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
    void HandleMidiOut( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );

    class InDeviceInfo
    {
    public:
        InDeviceInfo();

        ulong       mCbOp;
        void*       mCbOpData;
        HMIDIIN     mHandle;
        UINT_PTR    mDeviceId;
        MIDIINCAPS  mCaps;
    };

    class OutDeviceInfo
    {
    public:
        OutDeviceInfo();

        ulong       mCbOp;
        void*       mCbOpData;
        HMIDIOUT    mHandle;
        UINT_PTR    mDeviceId;
        MIDIOUTCAPS mCaps;
    };

    std::vector<InDeviceInfo>    mInputDevices;
    std::vector<OutDeviceInfo>   mOutputDevices;

    ForthThread*            mpThread;
    bool                    mbEnabled;
};

