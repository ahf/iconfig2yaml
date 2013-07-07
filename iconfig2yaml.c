/* vim: set sw=4 sts=4 et foldmethod=syntax : */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <yaml.h>

#include "iconfig.h"

void display_emitter_error(yaml_emitter_t *emitter)
{
    switch (emitter->error)
    {
        case YAML_MEMORY_ERROR:
            fprintf(stderr, "Error: Memory error: Not enough memory for emitting\n");
            break;

        case YAML_WRITER_ERROR:
            fprintf(stderr, "Error: Writer error: %s\n", emitter->problem);
            break;

        case YAML_EMITTER_ERROR:
            fprintf(stderr, "Error: Emitter error: %s\n", emitter->problem);
            break;

        default:
            fprintf(stderr, "Error: Internal error\n");
            assert(false);
            break;
    }
}

void logprintf(bool verbose, int indentation_level, const char *format, ...)
{
    if (! verbose)
        return;

    int i;
    for (i = 0; i != indentation_level * 4; ++i)
        printf(" ");

    va_list arguments;
    va_start(arguments, format);
    vprintf(format, arguments);
    va_end(arguments);
}

void dump(bool verbose, int indentation_level, CONFIG_NODE *node)
{
    assert(node != NULL);

    switch (node->type)
    {
        case NODE_TYPE_KEY:
        {
            logprintf(verbose, indentation_level, "Key: '%s' => '%s'\n", node->key, (char *)node->value);
            break;
        }
        case NODE_TYPE_VALUE:
        {
            logprintf(verbose, indentation_level, "Value: %s (%p)\n", (char *)node->value, node->value);
            break;
        }
        case NODE_TYPE_BLOCK:
        {
            logprintf(verbose, indentation_level, "Block: %s\n", node->key);

            GSList *list = node->value;
            GSList *current = NULL;

            for (current = list; current != NULL; current = current->next)
                dump(verbose, indentation_level + 1, current->data);

            break;
        }
        case NODE_TYPE_LIST:
        {
            logprintf(verbose, indentation_level, "List: %s\n", node->key);

            GSList *list = node->value;
            GSList *current = NULL;

            for (current = list; current != NULL; current = current->next)
                dump(verbose, indentation_level + 1, current->data);

            break;
        }
        case NODE_TYPE_COMMENT:
        {
            logprintf(verbose, indentation_level, "Comment: %s\n", (char *)node->value);
            break;
        }
    }
}

void serialize(bool verbose, int indentation_level, yaml_emitter_t *emitter, CONFIG_NODE *node)
{
    assert(node != NULL);

    yaml_event_t event;
    memset(&event, 0, sizeof(event));

    switch (node->type)
    {
        case NODE_TYPE_KEY:
        {
            char *key = node->key;
            char *value = node->value;

            logprintf(verbose, indentation_level, "Key: %s -> %s\n", key, value);

            logprintf(verbose, indentation_level, "Scalar: %s\n", key);
            yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t *)key, strlen(key), true, true, YAML_PLAIN_SCALAR_STYLE);
            if (! yaml_emitter_emit(emitter, &event))
            {
                display_emitter_error(emitter);
                return;
            }

            logprintf(verbose, indentation_level, "Scalar: %s\n", value);
            yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t *)value, strlen(value), true, true, YAML_PLAIN_SCALAR_STYLE);
            if (! yaml_emitter_emit(emitter, &event))
            {
                display_emitter_error(emitter);
                return;
            }

            break;
        }
        case NODE_TYPE_VALUE:
        {
            char *value = node->value;

            logprintf(verbose, indentation_level, "Value: %s\n", value);

            logprintf(verbose, indentation_level, "Scalar: %s\n", value);
            yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t *)value, strlen(value), true, true, YAML_PLAIN_SCALAR_STYLE);
            if (! yaml_emitter_emit(emitter, &event))
            {
                display_emitter_error(emitter);
                return;
            }

            break;
        }
        case NODE_TYPE_BLOCK:
        {
            if (node->key == NULL)
            {
                // Anonymous block.
                logprintf(verbose, indentation_level, "Begin Anonymous Block\n");

                logprintf(verbose, indentation_level, "Start Mapping\n");
                yaml_mapping_start_event_initialize(&event, NULL, NULL, 0, YAML_BLOCK_MAPPING_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                GSList *list = node->value;
                GSList *current = NULL;

                for (current = list; current != NULL; current = current->next)
                    serialize(verbose, indentation_level + 1, emitter, current->data);

                logprintf(verbose, indentation_level, "End Mapping\n");
                yaml_mapping_end_event_initialize(&event);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }
            }
            else
            {
                // Nonymous block.
                char *key = node->key;

                logprintf(verbose, indentation_level, "Begin Nonymous Block: %s\n", key);

                logprintf(verbose, indentation_level, "Scalar: %s\n", key);
                yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t *)key, strlen(key), true, true, YAML_PLAIN_SCALAR_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                logprintf(verbose, indentation_level, "Start Mapping\n");
                yaml_mapping_start_event_initialize(&event, NULL, NULL, 0, YAML_BLOCK_MAPPING_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                GSList *list = node->value;
                GSList *current = NULL;

                for (current = list; current != NULL; current = current->next)
                    serialize(verbose, indentation_level + 1, emitter, current->data);

                logprintf(verbose, indentation_level, "End Mapping\n");
                yaml_mapping_end_event_initialize(&event);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }
            }

            break;
        }
        case NODE_TYPE_LIST:
        {
            GSList *list = node->value;
            GSList *current = NULL;

            if (node->key == NULL)
            {
                // Anonymous list.
                logprintf(verbose, indentation_level, "Being Anonymous List\n");

                logprintf(verbose, indentation_level, "Start Sequence\n");
                yaml_sequence_start_event_initialize(&event, NULL, (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG, 1, YAML_BLOCK_SEQUENCE_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                for (current = list; current != NULL; current = current->next)
                    serialize(verbose, indentation_level + 1, emitter, current->data);

                logprintf(verbose, indentation_level, "End Sequence\n");
                yaml_sequence_end_event_initialize(&event);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }
            }
            else
            {
                logprintf(verbose, indentation_level, "Begin Nonymous List\n");

                // Nonymous list.
                char *key = node->key;

                logprintf(verbose, indentation_level, "Scalar: %s\n", key);
                yaml_scalar_event_initialize(&event, NULL, NULL, (yaml_char_t *)key, strlen(key), true, true, YAML_PLAIN_SCALAR_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                logprintf(verbose, indentation_level, "Start Sequence\n");
                yaml_sequence_start_event_initialize(&event, NULL, (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG, 1, YAML_BLOCK_SEQUENCE_STYLE);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }

                for (current = list; current != NULL; current = current->next)
                    serialize(verbose, indentation_level + 1, emitter, current->data);

                logprintf(verbose, indentation_level, "End Sequence\n");
                yaml_sequence_end_event_initialize(&event);
                if (! yaml_emitter_emit(emitter, &event))
                {
                    display_emitter_error(emitter);
                    return;
                }
            }

            break;
        }

        case NODE_TYPE_COMMENT:
            // FIXME: Ignore comments for now.
            logprintf(verbose, indentation_level, "Comment: %s\n", (char *)node->value);
            break;
    }
}

void display_help(const char *program)
{
    fprintf(stderr, "%s [-vdcu] [-i file]\n\n", program);
    fprintf(stderr, "   -i <file> - Specify input configuration file.\n");
    fprintf(stderr, "   -c        - Dump data in YAML's canonical form.\n");
    fprintf(stderr, "   -u        - Dump data in the Unicode format.\n");
    fprintf(stderr, "   -d        - Dump data in internal debugging format.\n");
    fprintf(stderr, "   -v        - Enables verbose logging.\n");
}

int main(int argc, char *argv[])
{
    char *input_file = NULL;
    bool canonical = false;
    bool unicode = false;
    bool dump_config = false;
    bool verbose = false;

    // Irssi's iconfig data structure.
    CONFIG_REC *config = NULL;

    int c;

    while ((c = getopt(argc, argv, "hvdcui:")) != -1)
    {
        switch (c)
        {
            case 'i':
                input_file = optarg;
                break;
            case 'c':
                canonical = true;
                break;
            case 'u':
                unicode = true;
                break;
            case 'd':
                dump_config = true;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                display_help(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (input_file == NULL)
    {
        fprintf(stderr, "Error: Input file missing\n");
        display_help(argv[0]);
        return EXIT_FAILURE;
    }

    // Prepare the YAML emitter.
    yaml_emitter_t emitter;
    yaml_event_t event;

    memset(&emitter, 0, sizeof(emitter));
    memset(&event, 0, sizeof(event));

    assert(yaml_emitter_initialize(&emitter));

    // Print to stdout.
    yaml_emitter_set_output_file(&emitter, stdout);

    yaml_emitter_set_canonical(&emitter, canonical);
    yaml_emitter_set_unicode(&emitter, unicode);

    // Create mode -1 means that we wont create the configuration file in case
    // it doesn't exist already.
    config = config_open(input_file, -1);

    if (config == NULL)
    {
        fprintf(stderr, "Error: Unable to read configuration file: %s\n", input_file);
        return EXIT_FAILURE;
    }

    if (config_parse(config))
    {
        fprintf(stderr, "Error: Unable to parse configuration file\n");
        return EXIT_FAILURE;
    }

    if (dump_config)
    {
        printf("Irssi Configuration Dump\n");
        printf("========================\n");
        dump(verbose, /* indentation_level */ 0, config->mainnode);
        printf("========================\n");
    }

    logprintf(verbose, /* indentation_level */ 0, "Start Event\n");
    yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
    if (! yaml_emitter_emit(&emitter, &event))
    {
        display_emitter_error(&emitter);
        return EXIT_FAILURE;
    }

    logprintf(verbose, /* indentation_level */ 0, "Start Document\n");
    yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 0);
    if (! yaml_emitter_emit(&emitter, &event))
    {
        display_emitter_error(&emitter);
        return EXIT_FAILURE;
    }

    // Recursively serialize the configuration structure.
    serialize(verbose, /* indentation_level */ 0, &emitter, config->mainnode);

    logprintf(verbose, /* indentation_level */ 0, "End Document\n");
    yaml_document_end_event_initialize(&event, 1);
    if (! yaml_emitter_emit(&emitter, &event))
    {
        display_emitter_error(&emitter);
        return EXIT_FAILURE;
    }

    logprintf(verbose, /* indentation_level */ 0, "End Event Stream\n");
    yaml_stream_end_event_initialize(&event);
    if (! yaml_emitter_emit(&emitter, &event))
    {
        display_emitter_error(&emitter);
        return EXIT_FAILURE;
    }

    yaml_emitter_delete(&emitter);

    return EXIT_SUCCESS;
}
