/*
 * GCodeBuffer.cpp
 *
 *  Created on: 6 Feb 2015
 *      Author: David
 */

//*************************************************************************************

#include "GCodeBuffer.h"
#if HAS_HIGH_SPEED_SD
# include "GCodes/GCodeInput.h"
#endif
#include "BinaryParser.h"
#include "StringParser.h"

// Create a default GCodeBuffer
GCodeBuffer::GCodeBuffer(const char *id, MessageType mt, bool usesCodeQueue)
	: identity(id), responseMessageType(mt), queueCodes(usesCodeQueue), toolNumberAdjust(0),
	  binaryParser(*this), stringParser(*this), machineState(new GCodeMachineState())
#if HAS_LINUX_INTERFACE
	  , reportMissingMacro(false), cancelMacro(false)
#endif
{
	Init();
}

// Reset it to its state after start-up
void GCodeBuffer::Reset()
{
	while (PopState()) { }
	Init();
}

// Set it up to parse another G-code
void GCodeBuffer::Init()
{
	isBinaryBuffer = false;
	binaryParser.Init();
	stringParser.Init();

	timerRunning = false;
}

// Write some debug info
void GCodeBuffer::Diagnostics(MessageType mtype)
{
	if (isBinaryBuffer)
	{
		binaryParser.Diagnostics(mtype);
	}
	else
	{
		stringParser.Diagnostics(mtype);
	}
}

// Add a character to the end
bool GCodeBuffer::Put(char c)
{
	isBinaryBuffer = false;
	return stringParser.Put(c);
}

// Add an entire G-Code, overwriting any existing content
void GCodeBuffer::Put(const char *str, size_t len, bool isBinary)
{
	isBinaryBuffer = isBinary;
	if (isBinary)
	{
		binaryParser.Put(str, len);
	}
	else
	{
		stringParser.Put(str, len);
	}
}

// Add a null-terminated string, overwriting any existing content
void GCodeBuffer::Put(const char *str)
{
	isBinaryBuffer = false;
	stringParser.Put(str);
}

// Called when we reach the end of the file we are reading from
void GCodeBuffer::FileEnded()
{
	if (!isBinaryBuffer)
	{
		stringParser.FileEnded();
	}
}

char GCodeBuffer::GetCommandLetter() const
{
	return isBinaryBuffer ? binaryParser.GetCommandLetter() : stringParser.GetCommandLetter();
}

bool GCodeBuffer::HasCommandNumber() const
{
	return isBinaryBuffer ? binaryParser.HasCommandNumber() : stringParser.HasCommandNumber();
}

int GCodeBuffer::GetCommandNumber() const
{
	return isBinaryBuffer ? binaryParser.GetCommandNumber() : stringParser.GetCommandNumber();
}

int8_t GCodeBuffer::GetCommandFraction() const
{
	return isBinaryBuffer ? binaryParser.GetCommandFraction() : stringParser.GetCommandFraction();
}


// Is a character present?
bool GCodeBuffer::Seen(char c)
{
	return isBinaryBuffer ? binaryParser.Seen(c) : stringParser.Seen(c);
}

// Get a float after a key letter
float GCodeBuffer::GetFValue()
{
	return isBinaryBuffer ? binaryParser.GetFValue() : stringParser.GetFValue();
}

// Get an integer after a key letter
int32_t GCodeBuffer::GetIValue()
{
	return isBinaryBuffer ? binaryParser.GetIValue() : stringParser.GetIValue();
}

// Get an unsigned integer value
uint32_t GCodeBuffer::GetUIValue()
{
	return isBinaryBuffer ? binaryParser.GetUIValue() : stringParser.GetUIValue();
}

// Get an IP address quad after a key letter
bool GCodeBuffer::GetIPAddress(IPAddress& returnedIp)
{
	return isBinaryBuffer ? binaryParser.GetIPAddress(returnedIp) : stringParser.GetIPAddress(returnedIp);
}

// Get a MAC address sextet after a key letter
bool GCodeBuffer::GetMacAddress(uint8_t mac[6])
{
	return isBinaryBuffer ? binaryParser.GetMacAddress(mac) : stringParser.GetMacAddress(mac);
}

// Get a string with no preceding key letter
bool GCodeBuffer::GetUnprecedentedString(const StringRef& str)
{
	return isBinaryBuffer ? binaryParser.GetUnprecedentedString(str) : stringParser.GetUnprecedentedString(str);
}

// Get and copy a quoted string
bool GCodeBuffer::GetQuotedString(const StringRef& str)
{
	return isBinaryBuffer ? binaryParser.GetQuotedString(str) : stringParser.GetQuotedString(str);
}

// Get and copy a string which may or may not be quoted
bool GCodeBuffer::GetPossiblyQuotedString(const StringRef& str)
{
	return isBinaryBuffer ? binaryParser.GetPossiblyQuotedString(str) : stringParser.GetPossiblyQuotedString(str);
}

// Get a colon-separated list of floats after a key letter
const void GCodeBuffer::GetFloatArray(float arr[], size_t& length, bool doPad)
{
	if (isBinaryBuffer)
	{
		binaryParser.GetFloatArray(arr, length, doPad);
	}
	else
	{
		stringParser.GetFloatArray(arr, length, doPad);
	}
}

// Get a :-separated list of ints after a key letter
const void GCodeBuffer::GetIntArray(int32_t arr[], size_t& length, bool doPad)
{
	if (isBinaryBuffer)
	{
		binaryParser.GetIntArray(arr, length, doPad);
	}
	else
	{
		stringParser.GetIntArray(arr, length, doPad);
	}
}

// Get a :-separated list of unsigned ints after a key letter
const void GCodeBuffer::GetUnsignedArray(uint32_t arr[], size_t& length, bool doPad)
{
	if (isBinaryBuffer)
	{
		binaryParser.GetUnsignedArray(arr, length, doPad);
	}
	else
	{
		stringParser.GetUnsignedArray(arr, length, doPad);
	}
}

// If the specified parameter character is found, fetch 'value' and set 'seen'. Otherwise leave val and seen alone.
bool GCodeBuffer::TryGetFValue(char c, float& val, bool& seen)
{
	const bool ret = Seen(c);
	if (ret)
	{
		val = GetFValue();
		seen = true;
	}
	return ret;
}

// If the specified parameter character is found, fetch 'value' and set 'seen'. Otherwise leave val and seen alone.
bool GCodeBuffer::TryGetIValue(char c, int32_t& val, bool& seen)
{
	const bool ret = Seen(c);
	if (ret)
	{
		val = GetIValue();
		seen = true;
	}
	return ret;
}

// If the specified parameter character is found, fetch 'value' and set 'seen'. Otherwise leave val and seen alone.
bool GCodeBuffer::TryGetUIValue(char c, uint32_t& val, bool& seen)
{
	const bool ret = Seen(c);
	if (ret)
	{
		val = GetUIValue();
		seen = true;
	}
	return ret;
}

// If the specified parameter character is found, fetch 'value' as a Boolean and set 'seen'. Otherwise leave val and seen alone.
bool GCodeBuffer::TryGetBValue(char c, bool& val, bool& seen)
{
	const bool ret = Seen(c);
	if (ret)
	{
		val = GetIValue() > 0;
		seen = true;
	}
	return ret;
}

// Try to get an int array exactly 'numVals' long after parameter letter 'c'.
// If the wrong number of values is provided, generate an error message and return true.
// Else set 'seen' if we saw the letter and value, and return false.
bool GCodeBuffer::TryGetUIArray(char c, size_t numVals, uint32_t vals[], const StringRef& reply, bool& seen, bool doPad)
{
	if (Seen(c))
	{
		size_t count = numVals;
		GetUnsignedArray(vals, count, doPad);
		if (count == numVals)
		{
			seen = true;
		}
		else
		{
			reply.printf("Wrong number of values after '\''%c'\'', expected %d", c, numVals);
			return true;
		}
	}
	return false;
}

// Try to get a float array exactly 'numVals' long after parameter letter 'c'.
// If the wrong number of values is provided, generate an error message and return true.
// Else set 'seen' if we saw the letter and value, and return false.
bool GCodeBuffer::TryGetFloatArray(char c, size_t numVals, float vals[], const StringRef& reply, bool& seen, bool doPad)
{
	if (Seen(c))
	{
		size_t count = numVals;
		GetFloatArray(vals, count, doPad);
		if (count == numVals)
		{
			seen = true;
		}
		else
		{
			reply.printf("Wrong number of values after '\''%c'\'', expected %d", c, numVals);
			return true;
		}
	}
	return false;
}

// Try to get a quoted string after parameter letter.
// If we found it then set 'seen' true and return true, else leave 'seen' alone and return false
bool GCodeBuffer::TryGetQuotedString(char c, const StringRef& str, bool& seen)
{
	if (Seen(c) && GetQuotedString(str))
	{
		seen = true;
		return true;
	}
	return false;
}

// Try to get a string, which may be quoted, after parameter letter.
// If we found it then set 'seen' true and return true, else leave 'seen' alone and return false
bool GCodeBuffer::TryGetPossiblyQuotedString(char c, const StringRef& str, bool& seen)
{
	if (Seen(c) && GetPossiblyQuotedString(str))
	{
		seen = true;
		return true;
	}
	return false;
}

bool GCodeBuffer::IsIdle() const
{
	return isBinaryBuffer ? binaryParser.IsIdle() : stringParser.IsIdle();
}

bool GCodeBuffer::IsCompletelyIdle() const
{
	return isBinaryBuffer ? binaryParser.IsCompletelyIdle() : stringParser.IsCompletelyIdle();
}

bool GCodeBuffer::IsReady() const
{
	return isBinaryBuffer ? binaryParser.IsReady() : stringParser.IsReady();
}

bool GCodeBuffer::IsExecuting() const
{
	return isBinaryBuffer ? binaryParser.IsExecuting() : stringParser.IsExecuting();
}

void GCodeBuffer::SetFinished(bool f)
{
	if (isBinaryBuffer)
	{
		binaryParser.SetFinished(f);
	}
	else
	{
		stringParser.SetFinished(f);
	}
}

void GCodeBuffer::SetCommsProperties(uint32_t arg)
{
	if (!isBinaryBuffer)
	{
		stringParser.SetCommsProperties(arg);
	}
}

// Get the original machine state before we pushed anything
GCodeMachineState& GCodeBuffer::OriginalMachineState() const
{
	GCodeMachineState *ms = machineState;
	while (ms->previous != nullptr)
	{
		ms = ms->previous;
	}
	return *ms;
}

// Convert from inches to mm if necessary
float GCodeBuffer::ConvertDistance(float distance) const
{
	return (machineState->usingInches) ? distance * InchToMm : distance;
}

// Convert from mm to inches if necessary
float GCodeBuffer::InverseConvertDistance(float distance) const
{
	return (machineState->usingInches) ? distance/InchToMm : distance;
}

// Push state returning true if successful (i.e. stack not overflowed)
bool GCodeBuffer::PushState()
{
	// Check the current stack depth
	unsigned int depth = 0;
	for (const GCodeMachineState *m1 = machineState; m1 != nullptr; m1 = m1->previous)
	{
		++depth;
	}
	if (depth >= MaxStackDepth + 1)				// the +1 is to allow for the initial state record
	{
		return false;
	}

	GCodeMachineState * const ms = GCodeMachineState::Allocate();
	ms->previous = machineState;
	ms->feedRate = machineState->feedRate;
#if HAS_HIGH_SPEED_SD
	ms->fileState.CopyFrom(machineState->fileState);
#elif HAS_LINUX_INTERFACE
	ms->fileId = machineState->fileId;
	ms->isFileFinished = machineState->isFileFinished;
#endif
	ms->lockedResources = machineState->lockedResources;
	ms->drivesRelative = machineState->drivesRelative;
	ms->axesRelative = machineState->axesRelative;
	ms->usingInches = machineState->usingInches;
	ms->doingFileMacro = machineState->doingFileMacro;
	ms->waitWhileCooling = machineState->waitWhileCooling;
	ms->runningM501 = machineState->runningM501;
	ms->runningM502 = machineState->runningM502;
	ms->volumetricExtrusion = false;
	ms->g53Active = false;
	ms->runningSystemMacro = machineState->runningSystemMacro;
	ms->messageAcknowledged = false;
	ms->waitingForAcknowledgement = false;
	machineState = ms;

#if HAS_LINUX_INTERFACE
	reportStack = true;
#endif
	return true;
}

// Pop state returning true if successful (i.e. no stack underrun)
bool GCodeBuffer::PopState()
{
	GCodeMachineState * const ms = machineState;
	if (ms->previous == nullptr)
	{
		ms->messageAcknowledged = false;			// avoid getting stuck in a loop trying to pop
		ms->waitingForAcknowledgement = false;
		return false;
	}

	machineState = ms->previous;
	GCodeMachineState::Release(ms);

#if HAS_LINUX_INTERFACE
	reportStack = true;
#endif
	return true;
}

// Abort execution of any files or macros being executed, returning true if any files were closed
// We now avoid popping the state if we were not executing from a file, so that if DWC or PanelDue is used to jog the axes before they are homed, we don't report stack underflow.
#if HAS_HIGH_SPEED_SD
void GCodeBuffer::AbortFile(FileGCodeInput* fileInput)
{
	if (machineState->DoingFile())
	{
		do
		{
			if (machineState->DoingFile())
			{
				fileInput->Reset(machineState->fileState);
				machineState->CloseFile();
			}
		} while (PopState());							// abandon any macros
	}
}
#elif HAS_LINUX_INTERFACE
void GCodeBuffer::AbortFile()
{
	if (machineState->DoingFile())
	{
		do
		{
			if (machineState->DoingFile())
			{
				machineState->CloseFile();
			}
		} while (PopState());							// abandon any macros
	}

	cancelMacro = true;
}

bool GCodeBuffer::IsFileFinished() const
{
	return machineState->isFileFinished;
}

void GCodeBuffer::SetPrintFinished()
{
	const uint32_t fileId = OriginalMachineState().fileId;
	for (GCodeMachineState *ms = machineState; ms != nullptr; ms = ms->previous)
	{
		if (ms->fileId == fileId)
		{
			ms->SetFileFinished();
		}
	}
}

void GCodeBuffer::RequestMacroFile(const char *filename, bool reportMissing)
{
	machineState->SetFileExecuting();

	requestedMacroFile.copy(filename);
	reportMissingMacro = reportMissing;
	cancelMacro = false;
}

const char *GCodeBuffer::GetRequestedMacroFile(bool& reportMissing) const
{
	reportMissing = reportMissingMacro;
	return requestedMacroFile.IsEmpty() ? nullptr : requestedMacroFile.c_str();
}

bool GCodeBuffer::IsMacroCancellationRequested() const
{
	return cancelMacro;
}

void GCodeBuffer::AcknowledgeCancellation()
{
	cancelMacro = false;
}

bool GCodeBuffer::IsStackEventFlagged() const
{
	return reportStack;
}

void GCodeBuffer::AcknowledgeStackEvent()
{
	reportStack = false;
}
#endif

// Tell this input source that any message it sent and is waiting on has been acknowledged
// Allow for the possibility that the source may have started running a macro since it started waiting
void GCodeBuffer::MessageAcknowledged(bool cancelled)
{
	for (GCodeMachineState *ms = machineState; ms != nullptr; ms = ms->previous)
	{
		if (ms->waitingForAcknowledgement)
		{
			ms->waitingForAcknowledgement = false;
			ms->messageAcknowledged = true;
			ms->messageCancelled = cancelled;
		}
	}
}

MessageType GCodeBuffer::GetResponseMessageType() const
{
	if (isBinaryBuffer)
	{
		return (MessageType)((uint32_t)responseMessageType | (uint32_t)MessageType::BinaryCodeReplyFlag);
	}
	return responseMessageType;
}

FilePosition GCodeBuffer::GetFilePosition(size_t bytesCached) const
{
	return isBinaryBuffer ? binaryParser.GetFilePosition() : stringParser.GetFilePosition(bytesCached);
}

bool GCodeBuffer::OpenFileToWrite(const char* directory, const char* fileName, const FilePosition size, const bool binaryWrite, const uint32_t fileCRC32)
{
	if (isBinaryBuffer)
	{
		return false;
	}
	return stringParser.OpenFileToWrite(directory, fileName, size, binaryWrite, fileCRC32);
}

bool GCodeBuffer::IsWritingFile() const
{
	if (!isBinaryBuffer)
	{
		return stringParser.IsWritingFile();
	}
	return false;
}

void GCodeBuffer::WriteToFile()
{
	if (!isBinaryBuffer)
	{
		stringParser.WriteToFile();
	}
}

bool GCodeBuffer::IsWritingBinary() const
{
	if (!isBinaryBuffer)
	{
		return stringParser.IsWritingBinary();
	}
	return false;
}

void GCodeBuffer::WriteBinaryToFile(char b)
{
	if (!isBinaryBuffer)
	{
		stringParser.WriteBinaryToFile(b);
	}
}

void GCodeBuffer::FinishWritingBinary()
{
	if (!isBinaryBuffer)
	{
		stringParser.FinishWritingBinary();
	}
}

const char* GCodeBuffer::DataStart() const
{
	return isBinaryBuffer ? binaryParser.DataStart() : stringParser.DataStart();
}

size_t GCodeBuffer::DataLength() const
{
	return isBinaryBuffer ? binaryParser.DataLength() : stringParser.DataLength();
}

void GCodeBuffer::PrintCommand(const StringRef& s) const
{
	if (isBinaryBuffer)
	{
		binaryParser.PrintCommand(s);
	}
	else
	{
		stringParser.PrintCommand(s);
	}
}

void GCodeBuffer::AppendFullCommand(const StringRef &s) const
{
	if (isBinaryBuffer)
	{
		binaryParser.AppendFullCommand(s);
	}
	else
	{
		stringParser.AppendFullCommand(s);
	}
}

// End
