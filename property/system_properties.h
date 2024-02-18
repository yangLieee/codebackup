#ifndef __SYSTEM_PROTERTIES_H__
#define __SYSTEM_PROTERTIES_H__

#define PROP_KEY_MAX  32
#define PROP_VALUE_MAX 92

void *read_file(const char *fn, unsigned *_sz);
int __system_property_list(void);
int __system_property_add(const char *key, const char *value);
int __system_property_get(const char *key, char *value);
int __system_property_set(const char *key, const char *value, const char *filepath);
void __system_property_release(void);
void __prop_memory_init(void);
void __prop_memory_destroy(void);

#endif /* __SYSTEM_PROTERTIES_H__ */

