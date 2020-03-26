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
*/

// Modified for I2C and Particle by Robert Mawrey

#include "Particle.h"
#include "FramI2C.h"

FramI2C::FramI2C(framPartNumber partNumber): _partNumber(partNumber) // Add in I2C address later
{
//	_topAddressForPartNumber[MB85RC16]		= 0x0007FFUL;
	_topAddressForPartNumber[MB85RC64]		= 0x001FFFUL;
	_topAddressForPartNumber[MB85RC128A]	= 0x003FFFUL;
	_topAddressForPartNumber[MB85RC128B]	= 0x003FFFUL;
	_topAddressForPartNumber[MB85RC256A]	= 0x007FFFUL;
	_topAddressForPartNumber[MB85RC256B]	= 0x007FFFUL;
	_topAddressForPartNumber[MB85RC256V]	= 0x007FFFUL;
	_topAddressForPartNumber[MB85RC512T]	= 0x00FFFFUL;
//	_topAddressForPartNumber[MB85RC1MT]		= 0x01FFFFUL;

	_baseAddress = 0x000000;
	_bottomAddress = _baseAddress + _maxBufferSize;
	_topAddress = _topAddressForPartNumber[_partNumber];
	_numberOfBuffers = (_topAddress - _bottomAddress + 1) / _maxBufferSize;
	_nextFreeByte = _bottomAddress;
}


//
// PLATFORM SPECIFIC, LOW LEVEL METHODS
//

void FramI2C::_readMemory(uint32_t address, uint8_t numberOfBytes, uint8_t *buffer)
{
	WITH_LOCK(Wire)
	{
		uint16_t framAddr = (uint16_t)address;
		// Address only correct for 64 through 512 kbit devices
		Wire.beginTransmission(framI2CAddress);

		Wire.write(framAddr >> 8);
		Wire.write(framAddr & 0xFF);

		Wire.endTransmission();

		// Maximum request size of 32 bytes
		Wire.requestFrom(framI2CAddress, (uint8_t)numberOfBytes);
		for (byte i=0; i < numberOfBytes; i++) {
			buffer[i] = Wire.read();
		}
		Wire.endTransmission();
	}
}


void FramI2C::_writeMemory(uint32_t address, uint8_t numberOfBytes, uint8_t *buffer)
{
	WITH_LOCK(Wire)
	{	
		uint16_t framAddr = (uint16_t)address;
		// Address only correct for 64 through 512 kbit devices

		Wire.beginTransmission(framI2CAddress);
		Wire.write(framAddr >> 8);
		Wire.write(framAddr & 0xFF);


		for (uint8_t i=0; i < numberOfBytes; i++) {
			Wire.write(buffer[i]);
		}
		Wire.endTransmission();
	}
}

//
// PLATFORM INDEPENDENT, HIGH LEVEL METHODS
//

framResult FramI2C::begin()
{
  // Initialize the I2C bus if not already enabled
  if (!Wire.isEnabled()) {
      Wire.begin();
  }
  Wire.beginTransmission(framI2CAddress);
  byte response = Wire.endTransmission();

  if (response == 0)
  {
	  return framOK;
  }
  else
  {
	  return framBadResponse;
  }
}


framPartNumber FramI2C::getPartNumber()
{
	return _partNumber;
}


byte FramI2C::getMaxBufferSize()
{
	return _maxBufferSize;
}


uint32_t FramI2C::getBottomAddress()
{
	return _bottomAddress;
}


uint32_t FramI2C::getTopAddress()
{
	return _topAddress;
}

uint32_t FramI2C::getNextFreeByte()
{
	return _nextFreeByte;
}


byte FramI2C::getControlBlockSize()
{
	return _maxBufferSize;
}


void FramI2C::writeControlBlock(uint8_t *buffer)
{
  write((uint32_t) _baseAddress, (unsigned int) _maxBufferSize, buffer);
}


void FramI2C::readControlBlock(uint8_t *buffer)
{
  read((uint32_t) _baseAddress, (unsigned int) _maxBufferSize, buffer);
}


framResult FramI2C::read(uint32_t startAddress, unsigned int numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from FRAM (starting at startAddress) into buffer (starting at 0)
	// Returns result code

	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes <= _topAddress

	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return framBadStartAddress;
	}
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return framBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return framBadFinishAddress;
	}
// Read in 30 byte blocks due to wire requestFrom() limit
  const uint8_t blockSize = 30;
  byte* buf = buffer;
  uint32_t address = startAddress;

  while (numberOfBytes >= blockSize)
  {
		_readMemory(address, blockSize, buf);
	  address += blockSize;
		buf += blockSize;
	  numberOfBytes -= blockSize;
  }
  if (numberOfBytes > 0)
  {
    _readMemory(address, numberOfBytes, buf);
  }
	return framOK;
}


framResult FramI2C::write(uint32_t startAddress, unsigned int numberOfBytes, byte *buffer)
{
	// Copies numberOfBytes bytes from buffer (starting at 0) into FRAM (starting at startAddress)
	// Returns result code

	// Validations:
	//		_bottomAddress <= startAddress <= _topAddress
	//		0 < numberOfBytes <= maxBuffer
	//		startAddress + numberOfBytes - 1 <= _topAddress

	if ((startAddress < _bottomAddress) || (startAddress > _topAddress))
	{
		return framBadStartAddress;
	}
	if ((numberOfBytes > _maxBufferSize) || (numberOfBytes == 0))
	{
		return framBadNumberOfBytes;
	}
	if ((startAddress + numberOfBytes - 1) > _topAddress)
	{
		return framBadFinishAddress;
	}

	// _writeMemory(startAddress, numberOfBytes, buffer);

	// Write in 32 byte blocks due to wire limit
	  const uint8_t blockSize = 30;
	  byte* buf = buffer;
	  uint32_t address = startAddress;

	  while (numberOfBytes >= blockSize)
	  {
			_writeMemory(address, blockSize, buf);
		  address += blockSize;
			buf += blockSize;
		  numberOfBytes -= blockSize;
	  }
	  if (numberOfBytes > 0)
	  {
	    _writeMemory(address, numberOfBytes, buf);
	  }

	return framOK;
}


uint32_t FramI2C::allocateMemory(uint32_t numberOfBytes, framResult& result)
{

	if ((_nextFreeByte + numberOfBytes) < _topAddress)
	{
		uint16_t base = _nextFreeByte;
		_nextFreeByte += numberOfBytes;
		result = framOK;
		return base;
	}
	else
	{
		result = framBadFinishAddress;
		return 0;
	}
}


framResult FramI2C::format()
{
	// Fills FRAM with 0 but does NOT overwrite control block
	// Returns result code from framWrite function, or framOK if format is successful
	byte buffer[_maxBufferSize];

	for (byte i = 0; i < _maxBufferSize; i++)
	{
		buffer[i] = 0;
	}

	framResult result = framOK;
	uint32_t i = _bottomAddress;
	while ((i < _topAddress) && (result == framOK))
	{
		result = write(i, _maxBufferSize, buffer);
		i += _maxBufferSize;
	}
	return result;
}


FramI2CArray::FramI2CArray(FramI2C& f, uint32_t numberOfElements, byte sizeOfElement, framResult &result): 
		_numberOfElements(numberOfElements), _sizeOfElement(sizeOfElement), _f(f)
		
{
	// Creates array in FRAM
	// Calculates and allocates required memory
	// Returns result code

	// Validations:
	//		_sizeOfElement <= _bufferSize

	if (_sizeOfElement < _f.getMaxBufferSize())
	{
		_startAddress = _f.allocateMemory(_numberOfElements * _sizeOfElement, result);
	}
	else
	{
		result = framArrayElementTooBig;
		_startAddress = 0;
	}
}


void FramI2CArray::readElement(uint32_t index, byte *buffer, framResult &result)
{
	// Reads element from array in FRAM
	// Returns result code

	// Validations:
	//		_startAddress > 0 (otherwise array has probably not been created)
	//		index < _numberOfElements

	if (_startAddress == 0)
	{
		result = framBadArrayStartAddress;
	}
	else if (index >= _numberOfElements)
	{
		result = framBadArrayIndex;
	}
	else
	{
		result = _f.read(_startAddress + (index * _sizeOfElement), _sizeOfElement, buffer);
	}
}



void FramI2CArray::writeElement(uint32_t index, byte *buffer, framResult &result)
{
	// Writes element to array in FRAM
	// Returns result code

	// Validations:
	//		_startAddress > 0 (otherwise array has probably not been created)
	//		index < _numberOfElements

	if (_startAddress == 0)
	{
		result = framBadArrayStartAddress;
	}
	else if (index >= _numberOfElements)
	{
		result = framBadArrayIndex;
	}
	else
	{
		result = _f.write(_startAddress + (index * _sizeOfElement), _sizeOfElement, buffer);
	}
}


uint32_t FramI2CArray::getStartAddress()
{
	return _startAddress;
}

// Ring_FramArray added by Mawrey
// Function to wrap around (find the mod of) for the circular buffer
uint32_t Ring_FramArray::myModulo(uint32_t a, uint32_t b)
{
	int x;
	x = (int)a % (int)b;
	if (x>=0)
	{
		return (uint32_t)x;
	}
	else
	{
		return (uint32_t)(x+b);
	}
}

Ring_FramArray::Ring_FramArray(FramI2C& f, uint32_t numberOfElements, byte sizeOfElement, framResult &result): 
	_numberOfElements(numberOfElements), _sizeOfElement(sizeOfElement), _f(f)
{
	if (_sizeOfElement < _f.getMaxBufferSize())
	{
		// The array size needs to be incremented so that there is room for one empty element
		_numberOfElements = _numberOfElements + 1;
		// allocateMemeory returns the base or start address and saves the new
		// _nextFreeByte = _bottomAddress
		// add sizeof(_pointers) to allocate 12 bytes to store the _pointer indices
		_startAddress = _f.allocateMemory(_numberOfElements * _sizeOfElement + sizeof(_pointers), result);
		_ringEndAddress = _f.getNextFreeByte();
		// if (result==framOK)
		// {
		// 	_pointersAddress = _f.getBottomAddress() - (uint32_t)sizeof(_pointers);

		// 	// Load pointers from FRAM
		// 	getPointers();
		// 	// Check for initialization or existance of pointers
		// 	// Rule to check _pointers.pointerCheck = _pointersAddress+_pointers.tail+pointers.head
		// 	uint32_t check = _pointersAddress + _pointers.tail + _pointers.head;
		// 	if (check==_pointers.pointerCheck)
		// 	{
		// 		// Not first run so initialize from FRAM
		// 		_tailAddress = _pointers.tail;
		// 		_headAddress = _pointers.head;
		// 	}
		// 	else
		// 	{
		// 		// First run
		// 		_tailAddress = 0;
		// 		_headAddress = 0;
		// 		setPointers();				
		// 	}
		// }
	}
	else
	{
		result = framArrayElementTooBig;
		// Add checks in the other functions to use start address
		_startAddress = 0;
		// _tailAddress = 0;
		// _headAddress = 0;
	}
}

void Ring_FramArray::initialize()
{
	// if (result==framOK)
	// {
		if (!Wire.isEnabled())
		{
			Wire.begin();
		}

		_pointersAddress = _ringEndAddress - (uint32_t)sizeof(_pointers);

		// Load pointers from FRAM
		getPointers();
		// Check for initialization or existance of pointers
		// Rule to check _pointers.pointerCheck = _pointersAddress+_pointers.tail+pointers.head
		uint32_t check = _pointersAddress + _pointers.tail + _pointers.head;
		if (check==_pointers.pointerCheck)
		{
			// Not first run so initialize from FRAM
			_tailAddress = _pointers.tail;
			_headAddress = _pointers.head;
		}
		else
		{
			// First run
			_tailAddress = 0;
			_headAddress = 0;			
		}
		setPointers();
	// }
}

uint32_t Ring_FramArray::getStartAddress()
{
	return _startAddress;
}

bool Ring_FramArray::popFirstElement(byte *buffer)
{
	if (!isEmpty()&&!(_startAddress==0))
	{
		_f.read(_startAddress + (_tailAddress * _sizeOfElement), _sizeOfElement, buffer);
		_tailAddress++;
		_tailAddress = myModulo(_tailAddress,_numberOfElements);
		setPointers();
		return true;
	}
	else
	{
		return false;
	}
}


bool Ring_FramArray::pop(byte *buffer)
{
	if (!isEmpty()&&!(_startAddress==0))
	{
		_f.read(_startAddress + (_tailAddress * _sizeOfElement), _sizeOfElement, buffer);
		_tailAddress++;
		_tailAddress = myModulo(_tailAddress,_numberOfElements);
		setPointers();
		return true;
	}
	else
	{
		return false;
	}
}

bool Ring_FramArray::popLastElement(byte *buffer)
{
	if (!isEmpty()&&!(_startAddress==0))
	{
		_f.read(_startAddress + (myModulo(_headAddress-1,_numberOfElements) * _sizeOfElement), _sizeOfElement, buffer);
		_headAddress--;
		_headAddress = myModulo(_headAddress,_numberOfElements);
		setPointers();		
		return true;
	}
	else
	{
		return false;
	}
}


// Write new element to ring and overwrite if full
void Ring_FramArray::pushElement(byte *buffer)
{
	if (!isFull())
	{
		_f.write(_startAddress + (_headAddress * _sizeOfElement), _sizeOfElement, buffer);
		_headAddress++;
		_headAddress = myModulo(_headAddress,_numberOfElements);
	}
	else
	{
		_f.write(_startAddress + (_headAddress * _sizeOfElement), _sizeOfElement, buffer);
		_headAddress++;
		_tailAddress++;
		_tailAddress = myModulo(_tailAddress,_numberOfElements);
		_headAddress = myModulo(_headAddress,_numberOfElements);
	}
	setPointers();
}


// Write new element to ring and overwrite if full
void Ring_FramArray::push(byte *buffer)
{
	if (!isFull())
	{
		_f.write(_startAddress + (_headAddress * _sizeOfElement), _sizeOfElement, buffer);
		_headAddress++;
		_headAddress = myModulo(_headAddress,_numberOfElements);
	}
	else
	{
		_f.write(_startAddress + (_headAddress * _sizeOfElement), _sizeOfElement, buffer);
		_headAddress++;
		_tailAddress++;
		_tailAddress = myModulo(_tailAddress,_numberOfElements);
		_headAddress = myModulo(_headAddress,_numberOfElements);
	}
	setPointers();
}

bool Ring_FramArray::peekFirstElement(byte *buffer)
{
	if (!isEmpty()&&!(_startAddress==0))
	{
		_f.read(_startAddress + (_tailAddress * _sizeOfElement), _sizeOfElement, buffer);
		return true;
	}
	else
	{
		return false;
	}
}

bool Ring_FramArray::peekLastElement(byte *buffer)
{
	if (!isEmpty()&&!(_startAddress==0))
	{
		_f.read(_startAddress + (myModulo(_headAddress-1,_numberOfElements) * _sizeOfElement), _sizeOfElement, buffer);

		return true;
	}
	else
	{
		return false;
	}

}


void Ring_FramArray::clearArray()
{
	if (!(_startAddress==0))
	{
		// Fills the array with 0
		byte buffer[_sizeOfElement];

		for (byte i = 0; i < _sizeOfElement; i++)
		{
			buffer[i] = 0;
		}

		framResult result;
		uint32_t i = 0;
		while (i < _numberOfElements)
		{
			result = _f.write(_startAddress + (i * _sizeOfElement), _sizeOfElement, (uint8_t*)&buffer);
			i++;
		}
		_tailAddress = 0;
		_headAddress = 0;
		setPointers();		
	}

}

bool Ring_FramArray::isEmpty()
{
	if (_tailAddress == _headAddress)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Ring_FramArray::isFull()
{
	uint32_t inc = myModulo(_headAddress+1,_numberOfElements);
	if (inc == _tailAddress)
	{
		return true;
	}
	else
	{
		return false;
	}
}


void Ring_FramArray::getIndices(uint32_t *startAdd, uint32_t *endAdd)
{
	*startAdd = _tailAddress;
	*endAdd = _headAddress;
}

// Careful using this as the Indices/Pointers are kept track of automatically
bool Ring_FramArray::setIndices(uint32_t startAddress, uint32_t endAddress)
{
	if (((startAddress>=0)&&(startAddress<_numberOfElements)) && ((endAddress>=0)&&(endAddress<_numberOfElements)))
	{
		_tailAddress = startAddress;
		_headAddress = endAddress;
		setPointers();
		return true;
	}
	else
	{
		return false;
	}
}

void Ring_FramArray::setPointers()
{
	if (!(_startAddress==0))
	{
		_pointers.head = _headAddress;
		_pointers.tail = _tailAddress;
		// Calculate the check pointer
		_pointers.pointerCheck = _pointersAddress + _pointers.tail + _pointers.head;
		// Save pointers to fram
		_f.write(_pointersAddress, sizeof(_pointers),(uint8_t*)&_pointers);
	}
}

void Ring_FramArray::getPointers()
{
_pointers.tail=99;
_pointers.head=100;

framResult checkresult;
	//read(uint32_t startAddress, unsigned int numberOfBytes, byte *buffer)
	checkresult = _f.read(_pointersAddress, sizeof(_pointers), (uint8_t*)&_pointers);
	// _headAddress = _pointers.head;
	// _tailAddress = _pointers.tail;
}
