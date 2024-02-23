/**
** @file json.h
** @author Petr Horáček
** @brief 
*/

#ifndef _JSON_H_
#define _JSON_H_

#include <stdbool.h>
#include <stdio.h>


typedef struct Json {
    enum JsonType {
        JsonInteger
        , JsonFrac
        , JsonBool
        , JsonNull
        , JsonString
        , JsonArray
        , JsonObject
    }type;

    union {
        char * string;
        struct {size_t size; struct Json ** value;} array;
        struct {size_t size; struct JsonObjectPair {char * name; size_t key; struct Json * value;} * pair;} object;
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

#define json_is_type(T, json_type)\
    ((T) != NULL && (T)->type == json_type)


Json * json_string_new(char * string);


Json * json_bool_new(bool value);


Json * json_integer_new(long value);


Json * json_frac_new(double value);


Json * json_null_new(void);


Json * json_array_new(size_t size);


bool json_array_append(Json * self, Json * value);


Json * json_object_new(size_t size);


bool json_object_append(Json * self, char * name, Json * value);


bool json_object_set_record(Json * self, size_t index, char * name, Json * value);


bool json_object_set(Json * self, char * name, Json * value);


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
