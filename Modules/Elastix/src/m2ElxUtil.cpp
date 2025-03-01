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

std::string m2::ElxUtil::Executable(const std::string &name, std::string)
{
  std::string correctedName = itksys::SystemTools::GetFilenameWithoutExtension(name);
  #ifdef __WIN32__
  correctedName += ".exe";
  #endif
  
  const std::string version = "5";
  const std::string regex = correctedName + "[a-z:\\s]+5\\.[0-9]+";

  if(m2::ElxUtil::CheckVersion(correctedName, regex)){
    return correctedName;
  }else{
    mitkThrow() << "Elastix executables could not be found!\n"
                  "Please specify the system variable ELASTIX_PATH";
  }
  
  
  
  
  // std::string elxPathExe = "";

  // std::string elxPath = additionalSearchPath;

  // if (elxPath.empty())
  //   elxPath = Elastix_DIR;



  // { // check PATH
  //   if ()
  //   {
  //     itksys::SystemTools::GetEnv("PATH", elxPath);
  //     MITK_INFO << "Use system Elastix found in [" << elxPath << "]";
  //     elxPathExe = correctedName;
  //     return elxPathExe;
  //   }
  //   else
  //   {
  //     MITK_ERROR << "Elastix executables could not be found!\n"
  //                   "Please specify the system variable ELASTIX_PATH";
  //   }
  // }

  // { // check additionalSearchPath

  //   if (!itksys::SystemTools::FileIsDirectory(elxPath))
  //     elxPath = itksys::SystemTools::GetParentDirectory(elxPath);

  //   elxPathExe = m2::ElxUtil::JoinPath({elxPath, "/", correctedName});
  //   if (m2::ElxUtil::CheckVersion(elxPathExe, regex))
  //   {
  //     return elxPathExe;
  //   }
  //   else
  //   {
  //     MITK_ERROR << "Elastix executables could not be found!\n"
  //                   "Please specify the system variable ELASTIX_PATH";
  //   }
  // }

  // { // check system variable ELASTIX_PATH
  //   itksys::SystemTools::GetEnv("ELASTIX_PATH", elxPath);
  //   elxPathExe = m2::ElxUtil::JoinPath({elxPath, "/", correctedName});
  //   if (m2::ElxUtil::CheckVersion(elxPathExe, regex))
  //   {
  //     MITK_INFO << "Use Elastix found at [" << elxPath << "]";
  //     return elxPathExe;
  //   }
  // }

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


