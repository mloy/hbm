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


#ifndef __HBM__JET__JSONEXCEPTION
#define __HBM__JET__JSONEXCEPTION

#include <string>

#include <json/value.h>
#include "hbm/exception/exception.hpp"

namespace hbm {
	namespace exception {
		class jsonException : public hbm::exception::exception	{
		public:
			jsonException(int code, const std::string& message="");
			jsonException(const Json::Value& error);

			virtual ~jsonException() throw();

			const char* what() const throw();

			const Json::Value& json() const;

		private:
			Json::Value m_error_obj;
			std::string m_localWhat;
		};
	}
}
#endif
