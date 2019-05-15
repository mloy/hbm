// Copyright 2014 Hottinger Baldwin Messtechnik
// Distributed under MIT license
// See file LICENSE provided


#ifndef __HBM__JSONRPCEXCEPTION
#define __HBM__JSONRPCEXCEPTION

#include <string>

#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace exception {
		class jsonrpcException : public hbm::exception::exception
		{
		public:
			jsonrpcException(int code, const std::string& message="");

			virtual ~jsonrpcException() noexcept;

			int code() const noexcept;

			std::string message() const noexcept;

			/// returns the message from the error object
			const char* what() const noexcept;

			//const Json::Value& json() const;

		protected:
			//Json::Value m_error_obj;
			int m_code;
			std::string m_message;
		};
	}
}
#endif
