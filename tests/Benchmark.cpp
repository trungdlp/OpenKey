#include "Engine.h"
#include <algorithm>
#include <chrono>
#include <iostream>

int main() {
    const string input = "tooi ddang gox tieengs vieetj vaaf kieemr tra hieu nang ";
    vector<Uint16> keys;
    for (unsigned char ch : input) keys.push_back(_characterMap.at(ch));
    keys.push_back(KEY_RETURN);
    vKeyInit();
    for (int n = 0; n < 1000; n++) for (Uint16 key : keys) vKeyHandleEvent(Keyboard, KeyDown, key);
    vector<double> samples;
    samples.reserve(10000 * keys.size());
    for (int n = 0; n < 10000; n++) {
        for (Uint16 key : keys) {
            auto start = std::chrono::steady_clock::now();
            vKeyHandleEvent(Keyboard, KeyDown, key);
            samples.push_back(std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count());
        }
    }
    std::sort(samples.begin(), samples.end());
    std::cout << "Engine ns/key: p50=" << samples[samples.size() / 2]
              << " p95=" << samples[samples.size() * 95 / 100]
              << " p99=" << samples[samples.size() * 99 / 100] << '\n';
    string source;
    for (int n = 0; n < 20000; n++) source += "Tooi yeeru tieengs Vieetj. ";
    const auto start = std::chrono::steady_clock::now();
    const string result = convertUtil(source, ConvertOptions{});
    std::cout << "Conversion " << source.size() << " bytes: "
              << std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count()
              << " ms; output " << result.size() << " bytes\n";
}
