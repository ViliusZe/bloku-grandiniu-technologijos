#include <filesystem>
#include <chrono>
#include <vector>
#include <random>
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
double measureHashBatch(const std::string& text,
    std::size_t repetitions, std::uint64_t& checksum) {
    // Volatile funkcijos rodykle neleidzia pakelti kvietimo už ciklo.
    std::string (*volatile hashFunction)(const std::string&) = &calculateHash;

    auto start = std::chrono::steady_clock::now();

    for (std::size_t i = 0; i < repetitions; ++i) {
        std::string hash = hashFunction(text);
        checksum += static_cast<unsigned char>(hash[0]);
    }

    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

int benchmarkFile(const std::string& filename) {
    std::string text;
    if (!readFile(filename, text)) {
        return 1;
    }
    if (text.empty()) {
        std::cerr << "Klaida: spartos bandymui reikia netuscio failo.\n";
        return 1;
    }

    // Saugome originalius baitus ir eilučiu skirtukus.
    std::vector<std::size_t> lineEnds;
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '\n') lineEnds.push_back(i + 1);
    }
    if (lineEnds.empty() || lineEnds.back() != text.size()) {
        lineEnds.push_back(text.size());
    }

    std::vector<std::size_t> lineCounts;
    for (std::size_t lines = 1; lines < lineEnds.size(); lines *= 2) {
        lineCounts.push_back(lines);
    }
    lineCounts.push_back(lineEnds.size());

    std::ofstream raw("benchmark_raw.csv");
    std::ofstream summary("benchmark_summary.csv");
    if (!raw.is_open() || !summary.is_open()) {
        std::cerr << "Klaida: nepavyko sukurti spartos rezultatu failu.\n";
        return 1;
    }

    raw << "lines,bytes,trial,iterations,total_us,per_hash_us\n";
    summary << "lines,bytes,iterations,mean_us,min_us,max_us\n";
    raw << std::fixed << std::setprecision(6);
    summary << std::fixed << std::setprecision(6);
    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Laikrodis: steady_clock\n"
        << "Vienetas: mikrosekundes vienai maisai\n"
        << "Matavimu kiekvienam dydziui: 5\n"
        << "lines,bytes,iterations,mean_us,min_us,max_us\n";

    std::uint64_t checksum = 0;
    for (std::size_t lines : lineCounts) {
        // Ištrauka sudaroma pries matuojant laika.
        std::string input = text.substr(0, lineEnds[lines - 1]);

        measureHashBatch(input, 3, checksum); // Apsilimas.

        // Parenkame grupę, kuriai reikia bent 20 ms, kai imanoma.
        std::size_t repetitions = 1;
        while (true) {
            double elapsed = measureHashBatch(input, repetitions, checksum);
            if (elapsed >= 20000.0 || repetitions >= 1048576) break;
            repetitions *= 2;
        }

        double minimum = 0, maximum = 0, sum = 0;
        for (int trial = 1; trial <= 5; ++trial) {
            double elapsed = measureHashBatch(input, repetitions, checksum);
            double perHash = elapsed / static_cast<double>(repetitions);

            if (trial == 1 || perHash < minimum) minimum = perHash;
            if (trial == 1 || perHash > maximum) maximum = perHash;
            sum += perHash;

            raw << lines << ',' << input.size() << ',' << trial << ','
                << repetitions << ',' << elapsed << ',' << perHash << '\n';
        }

        summary << lines << ',' << input.size() << ',' << repetitions << ','
            << sum / 5.0 << ',' << minimum << ',' << maximum << '\n';
        std::cout << lines << ',' << input.size() << ',' << repetitions << ','
            << sum / 5.0 << ',' << minimum << ',' << maximum << '\n';
    }

    raw.close();
    summary.close();
    if (!raw || !summary) {
        std::cerr << "Klaida: nepavyko issaugoti visu matavimu.\n";
        return 1;
    }

    std::cout << "Kontroline suma: " << checksum << '\n'
        << "Rezultatai: benchmark_raw.csv, benchmark_summary.csv\n";
    return 0;
}

struct InputTestCase {
    std::string filename;
    std::string category;
    std::string data;
    std::string reference = "";
    std::size_t changedPosition = 0;
};

std::vector<InputTestCase> makeInputCases(unsigned int seed,
    const std::string& alphabet) {
    std::vector<InputTestCase> cases = {
        {"empty.bin", "empty", ""},
        {"one_a.txt", "one_byte", "a"},
        {"one_b.txt", "one_byte", "b"}
    };

    std::mt19937 generator(seed);
    std::uniform_int_distribution<std::size_t> character(0, alphabet.size() - 1);
    const std::array<std::size_t, 3> lengths = {2000, 5000, 10000};
    const std::array<std::string, 3> labels = {"start", "middle", "end"};

    for (std::size_t length : lengths) {
        std::string original(length, ' ');
        for (char& ch : original) ch = alphabet[character(generator)];

        std::string base = "random_" + std::to_string(length);
        cases.push_back({base + ".txt", "random_original", original});
        const std::array<std::size_t, 3> positions = {0, length / 2, length - 1};

        for (std::size_t i = 0; i < positions.size(); ++i) {
            std::string changed = original;
            std::size_t position = positions[i];
            changed[position] = original[position] == 'a' ? 'b' : 'a';
            cases.push_back({base + "_" + labels[i] + ".txt",
                "single_byte_change", changed, base + ".txt", position});
        }
    }

    cases.push_back({"repeat_a_2000.txt", "repeated", std::string(2000, 'a')});
    std::string repeated;
    for (int i = 0; i < 1000; ++i) repeated += "ab";
    cases.push_back({"repeat_ab_2000.txt", "repeated", repeated});
    cases.push_back({"order_ab.txt", "permutation", "ab"});
    cases.push_back({"order_ba.txt", "permutation", "ba"});
    cases.push_back({"plain.txt", "spaces", "tekstas"});
    cases.push_back({"leading_space.txt", "spaces", " tekstas"});
    cases.push_back({"trailing_space.txt", "spaces", "tekstas "});
    cases.push_back({"both_spaces.txt", "spaces", " tekstas "});
    cases.push_back({"newline_lf.txt", "newline", "tekstas\n"});
    cases.push_back({"newline_crlf.txt", "newline", "tekstas\r\n"});
    cases.push_back({"utf8_lt.txt", "utf8", "\xC4\x85\xC5\xBE" "uolas"});
    cases.push_back({"binary.bin", "binary", std::string("\x00\x01\x7F\x80\xFF\n\r", 7)});
    return cases;
}

bool saveAndCheckInput(const std::filesystem::path& folder,
    const InputTestCase& test, std::ofstream& manifest) {
    std::filesystem::path path = folder / test.filename;
    std::ofstream out(path, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "Klaida: nepavyko sukurti " << path.string() << '\n';
        return false;
    }
    out.write(test.data.data(), static_cast<std::streamsize>(test.data.size()));
    out.close();
    if (!out) {
        std::cerr << "Klaida: nepavyko irasyti " << test.filename << '\n';
        return false;
    }

    std::string loaded;
    if (!readFile(path.string(), loaded)) return false;
    if (loaded != test.data) {
        std::cerr << "Klaida: nesutampa failo baitai: " << test.filename << '\n';
        return false;
    }

    std::string hash = calculateHash(loaded);
    if (hash != calculateHash(test.data) || hash.size() != 64
        || hash.find_first_not_of("0123456789abcdef") != std::string::npos) {
        std::cerr << "Klaida: netinkama maisa: " << test.filename << '\n';
        return false;
    }

    std::size_t differentBytes = 0;
    if (!test.reference.empty()) {
        std::string original;
        if (!readFile((folder / test.reference).string(), original)) return false;
        if (loaded.size() != original.size() || test.changedPosition >= loaded.size()) {
            std::cerr << "Klaida: netinkamas pakeistos ivesties dydis.\n";
            return false;
        }
        for (std::size_t i = 0; i < loaded.size(); ++i) {
            if (loaded[i] != original[i]) ++differentBytes;
        }
        if (differentBytes != 1
            || loaded[test.changedPosition] == original[test.changedPosition]) {
            std::cerr << "Klaida: pakeistas ne vienas nurodytas baitas.\n";
            return false;
        }
    }

    manifest << test.filename << ',' << test.category << ','
        << loaded.size() << ',' << hash << ',';
    if (!test.reference.empty()) {
        manifest << test.reference << ',' << test.changedPosition << ',' << differentBytes;
    } else {
        manifest << ",,";
    }
    manifest << ",OK\n";
    std::cout << "OK: " << test.filename << " | " << loaded.size() << " baitu\n";
    return true;
}

int testInputFiles() {
    const unsigned int seed = 2026;
    const std::string alphabet =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const std::filesystem::path folder = "test_inputs";
    std::error_code error;
    std::filesystem::create_directories(folder, error);
    if (error) {
        std::cerr << "Klaida: nepavyko sukurti test_inputs: " << error.message() << '\n';
        return 1;
    }

    std::ofstream manifest(folder / "manifest.csv");
    if (!manifest.is_open()) {
        std::cerr << "Klaida: nepavyko sukurti manifest.csv\n";
        return 1;
    }
    manifest << "file,category,bytes,hash,reference,changed_position,different_bytes,status\n";
    const auto cases = makeInputCases(seed, alphabet);
    std::size_t mutations = 0;
    for (const InputTestCase& test : cases) {
        if (!saveAndCheckInput(folder, test, manifest)) return 1;
        if (!test.reference.empty()) ++mutations;
    }
    manifest.close();
    if (!manifest) {
        std::cerr << "Klaida: nepavyko issaugoti manifesto.\n";
        return 1;
    }

    std::ofstream conditions(folder / "conditions.txt");
    if (!conditions.is_open()) {
        std::cerr << "Klaida: nepavyko sukurti conditions.txt\n";
        return 1;
    }
    conditions << "algorithm=v0.1\ngenerator=std::mt19937\nseed=" << seed
        << "\nalphabet=" << alphabet
        << "\nrandom_lengths_bytes=2000,5000,10000"
        << "\nmutation_positions=0,length/2,length-1 (zero-based)"
        << "\nutf8_lt.txt=7_characters,9_bytes,UTF-8_without_BOM"
        << "\nnewlines=exact_LF_and_CRLF_bytes"
        << "\nhash_bits=256\nhex_digits=64\nfiles=" << cases.size() << '\n';
#if defined(_MSC_FULL_VER)
    conditions << "compiler=MSVC " << _MSC_FULL_VER << '\n';
#elif defined(__clang__)
    conditions << "compiler=Clang " << __clang_version__ << '\n';
#elif defined(__GNUC__)
    conditions << "compiler=GCC " << __VERSION__ << '\n';
#endif
#if defined(_MSVC_LANG)
    conditions << "cpp_standard=" << _MSVC_LANG << '\n';
#else
    conditions << "cpp_standard=" << __cplusplus << '\n';
#endif
#if defined(_MSVC_STL_VERSION)
    conditions << "stdlib=MSVC_STL " << _MSVC_STL_VERSION << '\n';
#elif defined(__GLIBCXX__)
    conditions << "stdlib=libstdc++ " << __GLIBCXX__ << '\n';
#elif defined(_LIBCPP_VERSION)
    conditions << "stdlib=libc++ " << _LIBCPP_VERSION << '\n';
#endif
    conditions.close();
    if (!conditions) {
        std::cerr << "Klaida: nepavyko issaugoti bandymo salygu.\n";
        return 1;
    }

    std::cout << "Sukurta ir patikrinta failu: " << cases.size() << '\n'
        << "Vieno baito pakeitimu: " << mutations << '\n'
        << "Rezultatai: test_inputs/manifest.csv\n"
        << "Salygos: test_inputs/conditions.txt\n";
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif
    if (argc == 2 && std::string(argv[1]) == "--input-tests") {
        return testInputFiles();
    }
    if (argc >= 2 && std::string(argv[1]) == "--benchmark") {
        if (argc != 3) {
            std::cerr << "Naudojimas: MaisosGeneratorius.exe --benchmark failo_kelias\n";
            return 1;
        }
        return benchmarkFile(argv[2]);
    }
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
            << "Naudojimas: MaisosGeneratorius.exe [failo_kelias]\n"
            << "Spartos bandymas: MaisosGeneratorius.exe --benchmark failo_kelias\n";
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
