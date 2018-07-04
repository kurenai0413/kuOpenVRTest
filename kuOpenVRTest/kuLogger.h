#pragma once
#include <fstream>
#include <string>
#include <time.h>
#include <mutex>
#include <windows.h>

enum MessageType { Debug, Warning, Critical, Fatal };


class kuLogger
{
public:
	
	kuLogger();
	kuLogger(std::string fileName);
	~kuLogger();

	void WriteLine(const char * message);
	void WriteLine(std::string message, MessageType type);

private:
	bool			m_fFileOpened;

	std::string		m_Path;

	std::ofstream	m_FileStream;
	std::mutex		m_Mutex;
};

