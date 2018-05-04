
#pragma once

#include <string>
#include <vector>

class URIUtils
{
public:
  URIUtils(void);
  virtual ~URIUtils(void);
  static bool IsInPath(const std::string &uri, const std::string &baseURI);

  static std::string GetDirectory(const std::string &strFilePath);

  static const std::string GetFileName(const std::string& strFileNameAndPath);

  static std::string GetExtension(const std::string& strFileName);

  /*!
   \brief Check if there is a file extension
   \param strFileName Path or URL to check
   \return \e true if strFileName have an extension.
   \note Returns false when strFileName is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& strFileName);

  /*!
   \brief Check if filename have any of the listed extensions
   \param strFileName Path or URL to check
   \param strExtensions List of '.' prefixed lowercase extensions seperated with '|'
   \return \e true if strFileName have any one of the extensions.
   \note The check is case insensitive for strFileName, but requires
         strExtensions to be lowercase. Returns false when strFileName or
         strExtensions is empty.
   \sa GetExtension
   */
  static bool HasExtension(const std::string& strFileName, const std::string& strExtensions);

  static void RemoveExtension(std::string& strFileName);
  static std::string ReplaceExtension(const std::string& strFile,
                                     const std::string& strNewExtension);
  static void Split(const std::string& strFileNameAndPath, 
                    std::string& strPath, std::string& strFileName);
  static std::vector<std::string> SplitPath(const std::string& strPath);

  static void GetCommonPath(std::string& strPath, const std::string& strPath2);
  static std::string GetParentPath(const std::string& strPath);
  static bool GetParentPath(const std::string& strPath, std::string& strParent);

  /*! \brief Check whether a path starts with a given start.
   Comparison is case-sensitive.
   Use IsProtocol() to compare the protocol portion only.
   \param path a std::string path.
   \param start the string the start of the path should be compared against.
   \return true if the path starts with the given string, false otherwise.
   \sa IsProtocol, PathEquals
   */
  static bool PathStarts(const std::string& path, const char *start);

  /*! \brief Check whether a path equals another path.
   Comparison is case-sensitive.
   \param path1 a std::string path.
   \param path2 the second path the path should be compared against.
   \param ignoreTrailingSlash ignore any trailing slashes in both paths
   \return true if the paths are equal, false otherwise.
   \sa IsProtocol, PathStarts
   */
  static bool PathEquals(const std::string& path1, const std::string &path2, bool ignoreTrailingSlash = false);

  static bool IsDOSPath(const std::string &path);
  static void AddSlashAtEnd(std::string& strFolder);
  static bool HasSlashAtEnd(const std::string& strFile);
  static void RemoveSlashAtEnd(std::string& strFolder);
  static bool CompareWithoutSlashAtEnd(const std::string& strPath1, const std::string& strPath2);
  static std::string FixSlashesAndDups(const std::string& path, const char slashCharacter = '/', const size_t startFrom = 0);
  /**
   * Convert path to form without duplicated slashes and without relative directories
   * Strip duplicated slashes
   * Resolve and remove relative directories ("/../" and "/./")
   * Will ignore slashes with other direction than specified
   * Will not resolve path starting from relative directory
   * @warning Don't use with "protocol://path"-style URLs
   * @param path string to process
   * @param slashCharacter character to use as directory delimiter
   * @return transformed path
   */
  static std::string CanonicalizePath(const std::string& path, const char slashCharacter = '\\');


  static std::string AddFileToFolder(const std::string &strFolder, const std::string &strFile);


  /*!
   \brief Cleans up the given path by resolving "." and ".."
   and returns it.

   This methods goes through the given path and removes any "."
   (as it states "this directory") and resolves any ".." by
   removing the previous directory from the path. This is done
   for file paths and host names (in case of VFS paths).

   \param path Path to be cleaned up
   \return Actual path without any "." or ".."
   */
  static std::string GetRealPath(const std::string &path);

  /*!
   \brief Updates the URL encoded hostname of the given path

   This method must only be used to update paths encoded with
   the old (Eden) URL encoding implementation to the new (Frodo)
   URL encoding implementation (which does not URL encode -_.!().

   \param strFilename Path to update
   \return True if the path has been updated/changed otherwise false
   */
  static bool UpdateUrlEncoding(std::string &strFilename);

private:
  static std::string resolvePath(const std::string &path);
};

