

#ifndef RNG_BUF_H
#define RNG_BUF_H


#ifndef __cplusplus
#include <stdbool.h>
#else
#include <cstdbool>

extern "C" {
#endif
	//typedef int RingBufferDataType;                                 // set container data type 
	typedef unsigned char RingBufferDataType;                                 // set container data type 

	struct RingBuffer {
		unsigned int head;
		unsigned int tail;
		unsigned int maxSize;
		RingBufferDataType* dataBuffer;
		bool isFull;
	};

	typedef struct RingBuffer RingBuffer_t;

	static RingBuffer_t* GlobalRingBuffer;
	bool GetRingBufferInstance(unsigned int bufferSize);
	RingBuffer* GetRingBufferReadyInstance();
	bool ReleaseRingBufferInstance();

	void ringBufferDelete(RingBuffer_t*ringBuffer);                      // destroying the container
	void ringBufferReset(RingBuffer_t*ringBuffer);                       // set pointers to zero, and the data count, causing the buffer to be "empty"
	bool isRingBufferFull(RingBuffer_t*ringBuffer);                      // check that buffer is full
	bool isRingBufferNotFull(RingBuffer_t*ringBuffer);
	bool isRingBufferEmpty(RingBuffer_t*ringBuffer);                     // buffer empty if no data received
	bool isRingBufferNotEmpty(RingBuffer_t*ringBuffer);
	unsigned int getRingBufferSize(RingBuffer_t*ringBuffer);                 // return elements count in buffer

	void ringBufferWrite(RingBuffer_t *ringBuffer, PCHAR value);       // add item to buffer
	RingBufferDataType* ringBufferRead(RingBuffer_t *ringBuffer);      // get data from buffer, return item if buffer have value and NULL otherwise
	RingBufferDataType* ringBufferReadStart(RingBuffer_t* ringBuffer);

	bool ringBufferWriteBytes(RingBuffer_t* ringBuffer, RingBufferDataType* value, unsigned int writeBufferSize);
	bool ringBufferReadBytes(RingBuffer_t* ringBuffer, unsigned int readBufferSize, PCHAR toBufferPointer);

#ifdef __cplusplus
}
#endif

#endif /* RNG_BUF_H */


