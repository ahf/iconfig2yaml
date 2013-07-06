/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <yaml.h>

#include "iconfig.h"

void serialize(CONFIG_NODE *node)
{
    assert(node);

    switch (node->type)
    {
        case NODE_TYPE_KEY:
            printf("Key %s -> %s\n", node->key, (char *)node->value);
            break;

        case NODE_TYPE_VALUE:
            printf("Value: %s\n", node->key);
            break;

        case NODE_TYPE_BLOCK:
        {
            printf("Block: %s\n", node->key);

            GSList *list = node->value;
            GSList *current = NULL;

            for (current = list; current != NULL; current = current->next)
            {
                CONFIG_NODE *value = current->data;
                serialize(value);
            }

            break;
        }

        case NODE_TYPE_LIST:
        {
            printf("List\n");

            GSList *list = node->value;
            GSList *current = NULL;

            for (current = list; current != NULL; current = current->next)
            {
                CONFIG_NODE *value = current->data;
                serialize(value);
            }

            break;
        }

        case NODE_TYPE_COMMENT:
            printf("Comment: %s\n", (char *)node->value);
            break;
    }
}

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    char *output_file = "config.yaml";
    CONFIG_REC *config = NULL;

    int c;

    while ((c = getopt(argc, argv, "i:o:")) != -1)
    {
        switch (c)
        {
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            default:
                fprintf(stderr, "Error: Invalid argument");
                return EXIT_FAILURE;
        }
    }

    printf("Input file: %s\n", input_file);
    printf("Output file: %s\n", output_file);

    // Create mode -1 means that we wont create the configuration file in case
    // it doesn't exist.
    config = config_open(input_file, -1);
    config_parse(config);

    serialize(config->mainnode);

    return EXIT_SUCCESS;
}
