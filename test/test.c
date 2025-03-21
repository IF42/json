#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <alloc/arena.h>
#include <alloc/general.h>
#include "json/json.h"

#define LARGE_JSON_FILE "assets/large-file.json"
#define SMALL_JSON_FILE "assets/example.json"
#define JSON_FILE LARGE_JSON_FILE


#define MEMSIZE ((size_t) (1024*1024*112))


int main(void) {
    ArenaAlloc alloc = arena_alloc(MEMSIZE);    
    //GeneralAlloc alloc = general_alloc();

    FILE * f = fopen(JSON_FILE, "r");
    assert(f != NULL);

    fseek(f, 0, SEEK_END);
    size_t length = ftell(f);
    fseek(f, 0, SEEK_SET); 

    char * json_string = new(ALLOC(&alloc), sizeof(char) * length + 1);
    if(fread(json_string, 1, length, f) > 0) {
        Json * jsn = json_deserialize(ALLOC(&alloc), json_string);
        assert(jsn != NULL);
        //json_show(jsn, stdout);
        
        printf("%ld\n", json_array_size(JSON_ARRAY(jsn)));
        
        char * key = "actor";
        if(json_object_at(JSON_OBJECT(json_array_at(JSON_ARRAY(jsn), 0)), key) != NULL) {
            json_show(json_object_at(JSON_OBJECT(json_array_at(JSON_ARRAY(jsn), 0)), key), stdout);
        }
        
        //printf("%ld %ld %ld\n", MEMSIZE, alloc.capacity, alloc.size);

        //json_finalize(jsn);
    }

    fclose(f);
    finalize(ALLOC(&alloc));
    printf("Program exit..\n");

    return EXIT_SUCCESS;
}



