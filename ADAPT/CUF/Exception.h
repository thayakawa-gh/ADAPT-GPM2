//
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

class Exception
{
public:
	Exception(const std::string& message) : mMessage(message) {}
	Exception(std::string&& message) : mMessage(std::move(message)) {}
	virtual ~Exception() = default;

	virtual void What() const { std::cerr << "EXCEPTION : " << mMessage << std::endl; }
	const std::string& GetErrorMessage() const { return mMessage; }

protected:
	std::string mMessage;
};

class OutOfRange : public Exception
{
public:
	OutOfRange(int e) : Exception(ToHexString(e)) {}
	OutOfRange(const std::string& m) : Exception(m) {}
	OutOfRange(std::string&& m) : Exception(std::move(m)) {}
	void What() const { std::cerr << "OUT OF RANGE : " << mMessage << std::endl; }
};

class InvalidArg : public Exception
{
public:
	InvalidArg(int e) : Exception(ToHexString(e)) {}
	InvalidArg(const std::string& m) : Exception(m) {}
	InvalidArg(std::string&& m) : Exception(std::move(m)) {}
	void What() const { std::cerr << "INVALID ARG : " << mMessage << std::endl; }
};
class InvalidType : public Exception
{
public:
	InvalidType(int e) : Exception(ToHexString(e)) {}
	InvalidType(const std::string& m) : Exception(m) {}
	InvalidType(std::string&& m) : Exception(std::move(m)) {}
	void What() const { std::cerr << "INVALID TYPE : " << mMessage << std::endl; }
};
class InvalidValue : public Exception
{
public:
	InvalidValue(int e) : Exception(ToHexString(e)) {}
	InvalidValue(const std::string& m) : Exception(m) {}
	InvalidValue(std::string&& m) : Exception(std::move(m)) {}
	void What() const { std::cerr << "INVALID VALUE : " << mMessage << std::endl; }
};

}

#endif