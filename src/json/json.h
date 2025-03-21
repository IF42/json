#ifndef _JSON_H_
#define _JSON_H_ 

#include <alloc/alloc.h>
#include <iterator/iterator.h>


typedef enum {
    JsonType_Null
    , JsonType_Bool
    , JsonType_Number
    , JsonType_String
    , JsonType_Array
    , JsonType_Object
} Json;


#define JSON(T) ((Json*) (T))


typedef struct {
    Json type;
    Alloc * alloc;
} Json_Null;

#define JSON_NULL(T) ((Json_Null*) (T))


Json * json_null_new(Alloc * alloc);


typedef struct {
    Json type;
    Alloc * alloc;
    size_t length;
    char * c_str;    
} Json_Value;


#define JSON_VALUE(T) ((Json_Value*) (T))


Json * json_bool_new(Alloc * alloc, size_t length, char * c_str);
Json * json_string_new(Alloc * alloc, size_t length, char * c_str);
Json * json_number_new(Alloc * alloc, size_t length, char * c_str);


typedef struct {
    Json type;
    Alloc * alloc;
    size_t capacity;
    size_t size;
    Json ** front;
} Json_Array;


#define JSON_ARRAY(T) ((Json_Array*) (T))


Json * json_array_new(Alloc * alloc);


Iterator json_array_iterator(Json_Array * self);


size_t json_array_size(Json_Array * self);


void json_array_push_front(Json_Array * self, Json * child);


void json_array_push_back(Json_Array * self, Json * child);


void json_array_delete_front(Json_Array * self);


void json_array_delete_back(Json_Array * self);


void json_array_delete_at(Json_Array * self, size_t index);


Json * json_array_at(Json_Array * self, size_t index);


typedef struct {
    size_t length;
    char * c_str;
}Json_Object_Key;


typedef struct Json_Object_Node {
    size_t ID;
    Json_Object_Key key;
    Json * child;
} Json_Object_Node;


typedef struct {
    Json type;
    Alloc * alloc;
    size_t capacity;
    size_t size;
    Json_Object_Node * front;
} Json_Object;


#define JSON_OBJECT(T) ((Json_Object*) (T))


Json * json_object_new(Alloc * alloc);


Iterator json_object_iterator(Json_Object * self);


void json_object_push_front(Json_Object * self, Json_Object_Key key, Json * child);


void json_object_push_back(Json_Object * self, Json_Object_Key key, Json * child);


void json_object_delete_front(Json_Object * self);


void json_object_delete_back(Json_Object * self);


void json_object_delete_at(Json_Object * self, char * key);


Json * json_object_at(Json_Object * self, char * key);


Json * json_deserialize(Alloc * alloc, char * c_str);


#include <stdio.h>


void json_show(Json * self, FILE * stream);


void json_finalize(Json * self);


#endif



