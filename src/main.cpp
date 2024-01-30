/*
 * Copyright (C) 2022 Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as published
 * by the Free Software Foundation; either version 3 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "RtlFile.h"

#include "GateLFile.h"

#include "common.h"
#include <getopt.h>
#include <sstream> 

size_t nfiErrorCnt = 0;

typedef enum
{
	RTL,
	GATE,
} FileType_T;

typedef struct
{
	std::string File;
	std::string TopModule;
	FileType_T FileType;
	std::string LibFile;
	int threads = 1;
	bool dontTouchSignalsPresent = false;
	std::vector<std::string> dontTouchSignalsFileList;
} userConfig_t;

static std::string Usage(void)
{
	std::string usage = "Usage of netlistFaultInjector\n\
		-i --input-file input verilog file to inject signals\n\
		-t --top top module to inject signals in\n\
		-gate/RTL verilog file type\n\
		-l --lib-file library verilog file \n\
		-d --dont-touch use dont touch signals for fault injections\n\
		-f --file-signals list of files separated by , in which are defined dont touch signals name (per line in txt) or stil definition for scan chain\n";

	return usage;
}

static int argParse(userConfig_t *config, int argc, char **argv)
{
	int c;
	std::istringstream iss;
	std::string token;
	static struct option long_options[] = {
		{"input-file", required_argument, 0, 'i'},
		{"gate", no_argument, 0, 2},
		{"rtl", no_argument, 0, 1},
		{"top", required_argument, 0, 'm'},
		{"lib-file", required_argument, 0, 'l'},
		{"threads", required_argument, 0, 't'},
		{"dont-touch", no_argument, 0, 'd'},
		{"file-signals", required_argument, 0, 'f'},
		{0, 0, 0, 0}};

	while (1)
	{
		int option_index;
		c = getopt_long(argc, argv, "i:m:l:t:df:",
						long_options, &option_index);
		if (c == -1)
			break;

		switch (c)
		{
		case 'i':
			/*input file */
			config->File = optarg;
			break;
		case 1:
			/*rtl*/
			config->FileType = RTL;
			break;

		case 2:
			/*gate*/
			config->FileType = GATE;

			break;
		case 'm':
			/*top*/
			config->TopModule = optarg;
			break;
		case 'l':
			config->LibFile = optarg;
			break;
		case 't':
			config->threads = std::stoi(optarg);
			break;
		case 'd':
			config->dontTouchSignalsPresent = true;
			break;
		case 'f':
			iss.str(optarg);
			while (std::getline(iss, token, ','))
			{
				config->dontTouchSignalsFileList.push_back(token);
			}
			break;
		default:
		case '?':
			nfiError("%s\n", Usage().c_str());
			return -1;
		}
	}

	if (optind < argc)
	{
		nfiError("Non-option ARGV-elements\n");
		return -1;
	}

	/*error check */
	if (config->FileType == RTL && config->LibFile != "")
	{
		nfiWarning("Warning! Passed Library file in RTL parsing type\n");
	}
	else if (config->FileType == GATE && config->dontTouchSignalsFileList.size() < 1)
	{
		nfiFatal("Error! No dont touch signals file defined in Gate parsing type\n");
		return -1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	userConfig_t userConfig;

	if (argParse(&userConfig, argc, argv))
	{
		nfiFatal("argParse failed\n");
	}

	if (userConfig.FileType == GATE)
	{
		// get files
		GateLFile gateLFile;
		if (gateLFile.Get(userConfig.File.c_str(), userConfig.TopModule))
		{
			nfiFatal("fileGet failed\n");
		}
			if (userConfig.dontTouchSignalsPresent){
				if(gateLFile.AddDontTouch(userConfig.dontTouchSignalsFileList)){
					nfiFatal("Dont Touch files failed\n");
				}

			}
		// todo probably to remove
		if (gateLFile.GetLib(userConfig.LibFile.c_str()))
		{
			nfiFatal("LibfileGet failed\n");
		}

		if (gateLFile.FiSignalsCreate(GateLFile::FI_MODE_FLIP, userConfig.threads)) // TODO fault injection can become a class
		{
			nfiFatal("Failed to insert FiSignals\n");
		}
		if (gateLFile.WriteBack())
		{
			nfiFatal("WriteBack failed\n");
		}
		if (nfiErrorCnt)
		{
			nfiFatal("There were %lu errors\n", nfiErrorCnt);
		}
	}
	else
	{
		// Get file
		RtlFile rtlFile;
		if (rtlFile.Get(userConfig.File.c_str(), userConfig.TopModule))
		{
			nfiFatal("fileGet failed\n");
		}

		if (rtlFile.FiSignalsCreate(RtlFile::FI_MODE_FLIP))
		{
			nfiFatal("Failed to insert FiSignals\n");
		}
		if (rtlFile.WriteBack())
		{
			nfiFatal("WriteBack failed\n");
		}
		if (nfiErrorCnt)
		{
			nfiFatal("There were %lu errors\n", nfiErrorCnt);
		}
	}

	return 0;
}
