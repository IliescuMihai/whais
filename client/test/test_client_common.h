/*
 * test_client_common.h
 *
 *  Created on: Jan 9, 2013
 *      Author: ipopa
 */

#ifndef TEST_CLIENT_COMMON_H_
#define TEST_CLIENT_COMMON_H_

#include "whais.h"

#include "whais_connector.h"


#define SET_BIT(x, i) { x |= (1 << (i)); }
#define GET_BIT(x, i) (( (x) >> (i) ) & 1)


bool
tc_settup_connection(int              argc,
                      char const **    argv,
                      WH_CONNECTION*   pHnd);


std::string
decode_typeinfo(unsigned int type);


#endif /* TEST_CLIENT_COMMON_H_ */
