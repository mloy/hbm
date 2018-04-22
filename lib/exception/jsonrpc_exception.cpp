// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided

//#include <json/value.h>
//#include <json/writer.h>

#include "hbm/exception/jsonrpc_exception.h"
#include "hbm/jsonrpc/jsonrpc_defines.h"

namespace hbm {
	namespace exception {
//		jsonrpcException::jsonrpcException(const Json::Value& error)
//			: exception("")
//			, m_error_obj(error)
//			, m_code(-1)
//			, m_message()
//		{
//			const Json::Value& codeNode = error[jsonrpc::ERR][jsonrpc::CODE];
//			if (codeNode.isInt()) {
//				m_code = codeNode.asInt();
//			}

//			const Json::Value& messageNode = error[jsonrpc::ERR][jsonrpc::MESSAGE];
//			if (messageNode.isString()) {
//				m_message = messageNode.asString();
//			}
//		}

		jsonrpcException::jsonrpcException( int excCode, const std::string& excMessage)
			: exception("")
//			, m_error_obj()
			, m_code(excCode)
			, m_message(excMessage)
		{
//			if(excMessage.empty()==false) {
//				m_error_obj[jsonrpc::ERR][jsonrpc::MESSAGE] = excMessage;
//			}

//			m_error_obj[jsonrpc::ERR][jsonrpc::CODE] = excCode;
		}

		jsonrpcException::~jsonrpcException() throw()
		{
		}

//		const Json::Value& jsonrpcException::json() const
//		{
//			return m_error_obj;
//		}

		int jsonrpcException::code() const throw()
		{
			return m_code;
		}

		std::string jsonrpcException::message() const throw()
		{
			return m_message;
		}

		const char* jsonrpcException::what() const throw()
		{
			return m_message.c_str();
		}
	}
}
