//
// Copyright (C) 2013-2018 University of Amsterdam
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

#include "columnencoder.h"
#include "stringutils.h"
#include <regex>
#ifdef BUILDING_JASP
#include "log.h"
#define LOGGER Log::log()
#else
#include <Rcpp.h>
#define LOGGER Rcpp::Rcout
#endif



ColumnEncoder				*	ColumnEncoder::_columnEncoder				= nullptr;
std::set<ColumnEncoder*>	*	ColumnEncoder::_otherEncoders				= nullptr;
bool							ColumnEncoder::_encodingMapInvalidated		= true;
bool							ColumnEncoder::_decodingMapInvalidated		= true;
bool							ColumnEncoder::_decodingTypeInvalidated		= true;
bool							ColumnEncoder::_decoSafeMapInvalidated		= true;
bool							ColumnEncoder::_originalNamesInvalidated	= true;
bool							ColumnEncoder::_encodedNamesInvalidated		= true;


ColumnEncoder * ColumnEncoder::columnEncoder()
{
	if(!_columnEncoder)
		_columnEncoder = new ColumnEncoder();

	return _columnEncoder;
}

void ColumnEncoder::invalidateAll()
{
	_encodingMapInvalidated		= true;
	_decodingMapInvalidated		= true;
	_decodingTypeInvalidated	= true;
	_decoSafeMapInvalidated		= true;
	_originalNamesInvalidated	= true;
	_encodedNamesInvalidated	= true;
}

ColumnEncoder::ColumnEncoder(std::string prefix, std::string postfix)
	: _encodePrefix(prefix), _encodePostfix(postfix)
{
	if(!_otherEncoders)
	{
		_otherEncoders = new ColumnEncoder::ColumnEncoders();
		invalidateAll();
	}

	_otherEncoders->insert(this);
}

ColumnEncoder::ColumnEncoder(const std::map<std::string, std::string> & decodeDifferently)
	: _encodePrefix("JASPColumn_"), _encodePostfix("_For_Replacement")
{

	std::vector<std::string> originalNames;
	originalNames.reserve(decodeDifferently.size());

	for(const auto & oriNew : decodeDifferently)
		originalNames.push_back(oriNew.first);

	setCurrentNames(originalNames);

	for(const std::string & encodedName : _encodedNames)
		if(decodeDifferently.count(_decodingMap[encodedName]) > 0)
			_decodingMap[encodedName] = decodeDifferently.at(_decodingMap[encodedName]);
}

ColumnEncoder::~ColumnEncoder()
{
	if(this != _columnEncoder)
	{
		if(_otherEncoders && _otherEncoders->count(this) > 0) //The special "replacer-encoder" doesn't add itself to otherEncoders.
			_otherEncoders->erase(this);
	}
	else
	{
		_columnEncoder = nullptr;

		ColumnEncoders others = *_otherEncoders;

		for(ColumnEncoder * colEnc : others)
			delete colEnc;

		if(_otherEncoders->size() > 0)
			LOGGER << "Something went wrong removing other ColumnEncoders..." << std::endl;

		delete _otherEncoders;
		_otherEncoders = nullptr;

		invalidateAll();
	}
}

std::string ColumnEncoder::encode(const std::string &in)
{
	if(in == "") return "";

	if(encodingMap().count(in) == 0)
		throw std::runtime_error("Trying to encode columnName but '" + in + "' is not a columnName!");

	return encodingMap().at(in);
}

std::string ColumnEncoder::decode(const std::string &in)
{
	if(in == "") return "";

	if(decodingMap().count(in) == 0)
		throw std::runtime_error("Trying to decode columnName but '" + in + "' is not an encoded columnName!");

	return decodingMap().at(in);
}

columnType ColumnEncoder::columnTypeFromEncoded(const std::string &in)
{
	if(in == "" || decodingTypes().count(in) == 0) 
		return columnType::unknown;
	
	return decodingTypes().at(in);
}

void ColumnEncoder::setCurrentNames(const std::vector<std::string> & names, bool generateTypesEncoding)
{
	//LOGGER << "ColumnEncoder::setCurrentNames(#"<< names.size() << ")" << std::endl;

	_encodingMap.clear();
	_decodingMap.clear();

	_encodedNames.clear();
	_encodedNames.reserve(names.size());
	
	size_t runningCounter = 0;
	
	_originalNames = names;

	//First normal encoding decoding: (Although im not sure we would ever need those again?)
	for(size_t col = 0; col < names.size(); col++)
	{
		std::string newName			= _encodePrefix + std::to_string(runningCounter++) + _encodePostfix; //Slightly weird (but R-syntactically valid) name to avoid collisions with user stuff.
		_encodingMap[names[col]]	= newName;
		_decodingMap[newName]		= names[col];

		_encodedNames.push_back(newName);
	}
	
	if(generateTypesEncoding)
		for(size_t col = 0; col < names.size(); col++)
			for(columnType colType : { columnType::scale, columnType::ordinal, columnType::nominal })
				{
					std::string qualifiedName	= names[col] + "." + columnTypeToString(colType),
								newName			= _encodePrefix + std::to_string(runningCounter++) + _encodePostfix; //Slightly weird (but R-syntactically valid) name to avoid collisions with user stuff.
					_encodingMap[qualifiedName]	= newName;
					_decodingMap[newName]		= names[col]; //Decoding is back to the actual name in the data!
					_decodingTypes[newName]		= colType;
			
					_encodedNames	.push_back(newName);
					_originalNames	.push_back(qualifiedName);
				}

	
	sortVectorBigToSmall(_originalNames);
	invalidateAll();
}

void ColumnEncoder::sortVectorBigToSmall(std::vector<std::string> & vec)
{
	std::sort(vec.begin(), vec.end(), [](std::string & a, std::string & b) { return a.size() > b.size(); }); //We need this to make sure smaller columnNames do not bite chunks off of larger ones
}

const ColumnEncoder::colMap	&	ColumnEncoder::encodingMap()
{
	static ColumnEncoder::colMap map;

	if(_encodingMapInvalidated)
	{
		map = _columnEncoder->_encodingMap;

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const auto & keyVal : other->_encodingMap)
					if(map.count(keyVal.first) == 0)
						map[keyVal.first] = keyVal.second;

		_encodingMapInvalidated = false;
	}

	return map;
}

const ColumnEncoder::colMap	&	ColumnEncoder::decodingMap()
{
	static ColumnEncoder::colMap map;

	if(_decodingMapInvalidated)
	{
		map = _columnEncoder->_decodingMap;

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const auto & keyVal : other->_decodingMap)
					if(map.count(keyVal.first) == 0)
						map[keyVal.first] = keyVal.second;

		_decodingMapInvalidated = false;
	}

	return map;
}

const ColumnEncoder::colTypeMap &ColumnEncoder::decodingTypes()
{
	static ColumnEncoder::colTypeMap map;

	if(_decodingTypeInvalidated)
	{
		map = _columnEncoder->_decodingTypes;

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const auto & keyVal : other->_decodingTypes)
					if(map.count(keyVal.first) == 0)
						map[keyVal.first] = keyVal.second;

		_decodingTypeInvalidated = false;
	}

	return map;
}


const ColumnEncoder::colMap	&	ColumnEncoder::decodingMapSafeHtml()
{
	static ColumnEncoder::colMap map;

	if(_decoSafeMapInvalidated)
	{
		map.clear();
		
		for(const auto & keyVal : _columnEncoder->_decodingMap)
			if(map.count(keyVal.first) == 0)
				map[keyVal.first] = stringUtils::escapeHtmlStuff(keyVal.second, true);

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const auto & keyVal : other->_decodingMap)
					if(map.count(keyVal.first) == 0)
						map[keyVal.first] = stringUtils::escapeHtmlStuff(keyVal.second, true); // replace square brackets for https://github.com/jasp-stats/jasp-issues/issues/2625

		_decoSafeMapInvalidated = false;
	}

	return map;
}

const ColumnEncoder::colVec	&	ColumnEncoder::originalNames()
{
	static ColumnEncoder::colVec vec;

	if(_originalNamesInvalidated)
	{
		vec = _columnEncoder->_originalNames;

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const std::string & name : other->_originalNames)
					vec.push_back(name);

		_originalNamesInvalidated = false;
	}

	sortVectorBigToSmall(vec);

	return vec;
}

const ColumnEncoder::colVec	&	ColumnEncoder::encodedNames()
{
	static ColumnEncoder::colVec vec;

	if(_encodedNamesInvalidated)
	{
		vec = _columnEncoder->_encodedNames;

		if(_otherEncoders)
			for(const ColumnEncoder * other : *_otherEncoders)
				for(const std::string & name : other->_encodedNames)
					vec.push_back(name);

		_encodedNamesInvalidated = false;
	}

	sortVectorBigToSmall(vec);

	return vec;
}

bool ColumnEncoder::shouldEncode(const std::string & in)
{
	return _encodingMap.count(in) > 0;
}

bool ColumnEncoder::shouldDecode(const std::string & in)
{
	return _decodingMap.count(in) > 0;
}

std::string	ColumnEncoder::replaceAllStrict(const std::string & text, const std::map<std::string, std::string> & map)
{
	if (map.count(text) > 0)
		return map.at(text);
	else
		return text;
}

std::string	ColumnEncoder::replaceAll(std::string text, const std::map<std::string, std::string> & map, const std::vector<std::string> & names)
{
	size_t foundPos = 0;

	while(foundPos < std::string::npos)
	{
		size_t firstFoundPos	= std::string::npos;

		std::string replaceThis;

		//First we find the first occurence of a replaceable text.
		for(const std::string & replaceMe : names) //We follow names instead of keyvals from map because they ought to be sorted from largest to smallest string (_originalNames) to not make sub-replacements
		{
			size_t pos = text.find(replaceMe, foundPos);
			if(pos < firstFoundPos)
			{
				firstFoundPos = pos;
				replaceThis = replaceMe;
			}
		}

		//We found something to replace and this will be the first occurence of anything like that. Replace it!
		if(firstFoundPos != std::string::npos)
		{
			foundPos = firstFoundPos;
			const std::string & replacement = map.at(replaceThis);
			text.replace(foundPos, replaceThis.length(), replacement);
			foundPos += replacement.length(); //Let's make sure we start replacing from after where we just replaced
		}
		else
			foundPos = std::string::npos;
	}

	return text;
}

std::string ColumnEncoder::encodeRScript(std::string text, std::set<std::string> * columnNamesFound)
{
	return encodeRScript(text, encodingMap(), originalNames(), columnNamesFound);
}

std::string ColumnEncoder::encodeRScript(std::string text, const std::map<std::string, std::string> & map, const std::vector<std::string> & names, std::set<std::string> * columnNamesFound)
{
	if(columnNamesFound)
		columnNamesFound->clear();

	static std::regex nonNameChar("[^\\.A-Za-z0-9_]");

	//for now we simply replace any found columnname by its encoded variant if found
	for(const std::string & oldCol : names)
	{
		std::string	newCol	= map.at(oldCol);

		std::vector<size_t> foundColPositions = getPositionsColumnNameMatches(text, oldCol);
		std::reverse(foundColPositions.begin(), foundColPositions.end());

		for (size_t foundPos : foundColPositions)
		{
			size_t foundPosEnd = foundPos + oldCol.length();

			//First check if it is a "free columnname" aka is there some space or a kind in front of it. We would not want to replace a part of another term (Imagine what happens when you use a columname such as "E" and a filter that includes the term TRUE, it does not end well..)
			bool startIsFree	= foundPos == 0					|| std::regex_match(text.substr(foundPos - 1, 1),	nonNameChar);
			bool endIsFree		= foundPosEnd == text.length()	|| std::regex_match(text.substr(foundPosEnd, 1),	nonNameChar);

			//Check for "(" as well because maybe someone has a columnname such as rep or if or something weird like that. This might however have some whitespace in between...
			bool keepGoing = true;

			for(size_t bracePos = foundPosEnd; bracePos < text.size() && endIsFree && keepGoing; bracePos++)
				if(text[bracePos] == '(')
					endIsFree = false;
				else if(text[bracePos] != '\t' && text[bracePos] != ' ')
					keepGoing = false; //Aka something else than whitespace or a brace and that means that we can replace it!

			if(startIsFree && endIsFree)
			{
				text.replace(foundPos, oldCol.length(), newCol);

				if(columnNamesFound)
					columnNamesFound->insert(oldCol);
			}
		}
	}

	return text;
}

std::vector<size_t> ColumnEncoder::getPositionsColumnNameMatches(const std::string & text, const std::string & columnName)
{
	std::vector<size_t> positions;

	bool inString	= false;
	char delim		= '?';

	for (std::string::size_type pos = 0; pos < text.length(); ++pos)
		if (!inString && text.substr(pos, columnName.length()) == columnName)
			positions.push_back(int(pos));
		else if (text[pos] == '"' || text[pos] == '\'') //string starts or ends. This does not take into account escape characters though...
		{
			if (!inString)
			{
				delim		= text[pos];
				inString	= true;
			}
			else if(text[pos] == delim)
				inString = false;
		}

	return positions;
}


void ColumnEncoder::encodeJson(Json::Value & json, bool replaceNames, bool replaceStrict)
{
	//std::cout << "Json before encoding:\n" << json.toStyledString();
	replaceAll(json, encodingMap(), originalNames(), replaceNames, replaceStrict);
	//std::cout << "Json after encoding:\n" << json.toStyledString() << std::endl;
}

void ColumnEncoder::decodeJson(Json::Value & json, bool replaceNames)
{
	//std::cout << "Json before encoding:\n" << json.toStyledString();
	replaceAll(json, decodingMap(), encodedNames(), replaceNames, false);
	//std::cout << "Json after encoding:\n" << json.toStyledString() << std::endl;
}

void ColumnEncoder::decodeJsonSafeHtml(Json::Value & json)
{
	replaceAll(json, decodingMapSafeHtml(), encodedNames(), true, false);
}


void ColumnEncoder::replaceAll(Json::Value & json, const std::map<std::string, std::string> & map, const std::vector<std::string> & names, bool replaceNames, bool replaceStrict)
{
	switch(json.type())
	{
	case Json::arrayValue:
		for(Json::Value & option : json)
			replaceAll(option, map, names, replaceNames, replaceStrict);
		return;

	case Json::objectValue:
	{
		std::map<std::string, std::string> changedMembers;

		for(const std::string & optionName : json.getMemberNames())
		{
			replaceAll(json[optionName], map, names, replaceNames, replaceStrict);

			if(replaceNames)
			{
				std::string replacedName = replaceStrict ? replaceAllStrict(optionName, map) : replaceAll(optionName, map, names);

				if(replacedName != optionName)
					changedMembers[optionName] = replacedName;
			}
		}

		for(const auto & origNew : changedMembers) //map is empty if !replaceNames
		{
			json[origNew.second] = json[origNew.first];
			json.removeMember(origNew.first);
		}

		return;
	}

	case Json::stringValue:
		json = replaceStrict ? replaceAllStrict(json.asString(), map) : replaceAll(json.asString(), map, names);
		return;

	default:
		return;
	}
}

void ColumnEncoder::setCurrentNamesFromOptionsMeta(const Json::Value & options)
{
	std::vector<std::string> namesFound;

	if(!options.isNull() && options.isMember(".meta"))
		collectExtraEncodingsFromMetaJson(options[".meta"], namesFound);

	setCurrentNames(namesFound);
}

void ColumnEncoder::collectExtraEncodingsFromMetaJson(const Json::Value & json, std::vector<std::string> & namesCollected) const
{
	switch(json.type())
	{
	case Json::arrayValue:
		for(const Json::Value & option : json)
			collectExtraEncodingsFromMetaJson(option, namesCollected);
		return;

	case Json::objectValue:
		if(json.isMember("encodeThis"))
		{
			if(json["encodeThis"].isString())
				namesCollected.push_back(json["encodeThis"].asString());
			else if(json["encodeThis"].isArray())
				for(const Json::Value & enc : json["encodeThis"])
					namesCollected.push_back(enc.asString());
		}
		else
			for(const std::string & optionName : json.getMemberNames())
				collectExtraEncodingsFromMetaJson(json[optionName], namesCollected);
		return;

	default:
		return;
	}
}

std::string ColumnEncoder::removeColumnNamesFromRScript(const std::string & rCode, const std::vector<std::string> & colsToRemove)
{
	std::map<std::string, std::string> replaceBy;

	for(const std::string & col : colsToRemove)
		replaceBy[col] = "stop('column " + col + " was removed from this RScript')";

	return replaceColumnNamesInRScript(rCode, replaceBy);
}

std::string ColumnEncoder::replaceColumnNamesInRScript(const std::string & rCode, const std::map<std::string, std::string> & changedNames)
{
	//Ok the trick here is to reuse the encoding code, we will first encode the original names and then change the encodings to point back to the replaced names.
	ColumnEncoder tempEncoder(changedNames);

	return
		tempEncoder.replaceAll(
			tempEncoder.encodeRScript(
				rCode,
				tempEncoder._encodingMap,
				tempEncoder._originalNames
			),
			tempEncoder._decodingMap,
			tempEncoder._encodedNames
		);
}

ColumnEncoder::colVec ColumnEncoder::columnNames()
{
	return _columnEncoder ? _columnEncoder->_originalNames : colVec();
}

ColumnEncoder::colVec ColumnEncoder::columnNamesEncoded()
{
	return _columnEncoder ? _columnEncoder->_encodedNames : colVec();
}


ColumnEncoder::colsPlusTypes ColumnEncoder::encodeColumnNamesinOptions(Json::Value & options, bool preloadingData)
{
	colsPlusTypes getTheseCols;
	
	if (options.isObject())
	{
		if(!preloadingData) //make sure "optionname".types is available for analyses incapable of preloadingData, this should be considered deprecated
		{
			// For variables list the types of each variable are added in the option itself.
			// To ensure the analyses still work as before, remove these types from the option, and add them in a new option with name '<option mame>.types'
			// very much deprecated though as analyses should announce being capable of "preloadingData" and then using that instead.
			for (const std::string& optionName : options.getMemberNames())
				if (options[optionName].isObject() && options[optionName].isMember("value") && options[optionName].isMember("types"))
				{
					options[optionName + ".types"] = options[optionName]["types"];
					options[optionName] = options[optionName]["value"];
				}
		}
		else	//Here we make sure all the requested columns + their types are collected
		{		// they then are encoded with type included so that everything is accessible easily via those encoded names
				// some functionality has been added to ask for the type of an encoded column as well.
			
			for (const std::string& optionName : options.getMemberNames())
				if (options[optionName].isObject() && options[optionName].isMember("value") && options[optionName].isMember("types"))
				{
					if (options[optionName].isObject() && options[optionName].isMember("value") && options[optionName].isMember("types"))
					{
						Json::Value		newOption	=	Json::arrayValue,
									&	typeList	= options[optionName]["types"],
										valueList	= options[optionName]["value"];
		
						bool useSingleVal = false;
						
						if(!options[optionName]["value"].isArray())
						{
							valueList = Json::arrayValue;
							valueList.append(options[optionName]["value"].asString());
		
							useSingleVal = true; //Otherwise we break things like "splitBy" it seems
						}
		
						//I wanted to do a sanity check, but actually the data is insane by design atm.
						// data can be { value: "", types: [] } but also { value: [], types: [] } which is great...
						//if(typeList.size() != valueList.size())
						//	std::runtime_error("Expecting the same amount of values and types");
		
						for(int i=0; i<valueList.size(); i++)
						{
							std::string name = valueList[i].asString(),
										type = typeList.size() > i ? typeList[i].asString() : "";
		
							if(type == "unknown" || !columnTypeValidName(type))
								newOption.append(name);
							else
							{
								std::string nameWithType = name + "." + type;
								newOption.append(nameWithType);
		
								getTheseCols.insert(std::make_pair(nameWithType, columnTypeFromString(type)));
							}
						}
						
						options[optionName] = !useSingleVal ? newOption : newOption[0];
					}
				}
		}
	}

	_encodeColumnNamesinOptions(options, options[".meta"]);

	return getTheseCols;
}

void ColumnEncoder::_encodeColumnNamesinOptions(Json::Value & options, Json::Value & meta)
{
	if(meta.isNull())
		return;
	
	bool	encodePlease	= meta.isObject() && meta.get("shouldEncode",	false).asBool(),
			isRCode			= meta.isObject() && meta.get("rCode",			false).asBool();

	switch(options.type())
	{
	case Json::arrayValue:
		if(encodePlease)
			columnEncoder()->encodeJson(options, false, true); //If we already think we have columnNames just change it all
		
		else if(meta.type() == Json::arrayValue)
			for(int i=0; i<options.size() && i < meta.size(); i++)
				_encodeColumnNamesinOptions(options[i], meta[i]);
		
		else if(isRCode)
			for(int i=0; i<options.size(); i++)
				if(options[i].isString())
					options[i] = columnEncoder()->encodeRScript(options[i].asString());
	
		return;

	case Json::objectValue:
		for(const std::string & memberName : options.getMemberNames())
			if(memberName != ".meta" && meta.isMember(memberName))
				_encodeColumnNamesinOptions(options[memberName], meta[memberName]);
		
			else if(isRCode && options[memberName].isString())
				options[memberName] = columnEncoder()->encodeRScript(options[memberName].asString());
		
			else if(encodePlease)
				columnEncoder()->encodeJson(options, false, true); //If we already think we have columnNames just change it all I guess?
		
		return;

	case Json::stringValue:
			
			if(isRCode)				options = columnEncoder()->encodeRScript(options.asString());
			else if(encodePlease)	options = columnEncoder()->encodeAll(options.asString());
			
		return;

	default:
		return;
	}
}
