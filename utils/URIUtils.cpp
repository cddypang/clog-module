/*
 *      Copyright (C) 2005-2013 Team XBMC
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


#include "URIUtils.h"
#include "StringUtils.h"
#include <stdlib.h>
#include <cassert>
#include <algorithm>

#ifdef __gnu_linux__
#include <strings.h>
#define strnicmp strncasecmp
#endif

using namespace std;

bool URIUtils::IsInPath(const std::string &uri, const std::string &baseURI)
{
  return !baseURI.empty() && StringUtils::StartsWith(uri, baseURI);
}

/* returns filename extension including period of filename */

std::string URIUtils::GetExtension(const std::string& strFileName)
{
  size_t period = strFileName.find_last_of("./\\");
  if (period == string::npos || strFileName[period] != '.')
    return std::string();

  return strFileName.substr(period);
}

bool URIUtils::HasExtension(const std::string& strFileName)
{
  size_t iPeriod = strFileName.find_last_of("./\\");
  return iPeriod != string::npos && strFileName[iPeriod] == '.';
}

bool URIUtils::HasExtension(const std::string& strFileName, const std::string& strExtensions)
{
  // Search backwards so that '.' can be used as a search terminator.
  std::string::const_reverse_iterator itExtensions = strExtensions.rbegin();
  while (itExtensions != strExtensions.rend())
  {
    // Iterate backwards over strFileName untill we hit a '.' or a mismatch
    for (std::string::const_reverse_iterator itFileName = strFileName.rbegin();
         itFileName != strFileName.rend() && itExtensions != strExtensions.rend() &&
         tolower(*itFileName) == *itExtensions;
         ++itFileName, ++itExtensions)
    {
      if (*itExtensions == '.')
        return true; // Match
    }

    // No match. Look for more extensions to try.
    while (itExtensions != strExtensions.rend() && *itExtensions != '|')
      ++itExtensions;

    while (itExtensions != strExtensions.rend() && *itExtensions == '|')
      ++itExtensions;
  }

  return false;
}

void URIUtils::RemoveExtension(std::string& strFileName)
{
  size_t period = strFileName.find_last_of("./\\");
  if (period != string::npos && strFileName[period] == '.')
  {
    std::string strExtension = strFileName.substr(period);
    StringUtils::ToLower(strExtension);
    strFileName.erase(period);
  }
}

std::string URIUtils::ReplaceExtension(const std::string& strFile,
                                      const std::string& strNewExtension)
{
  std::string strChangedFile;
  std::string strExtension = GetExtension(strFile);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
  return strChangedFile;
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const std::string URIUtils::GetFileName(const std::string& strFileNameAndPath)
{
  /* find the last slash */
  const size_t slash = strFileNameAndPath.find_last_of("/\\");
  return strFileNameAndPath.substr(slash+1);
}

void URIUtils::Split(const std::string& strFileNameAndPath,
                     std::string& strPath, std::string& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  // take left including the directory separator
  strPath = strFileNameAndPath.substr(0, i+1);
  // everything to the right of the directory separator
  strFileName = strFileNameAndPath.substr(i+1);
}

// std::vector<std::string> URIUtils::SplitPath(const std::string& strPath)
// {
//   CURL url(strPath);
// 
//   // silly std::string can't take a char in the constructor
//   std::string sep(1, url.GetDirectorySeparator());
// 
//   // split the filename portion of the URL up into separate dirs
//   vector<string> dirs = StringUtils::Split(url.GetFileName(), sep);
//   
//   // we start with the root path
//   std::string dir = url.GetWithoutFilename();
//   
//   if (!dir.empty())
//     dirs.insert(dirs.begin(), dir);
// 
//   // we don't need empty token on the end
//   if (dirs.size() > 1 && dirs.back().empty())
//     dirs.erase(dirs.end() - 1);
// 
//   return dirs;
// }

void URIUtils::GetCommonPath(std::string& strParent, const std::string& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent.erase(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!HasSlashAtEnd(strParent))
  {
    strParent = GetDirectory(strParent);
    AddSlashAtEnd(strParent);
  }
}

std::string URIUtils::GetParentPath(const std::string& strPath)
{
  std::string strReturn;
  GetParentPath(strPath, strReturn);
  return strReturn;
}

bool URIUtils::GetParentPath(const std::string& strPath, std::string& strParent)
{
  strParent.clear();
  std::string strFile = strPath;

  if (HasSlashAtEnd(strPath) )
  {
    strFile.erase(strFile.size() - 1);
  }

  size_t iPos = strFile.rfind('/');
#ifndef TARGET_POSIX
  if (iPos == std::string::npos)
  {
    iPos = strFile.rfind('\\');
  }
#endif
  if (iPos == std::string::npos)
  {
    strParent = "";
    return true;
  }

  strFile.erase(iPos);
  AddSlashAtEnd(strFile);
  strParent = strFile;
  return true;
}

bool URIUtils::IsDOSPath(const std::string &path)
{
	if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
		return true;

	// windows network drives
	if (path.size() > 1 && path[0] == '\\' && path[1] == '\\')
		return true;

	return false;
}

void URIUtils::AddSlashAtEnd(std::string& strFolder)
{
  if (!HasSlashAtEnd(strFolder))
  {
//     if (IsDOSPath(strFolder))
//       strFolder += '\\';
//     else
//       strFolder += '/';
	  if (IsDOSPath(strFolder))
		  strFolder = FixSlashesAndDups(strFolder);
	  strFolder += '/';
  }
}

bool URIUtils::HasSlashAtEnd(const std::string& strFile)
{
  if (strFile.empty()) return false;

  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

void URIUtils::RemoveSlashAtEnd(std::string& strFolder)
{
  while (HasSlashAtEnd(strFolder))
    strFolder.erase(strFolder.size()-1, 1);
}

bool URIUtils::CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2)
{
  std::string strc1 = strPath1, strc2 = strPath2;
  RemoveSlashAtEnd(strc1);
  RemoveSlashAtEnd(strc2);
  return StringUtils::EqualsNoCase(strc1, strc2);
}


std::string URIUtils::FixSlashesAndDups(const std::string& path, const char slashCharacter /* = '/' */, const size_t startFrom /*= 0*/)
{
  const size_t len = path.length();
  if (startFrom >= len)
    return path;

  std::string result(path, 0, startFrom);
  result.reserve(len);

  const char* const str = path.c_str();
  size_t pos = startFrom;
  do
  {
    if (str[pos] == '\\' || str[pos] == '/')
    {
      result.push_back(slashCharacter);  // append one slash
      pos++;
      // skip any following slashes
      while (str[pos] == '\\' || str[pos] == '/') // str is null-terminated, no need to check for buffer overrun
        pos++;
    }
    else
      result.push_back(str[pos++]);   // append current char and advance pos to next char

  } while (pos < len);

  return result;
}


std::string URIUtils::CanonicalizePath(const std::string& path, const char slashCharacter /*= '\\'*/)
{
  assert(slashCharacter == '\\' || slashCharacter == '/');

  if (path.empty())
    return path;

  const std::string slashStr(1, slashCharacter);
  vector<std::string> pathVec, resultVec;
  StringUtils::Tokenize(path, pathVec, slashStr);

  for (vector<std::string>::const_iterator it = pathVec.begin(); it != pathVec.end(); ++it)
  {
    if (*it == ".")
    { /* skip - do nothing */ }
    else if (*it == ".." && !resultVec.empty() && resultVec.back() != "..")
      resultVec.pop_back();
    else
      resultVec.push_back(*it);
  }

  std::string result;
  if (path[0] == slashCharacter)
    result.push_back(slashCharacter); // add slash at the begin

  result += StringUtils::Join(resultVec, slashStr);

  if (path[path.length() - 1] == slashCharacter  && !result.empty() && result[result.length() - 1] != slashCharacter)
    result.push_back(slashCharacter); // add slash at the end if result isn't empty and result isn't "/"

  return result;
}

std::string URIUtils::AddFileToFolder(const std::string& strFolder, 
                                const std::string& strFile)
{
  std::string strResult = strFolder;
  if (!strResult.empty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.substr(1);
  else
    strResult += strFile;

  // correct any slash directions
//   if (!IsDOSPath(strFolder))
//     StringUtils::Replace(strResult, '\\', '/');
//   else
//     StringUtils::Replace(strResult, '/', '\\');

  return strResult;
}

std::string URIUtils::GetDirectory(const std::string &strFilePath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end and possible |option=foo options.

  size_t iPosSlash = strFilePath.find_last_of("/\\");
  if (iPosSlash == string::npos)
    return ""; // No slash, so no path (ignore any options)

  size_t iPosBar = strFilePath.rfind('|');
  if (iPosBar == string::npos)
    return strFilePath.substr(0, iPosSlash + 1); // Only path

  return strFilePath.substr(0, iPosSlash + 1) + strFilePath.substr(iPosBar); // Path + options
}

std::string URIUtils::resolvePath(const std::string &path)
{
  if (path.empty())
    return path;

  size_t posSlash = path.find('/');
  size_t posBackslash = path.find('\\');
  string delim = posSlash < posBackslash ? "/" : "\\";
  vector<string> parts = StringUtils::Split(path, delim);
  vector<string> realParts;

  for (vector<string>::const_iterator part = parts.begin(); part != parts.end(); ++part)
  {
    if (part->empty() || part->compare(".") == 0)
      continue;

    // go one level back up
    if (part->compare("..") == 0)
    {
      if (!realParts.empty())
        realParts.pop_back();
      continue;
    }

    realParts.push_back(*part);
  }

  std::string realPath;
  // re-add any / or \ at the beginning
  for (std::string::const_iterator itPath = path.begin(); itPath != path.end(); ++itPath)
  {
    if (*itPath != delim.at(0))
      break;

    realPath += delim;
  }
  // put together the path
  realPath += StringUtils::Join(realParts, delim);
  // re-add any / or \ at the end
  if (path.at(path.size() - 1) == delim.at(0) && realPath.at(realPath.size() - 1) != delim.at(0))
    realPath += delim;

  return realPath;
}
