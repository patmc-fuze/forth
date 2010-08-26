//////////////////////////////////////////////////////////////////////
//
// ForthMidiExtension.cpp: support for midi devices
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ForthEngine.h"
#include "ForthMidiExtension.h"
#include <vector>

//////////////////////////////////////////////////////////////////////
////
///     midi device support ops
//
//

extern "C"
{

FORTHOP( enableMidiOp )
{
    ForthMidiExtension::GetInstance()->Enable( true );
}

FORTHOP( disableMidiOp )
{
    ForthMidiExtension::GetInstance()->Enable( false );
}

FORTHOP( midiInGetNumDevsOp )
{
    long count = ForthMidiExtension::GetInstance()->InputDeviceCount();
    SPUSH( count );
}

FORTHOP( midiOutGetNumDevsOp )
{
    long count = ForthMidiExtension::GetInstance()->OutputDeviceCount();
    SPUSH( count );
}

FORTHOP( midiInOpenOp )
{
    long deviceId = SPOP;
    long cbData = SPOP;
    long cbOp = SPOP;
    long result = ForthMidiExtension::GetInstance()->OpenInput( cbOp, cbData, deviceId );
    SPUSH( result );
}

FORTHOP( midiInCloseOp )
{
    long deviceId = SPOP;
    long result = ForthMidiExtension::GetInstance()->CloseInput( deviceId );
    SPUSH( result );
}

FORTHOP( midiOutOpenOp )
{
    long deviceId = SPOP;
    long cbData = SPOP;
    long cbOp = SPOP;
    long result = ForthMidiExtension::GetInstance()->OpenOutput( cbOp, cbData, deviceId );
    SPUSH( result );
}

FORTHOP( midiOutCloseOp )
{
    long deviceId = SPOP;
    long result = ForthMidiExtension::GetInstance()->CloseOutput( deviceId );
    SPUSH( result );
}

FORTHOP( midiInGetDeviceNameOp )
{
    long deviceNum = SPOP;
    MIDIINCAPS* pCaps = ForthMidiExtension::GetInstance()->GetInputDeviceCapabilities( (UINT_PTR) deviceNum );
    long result = (pCaps == NULL) ? 0 : (long) (&pCaps->szPname);
    SPUSH( result );
}

FORTHOP( midiOutGetDeviceNameOp )
{
    long deviceNum = SPOP;
    MIDIOUTCAPS* pCaps = ForthMidiExtension::GetInstance()->GetOutputDeviceCapabilities( (UINT_PTR) deviceNum );
    long result = (pCaps == NULL) ? 0 : (long) (&pCaps->szPname);
    SPUSH( result );
}


baseDictionaryEntry midiDictionary[] =
{
    ///////////////////////////////////////////
    //  midi
    ///////////////////////////////////////////
    OP_DEF(    enableMidiOp,               "enableMidi" ),
    OP_DEF(    disableMidiOp,               "disableMidi" ),

    OP_DEF(    midiInGetNumDevsOp,                "midiInGetNumDevices" ),
    OP_DEF(    midiOutGetNumDevsOp,               "midiOutGetNumDevices" ),
    OP_DEF(    midiInGetDeviceNameOp,                "midiInGetDeviceName" ),
    OP_DEF(    midiOutGetDeviceNameOp,               "midiOutGetDeviceName" ),
    OP_DEF(    midiInOpenOp,               "midiInOpen" ),
    OP_DEF(    midiInOpenOp,               "midiInClose" ),
    OP_DEF(    midiOutOpenOp,               "midiOutOpen" ),
    OP_DEF(    midiOutOpenOp,               "midiOutClose" ),
    // following must be last in table
    OP_DEF(    NULL,                   "" )
};

};  // end extern "C"

//////////////////////////////////////////////////////////////////////
////
///     ForthMidiExtension
//
//

ForthMidiExtension *ForthMidiExtension::mpInstance = NULL;

ForthMidiExtension::ForthMidiExtension()
:   mbEnabled( false )
{
    mpThread = new ForthThread( ForthEngine::GetInstance() );
}


ForthMidiExtension::~ForthMidiExtension()
{
    delete mpThread;
}


void ForthMidiExtension::Initialize( ForthEngine* pEngine )
{
    ForthExtension::Initialize( pEngine );
}


void ForthMidiExtension::Reset()
{
    mInputDevices.resize( 0 );
    mOutputDevices.resize( 0 );
    mpThread->Reset();
    mbEnabled = false;
}


void ForthMidiExtension::Shutdown()
{
}


void ForthMidiExtension::ForgetOp( ulong opNumber )
{
#if 0
    for ( std::vector<ForthMidiExtension::InDeviceInfo>::iterator iter = mInputDevices.begin();
          iter != mInputDevices.end(); iter++ )
    {
        *iter.mCbOp = 0;
    }
    for ( std::vector<ForthMidiExtension::OutDeviceInfo>::iterator iter = mOutputDevices.begin();
          iter != mOutputDevices.end(); iter++ )
    {
        *iter.mCbOp = 0;
    }
#else
    for ( ulong i = 0; i < mInputDevices.size(); i++ )
    {
        if ( mInputDevices[i].mCbOp >= opNumber )
        {
            mInputDevices[i].mCbOp = 0;
        }
    }
    for ( ulong i = 0; i < mOutputDevices.size(); i++ )
    {
        if ( mOutputDevices[i].mCbOp >= opNumber )
        {
            mOutputDevices[i].mCbOp = 0;
        }
    }
#endif
}


ForthMidiExtension* ForthMidiExtension::GetInstance( void )
{
    if ( mpInstance == NULL )
    {
        mpInstance = new ForthMidiExtension;
    }
    return mpInstance;
}


long ForthMidiExtension::OpenInput( long cbOp, long cbOpData, long deviceId )
{
    if ( (size_t) deviceId >= mInputDevices.size() )
    {
        mInputDevices.resize( deviceId + 1 );
    }
    InDeviceInfo* midiIn = &(mInputDevices[deviceId]);
    midiIn->mCbOp = cbOp;
    midiIn->mCbOpData = (void *) cbOpData;
    midiIn->mDeviceId = (UINT_PTR) deviceId;
    MMRESULT result = midiInOpen( &(midiIn->mHandle), (UINT_PTR) deviceId,
                                  (DWORD_PTR) MidiInCallback, (DWORD_PTR) deviceId, (CALLBACK_FUNCTION | MIDI_IO_STATUS) );
    return (long) result;
}


long ForthMidiExtension::OpenOutput( long cbOp, long cbOpData, long deviceId )
{
    if ( (size_t) deviceId >= mOutputDevices.size() )
    {
        mOutputDevices.resize( deviceId + 1 );
    }
    OutDeviceInfo* midiOut = &(mOutputDevices[deviceId]);
    midiOut->mCbOp = cbOp;
    midiOut->mCbOpData = (void *) cbOpData;
    midiOut->mDeviceId = (UINT_PTR) deviceId;
    MMRESULT result = midiOutOpen( &(midiOut->mHandle), (UINT_PTR) deviceId,
                                   (DWORD_PTR) MidiOutCallback, (DWORD_PTR) deviceId, CALLBACK_FUNCTION );
    return (long) result;
}


long ForthMidiExtension::CloseInput( long deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( ((size_t) deviceId < mInputDevices.size()) && (mInputDevices[deviceId].mCbOp != 0) )
    {
        result = midiInClose( mInputDevices[deviceId].mHandle );
        mInputDevices[deviceId].mCbOp = 0;
    }
    return (result == MMSYSERR_NOERROR) ? 1 : 0;
}

long ForthMidiExtension::CloseOutput( long deviceId )
{
    MMRESULT result = MMSYSERR_NOERROR;
    if ( ((size_t) deviceId < mOutputDevices.size()) && (mOutputDevices[deviceId].mCbOp != 0) )
    {
        result = midiOutClose( mOutputDevices[deviceId].mHandle );
        mOutputDevices[deviceId].mCbOp = 0;
    }
    return (result == MMSYSERR_NOERROR) ? 1 : 0;
}

void ForthMidiExtension::MidiOutCallback( HMIDIOUT hMidiOut, UINT wMsg, DWORD_PTR dwInstance,
                                 DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    ForthMidiExtension* pThis = ForthMidiExtension::GetInstance();
    pThis->HandleMidiOut( wMsg, dwInstance, dwParam1, dwParam2 );
}


void ForthMidiExtension::MidiInCallback( HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance,
                                DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    ForthMidiExtension* pThis = ForthMidiExtension::GetInstance();
    pThis->HandleMidiIn( wMsg, dwInstance, dwParam1, dwParam2 );
}

void ForthMidiExtension::HandleMidiIn( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    TRACE( "ForthMidiExtension::HandleMidiIn   msg %x   cbData %x   p1 %x   p2 %x\n",
            wMsg, dwInstance, dwParam1, dwParam2 );
    if ( mbEnabled && (dwInstance < mInputDevices.size()) && (mInputDevices[dwInstance].mCbOp != 0) )
    {
        mpThread->Push( wMsg );
        mpThread->Push( (long) mInputDevices[dwInstance].mCbOpData );
        mpThread->Push( dwParam1 );
        mpThread->Push( dwParam2 );
        ForthEngine::GetInstance()->ExecuteOneOp( mInputDevices[dwInstance].mCbOp );
    }
    else
    {
        // TBD: Error
        TRACE( "ForthMidiExtension::HandleMidiIn enable %d   nInputDevices %d   cbOp  %x\n",
               mbEnabled, mInputDevices.size(), mInputDevices[dwInstance].mCbOp );
    }
}

void ForthMidiExtension::HandleMidiOut( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    if ( mbEnabled && (dwInstance < mOutputDevices.size()) && (mOutputDevices[dwInstance].mCbOp != 0) )
    {
        mpThread->Push( wMsg );
        mpThread->Push( (long) mOutputDevices[dwInstance].mCbOpData );
        mpThread->Push( dwParam1 );
        mpThread->Push( dwParam2 );
        ForthEngine::GetInstance()->ExecuteOneOp( mOutputDevices[dwInstance].mCbOp );
    }
    else
    {
        // TBD: Error
    }
}

void ForthMidiExtension::Enable( bool enable )
{
    mbEnabled = enable;
}


long ForthMidiExtension::InputDeviceCount()
{
    return (long) midiInGetNumDevs();
}


long ForthMidiExtension::OutputDeviceCount()
{
    return (long) midiOutGetNumDevs();
}

MIDIOUTCAPS* ForthMidiExtension::GetOutputDeviceCapabilities( UINT_PTR deviceId )
{
    if ( deviceId >= mOutputDevices.size() )
    {
        mOutputDevices.resize( deviceId + 1 );
    }
    MMRESULT result = midiOutGetDevCaps( deviceId, &(mOutputDevices[deviceId].mCaps), sizeof(MIDIOUTCAPS) );
    return (result == MMSYSERR_NOERROR) ? &(mOutputDevices[deviceId].mCaps) : NULL;
}

MIDIINCAPS* ForthMidiExtension::GetInputDeviceCapabilities( UINT_PTR deviceId )
{
    if ( deviceId >= mInputDevices.size() )
    {
        mInputDevices.resize( deviceId + 1 );
    }
    MMRESULT result = midiInGetDevCaps( deviceId, &(mInputDevices[deviceId].mCaps), sizeof(MIDIINCAPS) );
    return (result == MMSYSERR_NOERROR) ? &(mInputDevices[deviceId].mCaps) : NULL;
}


#if 0
void ForthMidiExtension::
{
}


void ForthMidiExtension::
{
}


void ForthMidiExtension::
{
}
#endif


//////////////////////////////////////////////////////////////////////
////
///     InDeviceInfo
//
//

ForthMidiExtension::InDeviceInfo::InDeviceInfo()
:   mCbOp( 0 )
,   mCbOpData( NULL )
,   mHandle( (HMIDIIN) ~0 )
,   mDeviceId( (UINT_PTR) ~0 )
{
}

//////////////////////////////////////////////////////////////////////
////
///     OutDeviceInfo
//
//

ForthMidiExtension::OutDeviceInfo::OutDeviceInfo()
:   mCbOp( 0 )
,   mCbOpData( NULL )
,   mHandle( (HMIDIOUT) ~0 )
,   mDeviceId( (UINT_PTR) ~0 )
{
}



