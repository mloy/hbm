// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#include "hbm/exception/jsonrpc_exception.h"
#include "hbm/jsonrpc/jsonrpc_defines.h"

namespace hbm {
	namespace exception {
		jsonrpcException::jsonrpcException( int excCode, const std::string& excMessage)
			: exception("")
			, m_code(excCode)
			, m_message(excMessage)
		{
		}

		jsonrpcException::~jsonrpcException() noexcept
		{
		}

		int jsonrpcException::code() const noexcept
		{
			return m_code;
		}

		std::string jsonrpcException::message() const noexcept
		{
			return m_message;
		}

		const char* jsonrpcException::what() const noexcept
		{
			return m_message.c_str();
		}
	}
}
