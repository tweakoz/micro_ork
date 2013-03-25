///////////////////////////////////////////////////////////////////////////////
// MicroOrk (Orkid)
// Copyright 1996-2013, Michael T. Mayers
// Provided under the MIT License (see LICENSE.txt)
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>
#include <vector>

namespace ork
{
	std::string FormatString( const char* formatstring, ... );
	void SplitString(const std::string& s, char delim, std::vector<std::string>& tokens);
	std::vector<std::string> SplitString(const std::string& instr, char delim);
}