/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * Copyright (C) 2007 Hottinger Baldwin Messtechnik GmbH
 * Im Tiefen See 45
 * 64293 Darmstadt
 * Germany
 * http://www.hbm.com
 * All rights reserved
 *
 * The copyright to the computer program(s) herein is the property of
 * Hottinger Baldwin Messtechnik GmbH (HBM), Germany. The program(s)
 * may be used and/or copied only with the written permission of HBM
 * or in accordance with the terms and conditions stipulated in the
 * agreement/contract under which the program(s) have been supplied.
 * This copyright notice must not be removed.
 *
 * This Software is licenced by the
 * "General supply and license conditions for software"
 * which is part of the standard terms and conditions of sale from HBM.
*/


#include <json/writer.h>

#include "jsonrpc_exception.h"
#include "hbm/jsonrpc/jsonrpc_defines.h"

namespace hbm {
	namespace exception {
		jsonrpcException::jsonrpcException(const Json::Value& error)
			: exception("")
			, m_error_obj(error)
			, m_localWhat()
		{
			Json::FastWriter writer;
			writer.omitEndingLineFeed();
			m_localWhat = writer.write(m_error_obj);
			m_localWhat += exception::what();
		}

		jsonrpcException::jsonrpcException( int code, const std::string& message)
			: exception("")
			, m_error_obj()
			, m_localWhat()
		{
			if(message.empty()==false) {
				m_error_obj[jsonrpc::ERR][jsonrpc::MESSAGE] = message;
			}

			m_error_obj[jsonrpc::ERR][jsonrpc::CODE] = code;
			Json::FastWriter writer;
			writer.omitEndingLineFeed();
			m_localWhat = writer.write(m_error_obj);
			m_localWhat += exception::what();
		}

		jsonrpcException::~jsonrpcException() throw()
		{
		}

		const Json::Value& jsonrpcException::json() const
		{
			return m_error_obj;
		}

		const char* jsonrpcException::what() const throw()
		{
			return m_localWhat.c_str();
		}
	}
}
