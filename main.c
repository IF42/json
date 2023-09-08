#include <stdio.h>
#include <stdlib.h>
#include <cjson/cJSON.h>

#define LARGE_JSON_FILE "large-file.json"
#define SMALL_JSON_FILE "example.json"

#define JSON_FILE LARGE_JSON_FILE


int
main(void)
{
    FILE * f = fopen(JSON_FILE, "r");

    if(f == NULL)
    {
        fprintf(stderr, "Can't open file: '%s'\n", JSON_FILE);
        return EXIT_FAILURE;
    }

    fseek(f, 0, SEEK_END);
    size_t lenght = ftell(f);
    fseek(f, 0, SEEK_SET); 

    char * code = malloc(sizeof(char) * lenght + 1);
    fread(code, lenght, 1, f);

    cJSON * json = cJSON_ParseWithLength(code, lenght);

    if(json != NULL)
    {
        cJSON * array = cJSON_GetArrayItem (json, 0);

        if(array != NULL)
        {
            char *string = cJSON_PrintUnformatted(array);

            if(string != NULL)
            {
                printf("%s\n", string);
                free(string);
            }

        }

        cJSON_Delete(json);
    }

    free(code);
    fclose(f);

    printf("Program exit..\n");
    return EXIT_SUCCESS;
}
