#pragma once

#include <stdio.h>
#include <stdlib.h>
//#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "definitions.h"
#include "RingBuffer.h"


//typedef int RingBufferDataType;                                 // set container data type 

RingBuffer* getRingBufferInstance(unsigned int bufferSize);   // get initialized byteBuffer, provide buffer capacity


static void advanceBufferPointer(RingBuffer *ringBuffer);
static void retreatBufferPointer(RingBuffer *ringBuffer);

static void advanceBytesBufferPointer(RingBuffer_t* ringBuffer, unsigned int writeBufferSize);
static void retreatBytesBufferPointer(RingBuffer* ringBuffer, unsigned int readBufferSize);

//NJ
#pragma warning (disable : 4505)
RingBuffer* GetRingBufferReadyInstance() {
    if (GlobalRingBuffer != NULL) {
        DbgPrint("[Debug 2024] >>>>>> GetRingBufferReadyInstance ");
        return GlobalRingBuffer;
    }
    return NULL;
}
bool GetRingBufferInstance(unsigned int bufferSize) {
    GlobalRingBuffer = NULL;
    GlobalRingBuffer = getRingBufferInstance(bufferSize);
    if (GlobalRingBuffer!=NULL) {
        return true;
    }
    return false;
}

#define NJ_VSFRAMESIZE 4096
#define NJRINGBUFF_POOLTAG 'BRJN'
RingBuffer* getRingBufferInstance(unsigned int bufferSize) {
    if (bufferSize < 1) return NULL;

    //RingBuffer_t* instance = (RingBuffer_t*)malloc(sizeof(struct RingBuffer));
    RingBuffer_t* instance = (RingBuffer_t*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(struct RingBuffer), NJRINGBUFF_POOLTAG);
    if (instance != NULL) {
        //RingBufferDataType* buffer = (RingBufferDataType*)malloc(sizeof(RingBufferDataType) * bufferSize);
        RingBufferDataType* buffer = (RingBufferDataType*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(RingBufferDataType) * NJ_VSFRAMESIZE * bufferSize, NJRINGBUFF_POOLTAG);
        if (buffer == NULL) {
            ringBufferDelete(instance);
            return NULL;
        }
        instance->dataBuffer = buffer;
        instance->maxSize = NJ_VSFRAMESIZE * bufferSize;
        memset(instance->dataBuffer, 0, instance->maxSize);
        ringBufferReset(instance);
    }
    return instance;
}

void ringBufferReset(RingBuffer_t *ringBuffer) {
    if (ringBuffer != NULL) {
        ringBuffer->head = 0;
        ringBuffer->tail = 0;
        ringBuffer->isFull = false;
    }
}

//NJ
bool ReleaseRingBufferInstance() {
    if (GlobalRingBuffer != NULL) {
        ringBufferDelete(GlobalRingBuffer);
        GlobalRingBuffer = NULL;
        if (GlobalRingBuffer == NULL) {
            return true;
        }
    }
    return false;
}
void ringBufferDelete(RingBuffer_t *ringBuffer) {
    if (ringBuffer != NULL) {
        //free(ringBuffer->dataBuffer);
        if (ringBuffer->dataBuffer)
        {
            ExFreePoolWithTag(ringBuffer->dataBuffer, NJRINGBUFF_POOLTAG);
            ringBuffer->dataBuffer = NULL;
            DbgPrint("[Debug 2024] >>>>>> ringBufferDelete ringBuffer->dataBuffer");
        }
        //free(ringBuffer);
        if (ringBuffer)
        {
            ExFreePoolWithTag(ringBuffer, NJRINGBUFF_POOLTAG);
            ringBuffer = NULL;
            DbgPrint("[Debug 2024] >>>>>> ringBufferDelete ringBuffer->dataBuffer");
        }
    }
}

bool isRingBufferFull(RingBuffer_t *ringBuffer) {
    return (ringBuffer != NULL) ? ringBuffer->isFull : true;
}

bool isRingBufferNotFull(RingBuffer_t *ringBuffer) {
    return !isRingBufferFull(ringBuffer);
}

bool isRingBufferEmpty(RingBuffer_t *ringBuffer) {
    if (ringBuffer != NULL) {
        return !ringBuffer->isFull && (ringBuffer->head == ringBuffer->tail);
    }
    return true;
}

bool isRingBufferNotEmpty(RingBuffer_t*ringBuffer) {
    return !isRingBufferEmpty(ringBuffer);
}

unsigned int getRingBufferSize(RingBuffer_t *ringBuffer) {
    if (ringBuffer != NULL) {
        unsigned int bufferSize = ringBuffer->maxSize;

        if (!ringBuffer->isFull) {
            if (ringBuffer->head >= ringBuffer->tail) {
                bufferSize = (ringBuffer->head) - (ringBuffer->tail);
            }
            else {
                bufferSize = (ringBuffer->maxSize + ringBuffer->head - ringBuffer->tail);
            }
        }
        return bufferSize;
    }
    return 0;
}

bool ringBufferWriteBytes(RingBuffer_t* ringBuffer, RingBufferDataType* value, unsigned int writeBufferSize) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        //ringBuffer->dataBuffer[ringBuffer->head] = value;
        //advanceBufferPointer(ringBuffer);
        if (ringBuffer->maxSize-(getRingBufferSize(ringBuffer))>= writeBufferSize){
            DbgPrint("[Debug 2024] ringBufferWriteBytes(WW) ringBuffer->head [%d] >>> ringBuffer->tail [%d]>>>>>> ringBuffer->maxSize [%d]", ringBuffer->head, ringBuffer->tail, ringBuffer->maxSize);
            //ringBuffer->dataBuffer[ringBuffer->head] = value;
            if (ringBuffer->head >= ringBuffer->tail) {
                DbgPrint("[Debug 2024] ringBufferWriteBytes ringBuffer->head >>> ringBuffer->tail >>>>>>");
                if((ringBuffer->maxSize - ringBuffer->head) >= writeBufferSize) {
                    DbgPrint("[Debug 2024] ringBufferWriteBytes buff enough for  >>>>>>");
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->head]);
                    RtlCopyMemory((PCHAR)data, (PCHAR)value, writeBufferSize);
                }else{
                    DbgPrint("[Debug 2024] ringBufferWriteBytes buff not enough for  ");
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->head]);
                    RtlCopyMemory((PCHAR)data, (PCHAR)value, (ringBuffer->maxSize - ringBuffer->head));
                    RtlCopyMemory((PCHAR)ringBuffer->dataBuffer, (PCHAR)value + (ringBuffer->maxSize - ringBuffer->head), writeBufferSize - (ringBuffer->maxSize - ringBuffer->head));
                }
                //
                //advanceBufferPointer(ringBuffer);
                advanceBytesBufferPointer(ringBuffer, writeBufferSize);
            //}else if (ringBuffer->head == ringBuffer->tail) {
            //    DbgPrint("[Debug 2024] ringBufferWriteBytes ringBuffer->head == ringBuffer->tail <<<<<<< impossible...........");
            }else {
                DbgPrint("[Debug 2024] ringBufferWriteBytes ringBuffer->head not> ringBuffer->tail <<<<<<<");
                RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->head]);
                RtlCopyMemory((PCHAR)data, (PCHAR)value, writeBufferSize);
                //
                //advanceBufferPointer(ringBuffer);
                advanceBytesBufferPointer(ringBuffer, writeBufferSize);
            }

            DbgPrint("[Debug 2024] ringBufferWriteBytes(W) ringBuffer->head [%d] >>> ringBuffer->tail [%d]>>>>>>", ringBuffer->head, ringBuffer->tail);
            return true;
        }else{
            DbgPrint("[Debug 2024] ringBufferWriteBytes writeBufferSize [%d] not enough>>>>>>", writeBufferSize);
        }
        return false;
    }
    return false;
}
void ringBufferWrite(RingBuffer_t *ringBuffer, PCHAR value) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        //ringBuffer->dataBuffer[ringBuffer->head] = value;
        RtlCopyBytes(ringBuffer->dataBuffer + ((ringBuffer->head) * NJ_VSFRAMESIZE),(PCHAR)value, NJ_VSFRAMESIZE);
        advanceBufferPointer(ringBuffer);
    }
}
void ringBufferAdd(RingBuffer_t* ringBuffer, RingBufferDataType value) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        ringBuffer->dataBuffer[ringBuffer->head] = value;
        advanceBufferPointer(ringBuffer);
    }
}

RingBufferDataType* ringBufferReadStart(RingBuffer_t* ringBuffer) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        DbgPrint("[Debug 2024] ringBufferReadStart ringBuffer->dataBuffer >>>>>>");
        RingBufferDataType* data = ringBuffer->dataBuffer;
        return data;
    }
    return NULL;
}

bool ringBufferReadBytes(RingBuffer_t* ringBuffer, unsigned int readBufferSize,PCHAR toBufferPointer) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        //if (isRingBufferNotEmpty(ringBuffer)) 
        DbgPrint("[Debug 2024] ringBufferReadBytes(R) ringBuffer->head [%d] >>> ringBuffer->tail [%d]>>>>>> ringBuffer->maxSize [%d]", ringBuffer->head, ringBuffer->tail, ringBuffer->maxSize);
        if (getRingBufferSize(ringBuffer) >= readBufferSize) {
            //RingBufferDataType data = ringBuffer->dataBuffer[ringBuffer->tail];
            if (ringBuffer->head > ringBuffer->tail) {
                DbgPrint("[Debug 2024] ringBufferReadBytes readBufferSize [%d] head >> tail >>>>>>", readBufferSize);
                if ((ringBuffer->head - ringBuffer->tail) >= readBufferSize) {
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->tail]);
                    RtlCopyMemory(toBufferPointer, (PCHAR)data, readBufferSize);
                }else{
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->tail]);
                    //copy first part
                    RtlCopyMemory(toBufferPointer, (PCHAR)data, ringBuffer->head - ringBuffer->tail);
                }
            }
            else {
                DbgPrint("[Debug 2024] ringBufferReadBytes readBufferSize [%d] head << tail <<<<<<<", readBufferSize);
                if ((ringBuffer->maxSize - ringBuffer->tail) >= readBufferSize) {
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->tail]);
                    RtlCopyMemory(toBufferPointer, (PCHAR)data, readBufferSize);
                }
                else {
                    RingBufferDataType* data = &(ringBuffer->dataBuffer[ringBuffer->tail]);
                    //copy first part
                    RtlCopyMemory(toBufferPointer, (PCHAR)data, ringBuffer->maxSize - ringBuffer->tail);
                    //copy second part
                    RtlCopyMemory(toBufferPointer + (ringBuffer->maxSize - ringBuffer->tail), (PCHAR)ringBuffer->dataBuffer, (readBufferSize - (ringBuffer->maxSize - ringBuffer->tail)));
                }
            }
            //retreatBufferPointer(ringBuffer);
            retreatBytesBufferPointer(ringBuffer, readBufferSize);
            return true;
        }else{
            DbgPrint("[Debug 2024] ringBufferReadBytes readBufferSize [%d] not enough>>>>>>", readBufferSize);
        }
        return false;
    }
    return false;
}
RingBufferDataType* ringBufferRead(RingBuffer_t *ringBuffer) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        if (isRingBufferNotEmpty(ringBuffer)) {
            DbgPrint("ringBufferRead ringBuffer->tail :=[%d]", ringBuffer->tail);
            //RingBufferDataType data = ringBuffer->dataBuffer[ringBuffer->tail];
            RingBufferDataType* data = ringBuffer->dataBuffer + ((ringBuffer->tail) * NJ_VSFRAMESIZE);
            retreatBufferPointer(ringBuffer);
            return data;
        }
        return NULL;
    }
    return NULL;
}
RingBufferDataType ringBufferGet(RingBuffer_t* ringBuffer) {
    if (ringBuffer != NULL && ringBuffer->dataBuffer != NULL) {
        if (isRingBufferNotEmpty(ringBuffer)) {
            RingBufferDataType data = ringBuffer->dataBuffer[ringBuffer->tail];
            retreatBufferPointer(ringBuffer);
            return data;
        }
        return (RingBufferDataType)0;
    }
    return (RingBufferDataType)0;
}

static void advanceBufferPointer(RingBuffer_t *ringBuffer) {
    if (ringBuffer->isFull) {
        ringBuffer->tail = (ringBuffer->tail + 1) % ringBuffer->maxSize;
    }
    ringBuffer->head = (ringBuffer->head + 1) % ringBuffer->maxSize;
    ringBuffer->isFull = (ringBuffer->head == ringBuffer->tail);
}

static void retreatBufferPointer(RingBuffer *ringBuffer) {
    ringBuffer->isFull = false;
    ringBuffer->tail = (ringBuffer->tail + 1) % ringBuffer->maxSize;
}

static void advanceBytesBufferPointer(RingBuffer_t* ringBuffer, unsigned int writeBufferSize) {
    DbgPrint("[Debug 2024] advanceBytesBufferPointer ringBuffer->head [%d] ringBuffer->tail [%d]  readBufferSize [%d] >>>>>>", ringBuffer->head, ringBuffer->tail, writeBufferSize);
    if (ringBuffer->isFull) {
        DbgPrint("[Debug 2024] advanceBytesBufferPointer isFull ringBuffer->tail [%d]  readBufferSize [%d] >>>>>>", ringBuffer->tail, writeBufferSize);
        ringBuffer->tail = (ringBuffer->tail + writeBufferSize) % ringBuffer->maxSize;
    }
    ringBuffer->head = (ringBuffer->head + writeBufferSize) % ringBuffer->maxSize;
    ringBuffer->isFull = (ringBuffer->head == ringBuffer->tail);
    DbgPrint("[Debug 2024] advanceBytesBufferPointer ok ringBuffer->head [%d] ringBuffer->tail [%d]  readBufferSize [%d] >>>>>>", ringBuffer->head, ringBuffer->tail, writeBufferSize);
}

static void retreatBytesBufferPointer(RingBuffer* ringBuffer, unsigned int readBufferSize) {
    DbgPrint("[Debug 2024] retreatBytesBufferPointer ringBuffer->tail [%d]  readBufferSize [%d] >>>>>>", ringBuffer->tail, readBufferSize);
    ringBuffer->isFull = false;
    //ringBuffer->tail = (ringBuffer->tail + 1) % ringBuffer->maxSize;
    ringBuffer->tail = (ringBuffer->tail + readBufferSize) % ringBuffer->maxSize;
    DbgPrint("[Debug 2024] retreatBytesBufferPointer ok ringBuffer->tail [%d]  readBufferSize [%d] >>>>>>", ringBuffer->tail, readBufferSize);
}
