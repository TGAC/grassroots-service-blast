/*
 * protein_blast_service.h
 *
 *  Created on: 13 Oct 2016
 *      Author: billy
 */

#ifndef SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTP_SERVICE_H_
#define SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTP_SERVICE_H_


#include "args_processor.hpp"
#include "blast_service.h"
#include "blast_service_api.h"
#include "blast_service_params.h"
#include "parameter_set.h"
#include "service.h"


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Get a Service capable of running BlastP searches.
 *
 * @return The BlastP Service or <code>NULL</code> upon error.
 * @ingroup blast_service
 */

BLAST_SERVICE_LOCAL Service *GetBlastPService (GrassrootsServer *grassroots_p);


BLAST_SERVICE_LOCAL bool AddProteinBlastParameters (BlastServiceData *data_p, ParameterSet *param_set_p);


BLAST_SERVICE_LOCAL bool ParseBlastPParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);


BLAST_SERVICE_LOCAL ParameterSet *CreateProteinBlastServiceParameters (Service *service_p, const char *param_set_name_s, const char *param_set_description_s, AddAdditionalParamsFn query_sequence_callback_fn, void *callback_data_p, const BlastTask *tasks_p, const uint32 num_tasks);


BLAST_SERVICE_LOCAL bool GetProteinBlastParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p);


#ifdef __cplusplus
}
#endif


#endif /* SERVER_SRC_SERVICES_BLAST_INCLUDE_BLASTP_SERVICE_H_ */
