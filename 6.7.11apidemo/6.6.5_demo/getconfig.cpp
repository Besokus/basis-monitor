#pragma  once
#include "stdafx.h"
#include "getconfig.h"
#ifdef _WIN32
#include <wtypes.h>
#include <conio.h>
#else
#include "linux_compat.h"
#endif
#include <iostream>
#include <locale>
#include <string>
#include <fstream>
#include <string>
#include <time.h>
#include <sstream>
#include <locale.h>
#include <vector>
#include <cctype>

/*�������ƣ�getConfig()
�������ܣ���ȡ�����ļ�ini����Ӧ�����title��ָ�������ֶ�cfgname��ֵ
����1��string title		�����[***]
����2��string cfgName		������µ������ֶ�
����ֵ�������ļ�ini����Ӧ�����title��ָ�������ֶ�cfgname��ֵ
*/

vector<string> split(const string &str, const string &pattern)
{
	//const char* convert to char*
	char * strc = new char[strlen(str.c_str()) + 1];
	strcpy(strc, str.c_str());
	vector<string> resultVec;
	char* tmpStr = strtok(strc, pattern.c_str());
	while (tmpStr != NULL)
	{
		resultVec.push_back(string(tmpStr));
		tmpStr = strtok(NULL, pattern.c_str());
	}

	delete[] strc;

	return resultVec;
}

static const int CFG_FILE_NOT_FOUND = 91001;
static const int CFG_KEY_NOT_FOUND = 91002;

static void LogConfigStatus(int code, const char* statusId, const string& detail)
{
	cout << "[CONFIG_STATUS] code=" << code << " id=" << statusId << " detail=" << detail << endl;
}

static string Trim(const string& input)
{
	size_t start = 0;
	while (start < input.size() && std::isspace(static_cast<unsigned char>(input[start])))
	{
		++start;
	}
	size_t end = input.size();
	while (end > start && std::isspace(static_cast<unsigned char>(input[end - 1])))
	{
		--end;
	}
	return input.substr(start, end - start);
}

static string StripUtf8Bom(const string& input)
{
	if (input.size() >= 3 &&
		static_cast<unsigned char>(input[0]) == 0xEF &&
		static_cast<unsigned char>(input[1]) == 0xBB &&
		static_cast<unsigned char>(input[2]) == 0xBF)
	{
		return input.substr(3);
	}
	return input;
}

static bool TryReadConfigValue(const string& title, const string& cfgName, string& returnValue, string& iniFilePath)
{
	vector<string> iniCandidates;
	iniCandidates.push_back("config.ini");
	iniCandidates.push_back("./config.ini");
	iniCandidates.push_back("../config.ini");

	ifstream inifile;
	iniFilePath = "config.ini";
	for (size_t i = 0; i < iniCandidates.size(); ++i)
	{
		inifile.open(iniCandidates[i].c_str());
		if (inifile.is_open())
		{
			iniFilePath = iniCandidates[i];
			break;
		}
		inifile.clear();
	}
	if (!inifile.is_open())
	{
		LogConfigStatus(CFG_FILE_NOT_FOUND, "CONFIG_FILE_NOT_FOUND", "tried=config.ini|./config.ini|../config.ini");
		return false;
	}

	string strtmp, strtitle, strcfgname;
	int flag = 0;
	while (getline(inifile, strtmp, '\n'))
	{
		strtmp = StripUtf8Bom(strtmp);
		strtmp = Trim(strtmp);
		if (strtmp.empty())
		{
			continue;
		}
		if (strtmp.substr(0, 1) == "#" || strtmp.substr(0, 1) == ";")
		{
			continue;
		}
		if (flag == 0)
		{
			if (strtmp.substr(0, 1) == "[" && strtmp.find("]") != string::npos)
			{
				strtitle = Trim(strtmp.substr(1, strtmp.find("]") - 1));
				if (strtitle == title)
				{
					flag = 1;
				}
			}
			continue;
		}
		if (flag == 1)
		{
			if (strtmp.substr(0, 1) == "[")
			{
				break;
			}
			size_t equalPos = strtmp.find("=");
			if (equalPos == string::npos)
			{
				continue;
			}
			strcfgname = Trim(strtmp.substr(0, equalPos));
			if (strcfgname == cfgName)
			{
				returnValue = Trim(strtmp.substr(equalPos + 1));
				return true;
			}
		}
	}

	LogConfigStatus(
		CFG_KEY_NOT_FOUND,
		"CONFIG_KEY_NOT_FOUND",
		string("section=") + title + " key=" + cfgName + " file=" + iniFilePath);
	return false;
}

//void split(const string& md, vector<string>& sv, const char flag = ' ')
//{
//	sv.clear();
//	istringstream iss(md);
//	string temp;
//
//	while (getline(iss, temp, flag)) {
//		sv.push_back(temp);
//	}
//	return;
//}

string getConfig(string title, string cfgName)
{
	string returnValue;
	string iniFilePath;
	if (TryReadConfigValue(title, cfgName, returnValue, iniFilePath))
	{
		return returnValue;
	}
	_getch();
	exit(-1);
}

string getConfigOptional(string title, string cfgName, const string& defaultValue)
{
	string returnValue;
	string iniFilePath;
	if (TryReadConfigValue(title, cfgName, returnValue, iniFilePath))
	{
		return returnValue;
	}
	LogConfigStatus(
		CFG_KEY_NOT_FOUND,
		"CONFIG_OPTIONAL_KEY_FALLBACK",
		string("section=") + title + " key=" + cfgName + " default=" + defaultValue);
	return defaultValue;
}
