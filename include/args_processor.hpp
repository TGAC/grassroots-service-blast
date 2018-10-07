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
 * args_processor.hpp
 *
 *  Created on: 27 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_ARGS_PROCESSOR_HPP_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_ARGS_PROCESSOR_HPP_

#include "../../blast-service/include/blast_service_api.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * An ArgsProcessor is a class to abstract out how a
 * particular BlastTool stores a value when an argument
 * is passed to it.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL ArgsProcessor
{
public:
	/**
	 *  Construct an ArgsProcessor.
	 */
	ArgsProcessor ();

	/**
	 * The ArgsProcessor destructor.
	 */
	virtual ~ArgsProcessor ();

	/**
	 * Add an argument to the underlying tool.
	 *
	 * @param arg_s The value to add.
	 * @param hyphen_flag If this is <code>true</code> then the value
	 * specified by arg_s will be prefixed by a '-' when adding to this
	 * ArgsProcessor.
	 * @return <code>true</code> if the argument was added successfully,
	 * <code>false</code> otherwise.
	 */
	virtual bool AddArg (const char *arg_s, const bool hyphen_flag) = 0;
};


#ifdef __cplusplus
}
#endif


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_ARGS_PROCESSOR_HPP_ */
