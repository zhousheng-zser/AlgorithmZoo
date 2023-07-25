#pragma once
#ifndef _FILES_
#define _FILES_

#include <stdio.h>
#include <string>
#include <string.h>
#include <iostream>

#ifdef WIN32
#include <io.h>
#include <direct.h> 
#else
#include <unistd.h>
#include <sys/stat.h>
#include <stdint.h>
#endif

#define MAX_PATH_LEN 256

#ifdef WIN32
#define ACCESS(fileName,accessMode) _access(fileName,accessMode)
#define MKDIR(path) _mkdir(path)
#define RMDIR(a) _rmdir((a))
#else
#define ACCESS(fileName,accessMode) access(fileName,accessMode)
#define MKDIR(path) mkdir(path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)
#define RMDIR(a) rmdir((a))
#endif

namespace noah {


	static inline std::string absolutePath(std::string path)
	{
#ifdef _WIN32
		char absPath[4096] = { 0 };
		_fullpath(absPath, path.c_str(), 4096);
#else
		//linux 需要大点的空间
		char absPath[40960] = { 0 };
		realpath(path.c_str(), absPath);
#endif
		for (auto& c : absPath)
			c = c == '\\' ? '/' : c;
		return std::string(absPath);
	}

	static inline int32_t createDirectory(std::string directoryPath)
	{
		if (directoryPath.find('.') != std::string::npos) {
			std::cout << "[files.hpp][createDirectory] create failure, " << directoryPath << " is invalid path !" << std::endl;
			return -1;
		}
		directoryPath = *directoryPath.rbegin() == '/' ? directoryPath : directoryPath + '/';
		std::string lastMakePath;
		uint32_t dirPathLen = directoryPath.length();
		if (dirPathLen > MAX_PATH_LEN)
		{
			return -1;
		}
		char tmpDirPath[MAX_PATH_LEN] = { 0 };
		for (uint32_t i = 0; i < dirPathLen; ++i)
		{
			tmpDirPath[i] = directoryPath[i];
			if (tmpDirPath[i] == '\\' || tmpDirPath[i] == '/')
			{
				if (ACCESS(tmpDirPath, 0) != 0)
				{
					int32_t ret = MKDIR(tmpDirPath);
					if (ret != 0)
					{
						std::cout << "[files.hpp][createDirectory] MKDIR error !" << std::endl;
						return ret;
					}
					else {
						lastMakePath = tmpDirPath;
						std::cout << "[files.hpp][createDirectory] create "<< tmpDirPath << std::endl;
					}
				}
			}
		}

		if(!lastMakePath.empty())
			std::cout << "[files.hpp][createDirectory] create " << lastMakePath << " success !" << std::endl;
		else
			std::cout << "[files.hpp][createDirectory] nothing" << std::endl;

		return 0;
	}

	static inline bool removeDirectory(std::string strPath)
	{
#ifdef _WIN32
		struct _finddata_t fb;   //查找相同属性文件的存储结构体
		//制作用于正则化路径
		if (strPath.at(strPath.length() - 1) != '\\' || strPath.at(strPath.length() - 1) != '/')
			strPath.append("\\");
		std::string findPath = strPath + "*";
		intptr_t handle;//用long类型会报错
		handle = _findfirst(findPath.c_str(), &fb);
		//找到第一个匹配的文件
		if (handle != -1L)
		{
			std::string pathTemp;
			do//循环找到的文件 
			{
				//系统有个系统文件，名为“..”和“.”,对它不做处理  
				if (strcmp(fb.name, "..") != 0 && strcmp(fb.name, ".") != 0)//对系统隐藏文件的处理标记
				{
					//制作完整路径
					pathTemp.clear();
					pathTemp = strPath + std::string(fb.name);
					//属性值为16，则说明是文件夹，迭代  
					if (fb.attrib == _A_SUBDIR)//_A_SUBDIR=16
					{
						removeDirectory(pathTemp.c_str());
					}
					//非文件夹的文件，直接删除。对文件属性值的情况没做详细调查，可能还有其他情况。  
					else
					{
						remove(pathTemp.c_str());
					}
				}
			} while (0 == _findnext(handle, &fb));//判断放前面会失去第一个搜索的结果
			//关闭文件夹，只有关闭了才能删除。找这个函数找了很久，标准c中用的是closedir  
			//经验介绍：一般产生Handle的函数执行后，都要进行关闭的动作。  
			_findclose(handle);
		}
		//移除文件夹  
		return RMDIR(strPath.c_str()) == 0 ? true : false;

#elif __linux__
		if (strPath.at(strPath.length() - 1) != '\\' || strPath.at(strPath.length() - 1) != '/')
			strPath.append("/");
		DIR* d = opendir(strPath.c_str());//打开这个目录
		if (d != NULL)
		{
			struct dirent* dt = NULL;
			while (dt = readdir(d))//逐个读取目录中的文件到dt
			{
				//系统有个系统文件，名为“..”和“.”,对它不做处理
				if (strcmp(dt->d_name, "..") != 0 && strcmp(dt->d_name, ".") != 0)//判断是否为系统隐藏文件
				{
					struct stat st;//文件的信息
					std::string fileName;//文件夹中的文件名
					fileName = strPath + std::string(dt->d_name);
					stat(fileName.c_str(), &st);
					if (S_ISDIR(st.st_mode))
					{
						removeDirectory(fileName);
					}
					else
					{
						remove(fileName.c_str());
					}
				}
			}
			closedir(d);
		}
		return rmdir(strPath.c_str()) == 0 ? true : false;
#endif

	}

	static inline void clearDirectory(std::string Path) {
		removeDirectory(Path);
		createDirectory(Path);
	}
}
#endif // _FILES_