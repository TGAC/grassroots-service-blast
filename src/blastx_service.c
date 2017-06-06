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
 * blastx_service.c
 *
 *  Created on: 28 Oct 2016
 *      Author: billy
 */


#include <string.h>

#include "blastx_service.h"

#include "blast_app_parameters.h"
#include "blastp_service.h"


#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"


/*******************************/
/***** STATIC DECLARATIONS *****/
/*******************************/


static NamedParameterType S_GENETIC_CODE = { "query_gencode", PT_UNSIGNED_INT };


static const size_t S_NUM_TASKS = 2;
static const BlastTask s_tasks_p [S_NUM_TASKS] =
{
  { "blastx", "blastx: Traditional BLASTX to compare a nucelotide query to a protein database" },
  { "blastx-fast", "blastx-fast: BLASTX optimized for faster runtime" }
};


static const char *GetBlastXServiceName (Service *service_p);

static const char *GetBlastXServiceDescription (Service *service_p);

static ParameterSet *GetBlastXServiceParameters (Service *service_p, Resource *resource_p, UserDetails *user_p);

static ServiceJobSet *RunBlastXService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p);


static bool AddBlastXQuerySequenceParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p);

static bool ParseBlastXParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);


/*******************************/
/******* API DEFINITIONS *******/
/*******************************/


Service *GetBlastXService ()
{
	Service *blastx_service_p = (Service *) AllocMemory (sizeof (Service));

	if (blastx_service_p)
		{
			BlastServiceData *data_p = AllocateBlastServiceData (blastx_service_p, DT_PROTEIN);

			if (data_p)
				{
					InitialiseService (blastx_service_p,
														 GetBlastXServiceName,
														 GetBlastXServiceDescription,
														 NULL,
														 RunBlastXService,
														 IsResourceForBlastService,
														 GetBlastXServiceParameters,
														 ReleaseBlastServiceParameters,
														 CloseBlastService,
														 CustomiseBlastServiceJob,
														 true,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p);

					if (GetBlastServiceConfig (data_p))
						{
							return blastx_service_p;
						}
				}

			FreeService (blastx_service_p);
		}

	return NULL;
}



static const char *GetBlastXServiceName (Service * UNUSED_PARAM (service_p))
{
 	return "BlastX service";
}


static const char *GetBlastXServiceDescription (Service * UNUSED_PARAM (service_p))
{
	return "A service to search protein databases with nucleotide queries";
}


static ParameterSet *GetBlastXServiceParameters (Service *service_p, Resource * UNUSED_PARAM (resource_p), UserDetails * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = CreateProteinBlastServiceParameters (service_p, "Protein BlastX service parameters", "A service to search protein databases with nucleotide queries", AddBlastXQuerySequenceParameters, s_tasks_p, S_NUM_TASKS);

	return param_set_p;
}


static bool AddBlastXQuerySequenceParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = NULL;
	bool success_flag = false;
	SharedType def;
	const uint32 NUM_GENETIC_CODES = 20;

	const char *descriptions_ss [NUM_GENETIC_CODES] =
		{
			"Standard (1)",
			"Vertebrate Mitochondrial (2)",
			"Yeast Mitochondrial (3)",
			"Mold Mitochondrial; Protozoan Mitochondrial; Coelenterate Mitochondrial; Mycoplasma; Spiroplasma (4)",
			"Invertebrate Mitochondrial (5)",
			"iliate Nuclear; Dasycladacean Nuclear; Hexamita Nuclear (6)",
			"Echinoderm Mitochondrial; Flatworm Mitochondrial (9)",
			"Euplotid Nuclear (10)",
			"Bacterial, Archaeal and Plant Plastid (11)",
			"Alternative Yeast Nuclear (12)",
			"Ascidian Mitochondrial (13)",
			"Alternative Flatworm Mitochondrial (14)",
			"Blepharisma Macronuclear (15)",
			"Chlorophycean Mitochondrial (16)",
			"Trematode Mitochondrial (21)",
			"Scenedesmus obliquus Mitochondrial (22)",
			"Thraustochytrium Mitochondrial (23)",
			"Pterobranchia Mitochondrial (24)",
			"Candidate Division SR1 and Gracilibacteria (25)",
			"Pachysolen tannophilus Nuclear (26)"
		};

	const uint32 values_p [NUM_GENETIC_CODES] = { 1, 2, 3, 4, 5, 6, 9, 10, 11, 12, 13, 14, 15, 16, 21, 22, 23, 24, 25, 26 };


	InitSharedType (&def);

	/* default to Standard */
	def.st_ulong_value = 1;

	param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_GENETIC_CODE.npt_type, S_GENETIC_CODE.npt_name_s, "Genetic code", "Genetic code to use to translate query", def, PL_ALL);

	if (param_p)
		{
			uint32 i;

			success_flag = true;

			for (i = 0; i < NUM_GENETIC_CODES; ++ i)
				{
					def.st_ulong_value = * (values_p + i);

					if (!CreateAndAddParameterOptionToParameter (param_p, def, * (descriptions_ss + i)))
						{
							i = NUM_GENETIC_CODES;
							success_flag = false;
						}
				}

			if (!success_flag)
				{
					DetachParameter (param_set_p, param_p);
					FreeParameter (param_p);
				}
		}
	else
		{

		}

	return success_flag;

}


static ServiceJobSet *RunBlastXService (Service *service_p, ParameterSet *param_set_p, UserDetails *user_p, ProvidersStateTable *providers_p)
{
	BlastAppParameters app_params;
	ServiceJobSet *jobs_p = NULL;

	app_params.bap_parse_params_fn = ParseBlastXParameters;

	jobs_p = RunBlastService (service_p, param_set_p, user_p, providers_p, &app_params);

	return jobs_p;
}


static bool ParseBlastXParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p)
{
	/* Parse the BlastP stuff first */
	bool success_flag = ParseBlastPParameters (data_p, params_p, ap_p);

	if (success_flag)
		{
			/* Now do the BlastX-specific stuff */
			success_flag = GetAndAddBlastArgs (params_p, S_GENETIC_CODE.npt_name_s, false, ap_p);
		}

	return success_flag;
}



