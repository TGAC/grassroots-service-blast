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
 * blast_service_params.c
 *
 *  Created on: 13 Feb 2016
 *      Author: billy
 */

#include "blast_service_params.h"

#include <limits.h>
#include <string.h>

#include "provider.h"
#include "grassroots_server.h"
#include "string_utils.h"
#include "streams.h"
#include "service.h"


#include "boolean_parameter.h"
#include "unsigned_int_parameter.h"
#include "double_parameter.h"
#include "string_parameter.h"


static NamedParameterType S_MAX_TARGET_SEQS = { "max_target_seqs", PT_UNSIGNED_INT };
//static NamedParameterType S_SHORT_QUERIES = { "max_target_seqs", PT_UNSIGNED_INT };
static NamedParameterType S_EXPECT_THRESHOLD =  { "evalue", PT_UNSIGNED_REAL };
//static NamedParameterType S_MAX_MATCHES = { "max_target_seqs", PT_UNSIGNED_INT };

static const char * const S_DB_SEP_S = " -> ";

const char *BSP_OUTPUT_FORMATS_SS [BOF_NUM_TYPES] =
{
	"pairwise",
	"query-anchored showing identities",
	"query-anchored no identities",
	"flat query-anchored, show identities",
	"flat query-anchored, no identities",
	"XML Blast output",
	"tabular",
	"tabular with comment lines",
	"Text ASN.1",
	"Binary ASN.1",
	"Comma-separated values",
	"BLAST archive format (ASN.1)",
	"JSON Seqalign output",
	"Multiple file JSON Blast output",
	"Multiple file XML2 Blast output",
	"Single file JSON Blast output",
	"Single file XML2 Blast output",
	"Sequence Alignment/Map (SAM)",
	"Organism report",
	"Grassroots markup"
};



static Parameter *SetUpOutputFormatParameter (const char **formats_ss, const uint32 num_formats, const uint32 default_format, const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p);

static json_t *GetParametersFromResource (Resource *resource_p);

static int GetDatabaseActiveFlagFromJSON (const json_t *params_json_p, const char *db_param_name_s, bool *value_p);


/**************************************************/
/**************** PUBLIC FUNCTIONS ****************/
/**************************************************/

uint32 GetNumberOfDatabases (const BlastServiceData *data_p, const DatabaseType dt)
{
	uint32 i = 0;
	const DatabaseInfo *db_p = data_p -> bsd_databases_p;

	if (db_p)
		{
			while (db_p -> di_name_s)
				{
					if (db_p -> di_type == dt)
						{
							++ i;
						}

					++ db_p;
				}
		}

	return i;
}


char *CreateGroupName (const char *server_s)
{
	char *group_name_s = ConcatenateVarargsStrings (BS_DATABASE_GROUP_NAME_S, " provided by ", server_s, NULL);

	if (group_name_s)
		{
			#if PAIRED_BLAST_SERVICE_DEBUG >= STM_LEVEL_FINER
			PrintLog (STM_LEVEL_FINER, __FILE__, __LINE__, "Created group name \"%s\" for \"%s\" and \"%s\"", group_name_s, server_s);
			#endif
		}
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create group name for \"%s\"", group_name_s);
		}

	return group_name_s;
}





/*
 * The list of databases that can be searched
 */
uint16 AddDatabaseParams (BlastServiceData *data_p, ParameterSet *param_set_p, Resource *resource_p,  const DatabaseType db_type)
{
	uint16 num_added_databases = 0;
	size_t num_group_params = GetNumberOfDatabases (data_p, db_type);

	if (num_group_params)
		{
			ParameterGroup *group_p = NULL;
			const DatabaseInfo *db_p = data_p -> bsd_databases_p;
			GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (data_p -> bsd_base_data.sd_service_p);
			char *group_s = GetLocalDatabaseGroupName (grassroots_p);

			group_p = CreateAndAddParameterGroupToParameterSet (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, false, & (data_p -> bsd_base_data), param_set_p);

			if (db_p)
				{
					const ServiceData *service_data_p = & (data_p -> bsd_base_data);
					const json_t *params_json_array_p = GetParametersFromResource (resource_p);

					while (db_p -> di_name_s)
						{
							if (db_p -> di_type == db_type)
								{
									char *db_s = GetFullyQualifiedDatabaseName (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, db_p -> di_name_s);

									if (db_s)
										{
											bool active_flag;

											if (params_json_array_p)
												{
													GetDatabaseActiveFlagFromJSON (params_json_array_p, db_s, &active_flag);
												}
											else
												{
													active_flag = db_p -> di_active_flag;
												}

											if (!EasyCreateAndAddBooleanParameterToParameterSet (service_data_p, param_set_p, group_p, db_s, db_p -> di_name_s, db_p -> di_description_s, &active_flag, PL_ALL))
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add database \"%s\"", db_p -> di_name_s);
												}

											FreeCopiedString (db_s);
										}

								}		/* if (db_p -> di_type == db_type) */

							++ db_p;

						}		/* while (db_p -> di_name_s) */

				}		/* if (db_p) */

			if (group_s)
				{
					FreeCopiedString (group_s);
				}

		}		/* if (num_group_params) */

	return num_added_databases;
}


bool GetDatabaseParameterTypeForNamedParameter (BlastServiceData *data_p, const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = false;
	GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (data_p -> bsd_base_data.sd_service_p);
	char *group_s = GetLocalDatabaseGroupName (grassroots_p);
	const DatabaseInfo *db_p = data_p -> bsd_databases_p;

	if (db_p)
		{
			while ((!success_flag) && (db_p -> di_name_s))
				{
					char *db_s = GetFullyQualifiedDatabaseName (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, db_p -> di_name_s);

					if (db_s)
						{
							if (strcmp (param_name_s, db_s) == 0)
								{
									*pt_p = PT_BOOLEAN;
									success_flag = true;
								}

							FreeCopiedString (db_s);
						}
					else
						{
							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "GetFullyQualifiedDatabaseName failed for \"%s\" and \"%s\"", group_s ? group_s : BS_DATABASE_GROUP_NAME_S, db_p -> di_name_s);
						}

					++ db_p;
				}		/* while ((!success_flag) && (db_p -> di_name_s)) */

		}		/* if (db_p) */


	if (group_s)
		{
			FreeCopiedString (group_s);
		}

	return success_flag;
}




Parameter *SetUpPreviousJobUUIDParameter (const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_JOB_ID.npt_type, BS_JOB_ID.npt_name_s, "Job IDs", "The UUIDs for Blast jobs that have previously been run", NULL, PL_ADVANCED);

	return param_p;
}


int8 GetOutputFormatCodeForString (const char *output_format_s)
{
	int8 code = -1;
	const char **formats_ss = BSP_OUTPUT_FORMATS_SS;
	int8 i;

	for (i = 0; i < BOF_NUM_TYPES; ++ i, ++ formats_ss)
		{
			if (strcmp (*formats_ss, output_format_s) == 0)
				{
					code = i;
					i = BOF_NUM_TYPES; 	/* force exit from loop */
				}
		}

	return code;
}



Parameter *SetUpOutputFormatParameters (const char **formats_ss, const uint32 num_formats, const uint32 default_format, const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *format_param_p = SetUpOutputFormatParameter (formats_ss, num_formats, default_format, service_data_p, param_set_p, group_p);

	if (format_param_p)
		{
			const char * const description_s = "For the \"tabular\", \"tabular with comment lines\" and \"Comma-separated values\" output formats, you can customise the output by specifying space delimited format parameters here.";
			Parameter *param_p = EasyCreateAndAddStringParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_CUSTOM_OUTPUT_FORMAT.npt_type, BS_CUSTOM_OUTPUT_FORMAT.npt_name_s, "Custom output options", description_s, NULL, PL_ADVANCED);

			if (param_p)
				{
					return format_param_p;
				}
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add \"%s\" parameter", BS_CUSTOM_OUTPUT_FORMAT.npt_name_s);
				}
		}
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "SetUpOutputFormatParameter () failed");
		}

	return NULL;
}



static Parameter *SetUpOutputFormatParameter (const char **formats_ss, const uint32 num_formats, const uint32 default_format, const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_OUTPUT_FORMAT.npt_name_s, "Output format", "The output format for the results", &default_format, PL_ADVANCED);

	if (param_p)
		{
			uint32 i;
			bool success_flag = true;

			for (i = 1; i < num_formats + 1; ++ i)
				{
					if (!CreateAndAddUnsignedIntParameterOption ((UnsignedIntParameter *) param_p, i, * (formats_ss + i)))
						{
							i = num_formats + 1;
							success_flag = false;
						}
				}

			if (success_flag)
				{
					return param_p;
				}

			DetachParameter (param_set_p, param_p);
			FreeParameter (param_p);
		}		/* if (options_p) */

	return NULL;
}


bool AddQuerySequenceParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, void *callback_data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Query Sequence Parameters", false, & (data_p -> bsd_base_data), param_set_p);


	if ((param_p = SetUpPreviousJobUUIDParameter (data_p, param_set_p, group_p)) != NULL)
		{
			if ((param_p = EasyCreateAndAddStringParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_INPUT_QUERY.npt_type, BS_INPUT_QUERY.npt_name_s, "Query Sequence(s)", "Query sequence(s) to be used for a BLAST search should be pasted in the 'Search' text area. "
																													"It accepts a number of different types of input and automatically determines the format or the input."
																													" To allow this feature there are certain conventions required with regard to the input of identifiers (e.g., accessions or gi's)", NULL, PL_ALL))  != NULL)
				{
					const char *subrange_s = "Coordinates for a subrange of the query sequence. The BLAST search will apply only to the residues in the range. Valid sequence coordinates are from 1 to the sequence length. Set either From or To to 0 to ignore the range. The range includes the residue at the To coordinate.";

					if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_FROM.npt_name_s, "From", subrange_s, NULL, PL_ADVANCED)) != NULL)
						{
							if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_TO.npt_name_s, "To", subrange_s, NULL, PL_ADVANCED)) != NULL)
								{
									if (callback_fn)
										{
											success_flag = callback_fn (data_p, param_set_p, group_p, callback_data_p);
										}
									else
										{
											success_flag = true;
										}
								}
						}
				}
		}

	return success_flag;
}


bool GetQuerySequenceParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, BS_JOB_ID.npt_name_s) == 0)
		{
			*pt_p = BS_JOB_ID.npt_type;
		}
	else if (strcmp (param_name_s, BS_INPUT_QUERY.npt_name_s) == 0)
		{
			*pt_p = BS_INPUT_QUERY.npt_type;
		}
	else if (strcmp (param_name_s, BS_SUBRANGE_FROM.npt_name_s) == 0)
		{
			*pt_p = BS_SUBRANGE_FROM.npt_type;
		}
	else if (strcmp (param_name_s, BS_SUBRANGE_TO.npt_name_s) == 0)
		{
			*pt_p = BS_SUBRANGE_TO.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}


bool AddGeneralAlgorithmParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, const void *callback_data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	ParameterLevel level = PL_ADVANCED;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("General Algorithm Parameters", false, & (data_p -> bsd_base_data), param_set_p);
	uint32 def_ulong_value = 5;

	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_MAX_TARGET_SEQS.npt_name_s, "Max target sequences", "Select the maximum number of aligned sequences to display (the actual number of alignments may be greater than this).", &def_ulong_value, level)) != NULL)
		{
			double64 def_threshold = 10.0;

			if ((param_p = EasyCreateAndAddDoubleParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_EXPECT_THRESHOLD.npt_type, S_EXPECT_THRESHOLD.npt_name_s, "Expect threshold", "Expected number of chance matches in a random model", &def_threshold, level)) != NULL)
				{
					if ((param_p = SetUpOutputFormatParameters (BSP_OUTPUT_FORMATS_SS, BOF_NUM_TYPES, BOF_GRASSROOTS, data_p, param_set_p, group_p)) != NULL)
						{
							if (callback_fn)
								{
									success_flag = callback_fn (data_p, param_set_p, group_p, callback_data_p);
								}
							else
								{
									success_flag = true;
								}
						}
				}
		}

	return success_flag;
}


bool GetGeneralAlgorithmParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, S_MAX_TARGET_SEQS.npt_name_s) == 0)
		{
			*pt_p = S_MAX_TARGET_SEQS.npt_type;
		}
	else if (strcmp (param_name_s, S_EXPECT_THRESHOLD.npt_name_s) == 0)
		{
			*pt_p = S_EXPECT_THRESHOLD.npt_type;
		}
	else if (strcmp (param_name_s, BS_OUTPUT_FORMAT.npt_name_s) == 0)
		{
			*pt_p = BS_OUTPUT_FORMAT.npt_type;
		}
	else if (strcmp (param_name_s, BS_CUSTOM_OUTPUT_FORMAT.npt_name_s) == 0)
		{
			*pt_p = BS_CUSTOM_OUTPUT_FORMAT.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}



bool AddProgramSelectionParameters (BlastServiceData *blast_data_p, ParameterSet *param_set_p, const BlastTask *tasks_p, const BlastTask *default_task_p, const size_t task_mem_size, const size_t num_tasks)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Program Selection Parameters", false, & (blast_data_p -> bsd_base_data), param_set_p);
	Parameter *param_p = NULL;
	const ParameterLevel level = PL_ADVANCED;

	if ((param_p = EasyCreateAndAddStringParameterToParameterSet (& (blast_data_p -> bsd_base_data), param_set_p, group_p, BS_TASK.npt_type, BS_TASK.npt_name_s, "Program Selection", "The program to use to run the search.", default_task_p -> bt_name_s, level)) != NULL)
		{
		  size_t i;
		  /*
		   * Since we know the byte size of each task we need to increment
		   * using this value so we need to cast back and forth so the pointer
		   * moves by that number of bytes rather than that number of BlastTask-sized
		   * blocks
		   */
		  const char *ptr = (const char *) tasks_p;
			success_flag = true;

			for (i = 0; i < num_tasks; ++ i, ptr += task_mem_size)
				{
					const BlastTask *task_p = (const BlastTask *) ptr;

					if (!CreateAndAddStringParameterOption ((StringParameter *) param_p, task_p -> bt_name_s, task_p -> bt_description_s))
						{
							success_flag = false;
						}
				}

			if (success_flag)
				{
					/*
					 * Since each of the tasks have different defaults, we
					 * need to update the default values when the task is changed
					 */
					param_p -> pa_refresh_service_flag = true;
				}
			else
				{
					DetachParameter (param_set_p, param_p);
					FreeParameter (param_p);
				}
		}


	return success_flag;
}


bool AddProteinGeneralAlgorithmParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, const void *callback_data_p)
{
	bool success_flag = false;
	Parameter *param_p;
	uint32 value = * ((uint32 *) callback_data_p);

	if ((param_p = EasyCreateAndAddUnsignedIntParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_WORD_SIZE.npt_name_s, "Word size", "Expected number of chance matches in a random model", &value, PL_ADVANCED)) != NULL)
		{
			success_flag = true;
		}

	return success_flag;
}


bool GetProgramSelectionParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, BS_TASK.npt_name_s) == 0)
		{
			*pt_p = BS_TASK.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}

bool GetProteinGeneralAlgorithmParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, BS_WORD_SIZE.npt_name_s) == 0)
		{
			*pt_p = BS_WORD_SIZE.npt_type;
		}
	else
		{
			success_flag = false;
		}

	return success_flag;
}



char *GetLocalDatabaseGroupName (GrassrootsServer *grassroots_p)
{
	char *group_s = NULL;
	const char *provider_s = GetServerProviderName (grassroots_p);

	if (provider_s)
		{
			group_s = CreateGroupName (provider_s);
		}


	return group_s;
}


char *GetFullyQualifiedDatabaseName (const char *group_s, const char *db_s)
{
	char *fq_db_s = ConcatenateVarargsStrings (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, S_DB_SEP_S, db_s, NULL);

	return fq_db_s;
}


const char *GetLocalDatabaseName (const char *fully_qualified_db_s)
{
	const char *db_s = NULL;
	const char *sep_p = strstr (fully_qualified_db_s, S_DB_SEP_S);

	if (sep_p)
		{
			const size_t l = strlen (S_DB_SEP_S);

			if (strlen (sep_p) >= l)
				{
					db_s += l;
				}
		}

	return db_s;
}



static json_t *GetParametersFromResource (Resource *resource_p)
{
	json_t *params_json_p = NULL;

	/*
	 * Have we been set some parameter values to refresh from?
	 */
	if (resource_p && (resource_p -> re_data_p))
		{
			const json_t *param_set_json_p = json_object_get (resource_p -> re_data_p, PARAM_SET_KEY_S);

			if (param_set_json_p)
				{
					params_json_p = json_object_get (param_set_json_p, PARAM_SET_PARAMS_S);
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, resource_p -> re_data_p, "Failed to get param set with key \"%s\"", PARAM_SET_KEY_S);
				}

		}		/* if (resource_p && (resource_p -> re_data_p)) */

	return params_json_p;
}


static int GetDatabaseActiveFlagFromJSON (const json_t *params_json_p, const char *db_param_name_s, bool *value_p)
{
	int res = 0;
	const size_t num_params = json_array_size (params_json_p);
	size_t i;

	for (i = 0; i < num_params; ++ i)
		{
			const json_t *param_json_p = json_array_get (params_json_p, i);
			const char *name_s = GetJSONString (param_json_p, PARAM_NAME_S);

			if (name_s)
				{
					if (strcmp (name_s, db_param_name_s) == 0)
						{
							bool b;

							if (GetJSONBoolean (param_json_p, PARAM_CURRENT_VALUE_S, &b))
								{
									*value_p = b;
									res = 1;

									/* force exit from loop */
									i = num_params;
								}
							else
								{
									res = -1;
									PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, param_json_p, "Failed to get current_value with key \"%s\"", PARAM_CURRENT_VALUE_S);
								}
						}
				}
		}


	return res;
}




