// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef __HBM__JET__JSONRPCEXCEPTION
#define __HBM__JET__JSONRPCEXCEPTION

#include <string>

//#include <json/value.h>
#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace exception {
		class jsonrpcException : public hbm::exception::exception
		{
		public:
			jsonrpcException(int code, const std::string& message="");
			//jsonrpcException(const Json::Value& error);

			virtual ~jsonrpcException() throw();

			int code() const throw();

			std::string message() const throw();

			/// returns the message from the error object
			const char* what() const throw();

			//const Json::Value& json() const;

		private:
			//Json::Value m_error_obj;
			int m_code;
			std::string m_message;
		};
	}
}
#endif
