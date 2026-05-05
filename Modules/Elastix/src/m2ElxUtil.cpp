/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#include <m2ElxConfig.h>
#include <m2ElxUtil.h>
#include <mitkException.h>
#include <Poco/Environment.h>
#ifdef _WIN32
#  include <windows.h>
#elif defined(__APPLE__)
#  include <mach-o/dyld.h>
#  include <climits>
#else
#  include <climits>
#  include <unistd.h>
#endif

std::string m2::ElxUtil::Executable(const std::string &name, std::string additionalSearchPath)
{
  std::string executableName = name;

#ifdef _WIN32
  if (executableName.size() < 4 || executableName.substr(executableName.size() - 4) != ".exe")
    executableName += ".exe";
#endif

  const std::string version_regex = name + "[a-z:\\s]+5\\.[0-9]+";

  // Ordered list of directories to try:
  // 1. additionalSearchPath (caller override)
  // 2. Directory of the running executable (works for installed packages)
  // 3. Compile-time Elastix_DIR from m2ElxConfig.h (superbuild / dev build)
  // 4. ELASTIX_PATH environment variable
  // 5. /opt/elastix/bin (Unix fallback)
  std::vector<std::string> searchDirs;

  if (!additionalSearchPath.empty())
    searchDirs.push_back(additionalSearchPath);

  // Determine the running executable's directory so that a co-installed
  // elastix (placed in the same bin/ by the installer) is found first.
  {
    std::string execDir;
#if defined(_WIN32)
    char buf[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, buf, MAX_PATH))
      execDir = itksys::SystemTools::GetFilenamePath(std::string(buf));
#elif defined(__APPLE__)
    char buf[PATH_MAX] = {};
    uint32_t sz = sizeof(buf);
    if (_NSGetExecutablePath(buf, &sz) == 0)
      execDir = itksys::SystemTools::GetFilenamePath(std::string(buf));
#else
    char buf[PATH_MAX] = {};
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0)
      execDir = itksys::SystemTools::GetFilenamePath(std::string(buf, static_cast<size_t>(len)));
#endif
    if (!execDir.empty())
      searchDirs.push_back(execDir);
  }

  // Compile-time path baked in via m2ElxConfig.h (set by CMake superbuild)
  if (std::string(Elastix_DIR) != "")
    searchDirs.push_back(Elastix_DIR);

  const char *elastixPath = std::getenv("ELASTIX_PATH");
  if (elastixPath && *elastixPath)
    searchDirs.push_back(elastixPath);

#ifndef _WIN32
  searchDirs.push_back("/opt/elastix/bin");
#endif

  for (const auto &dir : searchDirs)
  {
    if (dir.empty())
      continue;
    auto fullPath = m2::ElxUtil::JoinPath({dir, "/", executableName});
    MITK_INFO << "Trying Elastix path: " << fullPath;
    if (m2::ElxUtil::CheckVersion(fullPath, version_regex))
      return fullPath;
  }

  mitkThrow() << "Elastix executable '" << name << "' could not be found.\n"
              << "Searched: additionalSearchPath, running-executable dir, "
              << "compile-time Elastix_DIR ('" << Elastix_DIR << "'), "
              << "ELASTIX_PATH env var"
#ifndef _WIN32
              << ", /opt/elastix/bin"
#endif
              << ".\nPlease install Elastix or set the ELASTIX_PATH environment variable.";

  return "";
}

bool m2::ElxUtil::CheckVersion(std::string executablePath,
                               std::string version_regex,
                               std::string)
{
  MITK_INFO << "Check [" + executablePath + "] ";
  // Poco::Pipe outPipe;
  // Poco::PipeInputStream istr(outPipe);
  // Poco::ProcessHandle ph(Poco::Process::launch(executablePath, args, nullptr, &outPipe, nullptr));
  // ph.wait();
  std::vector<std::string> args;
  args.push_back("--version");
  std::string output;
  try{
    output = m2::ElxUtil::run(executablePath, args);
  }catch(std::exception& e){
    MITK_ERROR << "m2::ElxUtil::run Faild " << e.what();
    return false;
  }  
  
  auto pass = std::regex_search(output, std::regex{version_regex});
  if (pass)
    MITK_INFO << "-> " << output;
  else{
    MITK_INFO << "Check Faild " << output;
    MITK_INFO << "Regex:" << version_regex;
  }
  return pass;
}

/**
 * @brief CreatePath can be used to normalize and join path arguments
 * Example call:
 * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
 * @param args is a vector of strings
 * @return std::string
 */
std::string m2::ElxUtil::JoinPath(std::vector<std::string> &&args)
{
  return itksys::SystemTools::ConvertToOutputPath(itksys::SystemTools::JoinPath(args));
}


