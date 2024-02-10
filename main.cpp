#include "gpx_reader.h"
void allocateGPS(GPXDataBuffer& data, const std::filesystem::path& searchPath) {
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
                if(readImageTime(image, "+08:00", false, t)) std::cerr << filePath << std::endl;
                data.getGPXPoint(t, loc);
                writePhotoGPS(image, loc);
            }
        }
    }
}

int main(){
    std::string patternStr = ".";
    std::string searchPath = ".";
    GPXDataBuffer data;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(searchPath)){
        if (entry.is_regular_file()) {
            std::string filePath = entry.path().string();
            std::string fileName = entry.path().filename().string();
            std::string extension = entry.path().extension().string();
            if(extension == ".gpx"){
                parseGPX(filePath,data);
            }
        }
    }
    
    allocateGPS(data, searchPath);
    return 0;
}