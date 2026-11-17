//
//  SmartSwitchKey.cpp
//  OpenKey
//
//  Created by Tuyen on 8/13/19.
//  Copyright © 2019 Tuyen Mai. All rights reserved.
//

#include "SmartSwitchKey.h"
#include <map>
#include <iostream>
#include <memory.h>

//main data, i use `map` because it has O(Log(n))
static map<string, Int8> _smartSwitchKeyData;
static string _cacheKey = ""; //use cache for faster
static Int8 _cacheData = 0; //use cache for faster
static bool _hasCache = false;

bool initSmartSwitchKey(const Byte* pData, const int& size) {
    if (size == 0) {
        _smartSwitchKeyData.clear();
        _hasCache = false;
        return true;
    }
    if (!pData || size < 2) return false;
    const size_t length = static_cast<size_t>(size);
    const Uint16 count = pData[0] | (pData[1] << 8);
    size_t cursor = 2;
    map<string, Int8> loaded;
    for (Uint32 n = 0; n < count; n++) {
        if (cursor >= length) return false;
        const size_t bundleIdSize = pData[cursor++];
        if (!bundleIdSize || bundleIdSize >= length - cursor) return false;
        string bundleId(reinterpret_cast<const char*>(pData + cursor), bundleIdSize);
        cursor += bundleIdSize;
        const Byte value = pData[cursor++];
        if (value > 9) return false;
        loaded.emplace(std::move(bundleId), value);
    }
    if (cursor != length) return false;
    _smartSwitchKeyData.swap(loaded);
    _hasCache = false;
    return true;
}

void getSmartSwitchKeySaveData(vector<Byte>& outData) {
    outData.clear();
    Uint16 count = (Uint16)_smartSwitchKeyData.size();
    outData.push_back((Byte)count);
    outData.push_back((Byte)(count>>8));
    
    for (std::map<string, Int8>::iterator it = _smartSwitchKeyData.begin(); it != _smartSwitchKeyData.end(); ++it) {
        outData.push_back((Byte)it->first.length());
        for (int j = 0; j < it->first.length(); j++) {
            outData.push_back(it->first[j]);
        }
        outData.push_back(it->second);
    }
}

int getAppInputMethodStatus(const string& bundleId, const int& currentInputMethod) {
    if (_hasCache && _cacheKey == bundleId) {
        return _cacheData;
    }
    if (bundleId.empty() || bundleId.size() > 255 || currentInputMethod < 0 || currentInputMethod > 9) return -1;
    const auto entry = _smartSwitchKeyData.find(bundleId);
    if (entry != _smartSwitchKeyData.end()) {
        _cacheKey = bundleId;
        _cacheData = entry->second;
        _hasCache = true;
        return _cacheData;
    }
    if (_smartSwitchKeyData.size() >= 65535) return -1;
    _hasCache = true;
    _cacheKey = bundleId;
    _cacheData = currentInputMethod;
    _smartSwitchKeyData[bundleId] = _cacheData;
    return -1;
}

void setAppInputMethodStatus(const string& bundleId, const int& language) {
    if (bundleId.empty() || bundleId.size() > 255 || language < 0 || language > 9) return;
    if (_smartSwitchKeyData.size() >= 65535 && _smartSwitchKeyData.find(bundleId) == _smartSwitchKeyData.end()) return;
    _smartSwitchKeyData[bundleId] = language;
    _cacheKey = bundleId;
    _cacheData = language;
    _hasCache = true;
}
