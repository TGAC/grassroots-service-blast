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
			GrassrootsServer *grassroots_p = GetGrassrootsServerFromService (data_p -> bsd_base_data.sd_service_p);
			char *group_s = GetLocalDatabaseGroupName (grassroots_p);

			group_p = CreateAndAddParameterGroupToParameterSet (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, false, & (data_p -> bsd_base_data), param_set_p);

			if (db_p)
				{
					const ServiceData *service_data_p = & (data_p -> bsd_base_data);

					while (db_p -> di_name_s)
						{
							if (db_p -> di_type == db_type)
								{
									def.st_boolean_value = db_p -> di_active_flag;

									char *db_s = GetFullyQualifiedDatabaseName (group_s ? group_s : BS_DATABASE_GROUP_NAME_S, db_p -> di_name_s);

									if (db_s)
										{

											if (!EasyCreateAndAddParameterToParameterSet (service_data_p, param_set_p, group_p, PT_BOOLEAN, db_s, db_p -> di_name_s, db_p -> di_description_s, def, PL_ALL))
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
	Parameter *param_p = NULL;
	SharedType def;

	InitSharedType (&def);

	param_p = EasyCreateAndAddParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_JOB_ID.npt_type, BS_JOB_ID.npt_name_s, "Job IDs", "The UUIDs for Blast jobs that have previously been run", def, PL_ADVANCED);

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


Parameter *SetUpOutputFormatParameter (const char **formats_ss, const uint32 num_formats, const char *default_format_s, const BlastServiceData *service_data_p, ParameterSet *param_set_p, ParameterGroup *group_p)
{
	Parameter *param_p = NULL;
	SharedType def;

	LinkedList *options_p = CreateParameterOptionsList ();

	if (options_p)
		{
			uint32 i;
			bool success_flag = true;

			for (i = 0; i < num_formats; ++ i)
				{
					def.st_string_value_s = (char *) * (formats_ss + i);

					if (!CreateAndAddParameterOption (options_p, def, * (formats_ss + i), PT_STRING))
						{
							i = BOF_NUM_TYPES;
							success_flag = false;
						}
				}

			if (success_flag)
				{
					/* default to grassroots */
					def.st_string_value_s = CopyToNewString (default_format_s, 0, false);

					param_p = CreateAndAddParameterToParameterSet (& (service_data_p -> bsd_base_data), param_set_p, group_p, BS_OUTPUT_FORMAT.npt_type, false, BS_OUTPUT_FORMAT.npt_name_s, "Output format", "The output format for the results", options_p, def, NULL, NULL, PL_ADVANCED, NULL);

					if (def.st_string_value_s)
						{
							FreeCopiedString (def.st_string_value_s);
						}


					if (param_p)
						{
							return param_p;
						}
				}

			FreeLinkedList (options_p);
		}		/* if (options_p) */

	return param_p;
}


bool AddQuerySequenceParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, void *callback_data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	SharedType def;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Query Sequence Parameters", false, & (data_p -> bsd_base_data), param_set_p);

	def.st_string_value_s = NULL;

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_INPUT_FILE.npt_type, BS_INPUT_FILE.npt_name_s, "Input", "The input file to read", def, PL_ADVANCED)) != NULL)
		{
			if ((param_p = SetUpPreviousJobUUIDParameter (data_p, param_set_p, group_p)) != NULL)
				{
					if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_INPUT_QUERY.npt_type, BS_INPUT_QUERY.npt_name_s, "Query Sequence(s)", "Query sequence(s) to be used for a BLAST search should be pasted in the 'Search' text area. "
																															"It accepts a number of different types of input and automatically determines the format or the input."
																															" To allow this feature there are certain conventions required with regard to the input of identifiers (e.g., accessions or gi's)", def, PL_ALL))  != NULL)
						{
							const char *subrange_s = "Coordinates for a subrange of the query sequence. The BLAST search will apply only to the residues in the range. Valid sequence coordinates are from 1 to the sequence length. Set either From or To to 0 to ignore the range. The range includes the residue at the To coordinate.";
							uint32 def_value = 0;

							def.st_ulong_value = def_value;

							if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_FROM.npt_type, BS_SUBRANGE_FROM.npt_name_s, "From", subrange_s, def, PL_ADVANCED)) != NULL)
								{
									def.st_ulong_value = def_value;

									if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_SUBRANGE_TO.npt_type, BS_SUBRANGE_TO.npt_name_s, "To", subrange_s, def, PL_ADVANCED)) != NULL)
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
		}

	return success_flag;
}


bool GetQuerySequenceParameterTypeForNamedParameter (const char *param_name_s, ParameterType *pt_p)
{
	bool success_flag = true;

	if (strcmp (param_name_s, BS_INPUT_FILE.npt_name_s) == 0)
		{
			*pt_p = BS_INPUT_FILE.npt_type;
		}
	else if (strcmp (param_name_s, BS_JOB_ID.npt_name_s) == 0)
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


bool AddGeneralAlgorithmParams (BlastServiceData *data_p, ParameterSet *param_set_p, AddAdditionalParamsFn callback_fn, void *callback_data_p)
{
	bool success_flag = false;
	Parameter *param_p = NULL;
	SharedType def;
	ParameterLevel level = PL_ADVANCED;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("General Algorithm Parameters", false, & (data_p -> bsd_base_data), param_set_p);
	uint32 def_ulong_value = 5;

	def.st_ulong_value = def_ulong_value;

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_MAX_TARGET_SEQS.npt_type, S_MAX_TARGET_SEQS.npt_name_s, "Max target sequences", "Select the maximum number of aligned sequences to display (the actual number of alignments may be greater than this).", def, level)) != NULL)
		{
			double64 def_threshold = 10.0;
			def.st_data_value = def_threshold;

			if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, S_EXPECT_THRESHOLD.npt_type, S_EXPECT_THRESHOLD.npt_name_s, "Expect threshold", "Expected number of chance matches in a random model", def, level)) != NULL)
				{
					def_ulong_value = 0;
					def.st_ulong_value = def_ulong_value;

					if ((param_p = SetUpOutputFormatParameter (BSP_OUTPUT_FORMATS_SS, BOF_NUM_TYPES, * (BSP_OUTPUT_FORMATS_SS + BOF_GRASSROOTS), data_p, param_set_p, group_p)) != NULL)
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
	else
		{
			success_flag = false;
		}

	return success_flag;
}



bool AddProgramSelectionParameters (BlastServiceData *blast_data_p, ParameterSet *param_set_p, const BlastTask *tasks_p, const size_t num_tasks)
{
	bool success_flag = false;
	ParameterGroup *group_p = CreateAndAddParameterGroupToParameterSet ("Program Selection Parameters", false, & (blast_data_p -> bsd_base_data), param_set_p);
	Parameter *param_p = NULL;
	SharedType def;
	const ParameterLevel level = PL_ADVANCED;

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


bool AddProteinGeneralAlgorithmParameters (BlastServiceData *data_p, ParameterSet *param_set_p, ParameterGroup *group_p, void *callback_data_p)
{
	bool success_flag = false;
	SharedType def;
	Parameter *param_p;

	def.st_ulong_value = * ((uint32 *) callback_data_p);

	if ((param_p = EasyCreateAndAddParameterToParameterSet (& (data_p -> bsd_base_data), param_set_p, group_p, BS_WORD_SIZE.npt_type, BS_WORD_SIZE.npt_name_s, "Word size", "Expected number of chance matches in a random model", def, PL_ADVANCED)) != NULL)
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




