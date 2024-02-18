#include <stdio.h>
#include <string.h>
#include <shell.h>
#include "property_service.h"

static char prop_key[PROP_KEY_MAX] = {0};
static char prop_value[PROP_VALUE_MAX] = {0};

static void cmd_func_property(struct cmd_arg *arg, int argc, char **argv)
{
    if(argc<2 || (!strcmp(argv[1], "--help"))) {
        printf("property get <prop_key>\n");
        printf("property set <prop_key> <prop_value>\n");
    }
    else {
        if(!strcmp(argv[1], "get")) {
            if(argc == 2)
                memset(prop_key, 0, PROP_KEY_MAX);
            else
                memcpy(prop_key, argv[2], PROP_KEY_MAX);
            get_property(prop_key, prop_value, NULL);
        }
        else if(!strcmp(argv[1], "set")) {
            if(argc < 4)
                printf("Usage : property set <prop_key> <prop_value>\n");
            else {
                memcpy(prop_key, argv[2], PROP_KEY_MAX);
                memcpy(prop_value, argv[3], PROP_VALUE_MAX);
            }
            set_property(prop_key, prop_value);
        }
        else if(!strcmp(argv[1], "ctl.start")) {
            if(argc < 3)
                printf("Usage : property ctl.start [section] \n");
            else {
                /* to do */
            }
        }
        else if(!strcmp(argv[1], "ctl.stop")) {
            if(argc < 3)
                printf("Usage : property ctl.stop [section] \n");
            else {
                /* to do */
            }
        }
        else
            printf("Unsupport \'%s\' control command! Please again!\n", argv[1]);
    }
}


void init_property_service(void)
{
    load_all_props();
    shell_cmd_register(cmd_func_property, "property", NULL, "system property ctrl");
}
