#include "Engine.h"
#include "CharacterOutput.h"
#include <cassert>
#include <iostream>

struct Event { bool down; vector<Uint16> units; };
using CGEventRef = Event*;
void* myEventSource = nullptr;
void* _proxy = nullptr;
int _flag = 0;
Uint16 _keycode = KEY_B;
const int kCGEventFlagMaskAlphaShift = 1, kCGEventFlagMaskShift = 2;
const int MASK_SHIFT = 2, MASK_CAPITAL = 1;
#ifndef _WIN32
const int VK_INSERT = 1, KEYEVENTF_EXTENDEDKEY = 2, CF_UNICODETEXT = 3;
using LPCTSTR = const Uint16*;
#endif
vKeyHookState* pData;
static vector<Uint16> emitted, _newCharString;
static vector<Uint8> _syncKey;
static vector<Uint32> controlKeys;
static bool clipboardAvailable = true;
static size_t pastes = 0;

static CGEventRef CGEventCreateKeyboardEvent(void*, int, bool down) { return new Event{down, {}}; }
static void CGEventKeyboardSetUnicodeString(CGEventRef event, size_t length, const Uint16* units) {
    assert(length > 0 && length <= 16);
    event->units.assign(units, units + length);
    assert(event->units.back() < 0xD800 || event->units.back() > 0xDBFF);
}
static void CGEventTapPostEvent(void*, CGEventRef event) {
    if (event->down) emitted.insert(emitted.end(), event->units.begin(), event->units.end());
}
static void CFRelease(CGEventRef event) { delete event; }
static void InsertKeyLength(Uint8 length) { _syncKey.push_back(length); }
static void SendKeyCode(Uint32 key) { controlKeys.push_back(key); }
static void SendCombineKey(int, int, int, int) { pastes++; }
struct OpenKeyHelper {
    static bool setClipboardText(LPCTSTR data, int length, int) {
        assert(length > 0 && data[length - 1] == 0);
        if (!clipboardAvailable) return false;
        emitted.assign(data, data + length - 1);
        return true;
    }
};

#include "Senders.inc"

static void reset(int code = 0) {
    pData = static_cast<vKeyHookState*>(vKeyInit());
    pData->code = vWillProcess;
    vCodeTable = code;
    emitted.clear();
    _syncKey.clear();
    controlKeys.clear();
    pastes = 0;
    _flag = 0;
    _keycode = KEY_B;
    clipboardAvailable = true;
}

static void check(void (*send)(const bool&)) {
    reset();
    pData->newCharCount = 1;
    pData->charData[0] = KEY_A;
    send(false);
    assert(emitted == vector<Uint16>{'a'});

    reset(3);
    pData->macroData.assign(15, KEY_B);
    pData->macroData.push_back(0x2061 | CHAR_CODE_MASK);
    pData->macroData.push_back(KEY_B);
    send(true);
    vector<Uint16> expected(15, 'b');
    expected.insert(expected.end(), {'a', 0x0301, 'b'});
    assert(emitted == expected);
    assert(_syncKey.size() == 17 && _syncKey[15] == 2);

    reset(3);
    pData->newCharCount = MAX_BUFF;
    for (size_t pos = 0; pos < MAX_BUFF; pos++) pData->charData[pos] = 0x2061 | CHAR_CODE_MASK;
    send(false);
    expected.clear();
    for (size_t pos = 0; pos < MAX_BUFF; pos++) expected.insert(expected.end(), {'a', 0x0301});
    assert(emitted == expected);

    reset();
    pData->macroData.assign(15, KEY_B);
    pData->macroData.push_back(0xD83D | PURE_CHARACTER_MASK);
    pData->macroData.push_back(0xDE00 | PURE_CHARACTER_MASK);
    send(true);
    expected.assign(15, 'b');
    expected.insert(expected.end(), {0xD83D, 0xDE00});
    assert(emitted == expected);

    reset(2);
    pData->newCharCount = 1;
    pData->charData[0] = 0xE161 | CHAR_CODE_MASK;
    pData->code = vRestore;
    _flag = MASK_SHIFT;
    send(false);
    assert(emitted == (vector<Uint16>{'a', 0xE1, 'B'}));

    reset();
    pData->code = vRestoreAndStartNewSession;
    pData->newCharCount = 1;
    pData->charData[0] = KEY_A;
    _keycode = KEY_TAB;
    send(false);
    assert(emitted == vector<Uint16>{'a'});
    assert(controlKeys == vector<Uint32>{KEY_TAB});

    reset();
    pData->macroData.assign(65535, KEY_B);
    send(true);
    assert(emitted == vector<Uint16>(65535, 'b'));
}

int main() {
    check(SendMacString);
    check(SendWindowsString);
    reset();
    clipboardAvailable = false;
    pData->newCharCount = 1;
    pData->charData[0] = KEY_A;
    SendWindowsString(false);
    assert(pastes == 0);
    std::cout << "Native sender regressions passed (OS calls mocked)\n";
}
