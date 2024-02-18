#ifndef __PROPERTY_SERVICE_H__
#define __PROPERTY_SERVICE_H__
#ifdef __cplusplus
extern "C" {
#endif

#define PROP_KEY_MAX   32
#define PROP_VALUE_MAX  92

//void load_all_props(void);
void load_all_props(void);
void release_all_props(void);
void get_property(const char *key, char *value, const char *default_value);
int set_property(const char *key, const char *value);

#ifdef __cplusplus
}
#endif

#endif /* __PROPERTY_SERVICE_H__ */
