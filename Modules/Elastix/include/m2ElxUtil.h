/*===================================================================

MSI applications for interactive analysis in MITK (M2aia)

Copyright (c) Jonas Cordes

All rights reserved.

This software is distributed WITHOUT ANY WARRANTY; without
even the implied warranty of MERCHANTABILITY or FITNESS FOR
A PARTICULAR PURPOSE.

See LICENSE.txt for details.

===================================================================*/
#pragma once

#include <MitkElastixExports.h>
#include <Poco/Pipe.h>
#include <Poco/PipeStream.h>
#include <Poco/Process.h>
#include <Poco/StreamCopier.h>
#include <boost/process.hpp>
#include <itksys/System.h>
#include <itksys/SystemTools.hxx>
// #include <m2CoreCommon.h>
#include <mitkIOUtil.h>
#include <mitkImage.h>
#include <mitkLabelSetImage.h>
#include <mitkPointSet.h>
#include <regex>
#include <string>

namespace m2
{
  class MITKELASTIX_EXPORT ElxUtil
  {
  public:
    /**
     * @brief Replaces the value of the `what` parameter with `by` in the `paramFileString` string.
     * If the `what` parameter doesn't exist in `paramFileString`, it will be added to the end of it.
     * 
     * @param[in,out] paramFileString The string to be modified.
     * @param[in] what The parameter name to be replaced.
     * @param[in] by The new value to replace the `what` parameter with.
     */
    static void ReplaceParameter(std::string &paramFileString, std::string what, std::string by)
    {
      auto pos1 = std::min(paramFileString.find("(" + what), paramFileString.find("// (" + what));
      auto pos2 = paramFileString.find(')', pos1);
      if (pos1 == std::string::npos || pos2 == std::string::npos)
        paramFileString += "\n(" + what + " " + by + ")\n";
      else
        paramFileString.replace(pos1, pos2 - pos1 + 1, "(" + what + " " + by + ")");
    }

    /**
     * @brief Removes the `what` parameter from the `paramFileString` string.
     * 
     * @param[in,out] paramFileString The string to be modified.
     * @param[in] what The parameter name to be removed.
     */
    static void RemoveParameter(std::string &paramFileString, std::string what)
    {
      auto pos1 = paramFileString.find("(" + what);
      auto pos2 = paramFileString.find(')', pos1);
      if (pos1 == std::string::npos || pos2 == std::string::npos)
        return;
      else
        paramFileString.replace(pos1, pos2 - pos1 + 1, "");
    }

    /**
     * @brief Returns the string representation of the `what` parameter in the `paramFileString` string.
     * 
     * @param[in] paramFileString The string to search the `what` parameter in.
     * @param[in] what The parameter name to search for.
     * @return The string representation of the `what` parameter. Empty string if not found.
     */
    static std::string GetParameterLine(std::string &paramFileString, std::string what)
    {
      auto pos1 = paramFileString.find("(" + what);
      auto pos2 = paramFileString.find(')', pos1);
      if (pos1 != std::string::npos && pos2 != std::string::npos)
        return paramFileString.substr(pos1, pos2 - pos1);
      return "";
    }

    /**
     * @brief Search the system for elastix executables (name)
     * 1) additional search path
     * 2) check ELASTIX_PATH
     * 3) check PATH
     *
     * @param name
     * @param additionalSearchPath
     * @return std::string
     */
    static std::string Executable(const std::string &name, std::string additionalSearchPath = "");


    /**
     * @brief Execute a command and return the output as a string.
     * 
     * @param cmd The command to be executed
     * @return std::string The output of the command
     */
    static std::string run(const std::string& command, const std::vector<std::string>& args) {
      namespace bp = boost::process;
      std::ostringstream output;
      bp::ipstream pipe_stream, pipe_errstream;

      MITK_INFO << "Check [" + command + "] ";
      std::cout  << "Arguments: ";
      for(auto arg : args){
        std::cout << arg << " ";
      }

      // Launch the process:
      // - 'command' is the executable,
      // - 'bp::args(args)' passes the command line arguments,
      // - 'bp::std_out > pipe_stream' redirects stdout to our pipe_stream.
      bp::child c("/opt/elastix/bin/"+command, bp::args(args), 
                  bp::env["LD_LIBRARY_PATH"] = "/opt/elastix/lib",
                  bp::std_out > pipe_stream,  
                  bp::std_err > pipe_errstream);

      // Read output line by line
      std::string line;
      while (std::getline(pipe_stream, line)) {
          output << line << "\n";
      }

      // Read error line by line
      std::string error;
      while (std::getline(pipe_errstream, error)) {
          output << error << "\n";
      }

      // Wait for the process to finish
      c.wait();
      return output.str();
  }

    /**
     * @brief CheckVersion can be used to evaluate the version of any external executable using regular expressions.
     * System::CheckVersion("path/to/exe", std::regex{"exeinfo[a-z:\\s]+5\\.[0-9]+"}, "--version")
     *
     * @param executablePath
     * @param version_regex
     * @param argVersion
     * @return bool
     */
    static bool CheckVersion(std::string executablePath,
                             std::string version_regex,
                             std::string argVersion = "--version");

    /**
     * @brief CreatePath can be used to normalize and join path arguments
     * Example call:
     * CreatPath({"this/is/a/directory/path/with/or/without/a/slash/","/","test"});
     * @param args is a vector of strings
     * @return std::string
     */
    static std::string JoinPath(std::vector<std::string> &&args);

    static std::string GetShape(const mitk::Image *img)
    {
      std::string shape;
      for (unsigned int i = 0; i < img->GetDimension(); ++i)
      {
        shape += std::to_string(img->GetDimensions()[i]);
        if (i < img->GetDimension() - 1)
          shape += ", ";
      }
      return shape;
    }

    static void SavePointSet(const mitk::PointSet::Pointer &pnts,  const std::string &path)
    {
      std::setlocale(LC_NUMERIC, "en_US.UTF-8");
      std::ofstream f(path);
      f << "point\n";
      f << std::to_string(pnts->GetSize()) << "\n";
      mitk::Point3D index;
      for (auto it = pnts->Begin(); it != pnts->End(); ++it)
      {
        const auto &p = it->Value();
        // if(reference_image){
        //   reference_image->GetGeometry()->WorldToIndex(p,index);
        //   f << index[0] << " " << index[1] << " " << index[2]<< "\n";
        // }else{

        f << p[0] << " " << p[1] << " " <<  p[2] << "\n";
        // }
      }

      f.close();
    }

      

    static inline std::string to_string(const std::vector<std::string> &list) noexcept
    {
      return std::accumulate(list.begin(), list.end(), std::string(), [](const std::string &a, const std::string &b) { return a + " " + b; });
    }
    
  };
} // namespace m2