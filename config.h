#pragma once

#ifdef FNC
	#undef FNC
#endif
#define FNC(TXT) config_##TXT

typedef void* CONFIG;
typedef void* KEY;
typedef unsigned short DATATYPE;
typedef struct
{
	char* name;
	char* value;
	unsigned int name_size;
	unsigned int value_size;
} PAIR;

#define DATATYPE_NA 0
#define DATATYPE_NODE 1
#define DATATYPE_ARG 2
#define DATATYPE_STRING 3


/* Creates new instance of a config object. */
CONFIG FNC(create)(void);
/* Destroy given instance of config object and nullifies ptr. */
void FNC(destroy)(CONFIG*);

/* Loads instance of config from disk. returns 0 on success. */
int FNC(load)(CONFIG, const char*);
/* Saves current state of config to disk. returns 0 on success. */
int FNC(save)(CONFIG, const char*);

/*
Searches for given key in config instance.
returns NULL if key was not found.
key format:
IDENT { '/' IDENT }
*/
KEY FNC(get_key)(CONFIG, const char*);
/*
Searches for given key in config instance.
if not found, key gets created with DATATYPE_NA.
NULL might be returned in case of invalid input (eg. "\0" passed)
key format:
IDENT { '/' IDENT }
*/
KEY FNC(get_or_create_key)(CONFIG, const char*);
/* Returns the DATATYPE of provided key. */
DATATYPE FNC(key_get_type)(KEY);
/* Returns the size of provided key. */
unsigned int FNC(key_get_size)(KEY);
/* Transforms given key to given DATATYPE and returns the same key again. */
KEY FNC(key_to_type)(KEY, DATATYPE);

/* Returns the string value of given key or a NULL ptr if key was not of type DATETYPE_STRING. */
const char* FNC(key_get_string)(KEY);
/* Sets string value of given key. In case key was not of type string, it will get transformed. */
void FNC(key_set_string)(KEY, const char*);

/* returns a PTR to the children array or NULL ptr if given key was not of type DATETYPE_NODE. */
KEY* FNC(key_get_children)(KEY);
/* creates a new KEY instance in provided key. In case key was not of type DATATYPE_NODE, it will get transformed. returns newly created key. */
KEY FNC(key_create_child)(KEY);
/* Drops provided key from other key. */
void FNC(key_drop_child)(KEY, KEY*);


/* returns a PTR to the PAIR of this key or NULL PTR if given key was not of type DATATYPE_ARG. */
const PAIR FNC(key_get_arg)(KEY);
/* sets given keys ARGPAIR value to provided values. In case key was not of the type DATATYPE_ARG, it will get transformed. */
void FNC(key_set_arg)(KEY, const char*);

/* Sets name of provided key. */
void FNC(key_set_name)(KEY, const char*);
/* Returns name of provided key. */
const char* FNC(key_get_name)(KEY);


#ifdef CONFIGPRIV
typedef struct
{
	DATATYPE type;

	void* value;
	unsigned int value_size;
	unsigned int value_length;
	char* name;
	unsigned int name_size;
} DATA;

typedef struct
{
	DATA* root;
} CONF;
typedef CONF* CONFPTR;

DATA* FNC(data_create)();
void FNC(data_destroy)(DATA**);
#endif