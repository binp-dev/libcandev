#pragma once

#include <cstring>

#include <string>
#include <exception>

class CANException : public std::exception
{
private:
	std::string _msg;
	int _err;
	
	std::string _what;
	
public:
	CANException(const std::string &message, int error = 0) noexcept
	  : _msg(message), _err(error)
	{
		if(_err)
			_what = _msg;
		else
			_what = _msg + std::string(" : ") + std::string(strerror(_err));
	}
	
	virtual ~CANException() = default;
	
	const std::string &getMessage() const noexcept
	{
		return _msg;
	}
	
	int getError() const noexcept
	{
		return _err;
	}
	
	virtual const char *what() const noexcept override
	{
		return _what.data();
	}
};
