#include <stdio.h>
#include <stdlib.h>

#include "../src/json.h"


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
    
    Json * j = json_load(f);
    fclose(f);

    if(j != NULL)
    {
        json_show(j->array[0] , stdout);
        json_delete(j);
    }
    else
        printf("error\n");

    printf("Program exit..\n");

    return EXIT_SUCCESS;
}
