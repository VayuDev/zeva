#include <iostream>
#include <json/json.h>
#include <string>
#include <chrono>
#include <fstream>
#include "Base64.hpp"

int main() {
    std::chrono::high_resolution_clock::time_point start;
    std::string converted;
    {
        std::ifstream t("../../assets/zeva.svg");
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        start = std::chrono::high_resolution_clock::now();
        str = base64_encode(reinterpret_cast<const unsigned char *>(str.c_str()), str.size());
        Json::Value v;
        v["nice_image"] = str;
        converted = v.toStyledString();
    }

    std::stringstream convertedStream{converted};
    Json::Value re;
    convertedStream >> re;
    auto output =  base64_decode(re["nice_image"].asString());

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Took: " << ((end - start).count() / 1'000'000.0) << "ms";

    std::ofstream out{"/tmp/out.png"};
    out << output;


}