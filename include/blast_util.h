/*
** Copyright 2014-2016 The Earlham Institute
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/**
 * @file
 * @brief
 */

/*
 * blast_util.h
 *
 *  Created on: 19 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_UTIL_H_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_UTIL_H_


#include "blast_service_api.h"
#include "byte_buffer.h"
#include "parameter_set.h"
#include "args_processor.hpp"


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Get the named Parameter from a ParameterSet and add it to an ArgsProcessor.
 *
 * @param param_set_p The ParameterSet to search for the named Parameter in.
 * @param param_name_s The name of the Parameter to search for.
 * @param required_flag <code>true</code> if the parameter refers to a required variable,
 * <code>false</code> if it is optional.
 * @param ap_p The ArgsProcessor to use.
 * @return <code>true</code> if the value was successfully added to the ArgsProcessor or
 * if the value could not be found and the required_flag was false.
 * <code>false</code> otherwise.
 * @ingroup blast_service
 * @memberof ArgsProcessor
 */
BLAST_SERVICE_LOCAL bool GetAndAddBlastArgs (const ParameterSet *param_set_p, const char *param_name_s, bool required_flag, ArgsProcessor *ap_p);


/**
 * Add an argument derived from the current value of a Parameter to an ArgsProcessor.
 *
 * This will use the datatype of the current value (e.g. string, integer, etc.) when
 * deciding how to add it to the ArgsProcessor
 *
 * @param param_p The Parameter to get the current value from.
 * @param ap_p The ArgsProcessor to use.
 * @return <code>true</code> if the value was successfully added to the ArgsProcessor,
 * <code>false</code> otherwise.
 * @ingroup blast_service
 * @memberof ArgsProcessor
 */
BLAST_SERVICE_LOCAL bool AddBlastArgs (const Parameter *param_p, ArgsProcessor *ap_p);


/**
 * Add the argument to the command line arguments stored
 * in a ByteBuffer.
 *
 * @param arg_s The argument to add.
 * @param ap_p The ArgsProcessor to use to store the argument.
 * @param hyphen_flag If this is <code>true</code> then a "-"
 * will be prepended to the argument that is being added to
 * the ByteBuffer.
 * @return <code>true</code> if the argument was added
 * successfully, <code>false</code> otherwise.
 */
BLAST_SERVICE_LOCAL bool AddArg (const char *arg_s, ArgsProcessor *ap_p, bool hyphen_flag);



/**
 * Add a pair of arguments to the command line arguments
 * stored in a ByteBuffer.
 *
 * @param ap_p The ArgsProcessor to use to store the argument.
 * @param key_s The first argument to add.
 * @param value_s The second argument to add.
 * @return <code>true</code> if the arguments were added
 * successfully, <code>false</code> otherwise.
 * @ingroup blast_service
 * @memberof ArgsProcessor
 */
BLAST_SERVICE_LOCAL bool AddArgsPair (const char *key_s, const char *value_s, ArgsProcessor *ap_p);


/**
 * Get the value of an integer-based Parameter and add it as key-value pair
 * to the command line arguments accessed by an ArgsProcessor
 *
 * @param params_p The ParameterSet to get the Parameter from.
 * @param param_name_s The name of the desired Parameter.
 * @param key_s The key to use when creating the command line arguments.
 * @param ap_p The ArgsProcessor to use to store the argument.
 * @param unsigned_flag Is the parameter value unsigned or not?
 * @param required_flag If this is <code>true</code> then failure to find
 * the Parameter will cause this function to return <code>false</code>. If the
 * value is optional, then set this to <code>false</code>.
 * @return <code>true</code> if the arguments were added
 * successfully, <code>false</code> otherwise.
 * @ingroup blast_service
 * @memberof ArgsProcessor
 */
BLAST_SERVICE_LOCAL bool AddArgsPairFromIntegerParameter (const ParameterSet *params_p, const char * const param_name_s, const char *key_s, ArgsProcessor *ap_p, const bool unsigned_flag, const bool required_flag);


/**
 * Get the value of an string-based Parameter and add it as key-value pair
 * to the command line arguments accessed by an ArgsProcessor
 *
 * @param params_p The ParameterSet to get the Parameter from.
 * @param param_name_s The name of the desired Parameter.
 * @param key_s The key to use when creating the command line arguments.
 * @param ap_p The ArgsProcessor to use to store the argument.
 * @param required_flag If this is <code>true</code> then failure to find
 * the Parameter will cause this function to return <code>false</code>. If the
 * value is optional, then set this to <code>false</code>.
 * @return <code>true</code> if the arguments were added
 * successfully, <code>false</code> otherwise.
 * @ingroup blast_service
 * @memberof ArgsProcessor
 */
BLAST_SERVICE_LOCAL bool AddArgsPairFromStringParameter (const ParameterSet *params_p, const char * const param_name_s, const char *key_s, ArgsProcessor *ap_p,  const bool required_flag);


#ifdef __cplusplus
}
#endif


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_UTIL_H_ */
