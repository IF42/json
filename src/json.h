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


typedef struct Json Json;

typedef struct 
{
    char * name; 
    size_t key;

    Json * value;
}JsonPair;


struct Json
{
    enum JsonType
    {
        JsonInteger
        , JsonFrac
        , JsonBool
        , JsonNull
        , JsonString
        , JsonArray
        , JsonObject
    }type;

    union
    {
        int integer;
        double frac;
        bool boolean;
        char * string;
        Vector(Json *) * array;
        Vector(JsonPair) * object;
    };
};


#define JSON_TYPE(T)                    \
    T == JsonInteger ? "JsonInteger":   \
    T == JsonFrac    ? "JsonFrac":      \
    T == JsonBool    ? "JsonBool":      \
    T == JsonNull    ? "JsonNull":      \
    T == JsonString  ? "JsonString":    \
    T == JsonArray   ? "JsonArray":     \
    T == JsonObject  ? "JsonObject":    \
                       "Unknonw"

Json * 
json_parse_string(char * code);


Json * 
json_parse_file(FILE * file);


#define json_parse(T)                \
    _Generic((T)                     \
        , char*: json_parse_string   \
        , FILE*: json_parse_file)    \
            (T)


Json *
json_lookup(
    Json * self
    , char * key);


Json *
json_clone(Json * self);


void
json_show(
    Json * self
    , FILE * stream);


void
json_delete(Json * self);


#endif 
