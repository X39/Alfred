#pragma once

typedef void* CONFIG;

CONFIG config_create(const char*);
void config_close(CONFIG*);

int config_load(CONFIG);
int config_save(CONFIG);

int config_get_key_size(CONFIG, const char*);
const char* config_get_key(CONFIG, const char*, unsigned int);
int config_set_key(CONFIG, const char*, const char*, int);
int config_remove_key(CONFIG, const char*, int);


#ifdef CONFIGPRIV
typedef struct
{
	char** values;
	unsigned int lastindex_values;
	unsigned int size_values;
	char* key;
} KEYPAIR;

typedef struct
{
	char* path;

	KEYPAIR* keypair_array;
	unsigned int lastindex_keypair_array;
	unsigned int size_keypair_array;
} CONF;
typedef CONF* CONFPTR;

KEYPAIR* config_get_keypair(CONFIG, const char*);
#endif