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

#include <limits.h>
#include <string.h>

#include "blast_service_params.h"
#include "provider.h"
#include "grassroots_config.h"
#include "string_utils.h"
#include "streams.h"
#include "service.h"



static const char *s_output_formats_ss [BOF_NUM_TYPES] =
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
uint16 AddDatabaseParams (BlastServiceData *data_p, ParameterSet *param_set_p, const DatabaseType db_type)
{
	uint16 num_added_databases = 0;
	SharedType def;
	size_t num_group_params = GetNumberOfDatabases (data_p, db_type);

	if (num_group_params)
		{
			ParameterGroup *group_p = NULL;
			const DatabaseInfo *db_p = data_p -> bsd_databases_p;
			char *group_s = NULL;
			const json_t *provider_p = NULL;
			const char *group_to_use_s = NULL;

			provider_p = GetGlobalConfigValue (SERVER_PROVIDER_S);

			if (provider_p)
				{
					const char *provider_s = GetProviderName (provider_p);

					if (provider_s)
						{
							group_s = CreateGroupName (provider_s);
						}
				}

			group_to_use_s = group_s ? group_s : BS_DATABASE_GROUP_NAME_S;

			group_p = CreateAndAddParameterGroupToParameterSet (group_to_use_s, NULL, false, & (data_p -> bsd_base_data), param_set_p);

			if (db_p)
				{
					const ServiceData *service_data_p = & (data_p -> bsd_base_data);

					while (db_p -> di_name_s)
						{
							if (db_p -> di_type == db_type)
								{
									def.st_boolean_value = db_p -> di_active_flag;

									if (!EasyCreateAndAddParameterToParameterSet (service_data_p, param_set_p, group_p, PT_BOOLEAN, db_p -> di_name_s, db_p -> di_name_s, db_p -> di_description_s, def, PL_ALL))
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add database \"%s\"", db_p -> di_name_s);
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


Parameter *SetUpPreviousJobUUIDParamater (const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = NULL;
	SharedType def;

	InitSharedType (&def);

	param_p = EasyCreateAndAddParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_JOB_ID.npt_type, BS_JOB_ID.npt_name_s, "Job IDs", "The UUIDs for Blast jobs that have previously been run", def, PL_ALL);

	return param_p;
}


int8 GetOutputFormatCodeForString (const char *output_format_s)
{
	int8 code = -1;
	const char **formats_ss = s_output_formats_ss;
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


Parameter *SetUpOutputFormatParamater (const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = NULL;
	SharedType def;

	LinkedList *options_p = CreateParameterOptionsList ();

	if (options_p)
		{
			uint32 i;
			bool success_flag = true;

			for (i = 0; i < BOF_NUM_TYPES; ++ i)
				{
					def.st_string_value_s = (char *) * (s_output_formats_ss + i);

					if (!CreateAndAddParameterOption (options_p, def, * (s_output_formats_ss + i), PT_STRING))
						{
							i = BOF_NUM_TYPES;
							success_flag = false;
						}
				}

			if (success_flag)
				{
					/* default to grassroots */
					def.st_string_value_s = CopyToNewString (* (s_output_formats_ss + BOF_GRASSROOTS), 0, false);

					param_p = CreateAndAddParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_OUTPUT_FORMAT.npt_type, false, BS_OUTPUT_FORMAT.npt_name_s, "Output format", "The output format for the results", options_p, def, NULL, NULL, PL_ALL, NULL);

					if (param_p)
						{
							return param_p;
						}
					else
						{
							if (def.st_string_value_s)
								{
									FreeCopiedString (def.st_string_value_s);
								}
						}
				}

			FreeLinkedList (options_p);
		}		/* if (options_p) */

	return param_p;
}


bool AddQuerySequenceParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	SharedType def;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Query Sequence Parameters", false, NULL, & (data_p -> bsd_base_data), param_set_p);

	def.st_string_value_s = NULL;

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_INPUT_FILE.npt_type, BS_INPUT_FILE.npt_name_s, "Input", "The input file to read", def, PL_ALL)) != NULL)
		{
			def.st_string_value_s = NULL;

			if ((param_p = SetUpPreviousJobUUIDParamater (data_p, param_set_p, group_p)) != NULL)
				{
					def.st_string_value_s = NULL;


					if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_INPUT_QUERY.npt_type, BS_INPUT_QUERY.npt_name_s, "Query Sequence(s)", "Query sequence(s) to be used for a BLAST search should be pasted in the 'Search' text area. "
																															"It accepts a number of different types of input and automatically determines the format or the input."
																															" To allow this feature there are certain conventions required with regard to the input of identifiers (e.g., accessions or gi's)", def, PL_ALL))  != NULL)
						{
							const char *subrange_s = "Coordinates for a subrange of the query sequence. The BLAST search will apply only to the residues in the range. Valid sequence coordinates are from 1 to the sequence length. Set either From or To to 0 to ignore the range. The range includes the residue at the To coordinate.";

							def.st_ulong_value = 0;

							if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_FROM.npt_type, BS_SUBRANGE_FROM.npt_name_s, "From", subrange_s, def, PL_INTERMEDIATE | PL_ADVANCED)) != NULL)
								{
									def.st_ulong_value = 0;

									if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_TO.npt_type, BS_SUBRANGE_TO.npt_name_s, "To", subrange_s, def, PL_INTERMEDIATE | PL_ADVANCED)) != NULL)
										{
											if (callback_fn)
												{
													success_flag = callback_fn (data_p, param_set_p, group_p);
												}
											else
												{
													success_flag = true;
												}
										}
								}
						}
				}
		}

	return success_flag;
}


bool AddGeneralAlgorithmParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	SharedType def;
	uint8 level = PL_INTERMEDIATE | PL_ALL;
	const char *param_name_s = "max_target_seqs";
	ParameterType pt = PT_UNSIGNED_INT;

	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("General Algorithm Parameters", NULL, false, & (data_p -> bsd_base_data), param_set_p);

	def.st_ulong_value = 5;


	if ((param_p = CreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, pt, false, param_name_s, "Max target sequences", "Select the maximum number of aligned sequences to display (the actual number of alignments may be greater than this).", NULL, def, NULL, NULL, level, NULL)) != NULL)
		{
			def.st_boolean_value = true;

			if ((param_p = CreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, PT_BOOLEAN, false, "short_queries", "Short queries", "Automatically adjust parameters for short input sequences", NULL, def, NULL, NULL, level, NULL)) != NULL)
				{
					def.st_ulong_value = 10;

					if ((param_p = CreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, PT_UNSIGNED_INT, false, "expect_threshold", "Expect threshold", "Expected number of chance matches in a random model", NULL, def, NULL, NULL, level, NULL)) != NULL)
						{
							def.st_ulong_value = 0;

							if ((param_p = CreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, PT_UNSIGNED_INT, false, "max_matches_in_a_query_range", "Max matches in a query range", "Limit the number of matches to a query range. This option is useful if many strong matches to one part of a query may prevent BLAST from presenting weaker matches to another part of the query", NULL, def, NULL, NULL, level, NULL)) != NULL)
								{
									if ((param_p = SetUpOutputFormatParamater (data_p, param_set_p, group_p)) != NULL)
										{
											if (callback_fn)
												{
													success_flag = callback_fn (data_p, param_set_p, group_p);
												}
											else
												{
													success_flag = true;
												}
										}
								}
						}
				}
		}

	return success_flag;
}



bool AddProgramSelectionParameters (BlastServiceData *blast_data_p, ParameterSet *param_set_p, const BlastTask *tasks_p, const size_t num_tasks)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Program Selection Parameters", NULL, false, & (blast_data_p -> bsd_base_data), param_set_p);
	Parameter *param_p = NULL;
	SharedType def;
	const ParameterLevel level = PL_INTERMEDIATE | PL_ADVANCED;

	def.st_string_value_s = (char *) tasks_p -> bt_name_s;

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (blast_data_p -> bsd_base_data), param_set_p, group_p, BS_TASK.npt_type, BS_TASK.npt_name_s, "Program Selection", "The program to use to run the search.", def, level)) != NULL)
		{
		  size_t i;

			success_flag = true;

			for (i = 0; i < num_tasks; ++ i)
				{
					def.st_string_value_s = (char *)  (tasks_p + i) -> bt_name_s;

					if (!CreateAndAddParameterOptionToParameter (param_p, def, (tasks_p + i) -> bt_description_s))
						{
							success_flag = false;
						}
				}

			if (!success_flag)
				{
					DetachParameter (param_set_p, param_p);
					FreeParameter (param_p);
				}
		}


	return success_flag;
}


