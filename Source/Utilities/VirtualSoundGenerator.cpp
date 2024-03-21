/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    ToneGenerator

Abstract:

    Implementation of Simple Audio Sample sine wave generator

--*/
#pragma warning (disable : 4127)
#pragma warning (disable : 26165)
#pragma warning (disable : 4101)
#pragma warning (disable : 4067)

#include "definitions.h"
#include "VirtualSoundGenerator.h"

#include "WinPCMs32le.h"

#include "RingBuffer.h"


const double TWO_PI = M_PI * 2;

extern DWORD g_DisableToneGenerator;

//
// Double to long conversion.
//
long ConvertToLong(double Value)
{
    return (long)(Value * _I32_MAX);
};

//
// Double to short conversion.
//
short ConvertToShort(double Value)
{
    return (short)(Value * _I16_MAX);
};

//
// Double to char conversion.
//
unsigned char ConvertToUChar(double Value)
{
    const double F_127_5 = 127.5;
    return (unsigned char)(Value * F_127_5 + F_127_5);
};

//
// Ctor: basic init.
//
VirtualSoundGenerator::VirtualSoundGenerator()
: m_Frequency(0),
  m_ChannelCount(0),
  m_BitsPerSample(0),
  m_SamplesPerSecond(0),
  m_Mute(false),
  m_PartialFrame(NULL),
  m_PartialFrameBytes(0),
  m_FrameSize(0)
{
    // Theta (double) and SampleIncrement (double) are init in the Init() method 
    // after saving the floating point state. 
}

// NJ release memory RingBuff
NTSTATUS VirtualSoundGenerator::UnInit()
{
    NTSTATUS        status = STATUS_SUCCESS;
    DPF(D_TERSE, ("[Debug NJ] UnInit VirtualSoundGenerator =============="));
    if (ReleaseRingBufferInstance()) {
        DPF(D_TERSE, ("[Debug NJ] ReleaseRingBufferInstance ok=============="));
    }
    else {
        DPF(D_TERSE, ("[Debug NJ] ReleaseRingBufferInstance fail=============="));
    }

    return status;
}

//
// Dtor: free resources.
//
VirtualSoundGenerator::~VirtualSoundGenerator()
{
    if (m_PartialFrame)
    {
        ExFreePoolWithTag(m_PartialFrame, SIMPLEAUDIOSAMPLE_POOLTAG);
        m_PartialFrame = NULL;
        m_PartialFrameBytes = 0;
    }
}

// 
// Init a new frame. 
// Note: caller will save and restore the floatingpoint state.
//
#pragma warning(push)
// Caller wraps this routine between KeSaveFloatingPointState/KeRestoreFloatingPointState calls.
#pragma warning(disable: 28110)

VOID VirtualSoundGenerator::InitFrameFromMicBuffer
(
    _Out_writes_bytes_(FrameSize)    BYTE* Frame,
    _In_                             DWORD  FrameSize
)
{
    double sinValue = m_ToneDCOffset + m_ToneAmplitude * sin(m_Theta);

    if (FrameSize != (DWORD)m_ChannelCount * m_BitsPerSample / 8)
    {
        ASSERT(FALSE);
        RtlZeroMemory(Frame, FrameSize);
        return;
    }

    /* Use __analysis_assume to suppress the reports of a false OACR error. */
    for (ULONG i = 0; i < m_ChannelCount; ++i)
    {
        if (m_BitsPerSample == 8)
        {
            __analysis_assume((DWORD)m_ChannelCount == FrameSize);
            unsigned char* dataBuffer = reinterpret_cast<unsigned char*>(Frame);
            dataBuffer[i] = ConvertToUChar(sinValue);
        }
        else if (m_BitsPerSample == 16)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 2 == FrameSize);
            short* dataBuffer = reinterpret_cast<short*>(Frame);
            dataBuffer[i] = ConvertToShort(sinValue);
        }
        else if (m_BitsPerSample == 24)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 3 == FrameSize);
            BYTE* dataBuffer = Frame;
            long val = ConvertToLong(sinValue);
            val = val >> 8;
            RtlCopyMemory(dataBuffer, &val, 3);
        }
        else if (m_BitsPerSample == 32)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 4 == FrameSize);
            long* dataBuffer = reinterpret_cast<long*>(Frame);
            dataBuffer[i] = ConvertToLong(sinValue);
        }
    }

    m_Theta += m_SampleIncrement;
    if (m_Theta >= TWO_PI)
    {
        m_Theta -= TWO_PI;
    }
}

VOID VirtualSoundGenerator::InitNewFrame
(
    _Out_writes_bytes_(FrameSize)    BYTE*  Frame, 
    _In_                             DWORD  FrameSize
)
{
    double sinValue = m_ToneDCOffset + m_ToneAmplitude * sin( m_Theta );

    if (FrameSize != (DWORD)m_ChannelCount * m_BitsPerSample/8)
    {
        ASSERT(FALSE);
        RtlZeroMemory(Frame, FrameSize);
        return;
    }

    /* Use __analysis_assume to suppress the reports of a false OACR error. */
    for(ULONG i = 0; i < m_ChannelCount; ++i)
    {
        if (m_BitsPerSample == 8)
        {
            __analysis_assume((DWORD)m_ChannelCount == FrameSize);
            unsigned char *dataBuffer = reinterpret_cast<unsigned char *>(Frame);
            dataBuffer[i] = ConvertToUChar(sinValue);
        }
        else if (m_BitsPerSample == 16)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 2 == FrameSize);
            short *dataBuffer = reinterpret_cast<short *>(Frame);
            dataBuffer[i] = ConvertToShort(sinValue);
        }
        else if (m_BitsPerSample == 24)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 3 == FrameSize);
            BYTE *dataBuffer = Frame;
            long val = ConvertToLong(sinValue);
            val = val >> 8;
            RtlCopyMemory(dataBuffer, &val, 3);
        }
        else if (m_BitsPerSample == 32)
        {
            __analysis_assume((DWORD)(m_ChannelCount) * 4 == FrameSize);
            long *dataBuffer = reinterpret_cast<long *>(Frame);
            dataBuffer[i] = ConvertToLong(sinValue);
        }
    }

    m_Theta += m_SampleIncrement;
    if (m_Theta >= TWO_PI)
    {
        m_Theta -= TWO_PI;
    }
}
#pragma warning(pop)

//
// GenerateSamples()
//
//  Generate a sine wave that fits into the specified buffer.
//
//  Buffer - Buffer to hold the samples
//  BufferLength - Length of the buffer.
//
//
void VirtualSoundGenerator::GenerateSine
(
    _Out_writes_bytes_(BufferLength) BYTE* Buffer,
    _In_                             size_t      BufferLength,
    _In_                             size_t      TargetBufferStart
)
{
    NTSTATUS        status;
    KFLOATING_SAVE  saveData;
    BYTE* buffer;
    size_t          length;
    size_t          copyBytes;

    size_t SourceBufferStart;

    // if muted, or tone generator disabled via registry,
    // we deliver silence.
    if (m_Mute || g_DisableToneGenerator)
    {
        goto ZeroBuffer;
    }

    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] GenerateSine BufferLength = [%u]", BufferLength));
    DPF(D_TERSE, ("[Debug NJ] GenerateSine SourceBufferStart = [%u]", TargetBufferStart));
    SourceBufferStart = TargetBufferStart % WinPCMs32le_raw_len;
    if (SourceBufferStart + BufferLength >= WinPCMs32le_raw_len) {
        DPF(D_TERSE, ("[Debug NJ] till end of SourceBuffer = [%u]", SourceBufferStart + BufferLength));
        SourceBufferStart = 0;
    }
    else {
        DPF(D_TERSE, ("[Debug NJ] inner of SourceBuffer = [%u]", SourceBufferStart + BufferLength));
        //bug is : SourceBufferStart = TargetBufferStart;
    }


    status = KeSaveFloatingPointState(&saveData);
    if (!NT_SUCCESS(status))
    {
        goto ZeroBuffer;
    }

    buffer = Buffer;
    length = BufferLength;

    //
    // Check if we have any residual frame bytes from the last time.
    //
    if (m_PartialFrameBytes)
    {
        /*
        ASSERT(m_FrameSize > m_PartialFrameBytes);
        DWORD offset = m_FrameSize - m_PartialFrameBytes;
        copyBytes = MIN(m_PartialFrameBytes, length);
        RtlCopyMemory(buffer, m_PartialFrame + offset, copyBytes);
        RtlZeroMemory(m_PartialFrame + offset, copyBytes);
        length -= copyBytes;
        buffer += copyBytes;
        m_PartialFrameBytes = 0;
        */
    }

    IF_TRUE_JUMP(length == 0, Done);

    //
    // Copy all the aligned frames.
    // 
    /*
    DPF(D_TERSE, ("[Debug NJ] GenerateSine m_FrameSize = [%u]", m_FrameSize));
    size_t frames = length / m_FrameSize;
    DPF(D_TERSE, ("[Debug NJ] GenerateSine frames = [%u]", frames));
    for (size_t i = 0; i < frames; ++i)
    {
        //InitNewFrame(buffer, m_FrameSize);
        InitFrameFromMicBuffer(buffer, m_FrameSize);
        buffer += m_FrameSize;
        length -= m_FrameSize;
    }
    */
    IF_TRUE_JUMP(length == 0, Done);

    //
    // Copy any partial frame at the end.
    //
    /*
    ASSERT(m_FrameSize > length);
    InitNewFrame(m_PartialFrame, m_FrameSize);
    RtlCopyMemory(buffer, m_PartialFrame, length);
    RtlZeroMemory(m_PartialFrame, length);
    m_PartialFrameBytes = m_FrameSize - (DWORD)length;
    */
    DPF(D_TERSE, ("[Debug NJ] GenerateSine2 m_BitsPerSample = [%u]", m_BitsPerSample));
    DPF(D_TERSE, ("[Debug NJ] GenerateSine2 frames = [%u]", length));
    if (m_BitsPerSample == 8)
    {
    }
    else if (m_BitsPerSample == 16)
    {
    }
    else if (m_BitsPerSample == 24)
    {
    }
    else if (m_BitsPerSample == 32)
    {
        DPF(D_TERSE, ("[Debug NJ] GenerateSine2 S32le = [%u]", length));            
        RtlCopyMemory(buffer, WinPCMs32le_raw + SourceBufferStart, length);
    }

Done:
    KeRestoreFloatingPointState(&saveData);
    return;

ZeroBuffer:
    RtlZeroMemory(Buffer, BufferLength);
    return;
}

void VirtualSoundGenerator::GenerateVirtualSound(
    _Out_writes_bytes_(BufferLength) BYTE* Buffer,
    _In_                             size_t      BufferLength,
    _In_                             size_t      TargetBufferStart
) {
    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound BufferLength = [%u]", BufferLength));
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound SourceBufferStart = [%u]", TargetBufferStart));
    NTSTATUS        status;
    KFLOATING_SAVE  saveData;
    BYTE* buffer;
    size_t          length;
    size_t          copyBytes;

    size_t SourceBufferStart;

    // if muted, or tone generator disabled via registry,
    // we deliver silence.
    if (m_Mute || g_DisableToneGenerator)
    {
        goto ZeroBuffer;
    }

    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound BufferLength = [%u]", BufferLength));
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound SourceBufferStart = [%u]", TargetBufferStart));
    SourceBufferStart = TargetBufferStart % WinPCMs32le_raw_len;
    if (SourceBufferStart + BufferLength >= WinPCMs32le_raw_len) {
        DPF(D_TERSE, ("[Debug NJ] till end of SourceBuffer = [%u]", SourceBufferStart + BufferLength));
        SourceBufferStart = 0;
    }
    else {
        DPF(D_TERSE, ("[Debug NJ] inner of SourceBuffer = [%u]", SourceBufferStart + BufferLength));
        //bug is : SourceBufferStart = TargetBufferStart;
    }


    status = KeSaveFloatingPointState(&saveData);
    if (!NT_SUCCESS(status))
    {
        goto ZeroBuffer;
    }

    buffer = Buffer;
    length = BufferLength;

    //
    // Check if we have any residual frame bytes from the last time.
    //
    if (m_PartialFrameBytes)
    {
        /*
        ASSERT(m_FrameSize > m_PartialFrameBytes);
        DWORD offset = m_FrameSize - m_PartialFrameBytes;
        copyBytes = MIN(m_PartialFrameBytes, length);
        RtlCopyMemory(buffer, m_PartialFrame + offset, copyBytes);
        RtlZeroMemory(m_PartialFrame + offset, copyBytes);
        length -= copyBytes;
        buffer += copyBytes;
        m_PartialFrameBytes = 0;
        */
    }

    IF_TRUE_JUMP(length == 0, Done);

    //
    // Copy all the aligned frames.
    // 
    /*
    DPF(D_TERSE, ("[Debug NJ] GenerateSine m_FrameSize = [%u]", m_FrameSize));
    size_t frames = length / m_FrameSize;
    DPF(D_TERSE, ("[Debug NJ] GenerateSine frames = [%u]", frames));
    for (size_t i = 0; i < frames; ++i)
    {
        //InitNewFrame(buffer, m_FrameSize);
        InitFrameFromMicBuffer(buffer, m_FrameSize);
        buffer += m_FrameSize;
        length -= m_FrameSize;
    }
    */
    IF_TRUE_JUMP(length == 0, Done);

    //
    // Copy any partial frame at the end.
    //
    /*
    ASSERT(m_FrameSize > length);
    InitNewFrame(m_PartialFrame, m_FrameSize);
    RtlCopyMemory(buffer, m_PartialFrame, length);
    RtlZeroMemory(m_PartialFrame, length);
    m_PartialFrameBytes = m_FrameSize - (DWORD)length;
    */
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound m_BitsPerSample = [%u]", m_BitsPerSample));
    DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound frames = [%u]", length));
    if (m_BitsPerSample == 8)
    {
    }
    else if (m_BitsPerSample == 16)
    {
    }
    else if (m_BitsPerSample == 24)
    {
    }
    else if (m_BitsPerSample == 32)
    {
        DPF(D_TERSE, ("[Debug NJ] GenerateVirtualSound S32le = [%u]", length));
        //RtlCopyMemory(buffer, WinPCMs32le_raw + SourceBufferStart, length);
        //RingBufferDataType* sourceBuff = ringBufferRead(GetRingBufferReadyInstance());
        RingBufferDataType* sourceBuffStart = ringBufferReadStart(GetRingBufferReadyInstance());
        if (sourceBuffStart != NULL) {
            //RtlCopyMemory(buffer, sourceBuffStart + SourceBufferStart, length);
            DPF(D_TERSE, ("[Debug NJ] copy>>>>> GenerateVirtualSound S32le = [%u]", length));
            if (ringBufferReadBytes(GetRingBufferReadyInstance(), (unsigned int)length, (PCHAR)buffer)) {
                DPF(D_TERSE, ("[Debug NJ] ok copy>>>>> ringBufferReadBytes "));
            }
            else {
                DPF(D_TERSE, ("[Debug NJ] fail copy>>>>> ringBufferReadBytes "));
            }
        }
        else {
            DPF(D_TERSE, ("[Debug NJ] fail copy>>>>> GenerateVirtualSound S32le = [%u]", length));
        }

    }

Done:
    KeRestoreFloatingPointState(&saveData);
    return;

ZeroBuffer:
    RtlZeroMemory(Buffer, BufferLength);
    return;

}


//
// GenerateSamples()
//
//  Generate a sine wave that fits into the specified buffer.
//
//  Buffer - Buffer to hold the samples
//  BufferLength - Length of the buffer.
//
//
void VirtualSoundGenerator::GenerateSine2
(
    _Out_writes_bytes_(BufferLength) BYTE       *Buffer, 
    _In_                             size_t      BufferLength
)
{
    NTSTATUS        status;
    KFLOATING_SAVE  saveData;
    BYTE *          buffer;
    size_t          length;
    size_t          copyBytes;

    // if muted, or tone generator disabled via registry,
    // we deliver silence.
    if (m_Mute || g_DisableToneGenerator)
    {
        goto ZeroBuffer;
    }
    
    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] GenerateSine BufferLength = [%u]", BufferLength));

    status = KeSaveFloatingPointState(&saveData);
    if (!NT_SUCCESS(status))
    {
        goto ZeroBuffer;
    }

    buffer = Buffer;
    length = BufferLength;

    //
    // Check if we have any residual frame bytes from the last time.
    //
    if (m_PartialFrameBytes)
    {
        ASSERT(m_FrameSize > m_PartialFrameBytes);
        DWORD offset = m_FrameSize - m_PartialFrameBytes;
        copyBytes = MIN(m_PartialFrameBytes, length);
        RtlCopyMemory(buffer, m_PartialFrame + offset, copyBytes);
        RtlZeroMemory(m_PartialFrame + offset, copyBytes);
        length -= copyBytes;
        buffer += copyBytes;
        m_PartialFrameBytes = 0;
    }
    
    IF_TRUE_JUMP(length == 0, Done);

    //
    // Copy all the aligned frames.
    // 

    size_t frames = length/m_FrameSize;

    for (size_t i = 0; i < frames; ++i)
    {
        InitNewFrame(buffer, m_FrameSize);
        buffer += m_FrameSize;
        length -= m_FrameSize;
    }

    IF_TRUE_JUMP(length == 0, Done);
    
    //
    // Copy any partial frame at the end.
    //
    ASSERT(m_FrameSize > length);
    InitNewFrame(m_PartialFrame, m_FrameSize);
    RtlCopyMemory(buffer, m_PartialFrame, length);
    RtlZeroMemory(m_PartialFrame, length);
    m_PartialFrameBytes = m_FrameSize - (DWORD)length;    
    
Done:
    KeRestoreFloatingPointState(&saveData);
    return;

ZeroBuffer:
    RtlZeroMemory(Buffer, BufferLength);
    return;
}

NTSTATUS VirtualSoundGenerator::Init
(
    _In_    DWORD                   ToneFrequency, 
    _In_    double                  ToneAmplitude,
    _In_    double                  ToneDCOffset,
    _In_    double                  ToneInitialPhase,
    _In_    PWAVEFORMATEXTENSIBLE   WfExt
)
{
    NTSTATUS        status      = STATUS_SUCCESS;
    KFLOATING_SAVE  saveData;
    
    //
    // This sample supports PCM formats only. 
    //
    if ((WfExt->Format.wFormatTag != WAVE_FORMAT_PCM &&
        !(WfExt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
          IsEqualGUIDAligned(WfExt->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    IF_FAILED_JUMP(status, Done);

    //
    // Save floating state (just in case).
    //
    status = KeSaveFloatingPointState(&saveData);
    IF_FAILED_JUMP(status, Done);

    //
    // Basic init.
    //
    m_Theta             = ToneInitialPhase;
    m_Frequency         = ToneFrequency;
    m_ToneAmplitude     = ToneAmplitude;
    m_ToneDCOffset      = ToneDCOffset;

    m_ChannelCount      = WfExt->Format.nChannels;      // # channels.
    m_BitsPerSample     = WfExt->Format.wBitsPerSample; // bits per sample.
    m_SamplesPerSecond  = WfExt->Format.nSamplesPerSec; // samples per sec.
    m_Mute              = false;
    m_SampleIncrement   = (m_Frequency * TWO_PI) / (double)m_SamplesPerSecond;
    m_FrameSize         = (DWORD)m_ChannelCount * m_BitsPerSample/8;
    ASSERT(m_FrameSize == WfExt->Format.nBlockAlign);
    
    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_ChannelCount = [%d]", m_ChannelCount));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_BitsPerSample = [%d]", m_BitsPerSample));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_SamplesPerSecond = [%d]", m_SamplesPerSecond));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_FrameSize = [%d]", m_FrameSize));
    //
    // Restore floating state.
    //
    KeRestoreFloatingPointState(&saveData);

    // 
    // Allocate a buffer to hold a partial frame.
    //
    m_PartialFrame = (BYTE*)ExAllocatePool2(
                                    POOL_FLAG_NON_PAGED,
                                    m_FrameSize,
                                    SIMPLEAUDIOSAMPLE_POOLTAG);

    IF_TRUE_ACTION_JUMP(m_PartialFrame == NULL, status = STATUS_INSUFFICIENT_RESOURCES, Done);
    
    status = STATUS_SUCCESS;

    // NJ allocator for RingBuffer
    /*
    unsigned int ringBufferbufferSize = 256;
    if (!GetRingBufferInstance(ringBufferbufferSize)) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DPF(D_TERSE, ("[Debug NJ] fail GetRingBufferInstance bufferSize= [%d]", ringBufferbufferSize));
    }
    else {
        status = STATUS_SUCCESS;
        RingBufferDataType* sourceBuffStart = ringBufferReadStart(GetRingBufferReadyInstance());
        if (sourceBuffStart != NULL) {        
            RtlCopyBytes(sourceBuffStart, WinPCMs32le_raw, WinPCMs32le_raw_len);
            DPF(D_TERSE, ("[Debug NJ] >>>>>>>>copy  WinPCMs32le_raw_len= [%d]", WinPCMs32le_raw_len));
        }
        DPF(D_TERSE, ("[Debug NJ] temp GetRingBufferInstance bufferSize= [%d]", ringBufferbufferSize));
    }*/

Done:
    return status;
}

NTSTATUS VirtualSoundGenerator::Init2
(
    _In_    DWORD                   ToneFrequency,
    _In_    double                  ToneAmplitude,
    _In_    double                  ToneDCOffset,
    _In_    double                  ToneInitialPhase,
    _In_    PWAVEFORMATEXTENSIBLE   WfExt
)
{
    NTSTATUS        status = STATUS_SUCCESS;
    KFLOATING_SAVE  saveData;

    //
    // This sample supports PCM formats only. 
    //
    if ((WfExt->Format.wFormatTag != WAVE_FORMAT_PCM &&
        !(WfExt->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
            IsEqualGUIDAligned(WfExt->SubFormat, KSDATAFORMAT_SUBTYPE_PCM))))
    {
        status = STATUS_NOT_SUPPORTED;
    }
    IF_FAILED_JUMP(status, Done);

    //
    // Save floating state (just in case).
    //
    status = KeSaveFloatingPointState(&saveData);
    IF_FAILED_JUMP(status, Done);

    //
    // Basic init.
    //
    m_Theta = ToneInitialPhase;
    m_Frequency = ToneFrequency;
    m_ToneAmplitude = ToneAmplitude;
    m_ToneDCOffset = ToneDCOffset;

    m_ChannelCount = WfExt->Format.nChannels;      // # channels.
    m_BitsPerSample = WfExt->Format.wBitsPerSample; // bits per sample.
    m_SamplesPerSecond = WfExt->Format.nSamplesPerSec; // samples per sec.
    m_Mute = false;
    m_SampleIncrement = (m_Frequency * TWO_PI) / (double)m_SamplesPerSecond;
    m_FrameSize = (DWORD)m_ChannelCount * m_BitsPerSample / 8;
    ASSERT(m_FrameSize == WfExt->Format.nBlockAlign);

    DPF(D_TERSE, ("[Debug NJ] ================================================="));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_ChannelCount = [%d]", m_ChannelCount));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_BitsPerSample = [%d]", m_BitsPerSample));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_SamplesPerSecond = [%d]", m_SamplesPerSecond));
    DPF(D_TERSE, ("[Debug NJ] ToneGenerator::Init m_FrameSize = [%d]", m_FrameSize));
    //
    // Restore floating state.
    //
    KeRestoreFloatingPointState(&saveData);

    // 
    // Allocate a buffer to hold a partial frame.
    //
    m_PartialFrame = (BYTE*)ExAllocatePool2(
        POOL_FLAG_NON_PAGED,
        m_FrameSize,
        SIMPLEAUDIOSAMPLE_POOLTAG);

    IF_TRUE_ACTION_JUMP(m_PartialFrame == NULL, status = STATUS_INSUFFICIENT_RESOURCES, Done);

    // NJ allocator for RingBuffer
    status = STATUS_SUCCESS;

Done:
    return status;
}

