/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* vim: set ts=4 et sw=4 tw=80: */
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

/*
$HeadURL:$
$Revision:$
$Author:$
$Date:$
*/
/// @file


#ifndef _HBM__JSON__JSONNAME_H
#define _HBM__JSON__JSONNAME_H
namespace hbm {
	namespace jsonrpc {
		/// some error codes from the JSON RPC spec
		static const int parseError = -32700;
		static const int invalidRequest = -32600;
		static const int methodNotFound = -32601;
		static const int invalidParams = -32602;
		static const int internalError = -32603;

		/// some string constants from the JSON RPC spec
		static const char JSONRPC[] = "jsonrpc";
		static const char METHOD[] = "method";
		static const char RESULT[] = "result";
		static const char ERR[] = "error";
		static const char CODE[] = "code";
		static const char MESSAGE[] = "message";
		static const char DATA[] = "data";
		static const char PARAMS[] = "params";
		static const char ID[] = "id";
	}
}
#endif
