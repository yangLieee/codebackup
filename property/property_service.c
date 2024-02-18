#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <shell.h>
#include "property_service.h"
#include "system_properties.h"

/* define load properties files path */
#define PROP_PATH_SYSTEM_BUILD     "/data/build.prop"
#define PROP_PATH_OTA_CONFIG       "/data/ota_res/recovery.conf"
#define PROP_PATH_SYSTEM_DEFAULT   "/usr/data/default.prop"
#define PROP_PATH_VENDOR_BUILD     "/usr/vendor/build.prop"
#define PROP_PATH_LOCAL_OVERRIDE   "/usr/data/local.prop"
#define PROP_PATH_FACTORY          "/factory/factory.prop"

static struct mutex rmutex, wmutex;
static int read_count= 0;

static bool is_legal_property_name(const char* name, size_t namelen)
{
    size_t i;
    if (namelen >= PROP_KEY_MAX) return false;
    if (namelen < 1) return false;
    if (name[0] == '.') return false;
    if (name[namelen - 1] == '.') return false;

    /* Only allow alphanumeric, plus '.', '-', or '_' */
    /* Don't allow ".." to appear in a property name */
    for (i = 0; i < namelen; i++) {
        if (name[i] == '.') {
            // i=0 is guaranteed to never have a dot. See above.
            if (name[i-1] == '.') return false;
            continue;
        }
        if (name[i] == '_' || name[i] == '-') continue;
        if (name[i] >= 'a' && name[i] <= 'z') continue;
        if (name[i] >= 'A' && name[i] <= 'Z') continue;
        if (name[i] >= '0' && name[i] <= '9') continue;
        return false;
    }
    return true;
}

static void load_properties_from_file(const char *, const char *);
/*
 * Filter is used to decide which properties to load: NULL loads all keys,
 * "ro.foo.*" is a prefix match, and "ro.foo.bar" is an exact match.
 */
static void load_properties(char *data, const char *filter)
{
    char *key, *value, *eol, *sol, *tmp, *fn;
    size_t flen = 0;

    if (filter) {
        flen = strlen(filter);
    }

    sol = data;
    while ((eol = strchr(sol, '\n'))) {
        key = sol;
        *eol++ = 0;
        sol = eol;

        while (isspace(*key)) key++;
        if (*key == '#') continue;

        tmp = eol - 2;
        while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

        if (!strncmp(key, "import ", 7) && flen == 0) {
            fn = key + 7;
            while (isspace(*fn)) fn++;

            key = strchr(fn, ' ');
            if (key) {
                *key++ = 0;
                while (isspace(*key)) key++;
            }

            load_properties_from_file(fn, key);

        } else {
            value = strchr(key, '=');
            if (!value) continue;
            *value++ = 0;

            tmp = value - 2;
            while ((tmp > key) && isspace(*tmp)) *tmp-- = 0;

            while (isspace(*value)) value++;

            if (flen > 0) {
                if (filter[flen - 1] == '*') {
                    if (strncmp(key, filter, flen - 1)) continue;
                } else {
                    if (strcmp(key, filter)) continue;
                }
            }
            if(!is_legal_property_name(key, strlen(key))) continue;
            __system_property_add(key, value);
        }
    }
}


static void load_properties_from_file(const char *fn, const char *filter)
{
    char *data = NULL;
    unsigned sz = 0;
    data = read_file(fn, &sz);

    if(data != NULL) {
        load_properties(data, filter);
        free(data);
    }
}

void load_all_props(void)
{
    mutex_init(&rmutex);
    mutex_init(&wmutex);
    mutex_lock(&rmutex);
    mutex_lock(&wmutex);
    __prop_memory_init();
    load_properties_from_file(PROP_PATH_SYSTEM_BUILD, NULL);
    load_properties_from_file(PROP_PATH_OTA_CONFIG, NULL);
    mutex_unlock(&rmutex);
    mutex_unlock(&wmutex);
}

void release_all_props(void)
{
    mutex_lock(&rmutex);
    mutex_lock(&wmutex);
    __prop_memory_destroy();
    mutex_unlock(&rmutex);
    mutex_unlock(&wmutex);
}

void get_property(const char *key, char *value, const char* default_value)
{
    mutex_lock(&rmutex);
    if(read_count == 0)
        mutex_lock(&wmutex);
    read_count++;
    mutex_unlock(&rmutex);
    if(*key == 0)
        __system_property_list();
    else {
        __system_property_get(key, value);
        if((!strcmp(value, "")) && default_value != NULL) {
            int len = strlen(default_value);
            if(len >= PROP_VALUE_MAX)
               len = PROP_VALUE_MAX - 1;
            memcpy(value, default_value, len);
            value[len] = '\0';
        }
        printf("%s\n", value);
    }
    mutex_lock(&rmutex);
    read_count--;
    if(read_count == 0)
        mutex_unlock(&wmutex);
    mutex_unlock(&rmutex);
}

int set_property(const char *key, const char *value)
{
    mutex_lock(&wmutex);
    if(!is_legal_property_name(key, strlen(key))) return -1;
    __system_property_set(key, value, PROP_PATH_SYSTEM_BUILD);
    mutex_unlock(&wmutex);
    return 0;
}
