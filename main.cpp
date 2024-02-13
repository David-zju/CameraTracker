#include "gpx_reader.h"
void allocateGPS(GPXDataBuffer& data, const std::filesystem::path& searchPath, std::string timezone) {
    using namespace Exiv2;
    if (!std::filesystem::exists(searchPath) || !std::filesystem::is_directory(searchPath)) {
        std::cerr << "[ERROR] Specified path is not a valid directory." << std::endl;
        return;
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)) {
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            std::string extension = entry.path().extension().string();
            if(fileName[0] == '.') continue; // ignore the hidden files
            // 使用正则表达式匹配文件名
            if (isSupported(extension)) {
                std::cout << "[INFO] Process file: " << filePath << std::endl;

                Image::UniquePtr image = ImageFactory::open(filePath);
                if (!image.get()) {
                    std::cerr << "[ERROR] Error opening image: " << filePath << std::endl;
                    return;
                }
                std::chrono::system_clock::time_point t;
                std::pair<double, double> loc;
                if(readImageTime(image, timezone, false, t)) std::cerr << filePath << std::endl;
                data.getGPXPoint(t, loc);
                writePhotoGPS(image, loc);
            }
        }
    }
}

int main(int argc, char* argv[]){
    std::map<std::string, std::string> args;
    for (int i = 0; i < argc; i ++) {
        if (!strncmp(argv[i], "-", 1)) {
            auto key = argv[i] + 1;
            auto val = "";
            if (i + 1 < argc && strncmp(argv[i + 1], "-", 1)) {
                val = argv[i + 1];
                i ++;
            }
            args[key] = val;
        }
    }

    std::string patternStr = ".";
    std::string searchPath = ".";
    std::string timezone = "+08:00"; // Beijing Timezone

    if(args.count("p")){
        searchPath = args["p"];
    }
    if(args.count("tz")){
        timezone = args["p"];
    }
    GPXDataBuffer data;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)){
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            std::string extension = entry.path().extension().string();
            if (fileName[0] == '.') continue; // ignore the hidden files
            if(extension == ".gpx"){
                parseGPX(filePath,data);
            }
        }
    }
    std::cout << "[INFO] Finish parsing gpx files" << std::endl;
    allocateGPS(data, searchPath, timezone);
    return 0;
}