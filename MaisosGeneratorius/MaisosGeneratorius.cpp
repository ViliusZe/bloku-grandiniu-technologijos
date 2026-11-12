#include <array>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <iomanip>
#ifdef _WIN32
#include <windows.h>
#include <random>
#endif
constexpr std::size_t MAX_INPUT_BYTES = 16u * 1024u * 1024u;
std::uint32_t rotateLeft13(std::uint32_t value) {
    return (value << 13) | (value >> 19);
}

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

    // Baigiamasis maišymas tarp būsenos skaičių.
    for (int round = 0; round < 8; ++round) {
        const auto previous = state;

        for (std::size_t i = 0; i < state.size(); ++i) {
            std::uint32_t mixed =
                previous[i] ^ previous[(i + 1) % state.size()];

            mixed = rotateLeft13(mixed);

            state[i] = mixed * multipliers[i]
                + previous[(i + 3) % state.size()];
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
int countDifferentBits(const std::string& first,
    const std::string& second) {
    int count = 0;

    for (std::size_t i = 0; i < first.size(); i += 2) {
        int a = std::stoi(first.substr(i, 2), nullptr, 16);
        int b = std::stoi(second.substr(i, 2), nullptr, 16);

        int difference = a ^ b;

        while (difference != 0) {
            count += difference & 1;
            difference >>= 1;
        }
    }

    return count;
}

void testAvalanche() {
    std::string first = calculateHash("ab");
    std::string second = calculateHash("ac");

    int differentBits = countDifferentBits(first, second);
    double percentage = 100.0 * differentBits / 256.0;

    std::cout << "ab maisa: " << first << '\n';
    std::cout << "ac maisa: " << second << '\n';
    std::cout << "Skirtingu bitu: " << differentBits << " is 256\n";
    std::cout << "Bitu skirtumas: " << percentage << "%\n";
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
int testFormat() {
    const std::array<std::string, 7> inputs = {
        "", "a", "ab", "ba", " a ",
        std::string(2000, 'x'),
        "\xC4\x85"
    };

    for (const std::string& text : inputs) {
        std::string hash = calculateHash(text);

        if (hash.size() != 64) {
            std::cerr << "KLAIDA: netinkamas maisos ilgis: "
                << hash.size() << '\n';
            return 1;
        }

        if (hash.find_first_not_of("0123456789abcdef")
            != std::string::npos) {
            std::cerr << "KLAIDA: netinkami hex simboliai.\n";
            return 1;
        }

        std::cout << "Formatas tinkamas. Ivesties baitu: "
            << text.size() << '\n';
    }

    std::cout << "Formato testai pavyko.\n";
    return 0;
}
struct MetricStats {
    double minimum = 100.0;
    double maximum = 0.0;
    double sum = 0.0;
    std::size_t count = 0;

    void add(double value) {
        if (value < minimum) minimum = value;
        if (value > maximum) maximum = value;

        sum += value;
        ++count;
    }

    double mean() const {
        return sum / static_cast<double>(count);
    }
};

void printMetric(const std::string& name, const MetricStats& stats) {
    std::cout << name << ": min=" << stats.minimum
        << "% vid=" << stats.mean()
        << "% max=" << stats.maximum << "%\n";
}

int testAvalancheStatistics() {
    const unsigned int seed = 2026;
    const int pairsPerLength = 25000;
    const std::array<std::size_t, 4> lengths = { 10, 100, 500, 1000 };

    const std::string alphabet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    std::mt19937 generator(seed);
    std::uniform_int_distribution<std::size_t> character(
        0, alphabet.size() - 1
    );

    std::ofstream out("avalanche_raw.csv");

    if (!out.is_open()) {
        std::cerr << "Klaida: nepavyko sukurti avalanche_raw.csv\n";
        return 1;
    }

    out << "length,pair,position,different_bits,different_hex,"
        "bits_percent,hex_percent\n";

    out << std::fixed << std::setprecision(6);
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "Seed: " << seed
        << "\nAbecele: " << alphabet << '\n';

    MetricStats allBits, allHex;

    for (std::size_t length : lengths) {
        MetricStats bitStats, hexStats;

        std::uniform_int_distribution<std::size_t> position(
            0, length - 1
        );

        for (int pair = 0; pair < pairsPerLength; ++pair) {
            std::string first(length, ' ');

            for (char& ch : first) {
                ch = alphabet[character(generator)];
            }

            std::string second = first;
            std::size_t changed = position(generator);

            do {
                second[changed] = alphabet[character(generator)];
            } while (second[changed] == first[changed]);

            std::string firstHash = calculateHash(first);
            std::string secondHash = calculateHash(second);

            int bits = countDifferentBits(firstHash, secondHash);
            int hex = 0;

            for (std::size_t i = 0; i < firstHash.size(); ++i) {
                if (firstHash[i] != secondHash[i]) {
                    ++hex;
                }
            }

            double bitsPercent = 100.0 * bits / 256.0;
            double hexPercent = 100.0 * hex / 64.0;

            bitStats.add(bitsPercent);
            hexStats.add(hexPercent);
            allBits.add(bitsPercent);
            allHex.add(hexPercent);

            out << length << ',' << pair + 1 << ',' << changed << ','
                << bits << ',' << hex << ','
                << bitsPercent << ',' << hexPercent << '\n';
        }

        std::cout << "\nIlgis: " << length
            << ", poru: " << bitStats.count << '\n';

        printMetric("Bitai", bitStats);
        printMetric("Hex", hexStats);
    }

    out.close();

    if (!out) {
        std::cerr << "Klaida: nepavyko issaugoti visu rezultatu.\n";
        return 1;
    }

    std::cout << "\nViso poru: " << allBits.count << '\n';
    printMetric("Bitai", allBits);
    printMetric("Hex", allHex);
    std::cout << "Rezultatai: avalanche_raw.csv\n";

    return 0;
}
int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc == 2 && std::string(argv[1]) == "--test") {
        if (testFormat() != 0) {
            return 1;
        }

        return testDeterminism();
    }
    if (argc == 2 && std::string(argv[1]) == "--avalanche") {
        testAvalanche();
        return 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--avalanche-stats") {
        return testAvalancheStatistics();
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