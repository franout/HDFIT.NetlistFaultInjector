#include "common.h"

#include "omp.h"

#include "GateLFile.h"

static size_t uuidGet()
{
	static size_t counter = 2; // 1 is reserved, see GlobalFiModInstNumberTop_
	return counter++;
}

GateLFile::GateLFile()
{
	// TODO Auto-generated constructor stub
}

GateLFile::~GateLFile()
{
	if (nullptr != Content_)
	{
		free(Content_);
	}
	if (nullptr != LibContent_)
	{
		free(LibContent_);
	}
}

int GateLFile::Get(const char *fileName, const std::string &topModule)
{
	if (nullptr != Content_)
	{
		nfiError("GateLFile already contains content\n");
		return -1;
	}

	Name_ = fileName;

	FILE *pFile = fopen(fileName, "r");
	if (nullptr == pFile)
	{
		nfiError("failed to open file %s\n", fileName);
		return -1;
	}

	if (fseek(pFile, 0L, SEEK_END))
	{
		nfiError("SEEK_END failed on file %s\n", fileName);
		fclose(pFile); // no write performed, so no need to check
		return -1;
	}

	const long nFile = ftell(pFile);
	if (0 > nFile)
	{
		nfiError("ftell failed on file %s\n", fileName);
		fclose(pFile); // no write performed, so no need to check
		return -1;
	}

	rewind(pFile);

	Content_ = (char *)malloc(nFile + 1);
	if (NULL == Content_)
	{
		nfiError("Could not malloc %lu bytes for %s\n", nFile + 1, fileName);
		fclose(pFile); // no write performed, so no need to check
		return -3;
	}

	Size_ = fread(
		(void *)Content_,
		sizeof(char), nFile, pFile);

	fclose(pFile); // no write performed, so no need to check

	if (Size_ != nFile)
	{
		nfiError("Could not read %s\n", fileName);
		return -4;
	}

	Content_[Size_] = '\0';
	Size_++;

	TopModule_ = topModule;

	return 0;
}

int GateLFile::GetLib(const char *fileName)
{

	if (nullptr != LibContent_)
	{
		nfiError("RtlFile already contains content\n");
		return -1;
	}

	LibName_ = fileName;

	FILE *pFile = fopen(fileName, "r");
	if (nullptr == pFile)
	{
		nfiError("failed to open file %s\n", fileName);
		return -1;
	}

	if (fseek(pFile, 0L, SEEK_END))
	{
		nfiError("SEEK_END failed on file %s\n", fileName);
		fclose(pFile); // no write performed, so no need to check
		return -1;
	}

	const long nFile = ftell(pFile);
	if (0 > nFile)
	{
		nfiError("ftell failed on file %s\n", fileName);
		fclose(pFile); // no write performed, so no need to check
		return -1;
	}

	rewind(pFile);

	LibContent_ = (char *)malloc(nFile + 1);
	if (NULL == LibContent_)
	{
		nfiError("Could not malloc %lu bytes for %s\n", nFile + 1, fileName);
		fclose(pFile); // no write performed, so no need to check
		return -3;
	}

	LibSize_ = fread(
		(void *)LibContent_,
		sizeof(char), nFile, pFile);

	fclose(pFile); // no write performed, so no need to check

	if (LibSize_ != nFile)
	{
		nfiError("Could not read %s\n", fileName);
		return -4;
	}

	LibContent_[LibSize_] = '\0';
	LibSize_++;

	return 0;
}

int GateLFile::SignalDeclarationParse(signal_t *signal, const char *declaration)
{
	// Get Type
	const char *typeStart[SIGNAL_TYPE_NROF];
	for (size_t type = 0; type < SIGNAL_TYPE_NROF; type++)
	{
		typeStart[type] = strstr(declaration, SignalTypes[type]);
	}

	signal->Type = SIGNAL_TYPE_NROF;
	const char *pType = declaration + 1000; // TODO: DIRTY
	for (size_t type = 0; type < SIGNAL_TYPE_NROF; type++)
	{
		if ((nullptr != typeStart[type]) && (pType > typeStart[type]))
		{
			pType = typeStart[type];
			signal->Type = (signalType_t)type;
		}
	}

	if (SIGNAL_TYPE_NROF == signal->Type)
	{
		nfiError("Couldn't get signal type\n");
		return -1;
	}

	// Get width
	const char *afterType = typeStart[signal->Type] + strlen(SignalTypes[signal->Type]);
	afterType = ParseUtils::firstNonSpaceGet(afterType);
	if ('[' == *afterType)
	{
		int width = ParseUtils::signalWidthGet(&afterType, afterType);
		if (0 >= width)
		{
			nfiError("signalWidthGet failed: %.30s\n", declaration);
			return -1;
		}

		signal->Width = width;

		afterType++; // after ]
		afterType = ParseUtils::firstNonSpaceGet(afterType);
	}
	else
	{
		signal->Width = 1;
	}

	// Get Name
	const char *endNameSemiColon = strchr(afterType, ';');
	const char *endNameSpace = strchr(afterType, ' ');

	const char *endName = nullptr;
	if ((nullptr != endNameSemiColon) && (nullptr != endNameSpace))
	{
		endName = (endNameSemiColon < endNameSpace) ? endNameSemiColon : endNameSpace;
	}
	else if (nullptr != endNameSemiColon)
	{
		endName = endNameSemiColon;
	}
	else if (nullptr != endNameSpace)
	{
		endName = endNameSpace;
	}
	else
	{
		nfiError("Name doesn't end\n");
		return -1;
	}

	const size_t nameLen = endName - afterType;
	std::vector<char> name(nameLen + 1);
	memcpy(name.data(), afterType, nameLen);
	name[nameLen] = '\0';
	signal->Name = name.data();

	const char *afterName = afterType + nameLen;

	// Get Array Element Count
	const char *arrayStart = strchr(afterName, '[');
	if ((nullptr != arrayStart) && (arrayStart < endNameSemiColon))
	{
		const char *tmp;
		int arraySize = ParseUtils::signalArraySizeGet(&tmp, arrayStart);
		if (0 >= arraySize)
		{
			nfiError("signalArraySizeGet failed\n");
			return -1;
		}

		signal->ElemCnt = arraySize;
	}
	else
	{
		signal->ElemCnt = 1;
	}

	return 0;
}

const char *GateLFile::NextNeedle(size_t *needleNr, const char *pHaystack, size_t nHaystack)
{
	const size_t nNeedles = sizeof(fiNeedles) / sizeof(fiNeedles[0]);

	std::vector<const char *> fiNeedle(nNeedles);
	for (size_t needle = 0; needle < nNeedles; needle++)
	{
		fiNeedle[needle] = strstr(pHaystack, fiNeedles[needle]);
		if (fiNeedle[needle] > pHaystack + nHaystack)
		{
			fiNeedle[needle] = nullptr;
		}
	}

	const char *ret = pHaystack + nHaystack;
	for (size_t needle = 0; needle < nNeedles; needle++)
	{
		if ((nullptr != fiNeedle[needle]) && (fiNeedle[needle] < ret))
		{
			ret = fiNeedle[needle];
			*needleNr = needle;
		}
	}

	if (ret == pHaystack + nHaystack)
	{
		return nullptr;
	}

	// Needle inside comment?
	if (PosInsideComment(ret, pHaystack, pHaystack + nHaystack))
	{
		return NextNeedle(needleNr, ret + 1, nHaystack - (ret + 1 - pHaystack));
	}

	return ret;
}

/* searches before in for type declaration*/
GateLFile::signalType_t GateLFile::TypeGet(const char **declStart, const char *in, const char *inModuleStart)
{
	const char *typeStart[SIGNAL_TYPE_NROF];
	for (size_t type = 0; type < SIGNAL_TYPE_NROF; type++)
	{
		typeStart[type] = ParseUtils::strrstr(inModuleStart, in, SignalTypes[type]);
	}

	*declStart = inModuleStart;
	signalType_t ret = SIGNAL_TYPE_NROF;
	for (size_t type = 0; type < SIGNAL_TYPE_NROF; type++)
	{
		if ((nullptr != typeStart[type]) && (*declStart < typeStart[type]))
		{
			*declStart = typeStart[type];
			ret = (signalType_t)type;
		}
	}

	// Check if the type declaration belongs to signal
	if (SIGNAL_TYPE_NROF != ret)
	{
		const char *illegalSemiColon = strchr(*declStart, ';');
		if ((nullptr != illegalSemiColon) && (illegalSemiColon <= in))
		{
			return SIGNAL_TYPE_NROF; // This is not a declaration
		}
	}

	return ret;
}

const char *GateLFile::SignalDeclarationGet(const std::string &signalName, const char *inModuleStart)
{
	const char *moduleEnd = strstr(inModuleStart, "endmodule");
	if (nullptr == moduleEnd)
	{
		nfiError("Module without endmodule\n");
		return nullptr;
	}

	// find end of module input list
	const char *moduleInputEnd = strchr(inModuleStart, ')');

	if (nullptr == moduleInputEnd)
	{
		nfiError("Module input list doesn't end\n");
		return nullptr;
	}

	if (moduleInputEnd >= moduleEnd)
	{
		nfiError("Module input list doesn't end\n");
		return nullptr;
	}

	// Search declaration
	bool foundDecl = false;
	const char *signalIoDecl = nullptr;
	const char *signalDecl = moduleInputEnd;
	while (signalDecl < moduleEnd)
	{
		signalDecl = strstr(signalDecl, signalName.c_str());
		if (nullptr == signalDecl)
		{
			break;
		}

		if (!PosInsideComment(signalDecl, inModuleStart, moduleEnd))
		{
			const char *typeStart;
			const signalType_t type = TypeGet(&typeStart, signalDecl, inModuleStart);
			if ((SIGNAL_TYPE_INPUT == type) || (SIGNAL_TYPE_OUTPUT == type))
			{
				signalIoDecl = typeStart;
			}
			else if (SIGNAL_TYPE_NROF != type)
			{
				foundDecl = true;
				signalDecl = typeStart;
				break;
			}
		}

		signalDecl++;
	}

	if (!foundDecl && (nullptr != signalIoDecl))
	{
		signalDecl = signalIoDecl; // e.g. "input [a:b] signal" will default to wire
	}
	else if (!foundDecl)
	{
		nfiError("Could not find signal declaration\n");
		return nullptr;
	}

	nfiDebug("declaration '%.30s' (io decl '%.30s')\n", signalDecl, signalIoDecl != nullptr ? signalIoDecl : "N.A.");

	return signalDecl;
}

int GateLFile::FiEnableInputAdd(std::map<const char *, diff_t> *diff, const char *start, const char *stop)
{
	const char *ioEnd = strstr(start, ");");
	if (nullptr == ioEnd)
	{
		nfiError("Could not find end of io\n");
		return -1;
	}

	const char *singleSemiColon = strchr(start, ';');
	if ((nullptr != singleSemiColon) && (singleSemiColon < ioEnd))
	{
		nfiError("Unexpected ';'\n");
		return -1;
	}

	const char *replaceStart = ioEnd; // before );

	if (diff->end() != diff->find(replaceStart))
	{
		nfiError("ioEnd already diff'ed\n");
		return -1;
	}

	auto &diffIt = (*diff)[replaceStart];
	diffIt.Replacement = ", ";
	diffIt.Replacement += FiEnableStr;
	diffIt.Replacement += ");\n input ";
	diffIt.Replacement += FiEnableStr;
	diffIt.Replacement += ";\n wire ";
	diffIt.Replacement += FiEnableStr + std::string(";");

	diffIt.End = ioEnd + 2; // after ");"

	return 0;
}

int GateLFile::SubSignalWidthGet(const std::string &inSubSignal, const char *inModuleStart)
{
	// Extract signal name
	const char *widthStart = nullptr;
	if ('\\' != inSubSignal[0])
	{
		widthStart = strchr(inSubSignal.c_str(), '[');
	}
	else
	{
		widthStart = strstr(inSubSignal.c_str(), " [");
		if (nullptr != widthStart)
		{
			widthStart++; // i.e. point to '['
		}
	}

	if (nullptr == widthStart)
	{
		nfiError("Could not find '['\n");
		return -1;
	}

	const size_t nameLen = widthStart - inSubSignal.c_str();

	std::vector<char> signalName(nameLen + 1);
	memcpy(signalName.data(), inSubSignal.c_str(), nameLen);
	signalName[nameLen] = '\0';

	// Find signal declaration
	const char *signalDecl = SignalDeclarationGet(signalName.data(), inModuleStart);
	if (nullptr == signalDecl)
	{
		nfiError("SignalDeclarationGet failed\n");
		return -1;
	}

	signal_t signal;
	if (SignalDeclarationParse(&signal, signalDecl))
	{
		nfiError("SignalDeclarationParse failed\n");
		return -1;
	}

	// What width is subsignal
	const char *tmp;
	int arraySize = ParseUtils::subSignalArraySizeGet(&tmp, widthStart);
	if (0 >= arraySize)
	{
		nfiError("signalArraySizeGet failed\n");
		return -1;
	}

	// TODO: Only supporting single dimensional sub and arrays
	// Is signal an array?
	if (1 < signal.ElemCnt)
	{
		// Assume the sub is an array index
		if (arraySize > signal.ElemCnt)
		{
			nfiError("subsignal is larger than array declaration\n");
			return -1;
		}

		return arraySize * signal.Width;
	}

	if (arraySize > signal.Width)
	{
		nfiError("subsignal is larger than width declaration\n");
		return -1;
	}

	return arraySize;
}

int GateLFile::NeedleCorrupt(
	fiMode_t fiMode, module_t *module, std::map<const char *, diff_t> *diff,
	const std::string &fiPrefix, const char *moduleStart, const char *moduleEnd, const char *needle, fiNeedle_t needleNr)
{
	nfiDebug("Needle '%.30s'\n", needle);

	// Find target signal
	const char *targetSignalStart = nullptr;
	switch (needleNr)
	{
	case FI_NEEDLE_ASSIGN:
		targetSignalStart = needle + strlen(fiNeedles[FI_NEEDLE_ASSIGN]);
		break;

	case FI_NEEDLE_ASSIGN_NON_BLOCKIN:
	{
		const char *assigneeEnd = ParseUtils::lastNonSpaceGet(needle - 1);
		targetSignalStart = ParseUtils::lastCharAfterGet(assigneeEnd, "\n )", 3);
	}
	break;

	default:
		nfiError("Unknown fiNeedle_t %i\n", needleNr);
		return -1;
	}

	if ((nullptr == targetSignalStart) || (moduleStart > targetSignalStart))
	{
		nfiError("Couldn't find signal name\n");
		return -1;
	}

	// Remove spaces
	targetSignalStart = ParseUtils::firstNonSpaceGet(targetSignalStart);
	nfiDebug("Needle expression: %.70s\n", targetSignalStart);

	// Find end of signal name
	const char *targetSignalEndEqual = nullptr;
	switch (needleNr)
	{
	case FI_NEEDLE_ASSIGN:
		targetSignalEndEqual = strchr(targetSignalStart, '=');
		break;

	case FI_NEEDLE_ASSIGN_NON_BLOCKIN:
		targetSignalEndEqual = strchr(targetSignalStart, '<');
		break;

	// coverity[DEADCODE]
	default:
		nfiError("Unknown fiNeedle_t %i\n", needleNr);
		return -1;
	}

	if ((nullptr == targetSignalEndEqual) || (moduleEnd < targetSignalEndEqual))
	{
		nfiError("Couldn't find signal name\n");
		return -1;
	}

	// Remove spaces
	targetSignalEndEqual -= 1; // remove char found above
	targetSignalEndEqual = ParseUtils::lastNonSpaceGet(targetSignalEndEqual);

	const char *targetSignalEnd = targetSignalEndEqual + 1; // one after last char

	std::vector<char> targetSignalName(targetSignalEnd - targetSignalStart + 1);
	memcpy(targetSignalName.data(), targetSignalStart, targetSignalEnd - targetSignalStart);
	targetSignalName[targetSignalEnd - targetSignalStart] = '\0';

	// Is it a compound signal? I.e. {signal1, signal2,..}
	std::vector<std::string> signalNames;
	if ('{' == targetSignalName[0])
	{
		// Extract the signals
		const char *compoundEnd = strchr(targetSignalName.data(), '}');
		if (nullptr == compoundEnd)
		{
			nfiError("Compound signal that doesn't end\n");
			return -1;
		}

		const char *currSignalStart = targetSignalName.data() + 1;
		while (currSignalStart < compoundEnd)
		{
			currSignalStart = ParseUtils::firstNonSpaceGet(currSignalStart);

			const char *nextSignalStart = strchr(currSignalStart, ',');
			bool lastRound = false;
			if (nullptr == nextSignalStart)
			{
				lastRound = true;
				nextSignalStart = compoundEnd;
			}

			const char *currSignalEnd = nextSignalStart - 1;

			currSignalEnd = ParseUtils::lastNonSpaceGet(currSignalEnd);

			std::vector<char> tmpBuff(currSignalEnd + 1 - currSignalStart + 1);
			memcpy(tmpBuff.data(), currSignalStart, currSignalEnd + 1 - currSignalStart);
			tmpBuff[currSignalEnd + 1 - currSignalStart] = '\0';

			signalNames.push_back(tmpBuff.data());

			if (!lastRound)
			{
				currSignalStart = nextSignalStart + 1; // one after ','
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		signalNames.push_back(targetSignalName.data());
	}

	// Get required signal width
	int compoundSignalWidth = 0;

	for (size_t sigIndex = 0; sigIndex < signalNames.size(); sigIndex++)
	{
		if (200 < signalNames[sigIndex].size())
		{
			nfiError("signalName longer than 200 chars: %.200s\n", signalNames[sigIndex].c_str());
			return -1;
		}

		nfiDebug("targetSignal '%s'\n", signalNames[sigIndex].c_str());

		int fiSignalWidth;

		// Subsignal?
		// TODO: Subsignal with escape name?!?
		// if signal identifier starts with escape character, '[' is allowed as part of name.
		// In that case, an actual subsignal '[' will be separated by a space
		const char *targetSignalEndBracket = nullptr;
		if ('\\' != signalNames[sigIndex][0])
		{
			targetSignalEndBracket = strchr(signalNames[sigIndex].c_str(), '[');
		}
		else
		{
			targetSignalEndBracket = strstr(signalNames[sigIndex].c_str(), " [");
		}

		if (nullptr != targetSignalEndBracket)
		{
			fiSignalWidth = SubSignalWidthGet(signalNames[sigIndex].c_str(), moduleStart);
			if (0 >= fiSignalWidth)
			{
				nfiError("SubSignalWidthGet failed\n");
				return -1;
			}
		}
		else
		{
			// Find signal declaration
			const char *signalDecl = SignalDeclarationGet(signalNames[sigIndex].c_str(), moduleStart);
			if (nullptr == signalDecl)
			{
				nfiError("SignalDeclarationGet failed\n");
				return -1;
			}

			signal_t signal;
			if (SignalDeclarationParse(&signal, signalDecl))
			{
				nfiError("SignalDeclarationParse failed\n");
				return -1;
			}

			fiSignalWidth = signal.ElemCnt * signal.Width;
		}

		compoundSignalWidth += fiSignalWidth;

		nfiDebug("fiSignalWidth = %i\n", fiSignalWidth);
	}

	if (1 < signalNames.size())
	{
		nfiDebug("compoundSignalWidth = %i\n", compoundSignalWidth);
	}

	// Corrupt
	// Place corruption between equal sign and before semi-colon
	const char *equal = strchr(targetSignalStart, '=');
	if (nullptr == equal)
	{
		nfiError("No equal sign in assignment\n");
		return -1;
	}

	const char *semiColon = strchr(targetSignalStart, ';');
	if (nullptr == semiColon)
	{
		nfiError("Needle operation doesn't stop\n");
		return -1;
	}

	const char *newLine = strchr(targetSignalStart, '\n');
	if (nullptr == newLine)
	{
		nfiError("No new line after needle operation\n");
		return -1;
	}

	if (semiColon <= equal)
	{
		nfiError("Equal after semi-colon\n");
		return -1;
	}

	if (newLine <= semiColon)
	{
		nfiError("New line before semi-colon\n");
		return -1;
	}

	if (PosInsideComment(equal, moduleStart, semiColon))
	{
		nfiError("Equal sign within comment\n");
		return -1;
	}

	equal += 1; // don't need to replace equal sign

	// Create corruption string
	signal_t fiSignal;
	fiSignal.Type = SIGNAL_TYPE_WIRE;
	fiSignal.Width = compoundSignalWidth;
	fiSignal.ElemCnt = 1;

	fiSignal.Name = "fi_";
	for (const auto &name : signalNames)
	{
		fiSignal.Name += name;
	}

	fiSignal.UUID = uuidGet();

	module->FiSignal.push_back(fiSignal);

	if (diff->end() != diff->find(equal))
	{
		nfiError("Diff already created\n");
		return -1;
	}

	diff_t &diffElem = (*diff)[equal];
	diffElem.End = semiColon;

	// Original assignment
	std::vector<char> originalAssignment(semiColon - equal + 1);
	memcpy(originalAssignment.data(), equal, semiColon - equal);
	originalAssignment[semiColon - equal] = '\0';

	nfiDebug("orig. Ass: '%s'\n", originalAssignment.data());

	diffElem.Replacement = "(";
	diffElem.Replacement += originalAssignment.data();
	diffElem.Replacement += ")";

	switch (fiMode)
	{
	case FI_MODE_STUCK_HIGH:
		diffElem.Replacement += " | ";
		break;

	case FI_MODE_STUCK_LOW:
		diffElem.Replacement += " & ~";
		break;

	case FI_MODE_FLIP:
		diffElem.Replacement += " ^ ";
		break;

	default:
		nfiError("Unknown fiMode\n");
		return -1;
	}

	diffElem.Replacement += "((" + std::string(FiEnableStr);
	diffElem.Replacement += " && (" + std::to_string(fiSignal.UUID) + " == " + fiPrefix + GlobalFiNumber_ + ")) ? ";
#if FI_SINGLE_BIT
	diffElem.Replacement += "(" + std::to_string(fiSignal.Width) + "'d1 << " + fiPrefix + GlobalFiSignal_ + ")";
#else  // !FI_SINGLE_BIT
	diffElem.Replacement += fiPrefix + GlobalFiSignal_;
	if (1 == fiSignal.Width)
	{
		diffElem.Replacement += "[0]";
	}
	else
	{
		diffElem.Replacement += "[" + std::to_string(fiSignal.Width - 1) + ":0]";
	}
#endif // !FI_SINGLE_BIT

	diffElem.Replacement += " : {" + std::to_string(fiSignal.Width) + "{1'b0}})";

	nfiDebug("Replacement: '%s'\n", diffElem.Replacement.c_str());

	nfiDebug("\n\n\n");

	return 0;
}

const std::string &GateLFile::signalTypeStr(signalType_t type)
{
	static const std::string strings[SIGNAL_TYPE_NROF + 1] = {
		"SIGNAL_TYPE_WIRE",
		"SIGNAL_TYPE_REG",
		"SIGNAL_TYPE_INPUT",
		"SIGNAL_TYPE_OUTPUT",
		"Error: Unknown Signal type"};

	switch (type)
	{
	case SIGNAL_TYPE_WIRE:	// no break intended
	case SIGNAL_TYPE_REG:	// no break intended
	case SIGNAL_TYPE_INPUT: // no break intended
	case SIGNAL_TYPE_OUTPUT:
		return strings[type];

	default:
		nfiError("Unknown type\n");
		return strings[SIGNAL_TYPE_NROF];
	}

	// coverity[UNREACHABLE]
	return strings[SIGNAL_TYPE_NROF];
}

int GateLFile::LibraryCreate(const std::map<std::string, module_t> &modules, const std::string &topName)
{
	std::map<std::string, size_t> moduleOffsets;
	size_t offset = 0;

	for (auto it = modules.begin(); it != modules.end(); it++)
	{
		moduleOffsets[it->first] = offset;
		offset++;
	}

	// Continue with the actual business
	const std::string fileName = topName + FiSignalsLibraryNameAppend_;
	FILE *filep = fopen(fileName.c_str(), "w");
	if (NULL == filep)
	{
		nfiError("Failed to write-open %s\n", fileName.c_str());
		return -1;
	}

	std::string header;
	header = "\n// Auto-generated file by HDFIT.NetlistFaultInjector for top module " + topName + "\n\n";
	header += "#include \"netlistFaultInjector.hpp\"\n\n";

	header += "const std::vector<module_t> modules = {\n";

	if (0 >= fprintf(filep, "%s", header.c_str()))
	{
		nfiError("Writing to %s failed\n", fileName.c_str());
		fclose(filep);
		return -1;
	}

	// Print values
	for (const auto &module : modules)
	{
		std::string moduleNameNoEscape;
		ParseUtils::backslashToDoubleBackslash(&moduleNameNoEscape, module.first);

		std::string toPrint;
		toPrint += "\t{\n";									// start module
		toPrint += "\t\t\"" + moduleNameNoEscape + "\",\n"; // module name
		toPrint += "\t\t{\n";								// start std::vector<signal_t>
		for (const auto &signal : module.second.FiSignal)
		{
			toPrint += "\t\t\t{\n"; // start signal_t
			toPrint += "\t\t\t\t" + signalTypeStr(signal.Type) + ",\n";
			toPrint += "\t\t\t\t" + std::to_string(signal.Width) + ",\n";
			toPrint += "\t\t\t\t" + std::to_string(signal.ElemCnt) + ",\n";
			toPrint += "\t\t\t\t" + std::to_string(signal.UUID) + ",\n";
			toPrint += "\t\t\t},\n"; // end signal_t
		}

		toPrint += "\t\t},\n"; // stop std::vector<signal_t>

		toPrint += "\t\t{\n"; // start std::vector<std::pair<size_t, size_t>> InstanceUuids
		for (const auto &instance : module.second.InstanceUuids)
		{
			toPrint += "\t\t\t{" + std::to_string(moduleOffsets[((module_t *)instance.first)->Name]) + ", " + std::to_string(instance.second) + "},\n";
		}
		toPrint += "\t\t}\n"; // stop std::vector<std::pair<size_t, size_t>> InstanceUuids
		toPrint += "\t},\n";  // end module

		if (0 >= fprintf(filep, "%s", toPrint.c_str()))
		{
			nfiError("Writing to %s failed\n", fileName.c_str());
			fclose(filep);
			return -1;
		}
	}

	std::string footer;
	footer += "}; // modules\n\n";

	footer += "const size_t modulesTopIndex = " + std::to_string(moduleOffsets[topName]) + ";\n\n";
	footer += "const size_t modulesTopUUID = " + std::to_string(GlobalFiModInstNumberTop_) + ";\n\n";

	if (0 >= fprintf(filep, "%s", footer.c_str()))
	{
		nfiError("Writing to %s failed\n", fileName.c_str());
		fclose(filep);
		return -1;
	}

	if (fclose(filep))
	{
		nfiError("Closing Distribution Export file failed\n");
		return -1;
	}

	return 0;
}

int GateLFile::HierarchyDepthGet(const std::map<std::string, module_t> &modules, const std::string &topName)
{
	auto topIt = modules.find(topName);
	if (modules.end() == topIt)
	{
		nfiError("Could not find module %s\n", topName.c_str());
		return -1;
	}

	// Get depth of each instance
	// Or stop recursion if module contains no instances
	std::vector<int> instDepth(topIt->second.InstanceUuids.size());
	for (size_t inst = 0; inst < topIt->second.InstanceUuids.size(); inst++)
	{
		const module_t *modPt = (module_t *)topIt->second.InstanceUuids[inst].first;
		instDepth[inst] = HierarchyDepthGet(modules, modPt->Name);

		if (0 >= instDepth[inst])
		{
			nfiError("HierarchyDepthGet failed\n");
			return -1;
		}
	}

	// find deepest depth
	int deepestDepth = 0;
	for (const auto &depth : instDepth)
	{
		if (deepestDepth < depth)
		{
			deepestDepth = depth;
		}
	}

	return deepestDepth + 1; // adding itself
}
// Returns negative on error, positive when module was found
// start	Pointer to module start (after name)
// end		Pointer to module end (after "endmodule")
int GateLFile::ModuleFind(std::string *name, const char **start, const char **end, const char *pFile, size_t nFile)
{
	const char modStartStr[] = "module ";
	const char modEndStr[] = "endmodule";

	const char *modStart = strstr(pFile, modStartStr); // TODO: no strnstr?!?
	if (nullptr == modStart)
	{
		return 0; // no module in this file
	}

	modStart += sizeof(modStartStr) - 1; // -1 for '\0'

	// Module name either ends with # or with (
	const char *modNameEndHash = strchr(modStart, '#');
	const char *modNameEndParenth = strchr(modStart, '(');

	const char *modNameEnd = NULL;
	if ((NULL != modNameEndHash) && (NULL != modNameEndParenth))
	{
		modNameEnd = modNameEndHash < modNameEndParenth ? modNameEndHash : modNameEndParenth;
	}
	else if (NULL != modNameEndHash)
	{
		modNameEnd = modNameEndHash;
	}
	else if (NULL != modNameEndParenth)
	{
		modNameEnd = modNameEndParenth;
	}
	else
	{
		nfiError("Could not find module name in %.30s\n", modStart);
		return -5;
	}

	if (modNameEnd >= pFile + nFile)
	{
		nfiError("Module name ends after file in %.30s\n", modStart);
		return -6;
	}

	// Remove whitespace
	while (modStart < modNameEnd)
	{
		if (' ' != *modStart)
		{
			break;
		}
		modStart++;
	}

	modNameEnd--; // don't be one after end
	while (modStart < modNameEnd)
	{
		if ((' ' != *modNameEnd) && ('\n' != *modNameEnd) &&
			('\r' != *modNameEnd))
		{
			break;
		}
		modNameEnd--;
	}
	modNameEnd++; // be one after end again

	// Module inside comment? Impossible, it is a post synthesis
	// if(PosInsideComment(modStart, pFile, pFile + nFile))
	//{
	//	return ModuleFind(name, start, end, modStart, nFile - (modStart - pFile));
	//}

	// Save module name
	if (modNameEnd == modStart)
	{
		nfiError("Empty module name\n");
		return -6;
	}

	char tmpName[200];
	if (sizeof(tmpName) < modNameEnd - modStart + 1) // +1 for '\0'
	{
		nfiError("Module name '%.30s' too large\n", modStart);
		return -7;
	}

	memcpy(tmpName, modStart, modNameEnd - modStart);
	tmpName[modNameEnd - modStart] = '\0';

	*name = tmpName;
	*start = modNameEnd;

	// Find module end
	const char *modEnd = strstr(pFile, modEndStr); // TODO: no strnstr?!?
	if (nullptr == modEnd)
	{
		nfiError("module doesn't end: %s\n", name->c_str());
		return -1;
	}

	modEnd += sizeof(modEndStr) - 1; // -1 for '\0'

	if (modEnd >= pFile + nFile)
	{
		nfiError("module ends after end of file: %s\n", name->c_str());
		return -2;
	}

	if (nullptr != ParseUtils::strrstr(modNameEnd, modEnd, modStartStr))
	{
		nfiError("Module declaration inside module: %s\n", name->c_str());
		return -3;
	}

	*end = modEnd;

	return 1;
}

int GateLFile::DiffApply(std::map<const char *, diff_t> &diff)
{

	// check that there is no overlapping diff
	auto lastIt = diff.begin();
	for (auto it = diff.begin(); it != diff.end(); it++)
	{
		if (it->first > it->second.End)
		{
			nfiError("Diff end before beginning\n");
			return -1;
		}

		if (it != diff.begin())
		{
			if (it->first <= lastIt->second.End)
			{
				nfiError("Overlapping diff\n");
				return -1;
			}
		}

		lastIt = it;
	}

	// Calculate new file size
	long fileSizeDiff = 0;
	for (auto it = diff.begin(); it != diff.end(); it++)
	{
		const long originalSize = it->second.End - it->first;
		const long newSize = it->second.Replacement.size();

		fileSizeDiff += newSize - originalSize;
	}

	nfiDebug("fileSizeDiff = %li\n", fileSizeDiff);

	// Create new content
	const size_t newContentSize = Size_ + fileSizeDiff;
	char *newContent = (char *)malloc(newContentSize);
	if (nullptr == newContent)
	{
		nfiError("malloc failed\n");
		return -1;
	}

	// Reverse iterate through map to apply diff
	newContent[newContentSize - 1] = '\0';
	char *nextWritePosNew = &newContent[newContentSize - 2];
	const char *lastReadPosOld = &Content_[Size_ - 1];
	for (auto it = diff.rbegin(); it != diff.rend(); it++)
	{
		// Copy old to new
		if (lastReadPosOld <= it->second.End)
		{
			nfiError("last Read position too large\n");
			free(newContent);
			return -1;
		}
		const size_t oldToCopySize = lastReadPosOld - it->second.End;

		nextWritePosNew -= oldToCopySize - 1;
		if (newContent > nextWritePosNew)
		{
			nfiError("Copy size too large\n");
			free(newContent);
			return -1;
		}
		memcpy(nextWritePosNew, lastReadPosOld - oldToCopySize, oldToCopySize);

		// Copy diff
		nextWritePosNew -= it->second.Replacement.size();
		if (newContent > nextWritePosNew)
		{
			nfiError("diff size too large\n");
			free(newContent);
			return -1;
		}
		memcpy(nextWritePosNew, it->second.Replacement.c_str(), it->second.Replacement.size());

		nextWritePosNew--;
		lastReadPosOld = it->first;
	}

	// Copy old to new
	if (lastReadPosOld <= Content_)
	{
		nfiError("last Read position too large\n");
		free(newContent);
		return -1;
	}
	const size_t oldToCopySize = lastReadPosOld - Content_;

	nextWritePosNew -= oldToCopySize - 1;
	if (newContent != nextWritePosNew)
	{
		nfiError("Not at beginning of new content by end of diff application\n");
		free(newContent);
		return -1;
	}

	const char *oldContentBeginning = lastReadPosOld - oldToCopySize;
	if (Content_ != oldContentBeginning)
	{
		nfiError("Not at beginning of old content by end of diff application\n");
		free(newContent);
		return -1;
	}

	memcpy(nextWritePosNew, oldContentBeginning, oldToCopySize);

	free(Content_);
	Content_ = newContent;
	Size_ = newContentSize;

	return 0;
}

int GateLFile::ModuleFi(
	bool isTop, const std::string &fiPrefix,
	fiMode_t fiMode,
	module_t *module, std::map<const char *, diff_t> *diff,
	const char *start, const char *stop)
{
	// Add Fi enable wire to inputs
	if (!isTop)
	{
		if (FiEnableInputAdd(diff, start, stop))
		{
			nfiError("FiEnableInputAdd failed\n");
			return -1;
		}
	}

	// Find all fi needles and add corruption
	const char *pos = start;
	do
	{
		size_t needleNr;
		const char *needle = NextNeedle(&needleNr, pos, stop - pos);
		if (nullptr == needle)
		{
			break; // no more needles
		}

		if (NeedleCorrupt(fiMode, module, diff, fiPrefix, start, stop, needle, (fiNeedle_t)needleNr))
		{
			nfiError("NeedleCorrupt failed\n");
			return -1;
		}

		// prepare next loop
		pos = needle + 1;

	} while (pos < stop);

	return 0;
}

bool GateLFile::PosInsideComment(const char *pos, const char *start, const char *end)
{
	// Inside block comment?
	for (size_t blockType = 0; blockType < sizeof(blockCommentStartStr) / sizeof(blockCommentStartStr[0]); blockType++)
	{
		// Find next block end
		const char *blockEnd = strstr(pos, blockCommentEndStr[blockType]);
		if ((nullptr != blockEnd) && (blockEnd < end))
		{
			// Is that blockEnd opened before pos?
			const char *blockStart = ParseUtils::strrstr(pos, blockEnd, blockCommentStartStr[blockType]);
			if (nullptr == blockStart)
			{
				return true;
			}
		}
	}

	// Inside line comment?
	const char *currPos = pos;
	do
	{
		if ('\n' == *currPos)
		{
			break;
		}

		if (('/' == *currPos) && ('/' == *(currPos - 1)))
		{
			return true;
		}

		currPos--;

	} while (currPos > start);

	return false;
}

int GateLFile::FiSignalsCreate(fiMode_t fiMode, const int threads)
{
	if (nullptr == Content_)
	{
		nfiError("Nullptr\n");
		return -1;
	}
	nfiDebug("Create fi signals for all modules\n");
	omp_set_num_threads(threads);
	// Get all module names
	int i;
	std::map<std::string, module_t> modules; // <module name, module_t> // TODO: Does this really need to be a map?
	const int CHUNCK_SIZE = ((int)Size_ / threads);
#ifdef VERBOSE_DEBUG
	int modulesCnt = 0;
#endif /*VERBOSE_DEBUG*/

	// TODO parallel check of errors

#pragma omp parallel for
	for (i = 0; i < threads; i++)
	{
		const char *Content_start = Content_ + CHUNCK_SIZE * i;
		const char *filePos = Content_ + CHUNCK_SIZE * i;
		const char *lineStart = Content_ + CHUNCK_SIZE * i;
		do
		{
			nfiDebug("Finding next module from thread %d\n", omp_get_thread_num());
			while (*filePos != '\n' && *filePos != '\0')
			{
				++filePos;
			}
			std::string line(lineStart, filePos - lineStart);

			std::string moduleName;
			const char *moduleStart;
			const char *moduleEnd;
			const int modFindRet = ModuleFind(
				&moduleName, &moduleStart, &moduleEnd,
				lineStart, Size_ - (lineStart - Content_));

			if (0 > modFindRet)
			{
				nfiError("moduleFind failed from thread %d\n", omp_get_thread_num());
				// return -1;
			}
			else if (0 == modFindRet)
			{
				break;
			}

			if (200 < moduleName.size())
			{
				nfiError("Module name longer than 200 chars: %.200s from thread %d\n", moduleName.c_str(), omp_get_thread_num());
				// return -1;
			}

			nfiDebug("\tFound module %s\n", moduleName.c_str());

			if (modules.end() != modules.find(moduleName))
			{
				// deprecated to warning since it may happen with parallel processing of the file
				nfiWarning("Module already in modules from thread %d\n", omp_get_thread_num());
				// return -1;
			}
			else
			{
#pragma omp critical
				{
					modules[moduleName].Name = moduleName;
					// lets save start and end
					modules[moduleName].start = moduleStart;
					modules[moduleName].end = moduleEnd;
#ifdef VERBOSE_DEBUG
					modulesCnt++;
#endif /*VERBOSE_DEBUG*/
				}
			}
			// Prepare next round
			filePos = moduleEnd;
			filePos++;
			lineStart = filePos;

		} while (filePos < Content_start + CHUNCK_SIZE);
	} // omp parallel for

	nfiDebug("Number of Modules find %d\n", modulesCnt);

	nfiDebug("Hiearachy visit\n");

#pragma omp parallel for
	for (i = 0; i < threads; i++)
	{
		const char *Content_start = Content_ + CHUNCK_SIZE * i;
		const char *filePos = Content_ + CHUNCK_SIZE * i;
		// assign for each thread a given number of modules
		int chunkSize = modules.size() / threads;

		auto startIt = std::next(modules.begin(), threads * chunkSize);
		auto endIt = (omp_get_thread_num() == threads - 1) ? modules.end() : std::next(startIt, chunkSize);

		// Use the assigned portions
		for (auto upp_module = startIt; upp_module != endIt; upp_module++)
		{

			const char *moduleStart = upp_module->second.start;
			const char *moduleEnd = upp_module->second.end;
			const std::string moduleName = upp_module->first;
			nfiDebug("Looking for instances of %s from thread %d and from char %ld to %ld in file\n", moduleName.c_str(), omp_get_thread_num(), Content_start - Content_, Content_start - Content_ + CHUNCK_SIZE);
			auto modIt = modules.find(moduleName);
			if (modules.end() == modIt)
			{
				nfiError("No such module in list from thread %d\n", omp_get_thread_num());
				// return -1;
			}

			// Find module instantiations in the chunck
			do
			{

				// Find next instantiation
				const char *instStart = strstr(filePos, upp_module->first.c_str());
				if ((nullptr == instStart) || (moduleEnd < instStart))
				{
					break; // no more instances of this module in this module
				}

				if (' ' != *(instStart + upp_module->first.size()) || // space between module name and instance name
					PosInsideComment(instStart, moduleStart, moduleEnd))
				{
#pragma omp critical
					{
						upp_module->second.instancesList.push_back(instStart);
					}
					filePos = instStart + 1;
				}

				nfiDebug("\tFound instance of '%s' from thread %d at char %ld\n", upp_module->first.c_str(), omp_get_thread_num(),filePos - Content_);
#pragma omp critical
				{
					modIt->second.InstanceUuids.push_back({&upp_module, 0}); // TODO: Dirty - UUIDs are set further below
				}
				filePos = instStart + upp_module->first.size();

				// filePos++;

			} while (filePos < Content_start + CHUNCK_SIZE);
		}

	} // omp parallel for

#ifdef VERBOSE_DEBUG
	nfiDebug("Printing instances for modules");

	for (auto &module : modules)
	{
		nfiDebug("Module %s \n", module.first.c_str());
		nfiDebug("Found instances: %ld\n", module.second.instancesList.size());
	}

#endif /*VERBOSE_DEBUG*/
	if (nfiErrorCnt)
	{
		nfiFatal("There were %lu errors\n", nfiErrorCnt);
	}
	const int hierarchyDepth = HierarchyDepthGet(modules, TopModule_);
	if (0 >= hierarchyDepth)
	{
		nfiError("HierarchyDepthGet failed\n");
		return -1;
	}

	nfiDebug("hierarchyDepth = %i\n", hierarchyDepth);

	// TODO: This is dirty
	// clear instances from modules...will be filled again below with UUIDs
	for (auto &module : modules)
	{
		module.second.InstanceUuids.clear();
	}

	nfiDebug("Starting netlist modification for Fault injections ");

	// Add fiEnable to each module's input and corruption signal to all assignments
	std::map<const char *, diff_t> diff; // <beginning of replace, replacement>

#pragma omp parallel for
	for (i = 0; i < threads; i++)
	{
		/*iterate in parallel over a chunck and for each module add fi signals*/
		const char *Content_start = Content_ + CHUNCK_SIZE * i;
		const char *filePos = Content_ + CHUNCK_SIZE * i;
		const char *lineStart = Content_ + CHUNCK_SIZE * i;
		do
		{
			while (*filePos != '\n' && *filePos != '\0')
			{
				++filePos;
			}
			std::string line(lineStart, filePos - lineStart);

			std::string moduleName;
			const char *moduleStart;
			const char *moduleEnd;
			const int modFindRet = ModuleFind(
				&moduleName, &moduleStart, &moduleEnd,
				lineStart, Size_ - (lineStart - Content_));

			if (0 > modFindRet)
			{
				nfiError("moduleFind failed from thread %d\n", omp_get_thread_num());
				// return -1;
			}
			else if (0 == modFindRet)
			{
				break;
			}

			if (200 < moduleName.size())
			{
				nfiError("Module name longer than 200 chars: %.200s from thread %d\n", moduleName.c_str(), omp_get_thread_num());
				// return -1;
			}

			nfiDebug("Insert FI for module %s from thread %d\n", moduleName.c_str(), omp_get_thread_num());

			const bool moduleIsTop = (moduleName == TopModule_);

			const std::string fiPrefix = moduleIsTop ? "" : TopModule_ + ".";

			if (ModuleFi(moduleIsTop, fiPrefix, fiMode, &modules[moduleName], &diff, moduleStart, moduleEnd))
			{
				nfiError("moduleFi failed\n");
				// return -1;
			}

			// Prepare next round
			filePos = moduleEnd;
			filePos++;
			lineStart = filePos;

		} while (filePos < Content_start + CHUNCK_SIZE);
	} // omp parallel for

	// Apply Diff
	if (DiffApply(diff))
	{
		nfiError("DiffApply failed\n");
		return -1;
	}

	diff.clear();

#if FI_SINGLE_BIT
	const size_t largestWidth = 16;
#else  // !FI_SINGLE_BIT
	// Get largest fi signal
	size_t largestWidth = 0;
	for (const auto &module : modules)
	{
		for (const auto &signal : module.second.FiSignal)
		{
			if (largestWidth < signal.Width)
			{
				largestWidth = signal.Width;
			}
		}
	}
	// TODO for gate level it is probably 1 (single wire fault injection)
	nfiDebug("Largest signal: %lu\n", largestWidth);
#endif // FI_SINGLE_BIT

	// Apply Diff
	if (DiffApply(diff))
	{
		nfiError("DiffApply failed\n");
		return -1;
	}

	diff.clear();

	// Create library with module hierarchy etc.
	if (LibraryCreate(modules, TopModule_))
	{
		nfiError("libraryCreate failed\n");
		return -1;
	}

	return 0;
}

int GateLFile::WriteBack() const
{
	FILE *pFile = fopen(Name_.c_str(), "w");
	if (nullptr == pFile)
	{
		nfiError("failed to open file %s\n", Name_.c_str());
		return -1;
	}

	fwrite(Content_, 1, Size_ - 1, pFile); // don't write '\0' back

	if (fclose(pFile))
	{
		nfiError("fclose failed\n");
		return -1;
	}

	return 0;
}
