BLAST Service 
=============

The [BLAST](http://blast.ncbi.nlm.nih.gov/Blast.cgi) Service allows BLAST queries to be submitted against a number of databases.

## Installation




## Server Configuration


 * **working_directory**: This is the directory where are any input, output and log files created by the BLAST Service. This directory must be writeable by the user running the Grassroots Server. For instance, the httpd server is often run as the daemon user.
 * **databases**: This is an array of objects giving the details of the available databases. The objects in this array have the following keys:
    * **name**:  This is the name to show to the user for this database. 
    * **filename**: This is the database value that the BLAST query will use to search against.
    * **description**: This is a user-friendly description to display to the user.
    * **active**: This is a boolean value that specifies whether the database is selected to search against by default. 
    * **type**: This specifies the type of database which in turn determines what BLAST tools can query it. The available values are:
        * **nuceleotide**: This declares the database as nucleotide one for usage with tools such as BlastN. If the *type* key is omitted, this is the default.
        * **protein**: This specifies that the database is for a protein for usage with tools such as BlastP, BlastX, *etc.*
 	* **download_uri**: This is an optional key used to specify a URI where the database file(s) can be downloaded from.
 	* **info_uri**: This is an optional key used to specify a URI where the more information about this database can be found.
 	* **scaffold_key**: 	The key used to get the scaffold name for any hits from BLAST searches from within the ``BlastOutput2.report.results.search.hits.description`` field of the search result in single file JSON format. This defaults to ``id``.
 	* **scaffold_regex**: The regular expression used to get the scaffold name for the value associated with the value retrieved from using the scaffold_key. attribute above. If this key is omitted, then the entire value retrieved using the scffold_key is used as the scaffold name. For instance to get the first string up to any whitespace, the regular expression to use will be `([^\\s]*)`. Note that the backslash character has had to be escaped.  
 * **blast_formatter**: This key determines how the output from the BLAST searches can be converted between the different available output formats. Currently the only available option for this is **system**. 
 * **blast_command**: This is the path to the executable used to perform the searches. 
 * **blast_tool**: This determines how the BLAST search will be run and currently has the following options:
    * **system**: This will be run using the executable specified by *blast_command* to the ANSI-specified *system()* function. This is the default *blast_tool* option.
    * **drmaa**: This will be run by submitting a job to a DRMAA environment.


### Linked %Service keys

 * **database**: The database from which each hit comes from.
 * **scaffold**: The list of scaffolds that each hit belongs to 