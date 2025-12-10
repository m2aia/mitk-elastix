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
  std::string fullPath;
  
#ifdef _WIN32
  // On Windows, add .exe extension if not present
  if (executableName.size() < 4 || executableName.substr(executableName.size() - 4) != ".exe") {
    executableName += ".exe";
  }
  
  // Try ELASTIX_PATH environment variable
  const char* elastixPath = std::getenv("ELASTIX_PATH");
  if (elastixPath && *elastixPath) {
    fullPath = m2::ElxUtil::JoinPath({elastixPath, "/", executableName});
    MITK_INFO << "Trying Windows ELASTIX_PATH: " << fullPath;
  } else {
    mitkThrow() << "ELASTIX_PATH environment variable not set on Windows!";
  }
#else
  // On Unix systems, try multiple paths in order of priority:
  // 1. additionalSearchPath (if provided)
  // 2. ELASTIX_PATH environment variable
  // 3. Default /opt/elastix/bin
  if (!additionalSearchPath.empty()) {
    fullPath = m2::ElxUtil::JoinPath({additionalSearchPath, executableName});
    MITK_INFO << "Trying additional search path: " << fullPath;
  } else {
    const char* elastixPath = std::getenv("ELASTIX_PATH");
    if (elastixPath && *elastixPath) {
      fullPath = m2::ElxUtil::JoinPath({elastixPath, executableName});
      MITK_INFO << "Trying Unix ELASTIX_PATH: " << fullPath;
    } else {
      fullPath = m2::ElxUtil::JoinPath({"/opt/elastix/bin", executableName});
      MITK_INFO << "Trying default Unix path: " << fullPath;
    }
  }
#endif

  const std::string version_regex = name + "[a-z:\\s]+5\\.[0-9]+";
  
  MITK_INFO << "Searching for Elastix executable: " << fullPath;

  if (m2::ElxUtil::CheckVersion(fullPath, version_regex)) {
    return fullPath;
  } else {
#ifdef _WIN32
    mitkThrow() << "Elastix executables could not be found!\n"
                   "Please specify the system variable ELASTIX_PATH";
#else
    mitkThrow() << "Elastix executables could not be found!\n"
                   "Please ensure Elastix is installed at /opt/elastix/bin, set ELASTIX_PATH environment variable, or specify an additional search path";
#endif
  }

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


