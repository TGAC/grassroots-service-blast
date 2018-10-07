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
 * byte_buffer_args_processor.hpp
 *
 *  Created on: 27 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_BYTE_BUFFER_ARGS_PROCESSOR_HPP_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_BYTE_BUFFER_ARGS_PROCESSOR_HPP_

#include "../../blast-service/include/args_processor.hpp"
#include "byte_buffer.h"


/**
 * An ArgsProcessor that adds all of its values to a given ByteBuffer.
 *
 * @ingroup blast_service
 */
class BLAST_SERVICE_LOCAL ByteBufferArgsProcessor : public ArgsProcessor
{
public:
	/**
	 * Construct a new ByteBufferArgsProcessor.
	 */
	ByteBufferArgsProcessor ();

	/**
	 * The ByteBufferArgsProcessor destructor.
	 */
	virtual ~ByteBufferArgsProcessor ();


	/**
	 * Add an argument to the underlying ByteBuffer
	 *
	 * @param arg_s The value to add.
	 * @param hyphen_flag If this is <code>true</code> then the value
	 * specified by arg_s will be prefixed by a '-' when adding to this
	 * ArgsProcessor.
	 * @return <code>true</code> if the argument was added successfully,
	 * <code>false</code> otherwise.
	 */
	virtual bool AddArg (const char *arg_s, const bool hyphen_flag);

	/**
	 * Get the complete set of arguments as a string.
	 *
	 * @return The arguments as a string
	 */
	const char *GetArgsAsString ();

private:
	ByteBuffer *bbap_buffer_p;
};



#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_BYTE_BUFFER_ARGS_PROCESSOR_HPP_ */
