/*
** Copyright 2014-2018 The Earlham Institute
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
 * polymarker_linked_service.c
 *
 *  Created on: 29 Jan 2019
 *      Author: billy
 */

#include <string.h>

#include "polymarker_linked_service.h"
#include "blast_service_job_markup_keys.h"
#include "string_utils.h"
#include "linked_service.h"



static int32 GetSNPIndex (const json_t *polymorphism_p);

static char *GetSequence (const char *original_sequence_s, const char query_base, const char hit_base, const int32 index);

static bool GetSNPBases (const json_t *polymorphism_p, char *query_base_p, char *hit_base_p);

static const char *GetSingleScaffold (const json_t *hit_p);

static ParameterSet *CreatePolymarkerParameters (const char *sequence_s, const char *scaffold_s, const char *gene_s);


bool PolymarkerServiceGenerator (LinkedService *linked_service_p, json_t *data_p, struct ServiceJob *job_p)
{
	bool success_flag = false;
	char *key_s = ConcatenateVarargsStrings (BSJMK_RESULTS_S, ".", BSJMK_REPORTS_S, NULL);

	PrintJSONToLog (STM_LEVEL_FINEST, __FILE__, __LINE__, data_p, "RunCustomLinkedServiceGenerator");

	if (key_s)
		{
			json_t *reports_p = GetCompoundJSONObject (data_p, key_s);

			FreeCopiedString (key_s);

			if (reports_p)
				{
					if (json_is_array (reports_p))
						{
							size_t i;
							json_t *report_p;

							json_array_foreach (reports_p, i, report_p)
								{
									json_t *hits_p = json_object_get (report_p, BSJMK_REPORT_RESULTS_S);

									if (hits_p)
										{
											if (json_is_array (hits_p))
												{
													size_t j;
													json_t *hit_p;

													json_array_foreach (hits_p, j, hit_p)
														{
															const char *scaffold_s = GetSingleScaffold (hit_p);

															if (scaffold_s)
																{
																	json_t *hsps_p = json_object_get (hit_p, BSJMK_HSPS_S);

																	if (hsps_p)
																		{
																			if (json_is_array (hsps_p))
																				{
																					size_t k;
																					json_t *hsp_p;

																					json_array_foreach (hsps_p, k, hsp_p)
																						{
																							const char *query_sequence_s = GetJSONString (hsp_p, BSJMK_QUERY_SEQUENCE_S);

																							json_t *polymorphisms_p = json_object_get (hsp_p, BSJMK_POLYMORPHISMS_S);

																							if (polymorphisms_p)
																								{
																									if (json_is_array (polymorphisms_p))
																										{
																											size_t l;
																											json_t *polymorphism_p;

																											json_array_foreach (polymorphisms_p, l, polymorphism_p)
																												{
																													/*
																													 * Polymarker requires a single SNP, so first let's check that it's not a MNP
																													 */
																													const char *type_s = GetJSONString (polymorphism_p, "@type");

																													if (type_s)
																														{
																															if (strcmp (type_s, BSJMK_SNP_S) == 0)
																																{
																																	int32 index = GetSNPIndex (polymorphism_p);

																																	if (index != -1)
																																		{
																																			char query_base = '\0';
																																			char hit_base = '\0';

																																			if (GetSNPBases (polymorphism_p, &query_base, &hit_base))
																																				{
																																					char *sequence_s = GetSequence (query_sequence_s, query_base, hit_base, index);

																																					if (sequence_s)
																																						{
																																							ParameterSet *params_p = CreatePolymarkerParameters (sequence_s, scaffold_s, NULL);

																																							if (params_p)
																																								{
																																									if (AddLinkedServiceToRequestJSON (polymorphism_p, linked_service_p, params_p))
																																										{
																																											success_flag = true;
																																										}

																																									FreeParameterSet (params_p);
																																								}		/* if (params_p) */

																																							FreeCopiedString (sequence_s);
																																						}		/* if (sequence_s) */

																																				}		/* if (GetSNPBases (polymorphism_p, query_base, hit_base)) */

																																		}		/* if (index != -1) */

																																}		/* if (strcmp (type_s, BSJMK_SNP_S) == 0) */

																														}		/* if (type_s) */

																												}		/* json_array_foreach (polymorphisms_p, l, polymorphism_p) */

																										}		/* if (json_is_array (polymorphisms_p)) */

																								}		/* if (polymorphisms_p) */

																						}		/* json_array_foreach (hsps_p, j, hsp_p) */

																				}		/* if (json_is_array (hsps_p)) */

																		}		/* if (hsps_p) */

																}		/* if (scaffold_s) */

														}		/* json_array_foreach (hits_p, j, hit_p) */

												}		/* if (json_is_array (hits_p)) */

										}		/* if (hits_p) */

								}		/* json_array_foreach (reports_p, i, report_p) */

						}		/* if (json_is_array (reports_p)) */

				}		/* if (reports_p) */

		}		/* if (key_s) */

	return success_flag;
}


static ParameterSet *CreatePolymarkerParameters (const char *sequence_s, const char *scaffold_s, const char *gene_s)
{
	ParameterSet *polymarker_params_p = AllocateParameterSet ("Polymarker Parameters", "Polymarker parameters generated as a Linked Service from the BLAST services");

	if (polymarker_params_p)
		{
			SharedType def;
			Parameter *param_p = NULL;
			bool success_flag = true;
			InitSharedType (&def);

			if (gene_s)
				{
					def.st_string_value_s = (char *) gene_s;

					if (! (param_p = EasyCreateAndAddParameterToParameterSet (NULL, polymarker_params_p, NULL, PT_STRING, "Gene", "Gene", "Gene", def, PL_ALL)))
						{
							success_flag =  false;
						}
				}		/* if (gene_s) */

			if (success_flag)
				{
					def.st_string_value_s = (char *) sequence_s;

					if ((param_p = EasyCreateAndAddParameterToParameterSet (NULL, polymarker_params_p, NULL, PT_LARGE_STRING, "Sequence", "Sequence", "Sequence", def, PL_ALL)) != NULL)
						{
							def.st_string_value_s = (char *) scaffold_s;

							if ((param_p = EasyCreateAndAddParameterToParameterSet (NULL, polymarker_params_p, NULL, PT_STRING, "Chromosome", "Chromosome", "Chromosome", def, PL_ALL)) != NULL)
								{
									return polymarker_params_p;
								}
						}
				}		/* if (success_flag) */

			FreeParameterSet (polymarker_params_p);
		}		/*if (polymarker_params_p) */

	return NULL;
}


static const char *GetSingleScaffold (const json_t *hit_p)
{
	const char *scaffold_s = NULL;
	const json_t *scaffolds_p = json_object_get (hit_p, BSJMK_SCAFFOLDS_S);

	if (scaffolds_p)
		{
			if (json_is_array (scaffolds_p))
				{
					const size_t l = json_array_size (scaffolds_p);

					if (l == 1)
						{
							const json_t *scaffold_p = json_array_get (scaffolds_p, 0);

							scaffold_s = GetJSONString (scaffold_p, BSJMK_SCAFFOLD_S);
						}
				}
		}

	return scaffold_s;
}


static char *GetSequence (const char *original_sequence_s, const char query_base, const char hit_base, const int32 index)
{
	char *sequence_s = NULL;
	const size_t l = strlen (original_sequence_s);

	if (index < (int32) l)
		{
			/*
			 * 4 for A -> [A/G] and 1 for the null terminating byte
			 */
			sequence_s = (char *) AllocMemory ((l + 5) * sizeof (char));

			if (sequence_s)
				{
					char *pos_p = sequence_s;

					strncpy (pos_p, original_sequence_s, index - 1);

					pos_p += (index - 1);
					*pos_p = '[';
					* (++ pos_p) = query_base;
					* (++ pos_p) = '/';
					* (++ pos_p) = hit_base;
					* (++ pos_p) = ']';

					strcpy (++ pos_p, original_sequence_s + index);
				}
		}

	return sequence_s;
}


static bool GetSNPBases (const json_t *polymorphism_p, char *query_base_p, char *hit_base_p)
{
	bool success_flag = false;
	const json_t *diff_p = json_object_get (polymorphism_p, BSJMK_SEQUENCE_DIFFERENCE_S);

	if (diff_p)
		{
			const char *query_s = GetJSONString (diff_p, BSJMK_QUERY_S);

			if (query_s)
				{
					const char *hit_s = GetJSONString (diff_p, BSJMK_HIT_S);

					if (hit_s)
						{
							*query_base_p = *query_s;
							*hit_base_p = *hit_s;

							success_flag = true;
						}
				}
		}

	return success_flag;
}


static int32 GetSNPIndex (const json_t *polymorphism_p)
{
	int32 index = -1;
	char *key_s = ConcatenateVarargsStrings (BSJMK_LOCUS_S, ".", BSJMK_FALDO_BEGIN_S, NULL);

	if (key_s)
		{
			json_t *pos_p = GetCompoundJSONObject (polymorphism_p, key_s);

			if (pos_p)
				{
					if (!GetJSONInteger (pos_p, BSJMK_FALDO_POSITION_S, &index))
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, pos_p, "Failed to get value for \"%s\"", BSJMK_FALDO_POSITION_S);
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, polymorphism_p, "Failed to get value for \"%s\"", key_s);
				}

			FreeCopiedString (key_s);
		}		/* if (key_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "ConcatenateVarargsStrings failed for SNPIndex key");
		}

	return index;
}

