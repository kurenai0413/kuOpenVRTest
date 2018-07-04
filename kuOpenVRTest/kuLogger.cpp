#include "kuLogger.h"



kuLogger::kuLogger()
{
	m_fFileOpened = false;
}

kuLogger::kuLogger(std::string fileName)
{
	m_fFileOpened = false;
	m_Path = fileName;
}


kuLogger::~kuLogger()
{
}

void kuLogger::WriteLine(const char * message)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_FileStream.open(m_Path, std::ios::out);

	if (m_FileStream.is_open())
	{
		m_fFileOpened = true;

		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);

		m_FileStream << "[" << systemTime.wYear << "-" << systemTime.wMonth << "-" << systemTime.wDay
					 << " " << systemTime.wHour << ":" << systemTime.wMinute << ":" << systemTime.wSecond << "." << systemTime.wMilliseconds;
		m_FileStream << message << std::endl;
		m_FileStream.close();
	}
}

void kuLogger::WriteLine(std::string message, MessageType type)
{
	std::lock_guard<std::mutex> lock(m_Mutex);

	m_FileStream.open(m_Path, std::ios::out);

	if (m_FileStream.is_open())
	{
		m_fFileOpened = true;
	
		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);

		m_FileStream << "[" << systemTime.wYear << "-" << systemTime.wMonth << "-" << systemTime.wDay
					 << " " << systemTime.wHour << ":" << systemTime.wMinute << ":" << systemTime.wSecond << "." << systemTime.wMilliseconds;
		
		switch (type)
		{
		case MessageType::Debug:
			m_FileStream << "] Debug: ";
			break;
		case MessageType::Warning:
			m_FileStream << "] Warning: ";
			break;
		case MessageType::Critical:
			m_FileStream << "] Critical: ";
			break;
		case MessageType::Fatal:
			m_FileStream << "] Fatal: ";
			break;
		default:
			break;
		}

		m_FileStream << message << std::endl;
		m_FileStream.close();
	}
}
