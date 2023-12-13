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


#include "blastx_service.h"

#include <string.h>

#include "blast_app_parameters.h"
#include "blast_service.h"
#include "blast_service_params.h"
#include "blast_util.h"
#include "blastp_service.h"

#include "unsigned_int_parameter.h"


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


static const char *GetBlastXServiceName (const Service *service_p);

static const char *GetBlastXServiceDescription (const Service *service_p);

static const char *GetBlastXServiceAlias (const Service *service_p);

static ParameterSet *GetBlastXServiceParameters (Service *service_p, DataResource *resource_p, User *user_p);

static ServiceJobSet *RunBlastXService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p);


static bool AddBlastXQuerySequenceParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, const void *callback_data_p);

static bool ParseBlastXParameters (const BlastServiceData *data_p, ParameterSet *params_p, ArgsProcessor *ap_p);

static ServiceMetadata *GetBlastXServiceMetadata (Service *service_p);

static bool GetBlastXServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p);


/*******************************/
/******* API DEFINITIONS *******/
/*******************************/


Service *GetBlastXService (GrassrootsServer *grassroots_p)
{
	Service *blastx_service_p = (Service *) AllocMemory (sizeof (Service));

	if (blastx_service_p)
		{
			BlastServiceData *data_p = AllocateBlastServiceData (blastx_service_p, DT_PROTEIN);

			if (data_p)
				{
					if (InitialiseService (blastx_service_p,
														 GetBlastXServiceName,
														 GetBlastXServiceDescription,
														 GetBlastXServiceAlias,
														 NULL,
														 RunBlastXService,
														 IsResourceForBlastService,
														 GetBlastXServiceParameters,
														 GetBlastXServiceParameterTypeForNamedParameter,
														 ReleaseBlastServiceParameters,
														 CloseBlastService,
														 CustomiseBlastServiceJob,
														 true,
														 SY_SYNCHRONOUS,
														 (ServiceData *) data_p,
														 GetBlastXServiceMetadata,
														 GetBlastIndexingData,
														 grassroots_p))
						{
							if (GetBlastServiceConfig (data_p))
								{
									return blastx_service_p;
								}
						}
				}

			FreeService (blastx_service_p);
		}

	return NULL;
}



static const char *GetBlastXServiceName (const Service * UNUSED_PARAM (service_p))
{
 	return "BlastX";
}


static const char *GetBlastXServiceDescription (const Service * UNUSED_PARAM (service_p))
{
	return "Search protein databases with nucleotide queries";
}


static const char *GetBlastXServiceAlias (const Service * UNUSED_PARAM (service_p))
{
	return (BS_GROUP_ALIAS_PREFIX_S SERVICE_GROUP_ALIAS_SEPARATOR "blastx");
}


static ParameterSet *GetBlastXServiceParameters (Service *service_p, DataResource *resource_p, User * UNUSED_PARAM (user_p))
{
	ParameterSet *param_set_p = CreateProteinBlastServiceParameters (service_p, resource_p, "Protein BlastX service parameters", "A service to search protein databases with nucleotide queries", AddBlastXQuerySequenceParameters, NULL, s_tasks_p, S_NUM_TASKS);

	return param_set_p;
}



static bool GetBlastXServiceParameterTypeForNamedParameter (const Service *service_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_GENETIC_CODE.npt_name_s) == 0)
		{
			*pt_p = S_GENETIC_CODE.npt_type;
		}
	else
		{
			if (!GetBaseBlastServiceParameterTypeForNamedParameter (service_p, param_name_s, pt_p))
				{
					if (!GetGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p))
						{
							if (!GetProteinGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p))
								{
									if (!GetProgramSelectionParameterTypeForNamedParameter (param_name_s, pt_p))
										{
											if (!GetProteinGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p))
												{
													if (!GetProteinBlastParameterTypeForNamedParameter (param_name_s, pt_p))
														{
															success_flag = false;
														}		/* if (!GetProteinBlastParameterTypeForNamedParameter (param_name_s, pt_p)) */

												}		/* if (!GetNucleotideBlastParameterTypeForNamedParameter (param_name_s, pt_p)) */

										}		/* if (!GetProgramSelectionParameterTypeForNamedParameter (param_name_s, pt_p)) */

								}		/* if (!GetProteinGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p)) */

						}		/* if (!GetGeneralAlgorithmParameterTypeForNamedParameter (param_name_s, pt_p)) */

				}		/* if (!GetBaseBlastServiceParameterTypeForNamedParameter (param_name_s, pt_p)) */
		}

	return success_flag;
}


static bool AddBlastXQuerySequenceParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, const void *callback_data_p)
{
	Parameter *param_p = NULL;
	bool success_flag = false;
	const uint32 NUM_GENETIC_CODES = 20;
	uint32 def_value;

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


	/* default to Standard */
	def_value = 1;

	param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_GENETIC_CODE.npt_name_s, "Genetic code", "Genetic code to use to translate query", &def_value, PL_ADVANCED);

	if (param_p)
		{
			uint32 i;

			success_flag = true;

			for (i = 0; i < NUM_GENETIC_CODES; ++ i)
				{
					if (!CreateAndAddUnsignedIntParameterOption ((UnsignedIntParameter *) param_p, i + 1, * (descriptions_ss + i)))
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


static ServiceJobSet *RunBlastXService (Service *service_p, ParameterSet *param_set_p, User *user_p, ProvidersStateTable *providers_p)
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


static ServiceMetadata *GetBlastXServiceMetadata (Service *service_p)
{
	ServiceMetadata *metadata_p = GetGeneralBlastServiceMetadata (service_p);

	if (metadata_p)
		{
			const char *term_url_s = CONTEXT_PREFIX_EDAM_ONTOLOGY_S "data_2977";
			SchemaTerm *input_p = AllocateSchemaTerm (term_url_s, "Nucleic acid sequence", "One or more nucleic acid sequences, possibly with associated annotation.");

			if (input_p)
				{
					if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p))
						{
							return metadata_p;
						}		/* if (AddSchemaTermToServiceMetadataInput (metadata_p, input_p)) */
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add input term %s to service metadata", term_url_s);
							FreeSchemaTerm (input_p);
						}

				}		/* if (input_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate input term %s for service metadata", term_url_s);
				}

			FreeServiceMetadata (metadata_p);
		}		/* if (metadata_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to allocate base service metadata");
		}

	return NULL;
}


