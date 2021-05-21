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
 * alloc_failure.hpp
 *
 *  Created on: 14 Apr 2016
 *      Author: tyrrells
 */

#ifndef SRC_SERVICES_BLAST_INCLUDE_ALLOC_FAILURE_HPP_
#define SRC_SERVICES_BLAST_INCLUDE_ALLOC_FAILURE_HPP_

#include <new>

#include "grassroots_util_library.h"

using namespace std;

/**
 * A class used for memory allocation failures.
 *
 * This is a thin wrapper on top of bad_alloc, allowing
 * the error message to be set when constructing the
 * Exception.
 *
 * @ingroup utility_group
 */
class AllocFailure : public bad_alloc
{
public:
	/**
	 * Construct an Exception for a memory allocation failure.
	 *
	 * @param error_s The string describing the cause of this AllocFailure.
	 */
	AllocFailure (const char *error_s);

	~AllocFailure () throw ();

	/**
	 * Get the string describing the cause of this AllocFailure.

	 * @return The string describing the cause of this AllocFailure.
	 */
	 virtual const char *what () const throw ();

private:
	 const char *af_error_s;
};


#endif /* SRC_SERVICES_BLAST_INCLUDE_ALLOC_FAILURE_HPP_ */
