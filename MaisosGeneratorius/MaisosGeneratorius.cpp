#include <array>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#endif
constexpr std::size_t MAX_INPUT_BYTES = 16u * 1024u * 1024u;
std::string calculateHash(const std::string& text) {
    std::array<std::uint32_t, 8> state = {
        1, 2, 3, 4, 5, 6, 7, 8
    };

    const std::array<std::uint32_t, 8> multipliers = {
        31, 37, 41, 43, 47, 53, 59, 61
    };

    for (unsigned char ch : text) {
        for (std::size_t i = 0; i < state.size(); ++i) {
            state[i] = (state[i] * multipliers[i]) ^ ch;
        }
    }

    std::ostringstream out;
    out << std::hex << std::setfill('0');

    for (std::uint32_t value : state) {
        out << std::setw(8) << value;
    }

    return out.str();
}
bool readFile(const std::string& filename, std::string& text) {
    std::ifstream in(filename, std::ios::binary);

    if (!in.is_open()) {
        std::cerr << "Klaida: nepavyko atidaryti failo "
            << filename << '\n';
        return false;
    }

    text.clear();
    char ch;

    while (in.get(ch)) {
        if (text.size() >= MAX_INPUT_BYTES) {
            std::cerr << "Klaida: ivestis virsija 16 MiB.\n";
            return false;
        }

        text += ch;
    }

    if (in.bad() || !in.eof()) {
        std::cerr << "Klaida: nepavyko perskaityti viso failo.\n";
        return false;
    }

    return true;
}
int testDeterminism() {
    const std::array<std::string, 6> inputs = {
        "", "a", "ab", "ba", " a ", std::string(2000, 'x')
    };

    for (const std::string& text : inputs) {
        std::string expected = calculateHash(text);

        for (int repeat = 0; repeat < 5; ++repeat) {
            calculateHash("tarpinis tekstas");
            std::string actual = calculateHash(text);

            if (actual != expected) {
                std::cerr << "KLAIDA: maisa pasikeite. Ivesties baitu: "
                    << text.size() << '\n';
                return 1;
            }
        }

        std::cout << "OK: " << text.size() << " baitu | "
            << expected << '\n';
    }

    std::cout << "Determinizmo testai pavyko.\n";
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc == 2 && std::string(argv[1]) == "--test") {
        return testDeterminism();
    }
    std::string text;

    if (argc == 1) {
        std::cout << "Rezimas: ranka ivestas tekstas\n";
        std::cout << "Iveskite teksta: ";

        if (!std::getline(std::cin, text)) {
            std::cerr << "Klaida: nepavyko perskaityti teksto.\n";
            return 1;
        }
    }
    else if (argc == 2) {
        std::cout << "Rezimas: failo turinys\n";

        if (!readFile(argv[1], text)) {
            return 1;
        }
    }
    else {
        std::cerr
            << "Naudojimas: MaisosGeneratorius.exe [failo_kelias]\n";
        return 1;
    }

    if (text.size() > MAX_INPUT_BYTES) {
        std::cerr << "Klaida: ivestis virsija 16 MiB.\n";
        return 1;
    }

    std::cout << "Ivesties baitu: " << text.size() << '\n';

    std::string hash = calculateHash(text);
    std::cout << "Maisa: " << hash << '\n';

    return 0;
}