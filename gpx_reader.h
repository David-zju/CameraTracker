# pragma once
#include <expat.h>
#include <exiv2/exiv2.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

const std::vector<std::string> supportedExtensions = {
    ".cr2", // Canon
    ".nef", // Nikon
    ".arw", // Sony
    ".dng", // Adobe
    ".orf", // Olympus
    ".raf", // Fujifilm
    ".raw", // Panasonic/Leica
    ".rw2", // Panasonic
    ".jpg", // JPEG
};

bool isSupported(const std::string& filename){
    std::string lowerExtension;
    std::transform(filename.begin(), filename.end(), std::back_inserter(lowerExtension), ::tolower);
    return std::find(supportedExtensions.begin(), supportedExtensions.end(), lowerExtension) != supportedExtensions.end();
}

std::string doubleToGpsInfo(double coord) {
    // 提取度
    int degrees = static_cast<int>(floor(coord));
    double decimalPart = fmod(coord, 1.0);

    // 将小数部分转换为分钟
    double minutesWithFraction = decimalPart * 60.0;
    int minutes = static_cast<int>(floor(minutesWithFraction));
    decimalPart = fmod(minutesWithFraction, 1.0);

    // 将剩余的小数部分转换为秒
    double seconds = decimalPart * 60.0;

    // 格式化字符串
    // 例如，如果你需要秒数以10000为分母的分数形式
    int secondsNumerator = static_cast<int>(seconds * 10000);
    int secondsDenominator = 10000;

    // 创建并返回结果字符串
    std::string result = std::to_string(degrees) + "/1 " +
                         std::to_string(minutes) + "/1 " +
                         std::to_string(secondsNumerator) + "/" + std::to_string(secondsDenominator);
    return result;
}

double fractionToDouble(const std::string& fraction) {
    std::istringstream iss(fraction);
    double numerator = 0.0, denominator = 1.0;
    char slash; // 用于读取分数中的斜杠'/'
    if (!(iss >> numerator >> slash >> denominator)) {
        std::cerr << "[ERROR] Invalid fraction format: " << fraction << std::endl;
        return 0.0;
    }
    return numerator / denominator;
}

double gpsInfoToDouble(const std::string& gpsInfo) {
    std::istringstream iss(gpsInfo);
    std::string token;
    std::vector<double> parts;

    // 按空格拆分字符串，并计算每个分数的double值
    while (std::getline(iss, token, ' ')) {
        parts.push_back(fractionToDouble(token));
    }

    // 假设GPS格式为 度 分 秒（DMS），转换为十进制度（DD）
    if (parts.size() == 3) {
        double degrees = parts[0];
        double minutes = parts[1] / 60.0;
        double seconds = parts[2] / 3600.0;
        return degrees + minutes + seconds;
    } else {
        std::cerr << "[ERROR] Unexpected number of parts in GPS info: " << gpsInfo << std::endl;
        return 0.0;
    }
}

struct GPXPoint{
    double lat, lon; // 经纬度
    std::string timeStr; // 时间 ISO8601格式
    std::chrono::system_clock::time_point time_point;
    std::string src; // 数据源
    double ele; // 高程（m）
    double course, speed; // 航向（度），速度（m/s） 
    short sat; // 卫星数量
    float geoidheight; // 大地高度：相对于大地水准面的高度（m）
    float hdop, vdop, pdop; // 水平、垂直、位置精度

    bool operator<(const GPXPoint& p) const{
        // compare the time
        return time_point < p.time_point;
    }
    void clear(){
        lat = lon = ele = course = speed = geoidheight = hdop = vdop = pdop = 0.0;
        timeStr = src = "";
        time_point = std::chrono::system_clock::time_point{};
    }
    void printPoint(short precision){
        // 目前主要使用这几个信息，其他的如果有需要可以添加
        std::cout.precision(precision);
        std::cout << std::setw(precision+1) << std::setfill(' ') << std::left
                  << lat << " "
                  << std::setw(precision+1) << std::setfill(' ') << std::left
                  << lon << " "
                  << std::setw(22) << std::setfill(' ') << std::left
                  << timeStr << " "
                  << std::setw(8) << std::setfill(' ') << std::left
                  << src << std::endl;
    }
};

class GPXDataBuffer{
    public:
        std::vector<GPXPoint> points;
        // 存储当前处理的位置点信息
        GPXPoint currentPoint;
        bool inTime, inEle, inSrc, inCourse, inSpeed, inSat, inGeoidheight, inHdop, inVdop, inPdop;
        GPXDataBuffer():inTime(false), inEle(false), inSrc(false), inCourse(false), inSpeed(false), 
                        inSat(false), inGeoidheight(false), inHdop(false), inVdop(false), inPdop(false){};
        void addPoint(){
            points.push_back(currentPoint);
        }
        void getGPXPoint(const std::chrono::system_clock::time_point& photoTime, std::pair<double, double>& location){
            bool processed = false;
            std::sort(points.begin(), points.end());
            if(photoTime > points.back().time_point || photoTime < points.front().time_point){
                std::time_t t_c = std::chrono::system_clock::to_time_t(photoTime);
                std::tm tm_c = *std::localtime(&t_c);
                std::cerr << "[Warning] photo shot time " << std::put_time(&tm_c, "%Y-%m-%d %H:%M:%S") <<" is not recorded in gpx file"<< std::endl;
                std::cerr << "[Warning] recorded time range: " << points.front().timeStr << " -- " << points.back().timeStr << std::endl;
                return;
            }
            for(size_t i = 0; i < points.size(); ++i){
                if(points[i].time_point > photoTime){
                    std::chrono::duration du2 = points[i].time_point - photoTime;
                    std::chrono::duration du1 = photoTime - points[i-1].time_point;
                    double param1 = du2 / (du1 + du2);
                    double param2 = du1 / (du1 + du2);
                    location.first = points[i-1].lat * param1 + points[i].lat * param2;
                    location.second = points[i-1].lon * param1 + points[i].lon * param2;
                    processed = true;
                }
            }
            if(!processed){
                std::time_t t_c = std::chrono::system_clock::to_time_t(photoTime);
                std::tm tm_c = *std::localtime(&t_c);
                std::cerr << "[Waring] photo shot time " << std::put_time(&tm_c, "%Y-%m-%d %H:%M:%S") <<" is not recorded in gpx file"<< std::endl;
                std::cerr << "[Waring] recorded time range: " << points.front().timeStr << " -- " << points.back().timeStr << std::endl;
            }
            return;
        }
};
void timeConvert(std::string time, std::chrono::system_clock::time_point& output){
    // convert time as ISO8601 standard
    auto zIndex = time.find("Z");
    if(zIndex == std::string::npos){
        std::cerr << "[ERROR] gpx time format error: " << time << std::endl;
        return ;
    }
    auto dotIndex = time.find(".");
    int ms = 0;
    if(dotIndex != std::string::npos){
        std::string millsecondsStr = time.substr(dotIndex+1, zIndex - dotIndex - 1); 
        ms = static_cast<int>(stod("0." + millsecondsStr) * 1000);
        time = time.substr(0, dotIndex);
    }
    // subtract ms from time because get_time() doesn't support millseconds

    std::istringstream iss(time);
    std::tm tm;
    tm.tm_isdst = -1;
    iss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if(iss.fail()){
        std::cerr << "[ERROR] gpx time format error: " << time << std::endl;
        return ;
    }
    time_t mk_t = std::mktime(&tm);
    output = std::chrono::system_clock::from_time_t(mk_t);
    output += std::chrono::milliseconds(ms);
    // output += std::chrono::hours(8);

}

// 当解析器遇到一个元素的开始标签时调用
void XMLCALL startElement(void *userData, const char *name, const char **atts) {
    auto data = static_cast<GPXDataBuffer*>(userData);
    GPXPoint& current = data->currentPoint;

    if (strcmp(name, "trkpt") == 0) {
        // 找到一个轨迹点元素，获取其纬度和经度属性
        double lat = 0.0, lon = 0.0;
        for (int i = 0; atts[i]; i += 2) {
            if (strcmp(atts[i], "lat") == 0) {
                current.lat = atof(atts[i + 1]);
            } else if (strcmp(atts[i], "lon") == 0) {
                current.lon = atof(atts[i + 1]);
            }
        }
    } else if (strcmp(name, "time") == 0) {
        data->inTime = true;
    } else if (strcmp(name, "ele") == 0) {
        data->inEle = true;
    } else if (strcmp(name, "src") == 0) {
        data->inSrc = true;
    } else if (strcmp(name, "course") == 0) {
        data->inCourse = true;
    } else if (strcmp(name, "speed") == 0) {
        data->inSpeed = true;
    } else if (strcmp(name, "geoidheight") == 0) {
        data->inGeoidheight = true;
    } else if (strcmp(name, "sat") == 0) {
        data->inSat = true;
    } else if (strcmp(name, "hdop") == 0) {
        data->inHdop = true;
    } else if (strcmp(name, "vdop") == 0) {
        data->inVdop = true;
    } else if (strcmp(name, "pdop") == 0) {
        data->inPdop = true;
    }
}

// 当解析器遇到一个元素的结束标签时调用
void XMLCALL endElement(void *userData, const char *name) {
    auto data = static_cast<GPXDataBuffer*>(userData);
    if (strcmp(name, "trkpt") == 0) {
        data->addPoint();
        data->currentPoint.clear();
    } else if(strcmp(name, "src") == 0){
        data->inSrc = false;
    } else if (strcmp(name, "time") == 0) {
        data->inTime = false;
    } else if (strcmp(name, "ele") == 0) {
        data->inEle = false;
    } else if (strcmp(name, "course") == 0) {
        data->inCourse = false;
    } else if (strcmp(name, "speed") == 0) {
        data->inSpeed = false;
    } else if (strcmp(name, "geoidheight") == 0) {
        data->inGeoidheight = false;
    } else if (strcmp(name, "sat") == 0) {
        data->inSat = false;
    } else if (strcmp(name, "hdop") == 0) {
        data->inHdop = false;
    } else if (strcmp(name, "vdop") == 0) {
        data->inVdop = false;
    } else if (strcmp(name, "pdop") == 0) {
        data->inPdop = false;
    }
}

// 当解析器遇到文本内容时调用
void XMLCALL characterDataHandler(void *userData, const XML_Char *s, int len) {
    auto data = static_cast<GPXDataBuffer*>(userData);
    if(data->inTime){
        std::chrono::system_clock::time_point tp;
        timeConvert(std::string(s, len), tp);
        data->currentPoint.time_point = tp;
        data->currentPoint.timeStr = std::string(s, len);
    } else if (data->inEle){
        data->currentPoint.ele = atof(std::string(s, len).c_str());
    } else if (data->inSrc){
        data->currentPoint.src = std::string(s, len);
    } else if (data->inCourse){
        data->currentPoint.course = atof(std::string(s, len).c_str());
    } else if (data->inSpeed){
        data->currentPoint.speed = atof(std::string(s, len).c_str());
    } else if (data->inGeoidheight){
        data->currentPoint.geoidheight = atof(std::string(s, len).c_str());
    } else if (data->inSat){
        data->currentPoint.sat = atoi(std::string(s, len).c_str());
    } else if (data->inPdop){
        data->currentPoint.pdop = atof(std::string(s, len).c_str());
    } else if (data->inHdop){
        data->currentPoint.hdop = atof(std::string(s, len).c_str());
    } else if (data->inVdop){
        data->currentPoint.vdop = atof(std::string(s, len).c_str());
    }
    
}

void parseGPX(const std::string filename, GPXDataBuffer& data) {
    // 创建一个XML解析器
    XML_Parser parser = XML_ParserCreate(NULL);
    if (!parser) {
        std::cerr << "[ERROR] cannot create xml parser" << std::endl;
        return;
    }

    // 注册回调函数
    XML_SetUserData(parser, &data);
    XML_SetElementHandler(parser, startElement, endElement);
    XML_SetCharacterDataHandler(parser, characterDataHandler);

    // 打开GPX文件
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "[ERROR] cannot open gpx file " << filename << std::endl;;
        XML_ParserFree(parser);
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    if (buffer.str().empty()) {
        std::cerr << "[WARNING] gpx file " << filename << " is empty" << std::endl;
        XML_ParserFree(parser);
        return;
    }
    // 读取文件内容并提供给解析器
    if (XML_Parse(parser, buffer.str().c_str(), buffer.str().length(), true) == XML_STATUS_ERROR) {
        std::cerr << "[ERROR] gpx " << filename << " parse error: " 
                << XML_ErrorString(XML_GetErrorCode(parser)) 
                << " at line " << XML_GetCurrentLineNumber(parser) << std::endl;
    }

    // 清理
    XML_ParserFree(parser);
}


// Photo Process

size_t readImageTime(const Exiv2::Image::UniquePtr& image, const std::string& timeOffset, bool photo_default, std::chrono::system_clock::time_point& utc_time) {
    // photo default == true means using time zone offset information in the photo
    using namespace Exiv2;
 
    time_t local_time = 0;
    // "Exif.Image.DateTime" is unsafe to use, use "Exif.Image.DateTimeOriginal" instead
    // "Exif.Image.DateTime" of photos modified by some software may be different from the original time
    const std::vector<std::string> dateStrings = {"Exif.Photo.DateTimeOriginal", "Exif.Photo.DateTimeDigitized"};
    const std::vector<std::string> offsetStrings = {"Exif.Photo.OffsetTimeOriginal", "Exif.Photo.OffsetTimeDigitized"};
    for (size_t i = 0; !local_time && i < 2; i++) {
        const std::string dateString = dateStrings[i];
        try {
            image->readMetadata();
            ExifData& exifData = image->exifData();
            std::istringstream iss(exifData[dateString].toString().c_str());
            std::tm tm;
            tm.tm_isdst = -1;
            iss >> std::get_time(&tm, "%Y:%m:%d %H:%M:%S");
            if(iss.fail()){
                std::cerr << "[ERROR] time format error: " << exifData[dateString].toString() << std::endl;
                return 1;
            }
            local_time = std::mktime(&tm);
            if(photo_default){
                const std::string offsetString = offsetStrings[i];
                int hours, minutes; 
                char sign;
                if(exifData[offsetString].toString().empty()){
                    std::cerr << "[ERROR] time offset not found in photo: ";
                    return 1;
                }
                std::sscanf(exifData[offsetString].toString().c_str(), "%c%02d:%02d", &sign, &hours, &minutes);
                auto offset = std::chrono::hours(hours) + std::chrono::minutes(minutes);
                if (sign == '+') offset = -offset;
                utc_time = std::chrono::system_clock::from_time_t(local_time) + offset;
            } else {
                int hours, minutes; 
                char sign;
                std::sscanf(timeOffset.c_str(), "%c%02d:%02d", &sign, &hours, &minutes);
                auto offset = std::chrono::hours(hours) + std::chrono::minutes(minutes);
                if (sign == '+') offset = -offset;
                utc_time = std::chrono::system_clock::from_time_t(local_time) + offset;
            }
        } catch (...) {};
    }

    if(!local_time){
        std::cerr << "[ERROR] time offset not found in photo: ";
    }
    return 0;
}

void writePhotoGPS(Exiv2::Image::UniquePtr& image, std::pair<double, double> gps){
    Exiv2::ExifData& exifData = image->exifData();
    exifData["Exif.GPSInfo.GPSLatitude"] = doubleToGpsInfo(gps.first);
    exifData["Exif.GPSInfo.GPSLongitude"] = doubleToGpsInfo(gps.second);
    image->writeMetadata();
}