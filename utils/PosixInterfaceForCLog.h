#pragma once
/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

struct FILEWRAP; // forward declaration, wrapper for FILE

class CPosixInterfaceForCLog
{
public:
  CPosixInterfaceForCLog();
  ~CPosixInterfaceForCLog();
  bool OpenLogFile(const std::string& logFilename, const std::string& backupOldLogToFilename);
  void CloseLogFile(void);
  bool WriteStringToLog(const std::string& logString);
  static void GetCurrentLocalTime(int& year, int& month, int& day,
	  int& hour, int& minute, int& second);
private:
  FILEWRAP* m_file;
};
