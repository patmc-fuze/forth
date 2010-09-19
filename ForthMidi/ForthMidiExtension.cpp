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
    OP_DEF(    enableMidiOp,                    "enableMidi" ),
    OP_DEF(    disableMidiOp,                   "disableMidi" ),
    OP_DEF(    midiInGetNumDevsOp,              "midiInGetNumDevices" ),
    OP_DEF(    midiOutGetNumDevsOp,             "midiOutGetNumDevices" ),
    OP_DEF(    midiInGetDeviceNameOp,           "midiInGetDeviceName" ),
    OP_DEF(    midiOutGetDeviceNameOp,          "midiOutGetDeviceName" ),
    OP_DEF(    midiInOpenOp,                    "midiInOpen" ),
    OP_DEF(    midiInCloseOp,                   "midiInClose" ),
    OP_DEF(    midiOutOpenOp,                   "midiOutOpen" ),
    OP_DEF(    midiOutCloseOp,                  "midiOutClose" ),
    // following must be last in table
    OP_DEF(    NULL,                        "" )
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
,   mpThread( NULL )
{
    mpInstance = this;
}


ForthMidiExtension::~ForthMidiExtension()
{
}


void ForthMidiExtension::Initialize( ForthEngine* pEngine )
{
    if ( mpThread != NULL )
    {
        delete mpThread;
    }
    mpThread = new ForthThread( ForthEngine::GetInstance() );
    ForthExtension::Initialize( pEngine );
    pEngine->AddBuiltinOps( midiDictionary );
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
    mbEnabled = false;
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
    TRACE( "Start OpenInput\n" );
    if ( (size_t) deviceId >= mInputDevices.size() )
    {
        mInputDevices.resize( deviceId + 1 );
    }
    InDeviceInfo* midiIn = &(mInputDevices[deviceId]);
    midiIn->mCbOp = cbOp;
    midiIn->mCbOpData = (void *) cbOpData;
    midiIn->mDeviceId = (UINT_PTR) deviceId;
#if 1
    MMRESULT result = midiInOpen( &(midiIn->mHandle), (UINT_PTR) deviceId,
                                  (DWORD_PTR) MidiInCallback, (DWORD_PTR) deviceId, (CALLBACK_FUNCTION | MIDI_IO_STATUS) );
#else
    long result = 55;
    MidiInCallback( midiIn->mHandle, MIM_OPEN, (DWORD_PTR) deviceId, 11, 22 );
#endif
    TRACE( "End OpenInput\n" );
    return (long) result;
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
    switch ( wMsg )
    {
    case MIM_DATA:      // Short message received
        TRACE( "Data\n" );
        break;

    case MIM_ERROR:     // Invalid short message received
        TRACE( "Error\n" );
        break;

    case MIM_LONGDATA:  // System exclusive message received
        TRACE( "LongData\n" );
        break;

    case MIM_LONGERROR: // Invalid system exclusive message received
        TRACE( "LongError\n" );
        break;

    case MIM_OPEN:
        TRACE( "Input Open\n" );
        break;

    case MIM_CLOSE:
        TRACE( "Input Close\n" );
        break;

    case MOM_OPEN:
        TRACE( "Output Open\n" );
        break;

    case MOM_CLOSE:
        TRACE( "Output Close\n" );
        break;

    case MOM_DONE:
        TRACE( "Output Done\n" );
        break;
    }
#if 0
    if ( mbEnabled && (dwInstance < mInputDevices.size()) )
    {
        InDeviceInfo* midiIn = &(mInputDevices[dwInstance]);
        if ( midiIn->mCbOp != 0 )
        {
            mpThread->Push( wMsg );
            mpThread->Push( (long) (midiIn->mCbOpData) );
            mpThread->Push( dwParam1 );
            mpThread->Push( dwParam2 );
            ForthEngine::GetInstance()->ExecuteOneOp( midiIn->mCbOp, mpThread );
        }
        else
        {
            TRACE( "No registered op for channel %d\n", dwInstance );
        }
    }
    else
    {
        // TBD: Error
        long cbOp = -1;
        if ( dwInstance < mInputDevices.size() )
        {
            cbOp = mInputDevices[dwInstance].mCbOp;
        }
        TRACE( "ForthMidiExtension::HandleMidiIn enable %d   nInputDevices %d   cbOp  %x\n",
               mbEnabled, mInputDevices.size(), cbOp );
    }
#endif
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


void ForthMidiExtension::HandleMidiOut( UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 )
{
    if ( mbEnabled && (dwInstance < mOutputDevices.size()) && (mOutputDevices[dwInstance].mCbOp != 0) )
    {
        mpThread->Push( wMsg );
        mpThread->Push( (long) mOutputDevices[dwInstance].mCbOpData );
        mpThread->Push( dwParam1 );
        mpThread->Push( dwParam2 );
        ForthEngine::GetInstance()->ExecuteOneOp( mOutputDevices[dwInstance].mCbOp, mpThread );
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



