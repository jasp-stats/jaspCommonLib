//
// Copyright (C) 2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


//
//	NOTICE:
//		`appinfo.cpp` is generated from `appinfo.cpp.in` and you should edit 
//		that file instead if you want your changes to reflect in the app
//

#include "appinfo.h"
#include <sstream> 

const Version AppInfo::version = Version(0, 16, 4, 0);
const std::string AppInfo::name = "JASP";
const std::string AppInfo::builddate = __DATE__ " " __TIME__ " (Netherlands)" ;

const std::string AppInfo::gitBranch = "commonData";
const std::string AppInfo::gitCommit = "83a6d1b71bbe4ef4a4f758b412e24eecc6228276";

std::string AppInfo::getShortDesc()
{
	return AppInfo::name + " " + AppInfo::version.asString();
}

std::string AppInfo::getBuildYear()
{
	std::string datum = __DATE__;
	return datum.substr(datum.length() - 4);
}

std::string AppInfo::getRVersion()
{
	return "4.2";
}

std::string AppInfo::getRDirName()
{
	return "4.2-arm64";
}

std::string AppInfo::getSigningIdentity()
{
	return "AWJJ3YVK9B";
}

std::string AppInfo::getArchLabel()
{
	return "Apple Silicon";
}

long long AppInfo::getSimpleCryptKey()
{
	return 0x0c2ad4a4acb9f023;
}
