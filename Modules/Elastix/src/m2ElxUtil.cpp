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
#include <filesystem>
#endif

std::string m2::ElxUtil::Executable(const std::string &name, std::string additionalSearchPath)
{
  std::string executableName = name;

#ifdef _WIN32
  if (executableName.size() < 4 || executableName.substr(executableName.size() - 4) != ".exe")
    executableName += ".exe";
#endif

  const std::string version_regex = name + "[a-z:\\s]+5\\.[0-9]+";

  // Ordered list of directories to try
  std::vector<std::string> searchDirs;

  if (!additionalSearchPath.empty())
    searchDirs.push_back(additionalSearchPath);

  const char *elastixPath = std::getenv("ELASTIX_PATH");
  if (elastixPath && *elastixPath)
    searchDirs.push_back(elastixPath);

  // Compile-time path baked in via m2ElxConfig.h (set by CMake superbuild)
  if (std::string(Elastix_DIR) != "")
    searchDirs.push_back(Elastix_DIR);

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
              << "Searched: additionalSearchPath, ELASTIX_PATH env var, "
              << "compile-time Elastix_DIR ('" << Elastix_DIR << "')"
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


