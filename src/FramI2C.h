/*
	FerroI2C Library
	==========================

	Connects Fujitsu Ferroelectric RAM (MB85RS range) to your
	Arduino to add up to 256KB of fast, non-volatile storage.

	For information on how to install and use the library,
	read "FerroI2C user guide.md".


	Created on 18 April 2014
	By Ray Benitez
	Last modified on 29 September 2014
	By Ray Benitez
	Change history in "README.md"

	This software is licensed by Ray Benitez under the MIT License.

	git@hackscribble.com | http://www.hackscribble.com | http://www.twitter.com/hackscribble

	Modified to support Particle Photon and Ring_FramArray class added by Mawrey 5 May 2017
	Modified for I2C October 2018 and November 2019
*/

#ifndef FramI2C_h
#define FramI2C_h

#include "Particle.h"

// MB85RC part numbers

enum framPartNumber
{
//	MB85RC16 = 0,		// 2KB
	MB85RC64 = 0,		// 8KB
	MB85RC128A,			// 16KB older model
	MB85RC128B,			// 16KB newer model
	MB85RC256A,			// 32KB older model
	MB85RC256B,			// 32KB newer model
	MB85RC256V,			// 32KB
	MB85RC512T,			// 64KB
//	MB85RC1MT,			// 128KB
	numberOfPartNumbers
};


// Result codes

enum framResult
{
	framOK = 0,
	framBadStartAddress,
	framBadNumberOfBytes,
	framBadFinishAddress,
	framArrayElementTooBig,
	framBadArrayIndex,
	framBadArrayStartAddress,
	framBadResponse,
	framPartNumberMismatch,
	framUnknownError = 99
};


class FramI2C
{
private:

	framPartNumber _partNumber;
	byte framI2CAddress = 0x50; // Put this in globals
	// Set maximum size of buffer used to write to and read from FRAM
	// Do not exceed 0x80 (128) to prevent problems with maximum size structs in FramArray
	static const byte _maxBufferSize = 128;

	// Used in constructor to set size of usable FRAM memory, reserving some bytes as a control block
	uint32_t _topAddressForPartNumber[numberOfPartNumbers];
	uint32_t _baseAddress;
	uint32_t _bottomAddress;
	uint32_t _topAddress;
	uint32_t _numberOfBuffers;

	// FRAM current next byte to allocate
	uint32_t _nextFreeByte;


public:

	FramI2C(framPartNumber partNumber = MB85RC128A);
	framResult begin();
	framPartNumber getPartNumber();
	byte getMaxBufferSize();
	uint32_t getBottomAddress();
	uint32_t getTopAddress();
	uint32_t getNextFreeByte();
	byte getControlBlockSize();
	void writeControlBlock(byte *buffer);
	void readControlBlock(byte *buffer);
	framResult read(uint32_t startAddress, unsigned int numberOfBytes, byte *buffer);
	framResult write(uint32_t startAddress, unsigned int numberOfBytes, byte *buffer);
	uint32_t allocateMemory(uint32_t numberOfBytes, framResult& result);
	framResult format();

	void _readMemory(uint32_t address, uint8_t numberOfBytes, uint8_t *buffer);
	void _writeMemory(uint32_t address, uint8_t numberOfBytes, uint8_t *buffer);

};


class FramI2CArray
{
private:

	uint32_t _numberOfElements;
	byte _sizeOfElement;
	uint32_t _startAddress;
	FramI2C& _f;

public:

	FramI2CArray(FramI2C& f, uint32_t numberOfElements, byte sizeOfElement, framResult &result);
	// Pass by pointer
	void readElement(uint32_t index, byte *buffer, framResult &result);

	// Pass by pointer
	void writeElement(uint32_t index, byte *buffer, framResult &result);

	uint32_t getStartAddress();

};

// Ring buffer class added by Mawrey
class Ring_FramArray
{
private:
	uint32_t _numberOfElements;
	byte _sizeOfElement;
	uint32_t _startAddress;
	FramI2C& _f;
	uint32_t _tailAddress;
	uint32_t _headAddress;
	uint32_t _ringEndAddress;
	uint32_t getStartAddress();
	uint32_t myModulo(uint32_t a, uint32_t b);
	// Struct for tail head and validity
	// The ring array keeps track of its own pointers in FRAM
	typedef struct
	{
	uint32_t pointerCheck;
	uint32_t tail;
	uint32_t head;
	}addr_t;
	addr_t _pointers;
	uint32_t _pointersAddress;
	void setPointers();
	void getPointers();

public:

	Ring_FramArray(FramI2C& f, uint32_t numberOfElements, byte sizeOfElement, framResult &result);

	void initialize();

	// Pop first element by default
	bool pop(byte *buffer);
	// Circular buffer overwrites when full!
	void push(byte *buffer);

	void clearArray();
	bool isEmpty();
	bool isFull();
	
	bool popFirstElement(byte *buffer);

	bool popLastElement(byte *buffer);

	void pushElement(byte *buffer);

	bool peekFirstElement(byte *buffer);

	bool peekLastElement(byte *buffer);


	// Call this using xxxx.getIndices(uint32_t &startadd, uint32_t &endadd);
	void getIndices(uint32_t *startAdd, uint32_t *endAdd);
	// Use with CAUTION as the Fram array keeps track of its own pointers in Fram
	bool setIndices(uint32_t startAddress, uint32_t endAddress);
};

#endif
