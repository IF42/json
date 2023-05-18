/**
** @file json.h
** @author Petr Horáček
** @brief 
*/

#ifndef _JSON_H_
#define _JSON_H_

#include <vector.h>
#include <stdbool.h>
#include <stdio.h>


typedef struct
{
    char * name; 
    size_t key;

    struct Json * value;
}JsonPair;


typedef struct Json
{
    enum JsonID
    {
        JsonInteger
        , JsonFrac
        , JsonBool
        , JsonNull
        , JsonString
        , JsonArray
        , JsonObject
    }id;

    union
    {
        int integer;
        double frac;
        bool boolean;
        char * string;
        Vector(struct Json*) * array;
        Vector(JsonPair) * object;
    };
}Json;


#define JSON_TYPE(T)                    \
    T == JsonInteger ? "JsonInteger":   \
    T == JsonFrac    ? "JsonFrac":      \
    T == JsonBool    ? "JsonBool":      \
    T == JsonNull    ? "JsonNull":      \
    T == JsonString  ? "JsonString":    \
    T == JsonArray   ? "JsonArray":     \
    T == JsonObject  ? "JsonObject":    \
                       "Unknonw"

typedef struct
{
    bool is_value;
    Json * json;
}O_Json;


O_Json 
json_load_string(char * code);


O_Json 
json_load_file(FILE * file);


#define json_load(T)                \
    _Generic((T)                    \
        , char*: json_load_string   \
        , FILE*: json_load_file)    \
            (T)


O_Json 
json_lookup(
    Json * self
    , char * key);


void
json_show(
    Json * self
    , FILE * stream);


void
json_delete(Json * self);


#endif 
