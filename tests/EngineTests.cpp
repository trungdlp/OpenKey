#include "Engine.h"
#include "CharacterOutput.h"
#include <algorithm>
#include <cassert>
#include <future>
#include <iostream>
#include <fstream>
#include <cstdio>

static vKeyHookState* state;

static void reset() {
    vCodeTable = 0;
    vInputType = vTelex;
    vCheckSpelling = 1;
    vUseMacro = 1;
    vAutoCapsMacro = 0;
    vFreeMark = vUseModernOrthography = vQuickTelex = vRestoreIfWrongSpelling = 0;
    vUpperCaseFirstChar = vAllowConsonantZFWJ = vQuickStartConsonant = vQuickEndConsonant = 0;
    state = static_cast<vKeyHookState*>(vKeyInit());
}

static void press(Uint16 key, Uint8 caps = 0) {
    vKeyHandleEvent(Keyboard, KeyDown, key, caps, false);
}

static string type(const string& keys) {
    vector<Uint16> text;
    for (unsigned char character : keys) {
        const Uint32 key = character == '\b' ? static_cast<Uint32>(KEY_DELETE) : _characterMap.at(character);
        press(key & CHAR_MASK, key & CAPS_MASK ? 1 : 0);
        if (state->code == vDoNothing) {
            if (character == '\b') {
                if (!text.empty()) text.pop_back();
            } else text.push_back(character);
        } else {
            assert(state->backspaceCount <= text.size());
            text.resize(text.size() - state->backspaceCount);
            const bool macro = state->code == vReplaceMaro;
            const size_t count = macro ? state->macroData.size() : state->newCharCount;
            assert(macro || count <= MAX_BUFF);
            for (size_t n = count; n > 0; n--) {
                Uint16 units[2];
                size_t length = getCharacterUnits(macro ? state->macroData[count - n] : state->charData[n - 1], vCodeTable, units);
                text.insert(text.end(), units, units + length);
            }
            if (macro || state->code == vRestore || state->code == vRestoreAndStartNewSession) text.push_back(character);
        }
    }
    return wideStringToUtf8(wstring(text.begin(), text.end()));
}

static void expect(const string& keys, const string& expected) {
    const string actual = type(keys);
    if (actual != expected) {
        std::cerr << "Input " << keys << ": expected [" << expected << "] got [" << actual << "]\n";
        std::abort();
    }
}

static void featureTests() {
    const vector<std::pair<string, string>> corpus = {
        {"as", u8"á"}, {"af", u8"à"}, {"ar", u8"ả"}, {"ax", u8"ã"}, {"aj", u8"ạ"},
        {"aas", u8"ấ"}, {"awf", u8"ằ"}, {"eer", u8"ể"}, {"oox", u8"ỗ"}, {"owj", u8"ợ"},
        {"uws", u8"ứ"}, {"ys", u8"ý"}, {"if", u8"ì"}, {"asf", u8"à"}, {"asz", "a"},
        {"z", "z"}, {"ass", "as"}, {"aaa", "aa"}, {"ooo", "oo"}, {"eee", "ee"},
        {"ddd", "dd"}, {"aww", "aw"}, {"uww", "uw"}, {"oww", "ow"}, {"ww", "w"},
        {"DDawng", u8"Đăng"}, {"TIEENGS VIEETJ", u8"TIẾNG VIỆT"},
        {"thuowng", u8"thương"}, {"tuowng", u8"tương"}, {"thow", u8"thơ"},
        {"uoww", "uow"}, {"uaw", u8"ưa"}, {"uiw", u8"ưi"}, {"oaiw", u8"oaiw"},
        {"queets", u8"quết"}, {"gifn", u8"gìn"}, {"nghieeng", u8"nghiêng"},
        {"tieengs\b", u8"tiến"}, {"ban...\b\b\bs", u8"bán"},
        {"ban   \b\b\bs", u8"bán"}, {"ban, as", u8"ban, á"}, {"ban.abc", "ban.abc"},
        {"[[", "["}, {"]]", "]"}, {"u[", u8"uơ"}, {"trw", u8"trư"},
        {string(100, 'b') + string(100, '\b') + "as", u8"á"}
    };
    for (const auto& sample : corpus) { reset(); expect(sample.first, sample.second); }
    for (int modern : {0, 1}) {
        for (const auto& sample : vector<std::pair<string, string>>{
            {"hoaf", modern ? u8"hoà" : u8"hòa"}, {"thuys", modern ? u8"thuý" : u8"thúy"},
            {"ngoais", u8"ngoái"}, {"khuya", "khuya"}, {"khuyur", u8"khuỷu"},
            {"ngoeor", u8"ngoẻo"}, {"mais", u8"mái"}, {"tuif", u8"tùi"},
            {"mays", u8"máy"}, {"muas", u8"múa"}, {"gias", u8"giá"},
            {"tieens", u8"tiến"}, {"yeeus", u8"yếu"}, {"cuoons", u8"cuốn"},
            {"muoons", u8"muốn"}, {"thuwowng", u8"thương"}}) {
            reset(); vUseModernOrthography = modern; expect(sample.first, sample.second);
        }
    }
    for (int input : {vSimpleTelex1, vSimpleTelex2}) {
        reset(); vInputType = input; expect("tieengs vieetj", u8"tiếng việt");
        reset(); vInputType = input; expect("[ban]", "[ban]");
    }
    for (const auto& sample : vector<std::pair<string, string>>{
        {"a1", u8"á"}, {"a2", u8"à"}, {"a3", u8"ả"}, {"a4", u8"ã"}, {"a5", u8"ạ"},
        {"a10", "a"}, {"a8", u8"ă"}, {"o7", u8"ơ"}, {"u7", u8"ư"},
        {"d9a8ng", u8"đăng"}, {"uo7ng", u8"ương"}, {"a7", "a7"}, {"o8", "o8"}}) {
        reset(); vInputType = vVNI; expect(sample.first, sample.second);
    }
    for (const auto& sample : vector<std::pair<string, string>>{
        {"cc", "ch"}, {"gg", "gi"}, {"kk", "kh"}, {"nn", "ng"},
        {"qq", "qu"}, {"pp", "ph"}, {"tt", "th"}, {"uu", "uu"}}) {
        reset(); vQuickTelex = 1; expect(sample.first, sample.second);
    }
    for (const auto& sample : vector<std::pair<string, string>>{
        {"fanh ", "phanh "}, {"jang ", "giang "}, {"wen.", "quen."},
        {"hag ", "hang "}, {"vih ", "vinh "}, {"bak ", "bach "},
        {"fag ", "phang "}, {"FAG ", "PHANG "}, {"b ", "b "}}) {
        reset(); vQuickStartConsonant = vQuickEndConsonant = 1;
        if (sample.first == "wen.") vInputType = vSimpleTelex1;
        expect(sample.first, sample.second);
    }
    for (const string ending : {" ", "."}) {
        reset(); vRestoreIfWrongSpelling = 1; expect("telex" + ending, "telex" + ending);
        reset(); vRestoreIfWrongSpelling = 1; expect("asbc" + ending, "asbc" + ending);
        reset(); vRestoreIfWrongSpelling = 1; expect("bc" + ending, "bc" + ending);
    }
    reset(); vUpperCaseFirstChar = 1; expect("ban. ban", "ban. Ban");
    reset(); vUpperCaseFirstChar = 1; press(KEY_RETURN); expect("as", u8"Á");
    reset(); vSetCheckSpelling(); vTempOffSpellChecking(); assert(!vCheckSpelling);
    vTempOffSpellChecking(); assert(vCheckSpelling);
    vCheckSpelling = 0; vSetCheckSpelling(); vTempOffSpellChecking(); assert(!vCheckSpelling);
    reset(); vTempOffEngine(); expect("as", "as"); press(KEY_SPACE); expect("as", u8"á");
    reset(); vFreeMark = 1; expect("as", u8"á");
    reset(); vAllowConsonantZFWJ = 1; expect("fas", u8"fá");
    for (size_t length : {size_t(30), size_t(31), size_t(32), size_t(64)}) {
        reset(); vInputType = vVNI; vCheckSpelling = 0; vQuickStartConsonant = vQuickEndConsonant = 1;
        const string word = "f" + string(length - 2, 'a') + "g";
        expect(word + " ", length == 30 ? "ph" + string(length - 2, 'a') + "ng " : word + " ");
    }
    reset(); type("as"); vKeyHandleEvent(Mouse, MouseDown, 0); expect("as", u8"á");
    reset(); type("as"); vKeyHandleEvent(Keyboard, KeyDown, KEY_A, 0, true); expect("as", u8"á");
}

static void inputTests() {
    for (Uint16 key : {KEY_LEFT_BRACKET, KEY_RIGHT_BRACKET, KEY_W}) {
        reset();
        press(key);
        assert(state->code == vWillProcess);
        assert(state->newCharCount == 1);
    }
    reset();
    assert(type("tieengs vieetj") == u8"tiếng việt");
    reset();
    assert(type("ban \b\bs") == u8"bá");
    reset();
    assert(type("ddawng") == u8"đăng");
    reset();
    vInputType = vVNI;
    assert(type("tie61ng") == u8"tiếng");
    reset();
    for (int n = 0; n < 10000; n++) { press(KEY_B); press(KEY_SPACE); }
    for (int n = 0; n < 20000; n++) press(KEY_DELETE);
    assert(type("as") == u8"á");
}

static void storageTests() {
    reset();
    assert(initMacroMap(nullptr, 0));
    assert(addMacro("btw", "by the way"));
    vector<Byte> saved;
    getMacroSaveData(saved);
    for (size_t length = 1; length < saved.size(); length++) {
        assert(!initMacroMap(saved.data(), static_cast<int>(length)));
        assert(hasMacro("btw"));
    }
    const Byte truncated[] = {1, 0};
    assert(!initMacroMap(truncated, 2));
    assert(!initMacroMap(nullptr, 2));
    assert(!initMacroMap(truncated, -1));
    assert(!addMacro(string(256, 'b'), "test"));
    assert(!addMacro("long", string(65536, 'b')));
    assert(!addMacro("invalid", string(1, static_cast<char>(0xFF))));
    assert(addMacro(string(255, 'b'), string(65535, 'c')));
    getMacroSaveData(saved);
    assert(initMacroMap(saved.data(), static_cast<int>(saved.size())));
    assert(hasMacro(string(255, 'b')));
    vAutoCapsMacro = 1;
    vector<Uint32> key = {KEY_B | CAPS_MASK, KEY_T | CAPS_MASK, KEY_W | CAPS_MASK};
    vector<Uint32> output;
    assert(findMacro(key, output));
    assert(keyCodeToCharacter(output.front()) == 'B');
    assert(keyCodeToCharacter(output.back()) == 'Y');
    vAutoCapsMacro = 0;

    assert(deleteMacro("btw"));
    assert(!deleteMacro("btw"));
    assert(!addMacro("", "empty"));
    assert(addMacro("btw", u8"đẹp 😀"));
    assert(addMacro("btw", u8"đẹp hơn"));
    vector<vector<Uint32>> keys;
    vector<string> names, contents;
    getAllMacro(keys, names, contents);
    assert(names.size() == 2 && keys.size() == 2 && contents.size() == 2);
    for (size_t n = 0; n < names.size(); n++) {
        if (names[n] == "btw") assert(contents[n] == u8"đẹp hơn");
    }
    for (int code = 0; code < 5; code++) {
        vCodeTable = code; onTableCodeChange();
        key = {KEY_B, KEY_T, KEY_W};
        assert(findMacro(key, output) && !output.empty());
    }
    vCodeTable = 0; onTableCodeChange(); vAutoCapsMacro = 1;
    for (bool allCaps : {false, true}) {
        key = {KEY_B | CAPS_MASK, KEY_T | (allCaps ? Uint32(CAPS_MASK) : 0u), KEY_W | (allCaps ? Uint32(CAPS_MASK) : 0u)};
        assert(findMacro(key, output));
        assert((output.front() & CHAR_MASK) == 0x0110); // Đ
    }
    key = {KEY_Z, KEY_Z}; assert(!findMacro(key, output));
    key.clear(); assert(!findMacro(key, output));
    vAutoCapsMacro = 0;
    const string path = "openkey-macro-test.txt";
    saveToFile(path);
    assert(initMacroMap(nullptr, 0));
    readFromFile(path, false);
    assert(hasMacro("btw") && hasMacro(string(255, 'b')));
    assert(addMacro("keep", "retained"));
    readFromFile(path, true); assert(hasMacro("keep"));
    readFromFile(path, false); assert(!hasMacro("keep"));
    {
        std::ofstream file(path);
        file << ";header\ninvalid line\n::colon\n:ignored\n::\nbtw:replacement\n";
    }
    readFromFile(path, true); assert(hasMacro(":"));
    std::remove(path.c_str());
    readFromFile(path, false); assert(hasMacro("btw"));

    assert(initMacroMap(nullptr, 0));
    assert(addMacro("btw", "by the way"));
    reset(); expect("btw ", "by the way ");
    reset(); expect("btw.", "by the way.");
    reset();
    auto english = [](Uint16 key, bool caps = false, bool control = false) {
        vEnglishMode(KeyDown, key, caps, control);
    };
    english(KEY_B); english(KEY_T); english(KEY_X); english(KEY_DELETE); english(KEY_W); english(KEY_SPACE);
    assert(state->code == vReplaceMaro && state->backspaceCount == 3);
    assert(state->macroData.size() == 10);
    english(KEY_SPACE); assert(state->code == vDoNothing);
    english(KEY_B); english(KEY_T); english(KEY_RETURN); english(KEY_W); english(KEY_SPACE);
    assert(state->code == vDoNothing);
    english(KEY_DELETE); english(KEY_DELETE);
    english(KEY_B); english(KEY_A, false, true); assert(state->macroKey.empty());
    english(KEY_B); vEnglishMode(MouseDown, 0, false, false); assert(state->macroKey.empty());
    vTempOffEngine(); english(KEY_B); assert(state->macroKey.empty());
    english(KEY_SPACE); english(KEY_DOT); assert(state->macroKey.size() == 1);
    assert(initMacroMap(nullptr, 0));

    assert(initSmartSwitchKey(nullptr, 0));
    setAppInputMethodStatus("editor", 7);
    getSmartSwitchKeySaveData(saved);
    for (size_t length = 1; length < saved.size(); length++) {
        assert(!initSmartSwitchKey(saved.data(), static_cast<int>(length)));
        assert(getAppInputMethodStatus("editor", 1) == 7);
    }
    assert(!initSmartSwitchKey(truncated, 2));
    assert(!initSmartSwitchKey(nullptr, 2));
    assert(initSmartSwitchKey(saved.data(), static_cast<int>(saved.size())));
    assert(getAppInputMethodStatus("editor", 0) == 7);
    assert(initSmartSwitchKey(nullptr, 0));
    assert(getAppInputMethodStatus("editor", 1) == -1);
    assert(getAppInputMethodStatus("editor", 0) == 1);
}

static void conversionTests() {
    ConvertOptions options;
    assert(convertUtil(u8"ABC ĐẸP", options) == u8"ABC ĐẸP");
    assert(convertUtil(u8"Tiếng Việt 😀", options) == u8"Tiếng Việt 😀");
    options.toAllNonCaps = true;
    assert(convertUtil(u8"ABC ĐẸP", options) == u8"abc đẹp");
    options.toAllNonCaps = false;
    options.toAllCaps = true;
    assert(convertUtil(u8"abc đẹp", options) == u8"ABC ĐẸP");
    options = {};
    options.toCapsFirstLetter = true;
    assert(convertUtil(u8"  hELLO. wORLD", options) == "  Hello. World");
    options = {};
    options.toCapsEachWord = true;
    assert(convertUtil(u8"tiẾng VIỆT", options) == u8"Tiếng Việt");
    options = {};
    options.removeMark = true;
    assert(convertUtil(u8"ĐẸP đẹp", options) == "DEP dep");
    options = {};
    options.toCode = 3;
    const string compound = convertUtil(u8"Á á ĐẸP", options);
    options.fromCode = 3;
    options.toCode = 0;
    assert(convertUtil(compound, options) == u8"Á á ĐẸP");
    // Every populated table entry must decode as the original scan did, including duplicate TCVN3 codes.
    for (Uint8 code = 0; code < 5; code++) {
        for (const auto& entry : _codeTable[code]) {
            for (Uint16 character : entry.second) {
                if (!character) continue;
                Uint32 key = 0; size_t index = 0;
                assert(findCharacter(character, code, key, index));
                bool first = true;
                for (const auto& scanned : _codeTable[code]) {
                    const auto found = std::find(scanned.second.begin(), scanned.second.end(), character);
                    if (found == scanned.second.end()) continue;
                    assert(key == scanned.first && index == size_t(found - scanned.second.begin()));
                    first = false; break;
                }
                assert(!first);
            }
        }
    }
    for (Uint8 code : {Uint8(2), Uint8(4)}) {
        options = {};
        options.toCode = code;
        const string encoded = convertUtil(u8"Tiếng Việt", options);
        options.fromCode = code;
        options.toCode = 0;
        assert(convertUtil(encoded, options) == u8"Tiếng Việt");
    }
    const string embeddedNull("a\0B", 3);
    assert(convertUtil(embeddedNull, ConvertOptions{}) == embeddedNull);
    options = {};
    options.fromCode = 5;
    bool rejected = false;
    try { convertUtil("text", options); } catch (const std::invalid_argument&) { rejected = true; }
    assert(rejected);
    // The UI-facing overload must snapshot every configured option.
    convertToolFromCode = convertToolToCode = 0;
    convertToolToAllCaps = 1;
    assert(convertUtil(u8"đẹp") == u8"ĐẸP");
    convertToolToAllCaps = 0;
    vector<std::future<void>> workers;
    for (int n = 0; n < 4; n++) {
        workers.push_back(std::async(std::launch::async, [] {
            for (int repeat = 0; repeat < 1000; repeat++) {
                assert(convertUtil(u8"Tiếng Việt", ConvertOptions{}) == u8"Tiếng Việt");
            }
        }));
    }
    for (auto& worker : workers) worker.get();
}

int main() {
    conversionTests();
    inputTests();
    featureTests();
    storageTests();
    std::cout << "Engine regressions passed\n";
}
