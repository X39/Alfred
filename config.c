#define CONFIGPRIV
#include "global.h"
#include "config.h"
#include "yxml.h"
#include "string_op.h"

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>


CONFIG FNC(create)(void)
{
	CONFPTR ptr = malloc(sizeof(CONF));
	ptr->root = FNC(data_create)();
	FNC(key_set_name)(ptr->root, "root");
	FNC(key_to_type)(ptr->root, DATATYPE_NODE);
	return ptr;
}
void FNC(destroy)(CONFIG* c)
{
	CONFPTR conf = *c;

	if (conf->root != NULL)
	{
		FNC(data_destroy)(&conf->root);
	}
	free(conf);
	*c = NULL;
}
DATA* FNC(data_create)()
{
	DATA* d = malloc(sizeof(DATA));
	memset(d, 0, sizeof(DATA));
	return d;
}
void FNC(data_destroy)(DATA** inPtr)
{
	DATA* d = *inPtr;
	DATA** children;
	unsigned int i;
	switch (d->type)
	{
		case DATATYPE_NODE:
		for (i = 0; i < d->value_size; i++)
		{
			children = d->value;
			if (children[i] == NULL)
				continue;
			FNC(data_destroy)(children + i);
		}
		case DATATYPE_ARG: case DATATYPE_STRING:
		if (d->name != NULL)
			free(d->name);
		if (d->value != NULL)
			free(d->value);
		break;
	}
	free(d);
	*inPtr = NULL;
}

int FNC(load)(CONFIG c, const char* path)
{
	/* ToDo: Fix that top hirarchie is not allowed to have multiple instances (eg. 'root/foobar/foo' foobar is not allowed to occur a second time or error will happen :3 "kinda") */
	CONFPTR conf = (CONFPTR)c;
	char yxmlBuffer[BUFF_SIZE_LARGE];
	char elNameBuffer[BUFF_SIZE_SMALL];
	char buffer[BUFF_SIZE_SMALL];
	int bufferIndex = 0;
	yxml_t yxml;
	yxml_ret_t res;
	FILE* f;
	int charcode;
	DATA *d, *d2;
	bool lastWasEnd = false;


	f = fopen(path, "rb");
	if (!f)
		return -1;
	yxml_init(&yxml, yxmlBuffer, BUFF_SIZE_LARGE);

	while ((charcode = fgetc(f)) >= 0)
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
				str_repchr(yxml.stack + 1, '\0', '/', yxml.elem - (char*)yxml.stack - 2);
				if (strncmp(yxml.stack + 1, "root/", 5))
				{
					d = conf->root;
					if (strcmp(yxml.elem, "root"))
					{
						d = FNC(key_create_child)(d);
						FNC(key_set_name)(d, yxml.elem);
					}
				}
				else
				{
					d = FNC(get_or_create_key)(c, yxml.stack + 1);
					d = FNC(key_create_child)(d);
					FNC(key_set_name)(d, yxml.elem);
				}
				str_repchr(yxml.stack + 1, '/', '\0', yxml.elem - (char*)yxml.stack - 2);
				lastWasEnd = false;
				break;

				case YXML_ELEMEND:
				if (bufferIndex > 0 && !lastWasEnd)
				{
					buffer[bufferIndex] = '\0';
					d2 = FNC(key_create_child)(d);
					FNC(key_set_string)(d2, buffer);
				}
				bufferIndex = 0;
				lastWasEnd = true;
				break;

				case YXML_CONTENT:
				if (bufferIndex != 0 || !chr_is(*yxml.data, "\n\r\t "))
				{
					buffer[bufferIndex] = yxml.data[0];
					bufferIndex++;
				}
				break;

				case YXML_ATTRSTART:
				bufferIndex = 0;
				lastWasEnd = false;
				break;

				case YXML_ATTRVAL:
				buffer[bufferIndex] = yxml.data[0];
				bufferIndex++;
				break;

				case YXML_ATTREND:
				buffer[bufferIndex] = '\0';
				d2 = FNC(key_create_child)(d);
				FNC(key_set_arg)(d2, buffer);
				FNC(key_set_name)(d2, yxml.attr);
				bufferIndex = 0;
				lastWasEnd = false;
				break;
			}
		}
	}
	fclose(f);
	return 0;
}
void FNC(save_helper)(FILE* f, DATA* d, unsigned int tabcount)
{
	unsigned int i, j;
	bool containsString;
	switch (d->type)
	{
		case DATATYPE_NODE:
		for (j = 0; j < tabcount; j++)
			fprintf(f, "\t");
		containsString = false;
		fprintf(f, "<%s", d->name);
		for (i = 0; i < d->value_length; i++)
		{
			if (((DATA**)d->value)[i] == NULL)
				continue;
			if (((DATA**)d->value)[i]->type != DATATYPE_ARG)
			{
				if (((DATA**)d->value)[i]->type == DATATYPE_STRING)
					containsString = true;
				continue;
			}
			FNC(save_helper)(f, ((DATA**)d->value)[i], tabcount + 1);
		}
		if (containsString)
		{
			fprintf(f, ">", d->name);
		}
		else
		{
			fprintf(f, ">\n", d->name);
		}
		for (i = 0; i < d->value_length; i++)
		{
			if (((DATA**)d->value)[i] == NULL)
				continue;
			if (((DATA**)d->value)[i]->type == DATATYPE_ARG)
			{
				continue;
			}
			FNC(save_helper)(f, ((DATA**)d->value)[i], tabcount + 1);
		}
		if (!containsString)
		{
			for (j = 0; j < tabcount; j++)
				fprintf(f, "\t");
		}
		fprintf(f, "</%s>\n", d->name);
		break;
		case DATATYPE_ARG:
		if (d->value != NULL)
		{
			fprintf(f, " %s=\"%s\"", d->name == NULL ? "NOTSET" : d->name, d->value);
		}
		break;
		case DATATYPE_STRING:
		if (d->name == NULL)
		{
			fprintf(f, "%s", d->value);
		}
		else
		{
			for (j = 0; j < tabcount; j++)
				fprintf(f, "\t");
			fprintf(f, "<%s>%s</%s>\n", d->name, d->value, d->name);
		}
		break;
	}
}
int FNC(save)(CONFIG c, const char* path)
{
	FILE* f;
	f = fopen(path, "wb");
	if (!f)
		return -1;
	fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");

	FNC(save_helper)(f, ((CONFPTR)c)->root, 0);

	fflush(f);
	fclose(f);
	return 0;
}

KEY FNC(get_key)(CONFIG c, const char* key)
{
	CONFPTR conf = c;
	char* keyCopy;
	unsigned int i, keyCount;
	DATA* d = conf->root;
	DATA* res;
	DATA** children;
	if (key == NULL || key[0] == '\0')
		return NULL;
	keyCopy = alloca(sizeof(char) * strlen(key) + 1);
	strcpy(keyCopy, key);
	keyCount = str_repchr(keyCopy, '/', '\0', -1);
	keyCopy += strlen(keyCopy) + 1;
	while (keyCount-- != 0)
	{
		res = NULL;
		for (i = 0; i < d->value_length; i++)
		{
			children = d->value;
			if (children[i] == NULL || children[i]->name == NULL)
				continue;
			if (strcmpi(children[i]->name, keyCopy))
				continue;
			d = res = children[i];
			keyCopy += strlen(keyCopy) + 1;
			break;
		}
	}
	return res;
}
KEY FNC(get_or_create_key)(CONFIG c, const char* key)
{
	CONFPTR conf = c;
	char* keyCopy;
	unsigned int i, keyCount;
	DATA* d = conf->root;
	DATA* res;
	DATA** children;
	if (key == NULL || key[0] == '\0')
		return NULL;
	keyCopy = alloca(sizeof(char) * strlen(key) + 1);
	strcpy(keyCopy, key);
	keyCount = str_repchr(keyCopy, '/', '\0', -1);
	keyCopy += strlen(keyCopy) + 1;
	while (keyCount-- != 0)
	{
		res = NULL;
		for (i = 0; i < d->value_length; i++)
		{
			children = d->value;
			if (children[i] == NULL || children[i]->name == NULL)
				continue;
			if (strcmpi(children[i]->name, keyCopy))
				continue;
			d = res = children[i];
			break;
		}
		keyCopy += strlen(keyCopy) + 1;
		if (res == NULL)
			break;
	}
	if (res == NULL)
	{
		res = d;
		while (keyCount-- != 0)
		{
			res = FNC(key_create_child)(res);
			FNC(key_to_type)(res, DATATYPE_NODE);
			FNC(key_set_name)(res, keyCopy);
			keyCopy += strlen(keyCopy) + 1;
		}
	}
	return res;
}

DATATYPE FNC(key_get_type)(KEY key)
{
	if (key == NULL)
		return DATATYPE_NA;
	return ((DATA*)key)->type;
}
unsigned int FNC(key_get_size)(KEY key)
{
	if (key == NULL)
		return 0;
	return ((DATA*)key)->type == DATATYPE_NODE ? ((DATA*)key)->value_length : ((DATA*)key)->value_size;
}
KEY FNC(key_to_type)(KEY key, DATATYPE type)
{
	DATA* d = key;
	DATA** children;
	unsigned int i;
	if (d->type == type)
		return key;
	switch (d->type)
	{
		case DATATYPE_NODE:
		for (i = 0; i < d->value_size; i++)
		{
			children = d->value;
			if (children[i] == NULL)
				continue;
			FNC(data_destroy)(children + i);
		}
		case DATATYPE_ARG: case DATATYPE_STRING:
		if (d->value != NULL)
			free(d->value);
		break;
	}
	switch (type)
	{
		case DATATYPE_NODE:
		d->value = malloc(sizeof(DATA*) * BUFF_SIZE_TINY);
		memset(d->value, 0, sizeof(DATA*) * BUFF_SIZE_TINY);
		d->value_size = BUFF_SIZE_TINY;
		d->value_length = 0;
		d->type = type;
		break;
		case DATATYPE_ARG: case DATATYPE_STRING:
		d->value = NULL;
		d->value_size = 0;
		d->type = type;
		break;
		default:
		d->type = DATATYPE_NA;
		break;
	}
	return key;
}

const char* FNC(key_get_string)(KEY key)
{
	if (key == NULL)
		return NULL;
	return ((DATA*)key)->value;
}
void FNC(key_set_string)(KEY key, const char* value)
{
	DATA* d;
	if (key == NULL)
		return;
	d = key;
	if (d->type != DATATYPE_STRING)
		FNC(key_to_type)(key, DATATYPE_STRING);
	if (d->value == NULL && value != NULL)
	{
		d->value_size = strlen(value) + 1;
		d->value = malloc(sizeof(char) * d->value_size);
		strcpy(d->value, value);
		((char*)d->value)[d->value_size - 1] = '\0';
	}
	else
	{
		d->value_size = strlen(value) + 1;
		d->value = realloc(d->value, sizeof(char) * d->value_size);
		strcpy(d->value, value);
		((char*)d->value)[d->value_size - 1] = '\0';
	}
}

KEY* FNC(key_get_children)(KEY key)
{
	if (key == NULL)
		return NULL;
	return ((DATA*)key)->value;
}
KEY FNC(key_create_child)(KEY key)
{
	DATA* d = key;
	unsigned int i;
	if (d->type != DATATYPE_NODE)
		FNC(key_to_type)(key, DATATYPE_NODE);
	for (i = 0; ((DATA**)d->value)[i] != NULL && i < d->value_size; i++);
	if (i >= d->value_size)
	{
		d->value_size += BUFF_INCREASE;
		d->value = realloc(d->value, sizeof(DATA*) * d->value_size);
		memset(&((DATA**)d->value)[i], 0, sizeof(DATA*) * (d->value_size - i));
	}
	((DATA**)d->value)[i] = FNC(data_create)();
	d->value_length++;
	return ((DATA**)d->value)[i];
}
void FNC(key_drop_child)(KEY key, KEY* drop)
{
	DATA* d = key;
	DATA* dDrop = *drop;
	unsigned int i, j = 0;

	for (i = 0; i < d->value_size; i++)
	{
		if (dDrop == ((DATA**)d->value)[i])
		{
			j = i;
		}
		if (((DATA**)d->value)[i] == NULL)
			break;
	}
	if (i >= d->value_size)
		return;
	((DATA**)d->value)[j] = ((DATA**)d->value)[i];
	FNC(data_destroy)((DATA**)drop);
	d->value_length--;
}


const PAIR FNC(key_get_arg)(KEY key)
{
	PAIR p;
	DATA* d = key;
	p.name = d->name;
	p.name_size = d->name_size;
	p.value = d->value;
	p.value_size = d->value_size;
	return p;
}
void FNC(key_set_arg)(KEY key, const char* value)
{
	DATA* d;
	if (key == NULL)
		return;
	d = key;
	if (d->type != DATATYPE_ARG)
		FNC(key_to_type)(key, DATATYPE_ARG);
	if (d->value == NULL && value != NULL)
	{
		d->value_size = strlen(value) + 1;
		d->value = malloc(sizeof(char) * d->value_size);
		strcpy(d->value, value);
		((char*)d->value)[d->value_size - 1] = '\0';
	}
	else
	{
		d->value_size = strlen(value) + 1;
		d->value = realloc(d->value, sizeof(char) * d->value_size);
		strcpy(d->value, value);
		((char*)d->value)[d->value_size - 1] = '\0';
	}
}
void FNC(key_set_name)(KEY key, const char* value)
{
	DATA* d;
	if (key == NULL)
		return;
	d = key;
	if (d->name == NULL && value != NULL)
	{
		d->name_size = strlen(value) + 1;
		d->name = malloc(sizeof(char) * d->name_size);
		strcpy(d->name, value);
		((char*)d->name)[d->name_size - 1] = '\0';
	}
	else
	{
		d->name_size = strlen(value) + 1;
		d->name = realloc(d->name, sizeof(char) * d->name_size);
		strcpy(d->name, value);
		((char*)d->name)[d->name_size - 1] = '\0';
	}
}
const char* FNC(key_get_name)(KEY key)
{
	if (key == NULL)
		return NULL;
	return ((DATA*)key)->name;
}