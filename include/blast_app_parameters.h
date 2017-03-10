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
 * blast_app_parameters.hpp
 *
 *  Created on: 19 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_APP_PARAMETERS_H_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_APP_PARAMETERS_H_


#include "blast_service_api.h"
#include "parameter_set.h"
#include "args_processor.hpp"


/* forward declaration */
struct BlastServiceData;

/**
 * This datatype is used to parse a given ParameterSet in a method
 * suitable for a particular BLAST configuration.
 *
 * @ingroup blast_service
 */
typedef struct BLAST_SERVICE_LOCAL BlastAppParameters
{
	/**
	 * Parse a given ParameterSet in a method suitable for a particular BLAST configuration defined by a given
	 * ArgsProcessor
	 *
	 * @param data_p The Blast service configuration data.
	 * @param params_p The ParameterSet to parse.
	 * @param ap_p The ArgsProcessor that will process the given ParameterSet and convert the values into a format
	 * suitable for the underlying BlastTool.
	 * @return <code>true</code> if the ParameterSet was parsed successfully, <code>false</code> otherwise.
	 */
	bool (*bap_parse_params_fn) (const struct BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);
} BlastAppParameters;


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Parse a given ParameterSet in a method suitable for a particular BLAST configuration defined by a given
 * ArgsProcessor
 *
 * @param app_p The given BlastAppParameters to use.
 * @param data_p The Blast service configuration data.
 * @param params_p The ParameterSet to parse.
 * @param ap_p The ArgsProcessor that will process the given ParameterSet and convert the values into a format
 * suitable for the underlying BlastTool.
 * @return <code>true</code> if the ParameterSet was parsed successfully, <code>false</code> otherwise.
 * @memberof BlastAppParameters
 */
BLAST_SERVICE_LOCAL bool ParseBlastAppParameters (BlastAppParameters *app_p, const struct BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_BLAST_APP_PARAMETERS_H_ */
