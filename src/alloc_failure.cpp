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
/*
 * alloc_failure.cpp
 *
 *  Created on: 14 Apr 2016
 *      Author: tyrrells
 */


#include "alloc_failure.hpp"

AllocFailure ::	AllocFailure (const char *error_s)
	: af_error_s (error_s)
{}


AllocFailure :: ~AllocFailure () throw ()
{}


const char *AllocFailure :: what () const throw ()
{
	return af_error_s;
}
