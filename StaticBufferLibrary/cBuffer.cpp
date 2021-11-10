/*
* Author:		Jarrid Steeper 0883583, Bogdan Tsyganok 0886354
* Class:		INFO6016 Network Programming
* Teacher:		Lukas Gustafson
* Project:		Project01
* Due Date:		Oct 22
* Filename:		cBuffer.cpp
* Purpose:		Buffer between client and server, can serialize and deserialize ints, shorts and strings. Contains definitions
*/

#include "cBuffer.h"
#include <iostream>

cBuffer::cBuffer(std::size_t size)
{
	mBuffer.resize(size);
}

void cBuffer::AddHeader(int commandId)
{
	//Header will be 8 bytes (uint length, int commandId)
	std::vector<uint8_t>* header = new std::vector<uint8_t>();

	//Ints size = 32

	//Size
	int packetSize = 8 + GetWriteIndex();
	
	header->push_back(packetSize >> 24);
	header->push_back(packetSize >> 16);
	header->push_back(packetSize >> 8);
	header->push_back(packetSize);

	//Command
	header->push_back(commandId >> 24);
	header->push_back(commandId >> 16);
	header->push_back(commandId >> 8);
	header->push_back(commandId);

	mBuffer.insert(mBuffer.begin(), header->begin(), header->end());
}

uint8_t* cBuffer::GetBuffer()
{
	return mBuffer.data();
}

size_t cBuffer::GetSize()
{
	return mBuffer.size();
}

int cBuffer::GetWriteIndex()
{
	return mWriteIndex;
}

int cBuffer::GetReadIndex()
{
	return mReadIndex;
}

void cBuffer::ResetSize(size_t newSize)
{
	mBuffer.resize(newSize);
}

void cBuffer::Flush()
{
	this->mBuffer.clear();
	this->mReadIndex = 0;
	this->mWriteIndex = 0;
}

//Ints
void cBuffer::WriteIntBE(std::size_t index, int32_t value)
{
	int bufferSize = mBuffer.size();
	if (mWriteIndex >= bufferSize - 4)
	{
		mBuffer.resize(bufferSize + 4);
	}

	mBuffer[index] = value >> 24;
	mBuffer[index + 1] = value >> 16;
	mBuffer[index + 2] = value >> 8;
	mBuffer[index + 3] = value;
	mWriteIndex += 4;
}

void cBuffer::WriteIntBE(int32_t value)
{
	int bufferSize = mBuffer.size();
	if (mWriteIndex >= bufferSize - 4)
	{
		mBuffer.resize(bufferSize + 4);
	}

	mBuffer[mWriteIndex] = value >> 24;
	mBuffer[mWriteIndex + 1] = value >> 16;
	mBuffer[mWriteIndex + 2] = value >> 8;
	mBuffer[mWriteIndex + 3] = value;
	mWriteIndex += 4;
}

int32_t cBuffer::ReadIntBE(std::size_t index)
{
	uint32_t value = 0;
	value |= mBuffer[index] << 24;
	value |= mBuffer[index + 1] << 16;
	value |= mBuffer[index + 2] << 8;
	value |= mBuffer[index + 3];
	return value;
}

int32_t cBuffer::ReadIntBE()
{
	uint32_t value = 0;
	value |= mBuffer[mReadIndex] << 24;
	value |= mBuffer[mReadIndex + 1] << 16;
	value |= mBuffer[mReadIndex + 2] << 8;
	value |= mBuffer[mReadIndex + 3];
	mReadIndex += 4;
	return value;
}

//Floats
void cBuffer::WriteShortBE(std::size_t index, int16_t value)
{
	int bufferSize = mBuffer.size();
	if (mWriteIndex >= bufferSize - 2)
	{
		mBuffer.resize(bufferSize + 2);
	}

	mBuffer[index] = value >> 8;
	mBuffer[index + 1] = value;
	mWriteIndex += 2;
}

void cBuffer::WriteShortBE(int16_t value)
{
	int bufferSize = mBuffer.size();
	if (mWriteIndex >= bufferSize - 2)
	{
		mBuffer.resize(bufferSize + 2);
	}

	mBuffer[mWriteIndex] = value >> 8;
	mBuffer[mWriteIndex + 1] = value;
	mWriteIndex += 2;
}

int16_t cBuffer::ReadShortBE(std::size_t index)
{
	int16_t value = 0;
	value |= mBuffer[index] << 8;
	value |= mBuffer[index + 1];
	return value;
}

int16_t cBuffer::ReadShortBE()
{
	int16_t value = 0;
	value |= mBuffer[mReadIndex] << 8;
	value |= mBuffer[mReadIndex + 1];
	mReadIndex += 2;
	return value;
}

//Strings
void cBuffer::WriteStringBE(std::size_t index, std::string value)
{
	int bufferSize = mBuffer.size();
	int valueSize = value.size();
	if (mWriteIndex >= bufferSize - valueSize)
	{
		mBuffer.resize(bufferSize + valueSize);
	}

	for (char c : value)
	{
		mBuffer[index++] = c;
		mWriteIndex++;
	}
}

void cBuffer::WriteStringBE(std::string value)
{
	int bufferSize = mBuffer.size();
	int valueSize = value.size();
	if (mWriteIndex >= bufferSize - valueSize)
	{
		mBuffer.resize(bufferSize + valueSize);
	}

	for (char c : value)
	{
		mBuffer[mWriteIndex++] = c;
	}
}

std::string cBuffer::ReadStringBE(std::size_t index, int32_t stringSize)
{
	char value;
	std::string outPut;
	for (int i = 0; i < stringSize; i++)
	{
		value = mBuffer[index++];
		outPut += value;
	}

	return outPut;
}

std::string cBuffer::ReadStringBE(int32_t stringSize)
{
	char value;
	std::string outPut = "";
	for (int i = 0; i < stringSize; i++)
	{
		value = mBuffer[mReadIndex++];
		outPut += value;
	}

	return outPut;
}
