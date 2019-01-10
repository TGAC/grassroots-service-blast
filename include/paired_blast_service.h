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
 * paired_blast_service.h
 *
 *  Created on: 9 Feb 2016
 *      Author: billy
 */

#ifndef SERVICES_BLAST_INCLUDE_PAIRED_BLAST_SERVICE_H_
#define SERVICES_BLAST_INCLUDE_PAIRED_BLAST_SERVICE_H_

#include "blast_service.h"
#include "blast_service_api.h"
#include "service.h"
#include "paired_service.h"
#include "providers_state_table.h"
#include "remote_service_job.h"


#ifdef __cplusplus
extern "C"
{
#endif


/**
 * Add the parameters for all PairedServices of a given Service to given ParameterSet.
 *
 * @param service_p The Blast Service whose PairedService Parameters will be added.
 * @param internal_params_p The ParameterSet to add the PairedService Parameters to.
 * @return <code>true</code> if the PairedServices were added successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool AddPairedServiceParameters (Service *service_p, ParameterSet *internal_params_p);


BLAST_SERVICE_LOCAL bool GetPairedServiceParameterTypeForNamedParameter (Service *service_p, const char *param_name_s, ParameterType *pt_p);


/**
 * Try to get the results of a previously run BlastServiceJob on a remote server.
 *
 * @param local_job_id_s The uuid as a c-style string on the local Server.
 * @param output_format_code The output format to convert the result to.
 * @param blast_data_p The configuration data for the local BlastService.
 * @return The previous result or <code>NULL</code> upon error.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL char *GetPreviousRemoteBlastServiceJob (const char *local_job_id_s, const uint32 output_format_code, const BlastServiceData *blast_data_p);


/**
 * Save the results of a RemoteServiceJob to a local file for ease of access
 *
 * @param job_p The RemoteServiceJob whose results will be saved.
 * @param blast_data_p The Service configuration data.
 * @return <code>true</code> if the results were saved successfully, <code>
 * false</code> otherwise.
 * @ingroup blast_service
 */
BLAST_SERVICE_LOCAL bool SaveRemoteBlastJobDetails (RemoteServiceJob *job_p, const ServiceData *blast_data_p);


#ifdef __cplusplus
}
#endif





#endif /* SERVICES_BLAST_INCLUDE_PAIRED_BLAST_SERVICE_H_ */
