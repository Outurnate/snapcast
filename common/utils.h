/***
    This file is part of snapcast
    Copyright (C) 2014-2016  Johannes Pohl

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
***/

#ifndef UTILS_H
#define UTILS_H

#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <string>
#include <cstring>
#include <vector>
#include <fstream>
#include <sstream>
#include <memory>
#include <cerrno>
#include <iterator>
#ifndef WINDOWS
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#endif
#include <iomanip>

#ifdef WINDOWS
#include <chrono>
#include <windows.h>
#include <direct.h>
#include <winsock2.h>
#include <iphlpapi.h>
#include <versionhelpers.h>
#endif

// trim from start
static inline std::string &ltrim(std::string &s)
{
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
	return s;
}

// trim from end
static inline std::string &rtrim(std::string &s)
{
	s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	return s;
}

// trim from both ends
static inline std::string &trim(std::string &s)
{
	return ltrim(rtrim(s));
}

// trim from start
static inline std::string ltrim_copy(const std::string &s)
{
	std::string str(s);
	return ltrim(str);
}

// trim from end
static inline std::string rtrim_copy(const std::string &s)
{
	std::string str(s);
	return rtrim(str);
}

// trim from both ends
static inline std::string trim_copy(const std::string &s)
{
	std::string str(s);
	return trim(str);
}

// decode %xx to char
static std::string uriDecode(const std::string& src) {
	std::string ret;
	char ch;
	for (size_t i=0; i<src.length(); i++)
	{
		if (int(src[i]) == 37)
		{
			unsigned int ii;
			sscanf(src.substr(i+1, 2).c_str(), "%x", &ii);
			ch = static_cast<char>(ii);
			ret += ch;
			i += 2;
		}
		else
		{
			ret += src[i];
		}
	}
	return (ret);
}



static std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim))
	{
		elems.push_back(item);
	}
	return elems;
}


static std::vector<std::string> split(const std::string &s, char delim)
{
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

#ifdef WINDOWS
static int mkdirRecursive(const char *path)
#else
static int mkdirRecursive(const char *path, mode_t mode)
#endif
{
	std::vector<std::string> pathes = split(path, '/');
	std::stringstream ss;
	int res = 0;
	for (const auto& p: pathes)
	{
		if (p.empty())
			continue;
		ss << "/" << p;
#ifndef WINDOWS
		int res = mkdir(ss.str().c_str(), mode);
#else
		int res = _mkdir(ss.str().c_str());
#endif
		if ((res != 0) && (errno != EEXIST))
			return res;
	}
	return res;
}

#ifndef WINDOWS
static std::string execGetOutput(const std::string& cmd)
{
	std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
	if (!pipe)
		return "";
	char buffer[1024];
	std::string result = "";
	while (!feof(pipe.get()))
	{
		if (fgets(buffer, 1024, pipe.get()) != NULL)
			result += buffer;
	}
	return trim(result);
}
#endif

#ifdef ANDROID
static std::string getProp(const std::string& prop)
{
	return execGetOutput("getprop " + prop);
}
#endif


static std::string getOS()
{
	std::string os;
#ifdef ANDROID
	os = trim_copy("Android " + getProp("ro.build.version.release"));
#elif WINDOWS
	if (/*IsWindows10OrGreater()*/FALSE)
		os = "Windows 10";
	else if (IsWindows8Point1OrGreater())
		os = "Windows 8.1";
	else if (IsWindows8OrGreater())
		os = "Windows 8";
	else if (IsWindows7SP1OrGreater())
		os = "Windows 7 SP1";
	else if (IsWindows7OrGreater())
		os = "Windows 7";
	else if (IsWindowsVistaSP2OrGreater())
		os = "Windows Vista SP2";
	else if (IsWindowsVistaSP1OrGreater())
		os = "Windows Vista SP1";
	else if (IsWindowsVistaOrGreater())
		os = "Windows Vista";
	else if (IsWindowsXPSP3OrGreater())
		os = "Windows XP SP3";
	else if (IsWindowsXPSP2OrGreater())
		os = "Windows XP SP2";
	else if (IsWindowsXPSP1OrGreater())
		os = "Windows XP SP1";
	else if (IsWindowsXPOrGreater())
		os = "Windows XP";
	else
		os = "Unknown Windows";
#else
	os = execGetOutput("lsb_release -d");
	if (os.find(":") != std::string::npos)
		os = trim_copy(os.substr(os.find(":") + 1));
	if (os.empty())
	{
		os = trim_copy(execGetOutput("cat /etc/os-release /etc/openwrt_release |grep -e PRETTY_NAME -e DISTRIB_DESCRIPTION"));
		if (os.find("=") != std::string::npos)
		{
			os = trim_copy(os.substr(os.find("=") + 1));
			os.erase(std::remove(os.begin(), os.end(), '"'), os.end());
			os.erase(std::remove(os.begin(), os.end(), '\''), os.end());
		}
	}
#endif
#ifndef WINDOWS
	if (os.empty())
	{
		utsname u;
		uname(&u);
		os = u.sysname;
	}
#endif
	return trim_copy(os);
}


static std::string getHostName()
{
#ifdef ANDROID
	std::string result = getProp("net.hostname");
	if (!result.empty())
		return result;
#endif
	char hostname[1024];
	hostname[1023] = '\0';
	gethostname(hostname, 1023);
	return hostname;
}

static std::string getArch()
{
	std::string arch;
#ifdef ANDROID
	arch = getProp("ro.product.cpu.abi");
	if (!arch.empty())
		return arch;
#endif
#ifndef WINDOWS
	arch = execGetOutput("arch");
	if (arch.empty())
		arch = execGetOutput("uname -i");
	if (arch.empty() || (arch == "unknown"))
		arch = execGetOutput("uname -m");
#else
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	switch (sysInfo.wProcessorArchitecture)
	{
	case PROCESSOR_ARCHITECTURE_AMD64:
		arch = "amd64";
		break;
		
	case PROCESSOR_ARCHITECTURE_ARM:
		arch = "arm";
		break;
		
	case PROCESSOR_ARCHITECTURE_IA64:
		arch = "ia64";
		break;
		
	case PROCESSOR_ARCHITECTURE_INTEL:
		arch = "intel";
		break;
		
	default:
	case PROCESSOR_ARCHITECTURE_UNKNOWN:
		arch = "unknown";
		break;
	}
#endif
	return trim_copy(arch);
}


static long uptime()
{
#ifndef WINDOWS
	struct sysinfo info;
	sysinfo(&info);
	return info.uptime;
#else
	return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(GetTickCount())).count();
#endif
}

#ifndef WINDOWS
static std::string getMacAddress(int sock)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[1024];
	int success = 0;

	if (sock < 0)
		return "";

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1)
		return "";

	struct ifreq* it = ifc.ifc_req;
	const struct ifreq* const end = it + (ifc.ifc_len / sizeof(struct ifreq));

	for (; it != end; ++it)
	{
		strcpy(ifr.ifr_name, it->ifr_name);
		if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0)
		{
			if (!(ifr.ifr_flags & IFF_LOOPBACK)) // don't count loopback
			{
				if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0)
				{
					success = 1;
					break;
				}
				else
				{
					std::stringstream ss;
					ss << "/sys/class/net/" << ifr.ifr_name << "/address";
					std::ifstream infile(ss.str().c_str());
					std::string line;
					if (infile.good() && std::getline(infile, line))
					{
						trim(line);
						if ((line.size() == 17) && (line[2] == ':'))
							return line;
					}
				}
			}
		}
		else { /* handle error */ }
	}

	if (!success)
		return "";

	char mac[19];
	sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
		(unsigned char)ifr.ifr_hwaddr.sa_data[0], (unsigned char)ifr.ifr_hwaddr.sa_data[1], (unsigned char)ifr.ifr_hwaddr.sa_data[2],
		(unsigned char)ifr.ifr_hwaddr.sa_data[3], (unsigned char)ifr.ifr_hwaddr.sa_data[4], (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
	return mac;
}
#else
static std::string getMacAddress(const std::string& address)
{
	IP_ADAPTER_INFO* first;
	IP_ADAPTER_INFO* pos;
	ULONG bufferLength = sizeof(IP_ADAPTER_INFO);
	first = (IP_ADAPTER_INFO*)malloc(bufferLength);

	if (GetAdaptersInfo(first, &bufferLength) == ERROR_BUFFER_OVERFLOW)
	{
		free(first);
		first = (IP_ADAPTER_INFO*)malloc(bufferLength);
	}

	char mac[19];
	if (GetAdaptersInfo(first, &bufferLength) == NO_ERROR)
		for (pos = first; pos != NULL; pos = pos->Next)
		{
			IP_ADDR_STRING* firstAddr = &pos->IpAddressList;
			IP_ADDR_STRING* posAddr;
			for (posAddr = firstAddr; posAddr != NULL; posAddr = posAddr->Next)
				if (_stricmp(posAddr->IpAddress.String, address.c_str()) == 0)
				{
					sprintf(mac, "%02x:%02x:%02x:%02x:%02x:%02x",
									pos->Address[0], pos->Address[1], pos->Address[2],
									pos->Address[3], pos->Address[4], pos->Address[5]);
					
					free(first);
					return mac;
				}
		}
	else
		free(first);
	
	return mac;
}
#endif

#endif
