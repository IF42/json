#include <stdio.h>
#include <stdlib.h>

#include "../src/json.h"


int
main(void)
{
    FILE * f = fopen("large-file.json", "r");

    if(f == NULL)
    {
        fprintf(stderr, "Can't open file '../example.json'\n");
        return EXIT_FAILURE;
    }
    
    O_Json j = json_load(f);
    fclose(f);

    if(j.is_value == true)
    {
        //json_show(j.json, stdout);

        
        printf("json type: %s\n", JSON_TYPE(j.json->id));
        printf("array length: %zu\n", VECTOR(j.json->array)->length);
        printf("json type of item at 0: %s\n", JSON_TYPE(j.json->array[0]->id));
        json_show(json_lookup(j.json->array[1], "actor").json, stdout);
        
        json_delete(j.json);
    }
    else
        printf("error\n");

    /*
    if(j.is_value == true 
        && j.json != NULL)
    {
        printf("%s: %s, %s: ",j.json->object[0].name, j.json->object[0].value->string, j.json->object[1].name);

        for(size_t i = 0; i < VECTOR(j.json->object[1].value->array)->length; i++)
            printf(i == 0 ? "%d" : ", %d", j.json->object[1].value->array[i]->integer);

        json_delete(j.json);
    }
    else
        printf("(NULL)\n");
    */


    return EXIT_SUCCESS;
}
