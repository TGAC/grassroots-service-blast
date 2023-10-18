
# BLAST Services

The [BLAST](http://blast.ncbi.nlm.nih.gov/Blast.cgi) Services module allows BLAST queries to be submitted against a number of databases.  This Grassroots service module contains three Services: 

 * **BlastN** for searching nucleotide databases using nucleotide queries 
 * **BlastP** for searching protein databases using protein queries 
 * **BlastX** for searching protein databases using translated nucleotide queries 

## Installation

To build this service, you need the [grassroots core](https://github.com/TGAC/grassroots-core) and [grassroots build config](https://github.com/TGAC/grassroots-build-config) installed and configured. 

The files to build the BLAST service are in the `build` directory. 

### Linux and Mac

Enter the build directory for your your given platform which is inside the `build/unix/<platform>` 

For example, under linux:

```
cd build/unix/linux
```

whereas on MacOS:

```
cd build/unix/mac
```

then

```
make all
```

and then 

```
make install
```

to install the library into the Grassroots system where it will be available for use immediately.


### Windows

Under Windows, there is a Visual Studio project in the `build/windows` folder that allows you to
build the BLAST services.


## Service Configuration

Each of the three services listed above can be configured by files with the same names in the `config` directory in the Grassroots application directory, *e.g.* ```config/BlastN```

 * **working_directory**: This is the directory where are any input, output and log files created by the BLAST Services. This directory must be writeable by the user running the Grassroots Server. For instance, the httpd server is often run as the daemon user.
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
 * **scaffold_regex**: The regular expression used to get the scaffold name for the value associated with the value retrieved from using the scaffold_key. attribute above. If this key is omitted, then the entire value retrieved using the scffold_key is used as the scaffold name. For instance to get the first string up to any whitespace, the regular expression to use will be `([^\\s]*)`. Note that the backslash character is escaped.
 * **blast_formatter**: This key determines how the output from the BLAST searches can be converted between the different available output formats. Currently the only available option for this is **system**. 
 * **blast_command**: This is the path to the executable used to perform the searches. 
 * **blast_tool**: This determines how the BLAST search will be run and currently has the following options:
    * **system**: This will be run using the executable specified by *blast_command* to the ANSI-specified *system()* function. This is the default *blast_tool* option.
    * **drmaa**: This will be run by submitting a job to a DRMAA environment.

An example configuration file for the BlastN service which could be used is:

~~~{.json}
{
	"blast_command": "/opt/grassroots-0/grassroots/extras/blast/bin/blastn",
	"blast_formatter": "system",
	"system_formatter_config": {
		"command": "/opt/grassroots-0/grassroots/extras/blast/bin/blast_formatter"
	},
	"working_directory": "/home/billy/blast_working_dir/apache0",
	"databases": [{
		"filename": "/opt/grassroots-0/grassroots/extras/blast/databases/Chinese_spring_TGAC_v1_arm-classified.fasta",
		"name": "ChineseSpring",
		"description": "Chinese Spring"
	}, {
		"filename": "/opt/grassroots-0/grassroots/extras/blast/databases/TRIUR3.120813.filter150.cds",
		"name": "Triticum urartu",
		"description": "Triticum urartu",
		"active": false
	}, {
		"filename": "/opt/grassroots-0/grassroots/extras/blast/databases/TA009XXX.fasta",
		"name": "TA009XXX",
		"description": "TA009XXX",
		"active": false
	}],
	"groups": {
		"General Algorithm Parameters": {
			"visible": false
		}
	},
	"parameters": {
		"max_target_sequences": {
			"default_value": 13
		}
	}, 
	"linked_services": {
		"so:name": "SamTools",
		"parameters": {
			"mappings": [{
				"input": "database",
				"output": "input_file"
			}, {
				"input": "scaffold",
				"output": "Scaffold"
			}]
		}
	}
}

~~~

### Linked Service keys

Each of the BLAST services have the ability to link their results to use as input values for other services using the Grassroots Linked Services architecture. For more information, see the main Grassroots documentation.

The available keys for the BLAST services are:   

 * **database**: The database from which each hit comes from.
 * **scaffold**: The list of scaffolds that each hit belongs to 
 * **hit_data**: This gives the JSON fragment the query sequence and its differences to the hit, This is described in the next section.
 
#### hit_data

The *hit_data* value contains two keys:
  * **query_sequence**: The genetic sequence that was the input used to run BLAST with.
  * **polymorphisms**: A JSON fragment that details the differences between the query sequence and the hit sequence found. 

  
For example consider a BLAST search for the following hit and query sequences where the query sequence in on the first line, the hit sequence on the third and the midline element on line 2 to help as a visual aid in seeing the differences between the two.
    
~~~
TCCAACCCAAAATCTTGTCA-CTCGATCCATTGTAGAGCGCGAGCGCAAGCAGCAGCAGCAGCCATGAACCCGGACAA
||||||||||||||||||||  ||||||||||||||||||||||||||||||||||||||||||||||||||||||||
TCCAACCCAAAATCTTGTCAGTTCGATCCATTGTAGAGCGCGAGCGCAAGCAGCAGCAGCAGCCATGAACCCGGACAA
~~~
      
Gives the following *polymorphisms* JSON fragment:
 
~~~.json
{
	"polymorphisms": [{
		"locus": {
			"faldo:begin": {
				"@type": "faldo:ExactPosition",
				"faldo:position": 21
			},
			"faldo:end": {
				"@type": "faldo:ExactPosition",
				"faldo:position": 22
			},
			"@type": "faldo:Region"
		},
		"@type": "mnp",
		"sequence_difference": {
			"query": "-C",
			"hit": "GT"
		}
	}]
}
~~~

So the complete *hit_data* value would be:

~~~.json
{
  "query_sequence": "TCCAACCCAAAATCTTGTCA-CTCGATCCATTGTAGAGCGCGAGCGCAAGCAGCAGCAGCAGCCATGAACCCGGACAA", 
	"polymorphisms": [{
		"locus": {
			"faldo:begin": {
				"@type": "faldo:ExactPosition",
				"faldo:position": 21
			},
			"faldo:end": {
				"@type": "faldo:ExactPosition",
				"faldo:position": 22
			},
			"@type": "faldo:Region"
		},
		"@type": "mnp",
		"sequence_difference": {
			"query": "-C",
			"hit": "GT"
		}
	}]
}
~~~

