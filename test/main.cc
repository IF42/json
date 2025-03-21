#include <rapidjson/document.h>
#include <iostream>
#include <fstream>
#include <sstream>

#define LARGE_JSON_FILE "assets/large-file.json"
#define SMALL_JSON_FILE "assets/example.json"

#define JSON_FILE LARGE_JSON_FILE


int main() {
    std::ifstream file(JSON_FILE);
    if (!file.is_open()) {
        std::cerr << "Chyba: Nelze otevřít soubor " << JSON_FILE << std::endl;
        return 1;
    }

    // Načtení obsahu souboru do řetězce
    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    // Parsování JSON
    rapidjson::Document doc;
    if (doc.Parse(buffer.str().c_str()).HasParseError()) {
        std::cerr << "Chyba: JSON má chybný formát." << std::endl;
        return 1;
    }

    std::cout << "Program exit..\n" << std::endl;
    return 0;
}
