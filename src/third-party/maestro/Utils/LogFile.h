/**
 * @file LogFile.h
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * A basic loggig class for debugging purposes.
 */

#pragma once

#ifndef __LOG_FILE_H__
#define __LOG_FILE_H__

#include <string>
#include <fstream>
#include <iostream>

namespace Utils {

	class LogFile
	{
	public:
		LogFile(const std::string& filename)
		{
			logFile.open(filename, std::ios::out | std::ios::app);
			if (!logFile.is_open())
			{
				std::cerr << "Failed to open log file: " << filename << std::endl;
			}
		}

		void Log(const std::string& message)
		{
			if (logFile.is_open())
			{
				logFile << message << std::endl;
			}
			else
			{
				std::cerr << "Log file is not open. Message: " << message << std::endl;
			}
		}

	private:
		std::ofstream logFile;
	};

}

#endif // __LOG_FILE_H__

