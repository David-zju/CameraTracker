# CameraTracker
一款用于设定相机拍摄照片gps信息的小工具

## 说明
目前的许多相册软件支持gps信息的读取，可以通过地图浏览照片。但是大多数相机没有gps功能，无法直接在图片的exif信息中添加拍摄位置。一些品牌的相机（例如SONY）支持使用手机app连接相机来解决这一问题，但是每次相机重启时都需要重新连接，而且有时也会出现连接不稳定。因此我们希望能够有一个工具可以离线地给拍摄的照片添加gps信息。

实现原理是使用一个可以记录gpx文件的设备（可以使用运动手表或在安卓手机上安装gpslogger）
https://github.com/mendhak/gpslogger
通过对齐gpx文件和照片的拍摄时间，将gpx文件中的gps信息添加到照片的exif信息中。

⚠️注意 目前由于Exiv2对部分RAW格式图片的exif信息修改后会导致图片本身损坏，因此目前暂时只支持JPEG格式的照片。

## 依赖
- Exiv2
- expat

## 使用
本项目使用cmake编译，在项目文件下创建文件夹build，然后在build文件夹下执行
```shell
cmake ..
make
```
使用时需要将build文件夹下的CameraTracker可执行文件添加到目录
```shell
export PATH="$PATH:/path/to/your/program"
```
将待处理的所有照片和gpx文件放在一个文件夹下，然后执行
```
CameraTracker
```

## TODO
- [ ] 文件筛选
- [ ] 手动设置时区

## Bug fixes
- [x] 排除读取MacOS下`.file`隐藏文件
  - 由于Finder会产生.DS_Store文件，以及一些其他的缓存文件，详细可以参考https://cn.bandisoft.com/kb/resource-fork-file/
