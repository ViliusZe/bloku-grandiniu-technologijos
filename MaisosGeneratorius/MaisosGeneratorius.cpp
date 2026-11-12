#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <iomanip>

std::uint32_t calculateHash(const std::string& text) {
    std::uint32_t result = 0;

    for (unsigned char ch : text) {
        result = (result * 31) ^ ch;
    }

    return result;
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
        text += ch;
    }

    if (in.bad() || !in.eof()) {
        std::cerr << "Klaida: nepavyko perskaityti viso failo.\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
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

    std::uint32_t hash = calculateHash(text);

    std::cout << "Maisa: "
        << std::hex << std::setfill('0') << std::setw(8)
        << hash << '\n';

    return 0;
}