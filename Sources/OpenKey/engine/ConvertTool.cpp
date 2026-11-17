//
//  ConvertTool.cpp
//  OpenKey
//
//  Created by Tuyen on 9/4/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//
#include <locale>
#include <codecvt>
#include "ConvertTool.h"
#include "Engine.h"
#include <iostream>
#include <memory.h>
#include <stdexcept>
#include <cwctype>

//option
bool convertToolDontAlertWhenCompleted = false;
bool convertToolToAllCaps = false;
bool convertToolToAllNonCaps = false;
bool convertToolToCapsFirstLetter = false;
bool convertToolToCapsEachWord = false;
bool convertToolRemoveMark = false;
Uint8 convertToolFromCode = 0;
Uint8 convertToolToCode = 0;
int convertToolHotKey = 0;

ConvertOptions getConvertOptions() {
    return {convertToolToAllCaps, convertToolToAllNonCaps, convertToolToCapsFirstLetter,
            convertToolToCapsEachWord, convertToolRemoveMark, convertToolFromCode, convertToolToCode};
}

static Uint16 getUnicodeCompoundMarkIndex(const Uint16& mark) {
    for (int i = 0; i < 5; i++) {
        if (mark == _unicodeCompoundMark[i]) return ((i + 1) << 13);
    }
    return 0;
}

string convertUtil(const string& sourceString) {
    return convertUtil(sourceString, getConvertOptions());
}

string convertUtil(const string& sourceString, const ConvertOptions& options) {
    if (options.fromCode >= 5 || options.toCode >= 5) throw std::invalid_argument("Invalid encoding");
    const wstring data = utf8ToWideString(sourceString);
    wstring result;
    result.reserve(data.size());
    const bool titleCase = options.toCapsFirstLetter || options.toCapsEachWord;
    bool capitalizeNext = titleCase;
    bool sentenceBreak = false;
    for (size_t pos = 0; pos < data.size(); pos++) {
        Uint16 character = static_cast<Uint16>(data[pos]);
        Uint32 key = 0;
        size_t index = 0;
        bool found = false;
        if (pos + 1 < data.size()) {
            Uint16 combined = 0;
            if ((options.fromCode == 2 || options.fromCode == 4) && data[pos] <= 255 && data[pos + 1] <= 255) {
                combined = character | (static_cast<Uint16>(data[pos + 1]) << 8);
            } else if (options.fromCode == 3) {
                const Uint16 mark = getUnicodeCompoundMarkIndex(data[pos + 1]);
                if (mark) combined = character | mark;
            }
            if (combined && findCharacter(combined, options.fromCode, key, index)) {
                found = true;
                pos++;
            }
        }
        if (!found) found = findCharacter(character, options.fromCode, key, index);
        const bool upper = options.toAllCaps || (capitalizeNext && !options.toAllNonCaps);
        const bool lower = options.toAllNonCaps || (titleCase && !capitalizeNext);
        if (found) {
            if (upper && index % 2) index--;
            else if (lower && index % 2 == 0) index++;
            Uint16 target = _codeTable[options.toCode].at(key)[index];
            if (options.removeMark) {
                target = keyCodeToCharacter((key & CHAR_MASK) | (index % 2 == 0 ? CAPS_MASK : 0));
                result.push_back(target);
            } else if (options.toCode == 2 || options.toCode == 4) {
                result.push_back(LOBYTE(target));
                if (HIBYTE(target) > 32) result.push_back(HIBYTE(target));
            } else if (options.toCode == 3) {
                result.push_back(target & 0x1FFF);
                if (target >> 13) result.push_back(_unicodeCompoundMark[(target >> 13) - 1]);
            } else {
                result.push_back(target);
            }
            capitalizeNext = false;
            sentenceBreak = false;
        } else {
            result.push_back(upper ? towupper(character) : (lower ? towlower(character) : character));
            if (iswalpha(character)) {
                capitalizeNext = false;
                sentenceBreak = false;
            } else if (character == '\n' || character == '\r' || (sentenceBreak && iswspace(character))) {
                if (titleCase) capitalizeNext = true;
            } else if (iswspace(character) && options.toCapsEachWord) {
                capitalizeNext = true;
            } else if (character == '.' || character == '?' || character == '!') {
                sentenceBreak = true;
            }
        }
    }
    return wideStringToUtf8(result);
}
