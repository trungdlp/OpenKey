//
//  Macro.cpp
//  OpenKey
//
//  Created by Tuyen on 8/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "Macro.h"
#include "Vietnamese.h"
#include "Engine.h"
#include <iostream>
#include <memory.h>
#include <fstream>
#include <stdexcept>

using namespace std;

//main data
map<vector<Uint32>, MacroData> macroMap;

extern int vCodeTable;
//local variable
static int c = 0;
static bool _macroFlag = false;
static Uint16 _kChar = 0;
static Uint32 _charBuff;

static void convert(const string& str, vector<Uint32>& outData) {
    outData.clear();
    wstring data = utf8ToWideString(str);
    outData.reserve(data.size());
    for (wchar_t character : data) {
        const auto normal = _characterMap.find(character);
        if (normal != _characterMap.end()) {
            outData.push_back(normal->second);
            continue;
        }
        Uint32 key;
        size_t index;
        if (findCharacter(character, 0, key, index)) {
            outData.push_back(_codeTable[vCodeTable].at(key)[index] | CHAR_CODE_MASK);
        } else {
            outData.push_back(static_cast<Uint32>(character) | PURE_CHARACTER_MASK);
        }
    }
}

/**
 * data structure:
 * byte 0 and 1: macro count
 *
 * byte n: macroText size (macroTextSize)
 * byte n + macroTextSize: macroText data
 *
 * byte m, m+1: macroContentSize
 * byte m+1 + macroContentSize: macroContent data
 *
 * ...
 * next macro
 */
bool initMacroMap(const Byte* pData, const int& size) {
    if (size == 0) {
        macroMap.clear();
        return true;
    }
    if (!pData || size < 2) return false;
    const size_t length = static_cast<size_t>(size);
    const Uint16 macroCount = pData[0] | (pData[1] << 8);
    size_t cursor = 2;
    map<vector<Uint32>, MacroData> loaded;
    try {
        for (Uint32 n = 0; n < macroCount; n++) {
            if (cursor >= length) return false;
            const size_t textSize = pData[cursor++];
            if (!textSize || textSize > length - cursor) return false;
            MacroData data;
            data.macroText.assign(reinterpret_cast<const char*>(pData + cursor), textSize);
            cursor += textSize;
            if (length - cursor < 2) return false;
            const size_t contentSize = pData[cursor] | (pData[cursor + 1] << 8);
            cursor += 2;
            if (contentSize > length - cursor) return false;
            data.macroContent.assign(reinterpret_cast<const char*>(pData + cursor), contentSize);
            cursor += contentSize;
            vector<Uint32> key;
            convert(data.macroText, key);
            convert(data.macroContent, data.macroContentCode);
            loaded.emplace(std::move(key), std::move(data));
        }
    } catch (const std::range_error&) {
        return false;
    }
    if (cursor != length) return false;
    macroMap.swap(loaded);
    return true;
}

void getMacroSaveData(vector<Byte>& outData) {
    outData.clear();
    Uint16 totalMacro = (Uint16)macroMap.size();
    outData.push_back((Byte)totalMacro);
    outData.push_back((Byte)(totalMacro>>8));
    
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        outData.push_back((Byte)it->second.macroText.size());
        for (int j = 0; j < it->second.macroText.size(); j++) {
            outData.push_back(it->second.macroText[j]);
        }
        
        Uint16 macroContentSize = (Uint16)it->second.macroContent.size();
        outData.push_back((Byte)macroContentSize);
        outData.push_back(macroContentSize>>8);
        for (int j = 0; j < macroContentSize; j++) {
            outData.push_back(it->second.macroContent[j]);
        }
    }
}

static bool modifyCaseUnicode(Uint32& code, const bool& isUpperCase=true) {
    _charBuff = code;
    if (!(code & CHAR_CODE_MASK)) { //for normal char
        if (isUpperCase) code |= CAPS_MASK;
        else code &= ~CAPS_MASK;
        return code != _charBuff;
    }
    
    Uint32 key;
    size_t index;
    if (findCharacter(static_cast<Uint16>(code), vCodeTable, key, index)) {
        if (index % 2 == 0 && !isUpperCase) index++;
        else if (index % 2 != 0 && isUpperCase) index--;
        code = _codeTable[vCodeTable].at(key)[index] | CHAR_CODE_MASK;
    }
    return code != _charBuff;
}

bool findMacro(vector<Uint32>& key, vector<Uint32>& macroContentCode) {
    for (c = 0; c < key.size(); c++) {
        key[c] = getCharacterCode(key[c]);
    }
    auto entry = macroMap.find(key);
    if (entry != macroMap.end()) {
        macroContentCode = entry->second.macroContentCode;
        return true;
    }
    if (vAutoCapsMacro) {
        _macroFlag = false;
        if (key.size() > 1 && modifyCaseUnicode(key[1], false)) {
            _macroFlag = true;
            for (c = 2; c < key.size(); c++) {
                modifyCaseUnicode(key[c], false);
            }
        }
        
        if (key.size() > 0 && modifyCaseUnicode(key[0], false)) {
            entry = macroMap.find(key);
            if (entry != macroMap.end()) {
                macroContentCode = entry->second.macroContentCode;
                for (c = 0; c < macroContentCode.size(); c++) {
                    if (c == 0 || _macroFlag) {
                        _kChar = keyCodeToCharacter(macroContentCode[c]);
                        if (_kChar != 0) {
                            _kChar = toupper(_kChar);
                            macroContentCode[c] = _characterMap[_kChar];
                            continue;
                        }
                        if (macroContentCode[c] & CHAR_CODE_MASK) {
                            modifyCaseUnicode(macroContentCode[c]);
                        }
                    }
                }
                return true;
            }
        }
    }
    return false;
}

bool hasMacro(const string& macroName) {
    vector<Uint32> key;
    try {
        convert(macroName, key);
    } catch (const std::range_error&) {
        return false;
    }
    return (macroMap.find(key) != macroMap.end());
}

void getAllMacro(vector<vector<Uint32>>& keys, vector<string>& macroTexts, vector<string>& macroContents) {
    keys.clear();
    macroTexts.clear();
    macroContents.clear();
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        keys.push_back(it->first);
        macroTexts.push_back(it->second.macroText);
        macroContents.push_back(it->second.macroContent);
    }
}

bool addMacro(const string& macroText, const string& macroContent) {
    if (macroText.empty() || macroText.size() > 255 || macroContent.size() > 65535) return false;
    vector<Uint32> key;
    MacroData data;
    data.macroText = macroText;
    data.macroContent = macroContent;
    try {
        convert(macroText, key);
        convert(macroContent, data.macroContentCode);
    } catch (const std::range_error&) {
        return false;
    }
    if (macroMap.size() >= 65535 && macroMap.find(key) == macroMap.end()) return false;
    macroMap[std::move(key)] = std::move(data);
    return true;
}

bool deleteMacro(const string& macroText) {
    vector<Uint32> key;
    try {
        convert(macroText, key);
    } catch (const std::range_error&) {
        return false;
    }
    if (macroMap.find(key) != macroMap.end()) {
        macroMap.erase(key);
        return true;
    }
    return false;
}

void onTableCodeChange() {
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        convert(it->second.macroContent, it->second.macroContentCode);
    }
}

void saveToFile(const string& path) {
    ofstream myfile;
    myfile.open(path.c_str());
    myfile << ";Compatible OpenKey Macro Data file for UniKey*** version=1 ***\n";
    for (std::map<vector<Uint32>, MacroData>::iterator it = macroMap.begin(); it != macroMap.end(); ++it) {
        myfile <<it->second.macroText << ":" << it->second.macroContent<<"\n";
    }
    myfile.close();
}

void readFromFile(const string& path, const bool& append) {
    ifstream myfile(path.c_str());
    string line;
    int k = 0;
    size_t pos = 0;
    string name, content;
    if (myfile.is_open()) {
        if (!append) {
            macroMap.clear();
        }
        while (getline (myfile,line) ) {
            k++;
            if (k == 1) continue;
            pos = line.find(":");
            if (string::npos != pos) {
                name = line.substr(0, pos);
                content = line.substr(pos + 1, line.length() - pos - 1);
				while (name.compare("") == 0 && content.compare("") != 0) {
					pos = content.find(":");
					if (string::npos != pos) {
						name += ":";
						name += content.substr(0, pos);
						content = content.substr(pos + 1, line.length() - pos - 1);
					} else {
						break;
					}
				}

                if (name.compare("") != 0 && !hasMacro(name)) {
                    addMacro(name, content);
                }
            }
        }
        myfile.close();
    }
}
