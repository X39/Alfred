#define CONFIGPRIV
#include "global.h"
#include "config.h"
#include "yxml.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>


KEYPAIR* config_get_keypair(CONFIG config, const char* key)
{
	CONFPTR conf = (CONFPTR)config;
	KEYPAIR* kpPointer;
	unsigned int i;
	#ifdef DEBUG
	printf("[DEBU]\tconfig_get_keypair - key{1}: %s\n", key);
	#endif
	for (i = 0; i < conf->lastindex_keypair_array; i++)
	{
		if (!str_cmpi(conf->keypair_array[i].key, key))
		{
			return conf->keypair_array + i;
		}
	}
	if (conf->lastindex_keypair_array == conf->size_keypair_array)
	{
		conf->size_keypair_array += BUFF_INCREASE;
		conf->keypair_array = (KEYPAIR*)realloc(conf->keypair_array, sizeof(KEYPAIR) * conf->size_keypair_array);
	}


	kpPointer = conf->keypair_array + conf->lastindex_keypair_array;
	kpPointer->key = (char*)malloc(sizeof(char) * strlen(key) + 1);
	strcpy(kpPointer->key, key);

	kpPointer->values = (char**)malloc(sizeof(char*) * BUFF_SIZE_TINY);
	memset(kpPointer->values, 0, sizeof(char*) * BUFF_SIZE_TINY);
	kpPointer->lastindex_values = 0;
	kpPointer->size_values = BUFF_SIZE_TINY;

	conf->lastindex_keypair_array++;
	return conf->keypair_array + conf->lastindex_keypair_array - 1;
}



CONFIG config_create(const char* filepath)
{
	CONFPTR conf = (CONFPTR)malloc(sizeof(CONF));
	#ifdef DEBUG
	printf("[DEBU]\tconfig_create - filepath{1}: %s\n", filepath);
	#endif

	conf->path = (char*)malloc(sizeof(char) * strlen(filepath) + 1);
	strcpy(conf->path, filepath);

	conf->keypair_array = (KEYPAIR*)malloc(sizeof(KEYPAIR) * BUFF_SIZE_TINY);
	conf->lastindex_keypair_array = 0;
	conf->size_keypair_array = BUFF_SIZE_TINY;

	return conf;
}
void config_close(CONFIG* config)
{
	CONFPTR conf = *config;
	unsigned int i, j;
	KEYPAIR* kp;

	free(conf->path);

	for (i = 0; i < conf->lastindex_keypair_array; i++)
	{
		kp = conf->keypair_array + i;
		for (j = 0; j < kp->lastindex_values; j++)
		{
			free(kp->values[j]);
		}
		free(kp->values);
		free(kp);
	}
	free(conf->keypair_array);

	free(conf);
	*config = NULL;
}
int config_load(CONFIG config)
{
	CONFPTR conf = (CONFPTR)config;
	char yxmlBuffer[BUFF_SIZE_LARGE];
	char elNameBuffer[BUFF_SIZE_SMALL];
	char buffer[BUFF_SIZE_SMALL];
	int bufferIndex = 0;
	yxml_t yxml;
	yxml_ret_t res;
	FILE* f;
	long fsize;
	KEYPAIR* kp;
	int charcode;
	int skip = 1;


	f = fopen(conf->path, "rb");
	if (!f)
		return -1;
	yxml_init(&yxml, yxmlBuffer, BUFF_SIZE_LARGE);

	while((charcode = fgetc(f)) >= 0)
	{
		res = yxml_parse(&yxml, charcode);
		if (res < 0)
		{
			switch (res)
			{
				case YXML_EREF:
				return -1;
				case YXML_ECLOSE:
				return -2;
				case YXML_ESTACK:
				return -3;
				case YXML_ESYN:
				return -4;
			}
		}
		else
		{
			switch (res)
			{
				case YXML_ELEMSTART:
				bufferIndex = 0;
				strncpy(elNameBuffer, yxml.elem, BUFF_SIZE_SMALL);
				break;
				case YXML_ELEMEND:
				if (!strcmp(yxml.elem, ""))
					break;
				buffer[bufferIndex] = '\0';
				config_set_key(config, elNameBuffer, buffer, -1);
				break;
				case YXML_CONTENT:
				buffer[bufferIndex] = yxml.data[0];
				bufferIndex++;
				break;
			}
		}
	}
	fclose(f);
	return 0;
}
int config_save(CONFIG config)
{
	FILE* f;
	CONFPTR conf = (CONFPTR)config;
	KEYPAIR* kp;
	unsigned int i, j;
	f = fopen(conf->path, "wb");
	if (!f)
		return -1;
	fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	fprintf(f, "<root>\n");

	for (i = 0; i < conf->lastindex_keypair_array; i++)
	{
		kp = conf->keypair_array + i;
		for (j = 0; j < kp->lastindex_values; j++)
		{
			fprintf(f, "\t<%s>%s</%s>\n", kp->key, kp->values[j], kp->key);
		}
	}

	fprintf(f, "</root>\n");
	fflush(f);
	fclose(f);
	return 1;
}

int config_get_key_size(CONFIG config, const char* key)
{
	KEYPAIR* kp = config_get_keypair(config, key);
	return kp->lastindex_values;
}
const char* config_get_key(CONFIG config, const char* key, unsigned int index)
{
	KEYPAIR* kp = config_get_keypair(config, key);
	#ifdef DEBUG
	printf("[DEBU]\tconfig_get_key - key{1}: %s index{2}: %d --> ", key, index);
	#endif
	if (kp->lastindex_values <= index)
	{
		#ifdef DEBUG
		printf("NULL\n");
		#endif
		return NULL;
	}
	#ifdef DEBUG
	printf("%s\n", kp->values[index]);
	#endif
	return kp->values[index];
}
int config_set_key(CONFIG config, const char* key, const char* value, int index)
{
	KEYPAIR* kp = config_get_keypair(config, key);
	#ifdef DEBUG
	printf("[DEBU]\tconfig_set_key - key{1}: %s value{2}: %s index{3}: %d\n", key, value, index);
	#endif
	if (index == -1)
	{
		if (value == NULL)
			return -1;
		kp->values[kp->lastindex_values] = (char*)malloc(sizeof(char) * strlen(value) + 1);
		strcpy(kp->values[kp->lastindex_values], value);
		kp->lastindex_values++;
	}
	else
	{
		if (index > kp->lastindex_values)
			return -2;
		else if (index == kp->lastindex_values)
			kp->lastindex_values++;
		if (kp->values[kp->lastindex_values])
		{
			free(kp->values[kp->lastindex_values]);
		}
		if (value == NULL)
		{
			kp->values[kp->lastindex_values] = NULL;
		}
		else
		{
			kp->values[kp->lastindex_values] = (char*)malloc(sizeof(char) * strlen(value) + 1);
			strcpy(kp->values[kp->lastindex_values], value);
		}
	}

	if (kp->lastindex_values == kp->size_values)
	{
		kp->size_values += BUFF_INCREASE;
		kp->values = (char**)realloc(kp->values, sizeof(char*) * kp->size_values);
	}
	return 0;
}
int config_remove_key(CONFIG config, const char* key, int index)
{
	KEYPAIR* kp = config_get_keypair(config, key);
	#ifdef DEBUG
	printf("[DEBU]\tconfig_remove_key - key{1}: %s index{2}: %d\n", key, index);
	#endif
	if (index < 0 || kp->lastindex_values <= index)
		return -1;
	free(kp->values[index]);
	kp->values[index] = kp->values[kp->lastindex_values--];
	return 0;
}