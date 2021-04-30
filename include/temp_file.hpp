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
 * temp_file.hpp
 *
 *  Created on: 22 Apr 2015
 *      Author: tyrrells
 */

#ifndef TEMP_FILE_HPP_
#define TEMP_FILE_HPP_

#include <fstream>
#include <cstdio>

#include "uuid/uuid.h"

#include "blast_service_api.h"
#include "memory_allocations.h"

using namespace std;

/**
 * Create a temporary file to read and write data to.
 *
 * @ingroup util
 */
class BLAST_SERVICE_LOCAL TempFile
{
public:

	/**
	 * Create a TempFile.
	 *
	 * The filename used will be of the form
	 *
	 * \<working_dir_s\>/\<id\>.\<suffix_s\>.
	 *
	 * @param template_s The template for the filename. The filename
	 * generated from this will depend upon the value of tenp_flag.
	 * @param temp_flag If this is <code>true</code> then template_s will be passed
	 * to tmpnam. If not, template_s will be used as is for the filename.
	 * @return The new TempFile upon success or 0 upon failure.
	 */
	static TempFile *GetTempFile (const char *template_s, const bool temp_flag);

	/**
	 * Create a TempFile.
	 *
	 * The filename used will be of the form
	 *
	 * \<working_dir_s\>/\<id\>.\<suffix_s\>.
	 *
	 * @param working_dir_s The directory where the underlying file will be created.
	 * @param id The prefix of the filename.
	 * @param suffix_s The suffix of the filename
	 * @return The new TempFile upon success or 0 upon failure.
	 */
	static TempFile *GetTempFile (const char *working_dir_s, const uuid_t id, const char * const suffix_s);

	/**
	 * The TempFile destructor.
	 */
	~TempFile ();

	/**
	 * Get the filename being used by this TempFile.
	 *
	 * @return The filename or 0 if it is not set.
	 */
	const char *GetFilename () const;

	/**
	 * Open the TempFile
	 *
	 * @param mode_s The mode string which is the same as the c standard library mode string
	 * used by fopen.
	 * @return <code>true</code> if the file was opened
	 * successfully, <code>false</code> otherwise.
	 */
	bool Open (const char *mode_s);


	/**
	 * Close the TempFile
	 *
	 * @return 0 upon success. The value returned is the same as the c standard library
	 * function fclose.
	 */
	int Close ();


	/**
	 * Print a c-style string to  the TempFile
	 *
	 * @param arg_s The strng to print.
	 * @return <code>true</code> if the string was printed
	 * successfully, <code>false</code> otherwise.
	 */
	bool Print (const char *arg_s);

	/**
	 * Get the contents of the TempFile as a c-style string.
   *
	 * @return The contents of the file or 0 upon error.
	 */
	const char *GetData ();


	/**
	 * Clear the data from the underlying file.
	 */
	void ClearData ();


	/**
	 * Check if a TempFile is currently open.
	 *
	 * @return <code>true</code> if the file is open
	 * <code>false</code> otherwise.
	 */
	bool IsOpen () const;

private:
	/**
	 * @private
	 * The actual underlying filename.
	 */
	char *tf_name_s;

	/**
	 * @private
	 * This determines whether tf_name_s is freed when this
	 * TempFile gets deleted.
	 */
	MEM_FLAG tf_name_mem;

	/**
	 * @private
	 * The underlying FILE used.
	 */
	FILE *tf_handle_f;

	/**
	 * @private
	 * The data within the underlying file.
	 */
	char *tf_data_s;

	/**
	 * The TempFile constructor.
	 */
	TempFile ();
};


#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Create a buffer of the required length to be used by
 * TempFile *GetTempFile (const char *template_s, const bool temp_flag).
 *
 * The filename used will be of the form
 *
 * \<working_dir_s\>/\<id\>.\<suffix_s\>.
 *
 * @param working_directory_s The directory where the underlying file will be created.
 * @param prefix_s The prefix of the filename.
 * @param temp_suffix_s The suffix of the filename
 * @return The buffer containing the required string or <code>NULL</code>
 * upon error.
 * @memberof TempFile
 */
GRASSROOTS_UTIL_API char *GetTempFilenameBuffer (const char * const working_directory_s, const char * const prefix_s, const char * const temp_suffix_s);

#ifdef __cplusplus
}
#endif



#endif /* TEMP_FILE_HPP_ */
