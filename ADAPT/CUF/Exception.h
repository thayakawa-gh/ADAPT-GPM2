﻿//
// Copyright (c) 2017-2019 Hayakawa
// Released under the 2-Clause BSD license.
// see https://opensource.org/licenses/BSD-2-Clause
//

#ifndef CUF_EXCEPTION_H
#define CUF_EXCEPTION_H

#include <iostream>
#include <string>
#include <ADAPT/CUF/Function.h>

namespace adapt
{

inline namespace cuf
{

#ifdef _MSC_VER
class Exception : public std::exception
{
public:
	Exception(int e) noexcept : std::exception(ToHexString(e).c_str()) {}
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
	Exception(int e)  noexcept : mMessage(ToHexString(e)) {}
	Exception(const char* c) noexcept : mMessage(c) {}
	Exception(const std::string& message) noexcept : mMessage(message) {}
	Exception(std::string&& message) noexcept : mMessage(std::move(message)) {}
	virtual ~Exception() = default;

	virtual const char* what() const { return mMessage.c_str(); }
	std::string GetErrorMessage() const { return mMessage; }

protected:
	std::string mMessage;
};
#endif


class OutOfRange : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const { return std::string("OUT OF RANGE : ") + what(); }
};

class InvalidArg : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const override { return std::string("INVALID ARG : ") + what(); }
};
class InvalidType : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const override { return std::string("INVALID TYPE : ") + what(); }
};
class InvalidValue : public Exception
{
public:
	using Exception::Exception;
	virtual std::string GetErrorMessage() const override { return std::string("INVALID VALUE : ") + what(); }
};

}

}

#endif