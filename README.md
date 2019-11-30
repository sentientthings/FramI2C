# FramI2C
Sentient Things FRAM I2C library. Based on the SPI FRAM work by Ray Benitez. 

Works with the following FRAM:
```
	MB85RC64 = 0,		      	// 8KB
	MB85RC128A,			// 16KB older model
	MB85RC128B,			// 16KB newer model
	MB85RC256A,			// 32KB older model
	MB85RC256B,			// 32KB newer model
	MB85RC256V,			// 32KB
	MB85RC512T,			// 64KB
```

The library works with structs up to 127 bytes in length.

```
// Create FRAM instances
#define PART_NUMBER MB85RC256V
FramI2C myFram(PART_NUMBER); // create an instance
framResult myResult = framUnknownError; // Error message
```

Basic functionality includes reading and writing elements:

	framResult read(unsigned long startAddress, unsigned int numberOfBytes, byte *buffer);
	framResult write(unsigned long startAddress, unsigned int numberOfBytes, byte *buffer);
	
Creating an array:
```
FramI2CArray framConfig(myFram, 10, sizeof(Settings_t), myResult);
```
and reading and writing to the array:

	void readElement(unsigned long index, byte *buffer, framResult &result);
	void writeElement(unsigned long index, byte *buffer, framResult &result);
	
A Ring buffer class is also included - i.e.
```
Ring_FramArray dataRing(myFram, 300, sizeof(sensorReadings), myResult);
```

with the ability to pop push peek etc:

	bool popFirstElement(byte *buffer);
	bool popLastElement(byte *buffer);
	void pushElement(byte *buffer);
	bool peekFirstElement(byte *buffer);
	bool peekLastElement(byte *buffer);
	void clearArray();
	bool isEmpty();
	bool isFull();

As of Version 0.1.0, the ring array keeps track of its pointers in FRAM.
Run initialize() to load the pointers before using the ring.


