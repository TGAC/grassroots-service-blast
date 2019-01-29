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
 * blast_service_job_markup.c
 *
 *  Created on: 21 Jan 2016
 *      Author: tyrrells
 */
#include "blast_service_job_markup.h"

#include <ctype.h>
#include <string.h>

#include "blast_service_job.h"

#define ALLOCATE_BLAST_SERVICE_JOB_MARKUP_KEYS_CONSTANTS (1)
#include "blast_service_job_markup_keys.h"
#include "blast_service_params.h"
#include "string_utils.h"
#include "regular_expressions.h"




/*
 * STATIC FUNCTION PROTOTYPES
 */

static bool AddSequenceOntologyTerms (json_t *context_p);

static bool AddEdamOntologyTerms (json_t *context_p);

static bool AddFaldoOntologyTerms (json_t *context_p);

static bool AddGenomicFeatureAndVariationOntologyTerms (json_t *context_p);

static bool AddSchemaOrgTerms (json_t *context_p);

static bool AddGap (json_t *gaps_p, const int32 from, const int32 to);

static bool AddGaps (json_t *marked_up_hits_p, const char * const gaps_key_s, const char *sequence_s);

static bool IsGap (const char c);

static json_t *AddAndGetMarkedUpReport (json_t *markup_reports_p, const DatabaseInfo *database_p, const json_t *blast_result_search_p, const json_t *blast_report_p);

static bool AddMarkedUpHit (json_t *marked_up_results_p, const json_t *blast_hit_p, const DatabaseInfo *db_p);

static json_t *GetBlastResult (BlastServiceJob *job_p, BlastServiceData *data_p);

static const DatabaseInfo *GetDatabaseFromBlastResult (const json_t *blast_report_p, const BlastServiceData *data_p);


static bool PopulateMarkedUpHit (json_t *marked_up_hit_p, const json_t *blast_hit_p, const DatabaseInfo *db_p);

static bool AddQueryMasks (const json_t *blast_search_p, json_t *mark_up_p);

static const char *GetDatabaseName (const json_t *marked_up_report_p);

static bool AddSoftwareDetails (const json_t *blast_report_p, json_t *mark_up_p);

static bool AddSoftwareVersion (const json_t *blast_report_p, json_t *software_mark_up_p);


/*
 * FUNCTION DEFINITIONS
 */

bool AddSequence (json_t *root_p, const char *key_s, const char *query_sequence_s)
{
	bool success_flag = false;

	if (json_object_set_new (root_p, key_s, json_string (query_sequence_s)) == 0)
		{
			char *gaps_key_s = ConcatenateStrings (key_s, "_gaps");

			if (gaps_key_s)
				{
					success_flag = AddGaps (root_p, gaps_key_s, query_sequence_s);

					FreeCopiedString (gaps_key_s);
				}

		}

	return success_flag;
}


bool AddHitDetails (json_t *marked_up_result_p, const json_t *blast_hit_p, const DatabaseInfo *db_p)
{
	bool success_flag = false;
	const json_t *hsps_p = json_object_get (blast_hit_p, BSJMK_HSPS_S);

	if (hsps_p)
		{
			if (PopulateMarkedUpHit	(marked_up_result_p, blast_hit_p, db_p))
				{
					json_t *marked_up_hsps_p = json_object_get (marked_up_result_p, BSJMK_HSPS_S);

					if (marked_up_hsps_p)
						{
							if (json_is_array (hsps_p))
								{
									size_t i;
									json_t *hsp_p;

									json_array_foreach (hsps_p, i, hsp_p)
										{
											json_t *marked_up_hsp_p = json_object ();

											if (marked_up_hsp_p)
												{
													bool added_flag = false;

													if (AddHsp (marked_up_hsp_p, hsp_p))
														{
															if (json_array_append_new (marked_up_hsps_p, marked_up_hsp_p) == 0)
																{
																	added_flag = true;
																}
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, marked_up_hsp_p, "Failed to add marked_up_hit to hits array");
																}

														}		/* if (AddHsp (marked_up_hit_p, hsp_p)) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, hsp_p, "Failed to add hsp to hit");
														}

													if (!added_flag)
														{
															json_decref (marked_up_hsp_p);
														}

												}		/* if (marked_up_hsp_p) */
											else
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create marked up hsp");

												}
										}

									success_flag = (json_array_size (hsps_p) == json_array_size (marked_up_hsps_p));
								}		/* if (json_is_array (hsps_p)) */


						}		/* if (json_object_set_new (marked_up_result_p, BSJMK_HSPS_S, marked_up_hsps_p) == 0) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, marked_up_result_p, "Failed to set marked up hsps array for marked up hits");
							json_decref (marked_up_hsps_p);
						}

				}		/* if (marked_up_hsps_p) */
			else
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create hsps array for marked up hits");
				}



		}		/* if (hsps_p) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get source hsps from blast result");
		}

	return success_flag;
}




bool AddHsp (json_t *marked_up_hsp_p, const json_t *hsp_p)
{
	if (json_object_set_new (marked_up_hsp_p, "@type", json_string ("match")) == 0)
		{
			if (GetAndAddDoubleScoreValue (marked_up_hsp_p, hsp_p, "bit_score", "bit_score"))
				{
					if (GetAndAddDoubleScoreValue (marked_up_hsp_p, hsp_p, "evalue", "evalue"))
						{
							if (GetAndAddIntScoreValue (marked_up_hsp_p, hsp_p, "score", "score"))
								{
									if (GetAndAddIntScoreValue (marked_up_hsp_p, hsp_p, "num", "hsp_num"))
										{
											if (GetAndAddSequenceValue (marked_up_hsp_p, hsp_p, "hseq", "hit_sequence"))
												{
													if (GetAndAddHitLocation (marked_up_hsp_p, hsp_p, "hit_from", "hit_to", "hit_strand", "hit_location"))
														{
															const char *hit_sequence_s = GetJSONString (hsp_p, "hseq");

															if (hit_sequence_s && AddSequence (marked_up_hsp_p, "hit_sequence", hit_sequence_s))
																{
																	const char *query_sequence_s = GetJSONString (hsp_p, "qseq");

																	if (query_sequence_s && AddSequence (marked_up_hsp_p, BSJMK_QUERY_SEQUENCE_S, query_sequence_s))
																		{
																			const char *midline_s = GetJSONString (hsp_p, "midline");

																			if (midline_s && AddSequence (marked_up_hsp_p, "midline", midline_s))
																				{
																					if (GetAndAddHitLocation (marked_up_hsp_p, hsp_p, "query_from", "query_to", "query_strand", "query_location"))
																						{
																							const uint32 hit_index = 0;
																							const int inc_value = 1;

																							if (GetAndAddNucleotidePolymorphisms (marked_up_hsp_p, query_sequence_s, hit_sequence_s, midline_s, hit_index, inc_value))
																								{
																									return true;
																								}
																						}
																					else
																						{
																							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add query location details");
																						}

																				}		/* if (midline_s && AddSequence (marked_up_hsp_p, "midline", midline_s)) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add midline \"%s\"", midline_s ? midline_s : "NULL");
																				}

																		}		/* if (query_sequence_s && AddSequence (marked_up_hsp_p, BSJMK_QUERY_SEQUENCE_S, query_sequence_s)) */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add query_sequence \"%s\"", query_sequence_s ? query_sequence_s : "NULL");
																		}

																}		/* if (GetAndAddSequenceValue (marked_up_hsp_p, hsps_p, "hseq", "hit_sequence")) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add hit_sequence \"%s\"", hit_sequence_s ? hit_sequence_s : "NULL");
																}

														}		/* if (GetAndAddHitLocation (marked_up_hsp_p, hsp_p, "hit_from", "hit_to", "hit_strand", "hit_location")) */
													else
														{
															PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add hit location details");
														}

												}		/* if (GetAndAddSequenceValue (marked_up_hsp_p, hsps_p, "hseq", "hit_sequence")) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add hit_sequence");
												}

										}		/* if (GetAndAddIntScoreValue (marked_up_hsp_p, hsps_p, "num", "num") */
									else
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add num");
										}

								}		/* if (GetAndAddIntScoreValue (marked_up_hsp_p, hsps_p, "score", "score") */
							else
								{
									PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add evalue");
								}

						}		/* if (GetAndAddDoubleScoreValue (marked_up_hsp_p, hsps_p, "evalue", "evalue")) */
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add evalue");
						}

				}		/* if (GetAndAddDoubleScoreValue (marked_up_hsp_p, hsps_p, "bit_score", "bit_score")) */
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add bit_score");
				}
		}		/* if (json_object_set_new (marked_up_hsp_p, "@type", json_string ("match")) == 0) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_hsp_p, "failed to add @type: match");
		}

	return false;
}


bool AddHitLocation (json_t *parent_p, const char *child_key_s, const int32 from, const int32 to, const Strand strand)
{
	json_t *location_p = json_object ();

	if (location_p)
		{
			if (AddFaldoTerminus (location_p, BSJMK_FALDO_BEGIN_S, from, strand))
				{
					if (AddFaldoTerminus (location_p, "faldo:end", to, strand))
						{
							if (json_object_set_new (location_p, "@type", json_string ("faldo:Region")) == 0)
								{
									if (json_object_set_new (parent_p, child_key_s, location_p) == 0)
										{
											return true;
										}		/* if (json_object_set_new (marked_up_result_p, child_key_s, location_p) == 0) */

								}		/* if (json_object_set_new (location_p, "@type", json_string ("faldo:Region")) == 0) */

						}		/* if (AddFaldoTerminus (location_p, "faldo:end", to, forward_strand_flag)) */

				}		/* if (AddFaldoTerminus (location_p, BSJMK_FALDO_BEGIN_S, from, forward_strand_flag)) */

			json_decref (location_p);
		}		/* if (location_p) */

	return false;
}


bool GetAndAddHitLocation (json_t *marked_up_result_p, const json_t *hsps_p, const char *hsp_from_key_s, const char *hsp_to_key_s, const char *strand_key_s, const char *child_key_s)
{
	bool success_flag = false;
	int32 from;

	if (GetJSONInteger (hsps_p, hsp_from_key_s, &from))
		{
			int32 to;

			if (GetJSONInteger (hsps_p, hsp_to_key_s, &to))
				{

					/*
						"faldo:location": {
							"@type": "faldo:Region",
							"faldo:begin": {
								"@type": [ "faldo:Position", "faldo:ExactPosition", "faldo:ForwardStrandPosition" ],
								"faldo:position": "99"
							},
							"faldo:end": {
								"@type": [ "faldo:Position", "faldo:ExactPosition", "faldo:ForwardStrandPosition" ],
								"faldo:position": "168"
							}
						}
					 */

					Strand strand = ST_NONE;
					const char *strand_s = GetJSONString (hsps_p, strand_key_s);

					if (strand_s)
						{
							if (strcmp (strand_s, "Plus") == 0)
								{
									strand = ST_FORWARD;
								}
							else if (strcmp (strand_s, "Minus") == 0)
								{
									strand = ST_REVERSE;
								}
						}

					if (AddHitLocation (marked_up_result_p, child_key_s, from, to, strand))
						{
							success_flag = true;
						}

				}		/* if (GetJSONInteger (hsps_p, hsp_to_key_s, &to)) */

		}		/* if (GetJSONInteger (hsps_p, hsp_from_key_s, &from)) */

	return success_flag;
}


bool AddFaldoTerminus (json_t *parent_json_p, const char *child_key_s, const int32 position, const Strand strand)
{
	json_t *faldo_p = json_object ();

	if (faldo_p)
		{
			bool success_flag = false;

			if (strand == ST_NONE)
				{
					success_flag = (json_object_set_new (faldo_p, "@type", json_string ("faldo:ExactPosition")) == 0);
				}
			else
				{
					json_t *type_array_p = json_array ();

					if (type_array_p)
						{
							if (json_array_append_new (type_array_p, json_string ("faldo:ExactPosition")) == 0)
								{
									switch (strand)
										{
											case ST_FORWARD:
												if (json_array_append_new (type_array_p, json_string ("faldo:ForwardStrandPosition")) == 0)
													{
														success_flag = true;
													}
												break;

											case ST_REVERSE:
												if (json_array_append_new (type_array_p, json_string ("faldo:ReverseStrandPosition")) == 0)
													{
														success_flag = true;
													}
												break;

											case ST_NONE:
												// nothing to do
												success_flag = true;
												break;
										}

									if (success_flag)
										{
											if (json_object_set_new (faldo_p, "@type", type_array_p) != 0)
												{
													success_flag = false;
												}
										}
								}

							if (!success_flag)
								{
									json_decref (type_array_p);
								}
						}
				}

			if (success_flag)
				{
					if (json_object_set_new (faldo_p, BSJMK_FALDO_POSITION_S, json_integer (position)) == 0)
						{
							if (json_object_set_new (parent_json_p, child_key_s, faldo_p) == 0)
								{
									return true;
								}

						}		/* if (json_object_set_new (faldo_p, BSJMK_FALDO_POSITION_S, json_integer (position)) */

				}		/* if (json_array_append_new (type_array_p, json_string (strand_s)) == 0) */


			json_decref (faldo_p);
		}		/* if (faldo_p) */

	return false;
}





bool GetAndAddSequenceValue (json_t *marked_up_result_p, const json_t *hsps_p, const char *hsp_key_s, const char *sequence_key_s)
{
	bool success_flag = false;
	const char *sequence_value_s = GetJSONString (hsps_p, hsp_key_s);

	if (sequence_value_s)
		{
			if (AddSequence (marked_up_result_p, sequence_key_s, sequence_value_s))
				{
					success_flag = true;
				}
		}

	return success_flag;
}


bool GetAndAddDoubleScoreValue (json_t *marked_up_result_p, const json_t *hsps_p, const char *hsp_key_s, const char *marked_up_key_s)
{
	bool success_flag = false;
	double64 value;

	if (GetJSONReal (hsps_p, hsp_key_s, &value))
		{
			if (AddDoubleScoreValue (marked_up_result_p, marked_up_key_s, value))
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_result_p, "failed to add int value for %s=" DOUBLE64_FMT, hsp_key_s, value);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, hsps_p, "failed to get real value for %s", hsp_key_s);
		}

	return success_flag;
}


bool GetAndAddIntScoreValue (json_t *marked_up_result_p, const json_t *hsps_p, const char *hsp_key_s, const char *marked_up_key_s)
{
	bool success_flag = false;
	int32 value;

	if (GetJSONInteger (hsps_p, hsp_key_s, &value))
		{
			if (AddIntScoreValue (marked_up_result_p, marked_up_key_s, value))
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, marked_up_result_p, "failed to add int value for %s=" INT32_FMT, hsp_key_s, value);
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, hsps_p, "failed to get int value for %s", hsp_key_s);
		}

	return success_flag;
}


bool AddIntScoreValue (json_t *parent_p, const char *key_s, int score_value)
{
	bool success_flag = false;

	if (json_object_set_new (parent_p, key_s, json_integer (score_value)) == 0)
		{
			success_flag = true;
		}

	return success_flag;
}



bool AddDoubleScoreValue (json_t *parent_p, const char *key_s, double64 score_value)
{
	bool success_flag = false;

	if (json_object_set_new (parent_p, key_s, json_real (score_value)) == 0)
		{
			success_flag = true;
		}

	return success_flag;
}



bool GetAndAddDatabaseDetails (json_t *marked_up_result_p, const DatabaseInfo *db_p)
{
	json_t *database_p = json_object ();

	if (database_p)
		{
			if (json_object_set_new (database_p, BSJMK_DATATBASE_NAME_S, json_string (db_p -> di_name_s)) == 0)
				{
					bool success_flag = true;

					if (db_p -> di_download_uri_s)
						{
							success_flag = json_object_set_new (database_p, "database_download_uri", json_string (db_p -> di_download_uri_s)) == 0;
						}

					if (success_flag)
						{
							if (db_p -> di_info_uri_s)
								{
									success_flag = json_object_set_new (database_p, "database_information_uri", json_string (db_p -> di_info_uri_s)) == 0;
								}
						}

					if (success_flag)
						{
							if (json_object_set_new (marked_up_result_p, BSJMK_DATATBASE_S , database_p) == 0)
								{
									return true;
								}
						}
				}

			json_decref (database_p);
		}

	return false;
}



json_t *GetHitsFromMarkedUpReport (json_t *report_p)
{
	json_t *hits_p = NULL;

	if (report_p)
		{
			hits_p = json_object_get (report_p, BSJMK_REPORT_RESULTS_S);
		}

	return hits_p;
}



json_t *ConvertBlastResultToGrassrootsMarkUp (const json_t *blast_job_output_p, BlastServiceData *data_p)
{
	bool success_flag = false;
	json_t *markup_p = GetInitialisedProcessedRequest ();

	if (markup_p)
		{
			const json_t *blast_output_p = json_object_get (blast_job_output_p, "BlastOutput2");

			if (blast_output_p)
				{
					if (json_is_array (blast_output_p))
						{
							size_t i;
							json_t *result_p;

							json_t *markup_reports_p = GetMarkupReports (markup_p);

							json_array_foreach (blast_output_p, i, result_p)
								{
									json_t *blast_report_p = json_object_get (result_p, "report");

									if (blast_report_p)
										{
											const DatabaseInfo *db_p = GetDatabaseFromBlastResult (blast_report_p, data_p);

											if (db_p)
												{
													/* Get the hits */
													const json_t *blast_result_search_p = GetCompoundJSONObject (blast_report_p, "results.search");

													if (blast_result_search_p)
														{
															json_t *marked_up_report_p = AddAndGetMarkedUpReport (markup_reports_p, db_p, blast_result_search_p, blast_report_p);

															if (marked_up_report_p)
																{
																	const json_t *blast_hits_p =  json_object_get (blast_result_search_p, BSJMK_REPORT_RESULTS_S);

																	if (blast_hits_p)
																		{
																			if (json_is_array (blast_hits_p))
																				{
																					size_t j = 0;
																					const size_t num_hits = json_array_size (blast_hits_p);

																					json_t *marked_up_hits_p = json_object_get (marked_up_report_p, BSJMK_REPORT_RESULTS_S);

																					success_flag = true;

																					while ((j < num_hits) && success_flag)
																						{
																							const json_t *blast_hit_p = json_array_get (blast_hits_p, j);

																							if (AddMarkedUpHit (marked_up_hits_p, blast_hit_p, db_p))
																								{
																									++ j;
																								}
																							else
																								{
																									success_flag = false;
																								}
																						}		/* while ((j < num_hits) && success_flag) */


																				}		/* if (json_is_array (blast_hits_p)) */

																		}		/* if (blast_hits_p) */
																	else
																		{

																		}

																}		/* if (marked_up_report_p) */

														}		/* if (blast_result_search_p) */

												}		/* if (db_p) */

										}		/* if (blast_report_p) */

								}		/* json_array_foreach (blast_output_p, i, result_p) */

						}		/* if (json_is_array (blast_output_p)) */

				}		/* if (blast_output_p) */

			if (!success_flag)
				{
					json_decref (markup_p);
					markup_p = NULL;
				}
		}		/* if (markup_p) */

	return markup_p;
}


json_t *MarkUpBlastResult (BlastServiceJob *job_p)
{
	json_t *markup_p = NULL;
	BlastServiceData *data_p = (BlastServiceData *) (job_p -> bsj_job.sj_service_p -> se_data_p);
	json_t *blast_job_output_p = GetBlastResult (job_p, data_p);

	if (blast_job_output_p)
		{
			markup_p = ConvertBlastResultToGrassrootsMarkUp (blast_job_output_p, data_p);
			json_decref (blast_job_output_p);
		}		/* if (blast_job_output_p) */


	return markup_p;
}



json_t *GetInitialisedProcessedRequest (void)
{
	json_t *root_p = json_object ();

	if (root_p)
		{
			json_t *context_p = json_object ();

			if (context_p)
				{
					if (json_object_set_new (root_p, "@context", context_p) == 0)
						{
							if (AddEdamOntologyTerms (context_p))
								{
									if (AddFaldoOntologyTerms (context_p))
										{
											if (AddSequenceOntologyTerms (context_p))
												{
													if (AddSchemaOrgTerms (context_p))
														{
															if (AddGenomicFeatureAndVariationOntologyTerms (context_p))
																{
																	json_t *sequence_search_results_p = json_object ();

																	if (sequence_search_results_p)
																		{
																			if (json_object_set_new (root_p, BSJMK_RESULTS_S, sequence_search_results_p) == 0)
																				{
																					json_t *reports_p = json_array ();

																					if (reports_p)
																						{
																							if (json_object_set_new (sequence_search_results_p, BSJMK_REPORTS_S, reports_p) == 0)
																								{
																									return root_p;
																								}
																							else
																								{
																									json_decref (reports_p);
																								}
																						}

																				}
																			else
																				{
																					json_decref (sequence_search_results_p);
																				}

																		}

																}
														}
												}
										}
								}

						}		/* if (json_object_set_new (root_p, "@context", context_p) == 0) */
					else
						{
							json_decref (context_p);
						}

				}		/* if (context_p) */

			json_decref (root_p);
		}		/* if (root_p) */

	return NULL;
}




bool MarkUpHit (const json_t *hit_p, json_t *mark_up_p, const DatabaseInfo *db_p)
{
	bool success_flag = false;

	if (GetAndAddScaffoldsFromHit (hit_p, mark_up_p, db_p))
		{
			if (AddHitDetails (mark_up_p, hit_p, db_p))
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mark_up_p, "Failed to add hit details");
				}

		}		/* if (GetAndAddScaffoldsFromHit (hit_p, mark_up_p)) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mark_up_p, "Failed to add scaffolds");
		}

	return success_flag;
}


bool GetAndAddQueryMetadata (const json_t *blast_search_p, json_t *mark_up_p)
{
	bool success_flag = false;


	if (CopyJSONKeyStringValuePair (blast_search_p, mark_up_p, "query_id", false))
		{
			/* Not all dbs have a query_title so make it optional */
			if (CopyJSONKeyStringValuePair (blast_search_p, mark_up_p, "query_title", true))
				{
					if (CopyJSONKeyIntegerValuePair (blast_search_p, mark_up_p, "query_len", false))
						{
							if (AddQueryMasks (blast_search_p, mark_up_p))
								{
									success_flag = true;
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, blast_search_p, "Failed to copy key \"%s\"", "query_len");
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, blast_search_p, "Failed to copy key \"%s\"", "query_title");
				}
		}
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, blast_search_p, "Failed to copy key \"%s\"", "query_id");
		}


	return success_flag;
}




LinkedList *GetScaffoldsFromHit (const json_t *hit_p, const DatabaseInfo *db_p)
{
	LinkedList *scaffolds_p = AllocateStringLinkedList ();

	if (scaffolds_p)
		{
			json_t *description_p = json_object_get (hit_p, "description");

			if (description_p)
				{
					if (json_is_array (description_p))
						{
							size_t k;
							json_t *item_p;

							json_array_foreach (description_p, k, item_p)
								{
									const char *value_s = GetJSONString (item_p, db_p -> di_scaffold_key_s);

									if (value_s)
										{
											StringListNode *node_p = NULL;

											if (db_p -> di_scaffold_regex_s)
												{
													RegExp *reg_ex_p = AllocateRegExp (32);

													if (reg_ex_p)
														{
															if (SetPattern (reg_ex_p, db_p -> di_scaffold_regex_s, 0))
																{
																	if (MatchPattern (reg_ex_p, value_s))
																		{
																			/*
																			 * We only want the first match for the scaffold name
																			 */
																			char *match_s = GetNextMatch (reg_ex_p);

																			if (match_s)
																				{
																					node_p = AllocateStringListNode (match_s, MF_SHALLOW_COPY);

																					if (!node_p)
																						{
																							FreeCopiedString (match_s);
																							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add \"%s\" to list of scaffold names", match_s);
																						}
																				}

																		}
																}


															FreeRegExp (reg_ex_p);
														}

												}
											else
												{
													node_p = AllocateStringListNode (value_s, MF_DEEP_COPY);

													if (!node_p)
														{
															PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add \"%s\" to list of scaffold names", value_s);
														}

												}


											if (node_p)
												{
													LinkedListAddTail (scaffolds_p, (ListItem *) node_p);
												}		/* if (node_p) */

										}

								}		/* json_array_foreach (description_p, k, item_p) */

						}		/* if (json_is_array (description_p)) */

				}		/* if (description_p) */

		}		/* if (scaffolds_p) */

	return scaffolds_p;
}



bool GetAndAddNucleotidePolymorphisms (json_t *marked_up_hsp_p, const char *reference_sequence_s, const char *hit_sequence_s, const char *midline_s, uint32 hit_index, const int32 inc_value)
{
	bool success_flag = false;

	if (*midline_s != '\0')
		{
			bool loop_flag;
			uint32 start_of_region = hit_index;
			const char *hit_gap_start_p = NULL;
			const char *reference_gap_start_p = NULL;
			bool match_flag = (*midline_s == '|');

			++ midline_s;
			hit_index += inc_value;
			loop_flag = (*midline_s != '\0');

			success_flag = true;

			while (loop_flag && success_flag)
				{
					bool current_match_flag = (*midline_s == '|');

					/* have we moved to a different region? */
					if (match_flag != current_match_flag)
						{
							if (match_flag)
								{
									/* we've just started a gap */
									start_of_region = hit_index + inc_value;
									hit_gap_start_p = hit_sequence_s + 1;
									reference_gap_start_p = reference_sequence_s + 1;
								}
							else
								{
									/* we've just finished a gap */
									const uint32 end_of_region = hit_index; // - inc_value;

									if (!AddPolymorphism (marked_up_hsp_p, hit_gap_start_p, reference_gap_start_p, start_of_region, end_of_region))
										{
											PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to the polymorphism starting at reference \"%s\"", reference_gap_start_p);
											success_flag = false;
										}
								}

							match_flag = current_match_flag;
						}


					hit_index += inc_value;
					++ midline_s;
					++ reference_sequence_s;
					++ hit_sequence_s;

					loop_flag = (*midline_s != '\0');
				}

		}

	return success_flag;
}


static bool IsGap (const char c)
{
	return ((c == '-') || (c == 'n') || (c == 'N'));
}


static bool AddGaps (json_t *marked_up_hits_p, const char * const gaps_key_s, const char *sequence_s)
{
	bool success_flag = false;
	bool dec_flag = true;
	json_t *gaps_p = json_array ();

	if (gaps_p)
		{
			bool loop_flag = (*sequence_s != '\0');

			if (loop_flag)
				{
					bool gap_flag = IsGap (*sequence_s);
					int32 gap_start = -1;
					int32 index = 2;

					if (gap_flag)
						{
							gap_start = 0;
						}

					++ sequence_s;
					loop_flag = (*sequence_s != '\0');

					success_flag = true;

					while (loop_flag && success_flag)
						{
							bool current_gap_flag = IsGap (*sequence_s);

							if (gap_flag != current_gap_flag)
								{
									if (current_gap_flag)
										{
											/* we've just started a gap */
											gap_start = index;
										}
									else
										{
											/* we've just finished a gap */
											int32 gap_end = index - 1;

											if (!AddGap (gaps_p, gap_start, gap_end))
												{
													PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to the gap starting at reference \"%s\"", sequence_s);
													success_flag = false;
												}

											gap_start = -1;
										}

									gap_flag = current_gap_flag;
								}

							++ sequence_s;
							++ index;
							loop_flag = (*sequence_s != '\0');
						}

					if (gap_start != -1)
						{
							/* we've just finished a gap */
							int32 gap_end = index;

							if (!AddGap (gaps_p, gap_start, gap_end))
								{
									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to the gap starting at reference \"%s\"", sequence_s);
									success_flag = false;
								}
						}

				}


			if (success_flag)
				{
					if (json_array_size (gaps_p) > 0)
						{
							if (json_object_set_new (marked_up_hits_p, gaps_key_s, gaps_p) == 0)
								{
									dec_flag = false;
								}
							else
								{
									success_flag = false;
								}		/* if (json_object_set_new (marked_up_hits_p, gaps_key_s, gaps_p) == 0) */
						}
				}

			if (dec_flag)
				{
					json_decref (gaps_p);
				}

		}		/* if (gaps_p) */



	return success_flag;
}


static bool AddGap (json_t *gaps_p, const int32 from, const int32 to)
{
	json_t *gap_p = json_object ();

	if (gap_p)
		{
			if (json_object_set_new (gap_p, "@type", json_string ("gap")) == 0)
				{
					if (AddHitLocation (gap_p, BSJMK_LOCUS_S, from, to, ST_NONE))
						{
							if (json_array_append_new (gaps_p, gap_p) == 0)
								{
									return true;
								}
						}		/* if (AddHitLocation (gap_p, "faldo:location", from, to, forward_strand_flag)) */

				}		/* if (json_object_set_new (gap_p, "@type", json_string ("gap")) == 0) */

			json_decref (gap_p);
		}		/* if (gap_p) */

	return false;
}


bool AddPolymorphism (json_t *marked_up_hsp_p, const char *hit_gap_start_p, const char *reference_gap_start_p, const uint32 start_of_region, const uint32 end_of_region)
{
	bool success_flag = false;
	uint32 length;
	json_t *polymorphisms_p = NULL;

	if (start_of_region > end_of_region)
		{
			length = 1 + start_of_region - end_of_region;
		}
	else
		{
			length = 1 + end_of_region - start_of_region;
		}

	polymorphisms_p = json_object_get (marked_up_hsp_p, BSJMK_POLYMORPHISMS_S);

	if (!polymorphisms_p)
		{
			polymorphisms_p = json_array ();

			if (polymorphisms_p)
				{
					if (json_object_set_new (marked_up_hsp_p, BSJMK_POLYMORPHISMS_S, polymorphisms_p) != 0)
						{
							json_decref (polymorphisms_p);
							polymorphisms_p = NULL;
						}
				}
		}

	if (polymorphisms_p)
		{
			json_t *polymorphism_p = json_object ();

			if (polymorphism_p)
				{
					const char *type_s = (length == 1) ? BSJMK_SNP_S : BSJMK_MNP_S;

					if (AddHitLocation (polymorphism_p, BSJMK_LOCUS_S, start_of_region, end_of_region, ST_NONE))
						{
							if (json_object_set_new (polymorphism_p, "@type", json_string (type_s)) == 0)
								{
									json_t *diff_p = json_object ();

									if (diff_p)
										{
											if (json_object_set_new (polymorphism_p, BSJMK_SEQUENCE_DIFFERENCE_S, diff_p) == 0)
												{
													if (AddSubsequenceMarkup (diff_p, BSJMK_QUERY_S, reference_gap_start_p, length))
														{
															if (AddSubsequenceMarkup (diff_p, BSJMK_HIT_S, hit_gap_start_p, length))
																{
																	if (json_array_append_new (polymorphisms_p, polymorphism_p) == 0)
																		{
																			success_flag = true;
																		}
																}

														}

												}
											else
												{
													json_decref (diff_p);
												}
										}


								}

						}

					if (!success_flag)
						{
							json_decref (polymorphism_p);
						}

				}		/* if (polymorphism_p) */

		}


	return success_flag;
}


bool AddSubsequenceMarkup (json_t *parent_p, const char *key_s, const char *subsequence_start_s, const uint32 length)
{
	bool success_flag = false;
	char *diff_s = CopyToNewString (subsequence_start_s, length, false);

	if (diff_s)
		{
			if (json_object_set_new (parent_p, key_s, json_string (diff_s)) == 0)
				{
					success_flag = true;
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, parent_p, "Failed to add \"%s\": \"%s\"", key_s, diff_s);
				}

			FreeCopiedString (diff_s);
		}		/* if (diff_s) */
	else
		{
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to the first " UINT32_FMT " characters of \"%s\" for subsequence for key \"%s\"", length, subsequence_start_s, key_s);
		}

	return success_flag;
}




bool GetAndAddDatabaseMappedParameter (LinkedService *linked_service_p, const json_t *report_p, ParameterSet *output_params_p, const char **database_ss)
{
	bool success_flag = false;
	MappedParameter *mapped_param_p = GetMappedParameterByInputParamName (linked_service_p, "database");

	if (mapped_param_p)
		{
			const char *value_s = GetDatabaseName (report_p);

			if (value_s)
				{
					SharedType value;

					InitSharedType (&value);
					value.st_string_value_s = (char *) value_s;

					if (SetMappedParameterValue (mapped_param_p, output_params_p, &value))
						{
							*database_ss = value_s;
							success_flag = true;
						}

				}

		}		/* if (mapped_param_p) */
	else
		{
			success_flag = true;
		}

	return success_flag;
}


bool GetAndAddScaffoldsParameter (LinkedService *linked_service_p, json_t *hit_p, ParameterSet *output_params_p)
{
	bool success_flag = false;
	MappedParameter *mapped_param_p = GetMappedParameterByInputParamName (linked_service_p, BSJMK_SCAFFOLD_S);

	if (mapped_param_p)
		{
			Parameter *param_p = GetParameterFromParameterSetByName (output_params_p, mapped_param_p -> mp_output_param_s);

			if (param_p)
				{
					const json_t *scaffolds_p = GetScaffoldsForDatabaseHit (hit_p);

					if (scaffolds_p)
						{
							if (json_is_array (scaffolds_p))
								{
									size_t i;
									size_t num_added = 0;
									const size_t num_scaffolds = json_array_size (scaffolds_p);

									for (i = 0; i < num_scaffolds; ++ i)
										{
											const json_t *scaffold_p = json_array_get (scaffolds_p, i);
											const char *scaffold_s = GetJSONString (scaffold_p, BSJMK_SCAFFOLD_S);

											if (scaffold_s)
												{
													if (SetParameterValueFromString (param_p, scaffold_s))
														{
															if (AddLinkedServiceToRequestJSON (hit_p, linked_service_p, output_params_p))
																{
																	++ num_added;
																}
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add linked service \%s\" for scaffold \"%s\"", linked_service_p -> ls_output_service_s, scaffold_s);
																}

														}		/* if (SetParameterValueFromString (param_p, scaffold_s)) */

												}		/* if (scaffold_s) */

										}		/* for (i = 0; i < num_scaffolds; ++ i) */

									success_flag = (num_added == num_scaffolds);

								}		/* if (json_is_array (scaffolds_p)) */
							else
								{

								}

						}		/* if (scaffolds_p) */

				}		/* if (param_p) */

		}		/* if (mapped_param_p) */
	else
		{
			/* parameter isn't needed */
			success_flag = true;
		}

	return success_flag;
}


bool GetAndAddSequencesParameter (LinkedService *linked_service_p, json_t *hit_p, ParameterSet *output_params_p)
{
	bool success_flag = false;
	MappedParameter *mapped_param_p = GetMappedParameterByInputParamName (linked_service_p, "hit_data");

	if (mapped_param_p)
		{
			Parameter *param_p = GetParameterFromParameterSetByName (output_params_p, mapped_param_p -> mp_output_param_s);

			if (param_p)
				{
					json_t *hsps_p = json_object_get (hit_p, BSJMK_HSPS_S);

					if (hsps_p)
						{
							json_t *hsp_p;
							size_t k;
							size_t num_added = 0;

							json_array_foreach (hsps_p, k, hsp_p)
								{
									const char * const QUERY_SEQUENCE_KEY_S = BSJMK_QUERY_SEQUENCE_S;
									const char * const POLYMORPHISMS_KEY_S = BSJMK_POLYMORPHISMS_S;

									const char *query_sequence_s = GetJSONString (hsp_p, QUERY_SEQUENCE_KEY_S);

									if (query_sequence_s)
										{
											json_t *polymorphisms_p = json_object_get (hsp_p, POLYMORPHISMS_KEY_S);

											if (polymorphisms_p)
												{
													json_t *hit_data_p = json_object ();

													if (hit_data_p)
														{
															if (json_object_set_new (hit_data_p, QUERY_SEQUENCE_KEY_S, json_string (query_sequence_s)) == 0)
																{
																	if (json_object_set (hit_data_p, POLYMORPHISMS_KEY_S, polymorphisms_p) == 0)
																		{
																			char *sequences_value_s = json_dumps (hit_data_p, 0);

																			if (sequences_value_s)
																				{
																					if (SetParameterValueFromString (param_p, sequences_value_s))
																						{
																							if (AddLinkedServiceToRequestJSON (hit_p, linked_service_p, output_params_p))
																								{
																									++ num_added;
																								}
																							else
																								{
																									PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to add linked service \%s\" for sequences value \"%s\"", linked_service_p -> ls_output_service_s, sequences_value_s);
																								}
																						}		/* if (SetParameterValueFromString (param_p, sequences_value_s)) */
																					else
																						{
																							PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set parameter \"%s\" to sequences value \"%s\"", param_p -> pa_name_s, sequences_value_s);
																						}

																					free (sequences_value_s);
																				}		/* if (sequences_value_s) */
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, hit_data_p, "Failed to dump json for sequences value");
																				}

																		}		/* if (json_object_set (hit_data_p, POLYMORPHISMS_KEY_S, polymorphisms_p) == 0 */
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, polymorphisms_p, "Failed to set \"%s\"", POLYMORPHISMS_KEY_S);
																		}

																}		/* if (json_object_set_new (hit_data_p, QUERY_SEQUENCE_KEY_S, json_string (query_sequence_s)) == 0) */
															else
																{
																	PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to set \"%s\": \"%s\"", QUERY_SEQUENCE_KEY_S, query_sequence_s);
																}

															json_decref (hit_data_p);
														}
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create hit_data object");
														}

												}		/* if (polymorphisms_p) */
											else
												{
													PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, hsp_p, "Failed to get \"%s\"", POLYMORPHISMS_KEY_S);
												}

										}		/* if (query_sequence_s) */
									else
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, hsp_p, "Failed to get \"%s\"", QUERY_SEQUENCE_KEY_S);
										}

								}		/* json_array_foreach (hsps_p, k, hsp_p) */

						}		/* if (hsps_p) */

				}		/* if (param_p) */

		}		/* if (mapped_param_p) */
	else
		{
			/* parameter isn't needed */
			success_flag = true;
		}

	return success_flag;
}



bool GetAndAddScaffoldsFromHit (const json_t *hit_p, json_t *mark_up_p, const DatabaseInfo *db_p)
{
	bool success_flag = false;

	LinkedList *scaffolds_p = GetScaffoldsFromHit (hit_p, db_p);

	if (scaffolds_p)
		{
			json_t *scaffolds_array_p = json_array ();

			if (scaffolds_array_p)
				{
					if (json_object_set_new (mark_up_p, BSJMK_SCAFFOLDS_S, scaffolds_array_p) == 0)
						{
							StringListNode *node_p = (StringListNode *) (scaffolds_p -> ll_head_p);

							success_flag = true;

							while (node_p && success_flag)
								{
									json_t *scaffold_p = json_object ();

									if (scaffold_p)
										{
											if (json_object_set_new (scaffold_p, BSJMK_SCAFFOLD_S, json_string (node_p -> sln_string_s)) == 0)
												{
													if (json_array_append_new (scaffolds_array_p, scaffold_p) != 0)
														{
															PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add scaffold object to scaffolds array for \"%s\"", node_p -> sln_string_s);
															success_flag = false;
														}
												}
											else
												{
													PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add scaffold \"%s\" to scaffold object", node_p -> sln_string_s);
													success_flag = false;
												}

											if (!success_flag)
												{
													json_decref (scaffold_p);
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create JSON scaffold object");
											success_flag = false;
										}

									node_p = (StringListNode *) (node_p -> sln_node.ln_next_p);
								}		/* while (node_p) */

						}		/* if (json_object_set_new (mark_up_p, BSJMK_SCAFFOLDS_S, scaffolds_array_p) == 0) */

				}		/* if (scaffolds_array_p) */

			FreeLinkedList (scaffolds_p);
		}		/* if (scaffolds_p) */

	return success_flag;
}



const json_t *GetScaffoldsForDatabaseHit (const json_t *hit_p)
{
	const json_t *scaffolds_p = NULL;

	if (hit_p)
		{
			scaffolds_p = json_object_get (hit_p, BSJMK_SCAFFOLDS_S);
		}

	return scaffolds_p;
}

static const char *GetDatabaseName (const json_t *marked_up_report_p)
{
	const char *database_name_s = NULL;

	if (marked_up_report_p)
		{
			const json_t *database_p = json_object_get (marked_up_report_p, BSJMK_DATATBASE_S);

			if (database_p)
				{
					database_name_s = GetJSONString (database_p, BSJMK_DATATBASE_NAME_S);
				}
		}

	return database_name_s;
}


static bool AddSequenceOntologyTerms (json_t *context_p)
{
	bool success_flag = false;

	if (AddOntologyContextTerm (context_p, BSJMK_SCAFFOLD_S, "http://www.sequenceontology.org/browser/current_svn/term/SO:0000148", true))
		{
			if (AddOntologyContextTerm (context_p, "query_sequence", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000149", true))
				{
					if (AddOntologyContextTerm (context_p, "hit_sequence", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000149", true))
						{
							if (AddOntologyContextTerm (context_p, "evalue", "http://www.sequenceontology.org/browser/current_svn/term/SO:0001686", true))
								{
									if (AddOntologyContextTerm (context_p, "bit_score", "http://www.sequenceontology.org/browser/current_svn/term/SO:0001685", true))
										{
											if (AddOntologyContextTerm (context_p, "contained_by", "http://www.sequenceontology.org/browser/current_svn/term/contained_by", true))
												{
													if (AddOntologyContextTerm (context_p, "snp", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000694", false))
														{
															if (AddOntologyContextTerm (context_p, "mnp", "http://www.sequenceontology.org/browser/current_svn/term/SO:0001013", false))
																{
																	if (AddOntologyContextTerm (context_p, "match", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000039", false))
																		{
																			if (AddOntologyContextTerm (context_p, "nucleotide_match", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000347", false))
																				{
																					if (AddOntologyContextTerm (context_p, "protein_match", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000349", false))
																						{
																							if (AddOntologyContextTerm (context_p, BSJMK_SEQUENCE_DIFFERENCE_S, "http://www.sequenceontology.org/browser/current_svn/term/SO:0000413", true))
																								{
																									if (AddOntologyContextTerm (context_p, "gap", "http://www.sequenceontology.org/browser/current_svn/term/SO:0000730", false))
																										{
																											success_flag = true;
																										}
																								}
																						}
																				}
																		}
																}
														}
												}
										}
								}
						}
				}
		}

	return success_flag;
}


static bool AddEdamOntologyTerms (json_t *context_p)
{
	bool success_flag = false;

	if (AddOntologyContextTerm (context_p, "sequence_length", "http://edamontology.org/data_1249", false))
		{
			if (AddOntologyContextTerm (context_p, BSJMK_RESULTS_S, "http://edamontology.org/data_0857", true))
				{
					if (AddOntologyContextTerm (context_p, "database_metadata", "http://edamontology.org/data_0957", true))
						{
							if (AddOntologyContextTerm (context_p, "database_name", "http://edamontology.org/data_1056", true))
								{
									if (AddOntologyContextTerm (context_p, "query_masks", "http://edamontology.org/operation_0368", false))
										{
											success_flag = true;
										}
								}
						}
				}
		}

	return success_flag;
}


static bool AddGenomicFeatureAndVariationOntologyTerms (json_t *context_p)
{
	bool success_flag = false;

	if (AddOntologyContextTerm (context_p, BSJMK_LOCUS_S, "http://www.biointerchange.org/gfvo#Locus", true))
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddFaldoOntologyTerms (json_t *context_p)
{
	bool success_flag = false;

	if (json_object_set_new (context_p, "faldo", json_string ("http://biohackathon.org/resource/faldo")) == 0)
		{
			success_flag = true;
		}


	return success_flag;
}



static bool AddSchemaOrgTerms (json_t *context_p)
{
	bool success_flag = false;

	if (AddOntologyContextTerm (context_p, "software", "http://schema.org/SoftwareApplication", false))
		{
			success_flag = true;
		}

	return success_flag;
}




static bool AddMarkedUpHit (json_t *marked_up_results_p, const json_t *blast_hit_p, const DatabaseInfo *db_p)
{
	json_t *output_p = json_object ();

	if (output_p)
		{
			if (MarkUpHit (blast_hit_p, output_p, db_p))
				{
					if (json_array_append_new (marked_up_results_p, output_p) == 0)
						{
							return true;
						}
				}

			json_decref (output_p);
		}		/* if (output_p) */

	return false;
}



/*
 * Get the result. Ideally we'd like to get this in a format that we can parse, so to begin with we'll use the single json format
 * available in blast 2.3+
 */
static json_t *GetBlastResult (BlastServiceJob *job_p, BlastServiceData *data_p)
{
	json_t *blast_output_p = NULL;

	/*
	 * Get the result. Ideally we'd like to get this in a format that we can parse, so to begin with we'll use the single json format
	 * available in blast 2.3+
	 */
	char *raw_result_s = GetBlastResultByUUID (data_p, job_p -> bsj_job.sj_id, BOF_SINGLE_FILE_JSON_BLAST);

	if (raw_result_s)
		{
			json_error_t err;
			blast_output_p = json_loads (raw_result_s, 0, &err);

			if (!blast_output_p)
				{
					PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "error decoding blast result: \"%s\"\n\"%s\"\n%d %d %d\n%s\n", err.text, err.source, err.line, err.column, err.position, raw_result_s);
				}

			FreeCopiedString (raw_result_s);
		}
	else
		{
			char uuid_s [UUID_STRING_BUFFER_SIZE];

			ConvertUUIDToString (job_p -> bsj_job.sj_id, uuid_s);
			PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to get blast result for \"%s\"", uuid_s);

		}

	return blast_output_p;
}


static json_t *AddAndGetMarkedUpReport (json_t *markup_reports_p, const DatabaseInfo *database_p, const json_t *blast_result_search_p, const json_t *blast_report_p)
{
	json_t *report_p = json_object ();

	if (report_p)
		{
			json_t *results_p = json_array ();

			if (results_p)
				{
					if (json_object_set_new (report_p, BSJMK_REPORT_RESULTS_S, results_p) == 0)
						{
							if (GetAndAddDatabaseDetails (report_p, database_p))
								{
									if (GetAndAddQueryMetadata (blast_result_search_p, report_p))
										{
											if (AddSoftwareDetails (blast_report_p, report_p))
												{
													if (json_array_append_new (markup_reports_p, report_p) == 0)
														{
															return report_p;
														}
												}
										}
								}
						}
					else
						{
							json_decref (results_p);
						}
				}

			json_decref (report_p);
		}

	return NULL;
}



static const DatabaseInfo *GetDatabaseFromBlastResult (const json_t *blast_report_p, const BlastServiceData *data_p)
{
	const DatabaseInfo *db_p = NULL;

	json_t *db_json_p = GetCompoundJSONObject (blast_report_p, "search_target.db");

	/* Get the database name */
	if (db_json_p)
		{
			if (json_is_string (db_json_p))
				{
					const char *database_filename_s = json_string_value (db_json_p);

					if (database_filename_s)
						{
							db_p = GetMatchingDatabaseByFilename (data_p, database_filename_s);
						}		/* if (database_filename_s) */
				}
		}

	return db_p;
}


json_t *GetMarkupReports (json_t *markup_p)
{
	json_t *value_p = json_object_get (markup_p, BSJMK_RESULTS_S);

	if (value_p)
		{
			value_p = json_object_get (value_p, BSJMK_REPORTS_S);
		}

	return value_p;
}



static bool PopulateMarkedUpHit (json_t *marked_up_hit_p, const json_t *blast_hit_p, const DatabaseInfo *db_p)
{
	bool success_flag = false;
	int hit_num;
	const char *hit_type_s = NULL;

	if (db_p -> di_type == DT_NUCLEOTIDE)
		{
			hit_type_s = "nucelotide_match";
		}
	else if (db_p -> di_type == DT_PROTEIN)
		{
			hit_type_s = "protein_match";
		}


	if (hit_type_s)
		{
			if (json_object_set_new (marked_up_hit_p, "@type", json_string (hit_type_s)) == 0)
				{
					if (GetJSONInteger (blast_hit_p, "num", &hit_num))
						{
							if (json_object_set_new (marked_up_hit_p, "hit_num", json_integer (hit_num)) == 0)
								{
									if (GetAndAddIntScoreValue (marked_up_hit_p, blast_hit_p, "len", "sequence_length"))
										{
											json_t *marked_up_hsps_p = json_array ();

											if (marked_up_hsps_p)
												{
													if (json_object_set_new (marked_up_hit_p, BSJMK_HSPS_S, marked_up_hsps_p) == 0)
														{
															success_flag = true;
														}
													else
														{
															json_decref (marked_up_hsps_p);
														}
												}

										}

								}
						}

				}		/* if (json_object_set_new (marked_up_hit_p, "@type", json_string (hit_type_s)) == 0) */

		}		/* if (hit_type_s) */



	return success_flag;
}


static bool AddQueryMasks (const json_t *blast_search_p, json_t *mark_up_p)
{
	bool success_flag = false;
	const json_t *blast_query_masking_p = json_object_get (blast_search_p, "query_masking");

	if (blast_query_masking_p)
		{
			json_t *masks_p = json_array ();

			if (masks_p)
				{
					if (json_is_array (blast_query_masking_p))
						{
							size_t i;
							const size_t size = json_array_size (blast_query_masking_p);

							success_flag = true;

							for (i = 0; i < size; ++ i)
								{
									const json_t *blast_mask_p = json_array_get (blast_query_masking_p, i);
									int32 from;

									if (GetJSONInteger (blast_mask_p, "from", &from))
										{
											int32 to;

											if (GetJSONInteger (blast_mask_p, "to", &to))
												{
													bool added_mask_flag = false;
													json_t *mask_p = json_object ();

													if (mask_p)
														{
															if (json_object_set_new (mask_p, "@type", json_string ("sequence_masking")) == 0)
																{
																	if (AddHitLocation (mask_p, BSJMK_LOCUS_S, from, to, ST_NONE))
																		{
																			if (json_array_append_new (masks_p, mask_p) == 0)
																				{
																					added_mask_flag = true;
																				}
																			else
																				{
																					PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mask_p, "Failed append query mask to list of query masks");
																				}

																		}
																	else
																		{
																			PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mask_p, "Failed add query mask location data");
																		}

																}		/* if (json_object_set_new (mask_p, "@type", json_string ("sequence_masking")) == 0) */
															else
																{
																	PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mask_p, "Failed to set type for query mask");
																}

															if (!added_mask_flag)
																{
																	json_decref (mask_p);
																	success_flag = false;
																	i = size;		/* force exit from loop */
																}

														}		/* if (mask_p) */
													else
														{
															PrintErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, "Failed to create mask");
														}

												}		/* if (GetJSONInteger (blast_mask_p, "to", &to)) */

										}		/* if (GetJSONInteger (blast_mask_p, "from", &from)) */
								}

							if (success_flag)
								{
									if (json_object_set (mark_up_p, "query_masks", masks_p) != 0)
										{
											PrintJSONToErrors (STM_LEVEL_SEVERE, __FILE__, __LINE__, mark_up_p, "Failed to add masks array");
											success_flag = false;
										}
								}

						}

					if (!success_flag)
						{
							json_decref (masks_p);
						}
				}
		}
	else
		{
			success_flag = true;
		}

	return success_flag;
}


static bool AddSoftwareDetails (const json_t *blast_report_p, json_t *mark_up_p)
{
	json_t *software_p = json_object ();

	if (software_p)
		{
			if (json_object_set_new (software_p, "applicationSuite", json_string ("BLAST Command Line Applications")) == 0)
				{
					const char *value_s = GetJSONString (blast_report_p, "program");

					if (value_s)
						{
							if (json_object_set_new (software_p, "name", json_string (value_s)) == 0)
								{
									if (AddSoftwareVersion (blast_report_p, software_p))
										{
											if (json_object_set_new (mark_up_p, "software", software_p) == 0)
												{
													return true;
												}
											else
												{
													PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, mark_up_p, "Failed to add software details to mark up");
												}
										}
									else
										{
											PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to add version details to software mark up");
										}
								}
							else
								{
									PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, software_p, "Failed to set software name to \"%s\"", value_s);
								}
						}
					else
						{
							PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, blast_report_p, "Failed to get program");
						}
				}
			else
				{
					PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, software_p, "Failed to set \"applicationSuite\": \"BLAST Command Line Applications\"");
				}

			json_decref (software_p);
		}		/* if (software_p) */
	else
		{
			PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to create software json");
		}

	return false;
}


static bool AddSoftwareVersion (const json_t *blast_report_p, json_t *software_mark_up_p)
{
	bool success_flag = false;
	const char *value_s = GetJSONString (blast_report_p, "version");

	if (value_s)
		{
			/*
			 * The version is in the format "BLASTN 2.5.0+" so we need to
			 * parse it
			 */
			const char *start_p = value_s;
			while ((*start_p != '\0') && (isdigit (*start_p) == 0))
				{
					++ start_p;
				}

			if (*start_p != '\0')
				{
					const char *end_p = start_p + 1;

					while ((*end_p != '\0') && ((isdigit (*end_p)) || (*end_p == '.')))
						{
							++ end_p;
						}

					if (*end_p != '\0')
						{
							const size_t l = end_p - start_p;
							char *copied_value_s = CopyToNewString (start_p, l, false);

							if (copied_value_s)
								{
									if (json_object_set_new (software_mark_up_p, "softwareVersion", json_string (copied_value_s)) == 0)
										{
											success_flag = true;
										}
									else
										{
											PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, software_mark_up_p, "Failed to add \"softwareVersion\": \"%s\"", value_s);
										}

									FreeCopiedString (copied_value_s);
								}
							else
								{
									PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to copy " SIZET_FMT " chars for string from \"%s\"", l, start_p);
								}
						}
					else
						{
							PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to find start of version from \"%s\"", start_p);
						}

				}		/* if (*start_p != '\0') */
			else
				{
					PrintErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, "Failed to find start of version from \"%s\"", value_s);
				}

		}		/* if ((value_s = GetJSONString (blast_report_p, "version")) != NULL) */
	else
		{
			PrintJSONToErrors (STM_LEVEL_WARNING, __FILE__, __LINE__, blast_report_p, "Failed to get version");
		}

	return success_flag;
}


