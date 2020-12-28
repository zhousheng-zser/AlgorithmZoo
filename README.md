# AlgorithmZoo
AlgorithmZoo是算法模型及算法逻辑的集合，目前包含了Longinus、Romancia、Cassius、Gaius、Irisviel五大基础模块。其依赖于[Primitives](https://github.com/CompileSense/Excalibur)和[Excalibur](https://github.com/CompileSense/Excalibur)，并实现了编译后动态链接库(.dll)或共享库(.so)的插件化。

- **Longinus** 集成了人脸检测和人脸跟踪算法逻辑及接口
- **Romancia** 集成了人脸对齐、人脸质量检测、人脸口罩检测、人脸活体检测算法逻辑及接口
- **Gaius** 实现了128维人脸特征提取功能及接口，适用于低算力平台，如arm
- **Cassius** 实现了512维人脸特征提取功能及接口，适用于高算力平台，如x86
- **Irisviel** 实现了人员库管理功能及接口，包含人员的添加、删除、更新、搜索比对等

#### 编译环境
|  系统 |  windows | ubuntu18.04  | centos7  | android  |
| ------------ | ------------ | ------------ | ------------ | ------------ |
|  编译器 | MSVC19.28  |  gcc7.4.1  |  gcc8.3.1 |  NDK-21d |
|  cmake版本 | 3.18.20081302-MSVC_2  |  3.15.0 | 3.15.0  |  3.15.0  |

#### 编译步骤
- **windows**
1. 将工程根目录下CMakeSettings.json.example重命名为CMakeSettings.json
2. 使用visual studio 2019以cmake工程形式打开AlgorithmZoo/cmake目录
3. 在VS中打开CMakeSettings.json，配置COMMON_LIBRARY_DIRS
4. 若未使用vcpkg安装boost及opencv，请在CMakeSettings.json中配置Boost_INCLUDE_DIRS、OpenCV_INCLUDE_DIRS、OpenCV_LIBRARY_DIRS、OpenCV_LIBS
5. 执行build

- **ubuntu**
1. 安装boost(version 1.65以上)及opencv(version 3.4.1以上)
1. cd AlgorithmZoo/cmake
2. mkdir build
3. cd build
4. cmake .. -DHAVE_SSE=ON -DHAVE_AVX=ON -DHAVE_AVX2=ON -DCOMMON_LIBRARY_DIRS=[where your excalibur and primitives library exist]
5. make

- **centos**
1. 安装gcc8.3.1、boost(version 1.65以上)及opencv(version 3.4.1以上)
2. scl enable devtoolset-8 bash
3. cd AlgorithmZoo/cmake
4. mkdir build
5. cd build
6. cmake .. -DHAVE_SSE=ON -DHAVE_AVX=ON -DHAVE_AVX2=ON -DCOMMON_LIBRARY_DIRS=[where your excalibur and primitives library exist]
7. make

- **Android**
1. cd AlgorithmZoo/cmake
2. 下载OpenCV-android-sdk，打开Dependencies.cmake，配置Opencv_INCLUDE_DIRS、OpenCV_LIBRARY_DIRS、OpenCV_LIBS
2. mkdir build
3. cmake.exe .. -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=E:/android-ndk-r21d/build/cmake/android.toolchain.cmake -DCMAKE_MAKE_PROGRAM=E:/android-ndk-r21d/prebuilt/windows-x86_64/bin/make.exe -DANDROID_PLATFORM=android-21 -DANDROID_ABI=arm64-v8a -DCOMMON_LIBRARY_DIRS=E:/Research/Source/Repos/Excalibur/cmake/build/lib -DBoost_INCLUDE_DIRS=C:/Tools/vcpkg/installed/x64-windows/include
4. make

### 未完待续