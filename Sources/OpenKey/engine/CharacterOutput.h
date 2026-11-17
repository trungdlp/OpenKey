#ifndef CharacterOutput_h
#define CharacterOutput_h

#include "Vietnamese.h"

inline size_t getCharacterUnits(Uint32 data, int codeTable, Uint16* units) {
    units[0] = static_cast<Uint16>(data);
    if (data & PURE_CHARACTER_MASK) return 1;
    if (!(data & CHAR_CODE_MASK)) {
        units[0] = keyCodeToCharacter(data);
        return units[0] ? 1 : 0;
    }
    if (codeTable == 1 || codeTable == 2 || codeTable == 4) {
        units[0] = data & 0xFF;
        units[1] = (data >> 8) & 0xFF;
        return units[1] > 32 ? 2 : 1;
    }
    if (codeTable == 3) {
        const Uint16 mark = (data & CHAR_MASK) >> 13;
        units[0] = data & 0x1FFF;
        if (mark > 0 && mark <= 5) {
            units[1] = _unicodeCompoundMark[mark - 1];
            return 2;
        }
    }
    return 1;
}

#endif
