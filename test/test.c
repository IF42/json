#include <stdio.h>
#include <stdlib.h>

#include "../src/json.h"


int
main(void)
{
    FILE * f = fopen("example.json", "r");

    if(f == NULL)
    {
        fprintf(stderr, "Can't open file '../example.json'\n");
        return EXIT_FAILURE;
    }
    

    fseek(f, 0, SEEK_END);
    size_t lenght = ftell(f);
    fseek(f, 0, SEEK_SET); 

    char * code = malloc(sizeof(char) * lenght + 1);
    fread(code, lenght, 1, f);
    fclose(f);

    json_show_tokens(code);

    

    return EXIT_SUCCESS;
}
