#ifndef CUF_EXCEPTION_H
#define CUF_EXCEPTION_H

#include <iostream>
#include <string>

namespace adapt
{

inline namespace cuf
{

#ifdef _MSC_VER
class Exception : public std::exception
{
public:

	Exception(const char* c) noexcept : std::exception(c) {}
	Exception(const std::string& message)  noexcept : std::exception(message.c_str()) {}
	Exception(std::string&& message)  noexcept : std::exception(std::move(message).c_str()) {}
	virtual ~Exception() = default;

	virtual std::string GetErrorMessage() const { return what(); }
};
#else
class Exception : public std::exception
{
public:

	Exception(const char* c) noexcept : mMessage(c) {}
	Exception(const std::string& message) noexcept : mMessage(message) {}
	Exception(std::string&& message) noexcept : mMessage(std::move(message)) {}
	virtual ~Exception() = default;

	virtual const char* what() const noexcept { return mMessage.c_str(); }
	std::string GetErrorMessage() const noexcept { return mMessage; }

protected:
	std::string mMessage;
};
#endif


class OutOfRange : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("OUT OF RANGE : ") + what(); }
};
class NotInitialized : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("NOT INITIALIZED : ") + what(); }
};
class NotDefined : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("NOT DEFINED : ") + what(); }
};
class InvalidArg : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("INVALID ARG : ") + what(); }
};
class InvalidType : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("INVALID TYPE : ") + what(); }
};
class InvalidValue : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const noexcept { return std::string("INVALID VALUE : ") + what(); }
};

}

}

#endif
