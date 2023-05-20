/*
** @file json.c
** @author Petr Horáček
*/

#include "json.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#ifndef strndup
char *
strndup(const char * str, size_t n)
{
    char * copy = malloc(sizeof(char) * (n + 1));
    size_t i;
    
    for(i = 0; i < n && str[i] != 0; i++)
        copy[i] = str[i];

    copy[i] = '\0';

    return copy;
}
#endif


size_t 
__hash__(char * str)
{
    if(str == NULL)
        return 0;

    size_t hash = 0;

    while(*str != 0)
    {
        hash += *str;
        str++;
    }

    return hash;
}


typedef struct
{
    size_t index;
    uint32_t row;
    uint32_t column;
    char character;
}LexerState;


typedef struct
{
    char * code;
    LexerState state;
}Lexer;


#define Lexer(code) \
    (Lexer) {code, (LexerState){0, 1, 1, *code}}


static inline LexerState
__store__(Lexer * self)
{
    return self->state;
}


static inline void
__restore__(Lexer * self, LexerState state)
{
    self->state = state;
}


static inline int
__char__(Lexer * self)
{
    return self->state.character;
}


static int
__peek__(Lexer * self)
{
    if(__char__(self) == '\0')
        return '\0';
    else
        return self->code[self->state.index + 1];
}


static void
__advance__(Lexer * self)
{
    if(__char__(self) == '\0')
        return;
    else if(__char__(self) == '\n')
    {
        self->state.row ++;
        self->state.column = 1;
    }
    else
        self->state.column ++;

    self->state.character = self->code[++self->state.index];
}


typedef enum
{
    TokenID_Keyword
    , TokenID_String
    , TokenID_Integer
    , TokenID_Decimal
    , TokenID_Exp
    , TokenID_Symbol
    , TokenID_EOF
}TokenID;


typedef struct
{
    TokenID id;
    size_t  length;
    char *  value;    
}Token;


#define Token(...) (Token){__VA_ARGS__}
 

typedef struct
{
    bool is_value;
    Token token;
}O_Token;


#define O_Token_Nothing (O_Token){.is_value=false}
#define O_Token_Value(T) (O_Token){.is_value=true, .token=T}


static inline char *
__ref__(Lexer * self)
{
    return &self->code[self->state.index];
}


static inline void
__skip_whitespace__(Lexer * self)
{
    while(isspace(__char__(self)))
        __advance__(self);
}


/*
** TODO: treat excape sequences
*/
static inline O_Token
__read_string__(Lexer * self)
{
    __advance__(self);

    Token token = Token(TokenID_String, 0, __ref__(self));

    while(__char__(self) != '"')
    {
        if(__char__(self) == '\\')
        {
            token.length++;
            __advance__(self);
        }

        token.length++;
        __advance__(self);
    }

    __advance__(self);

    return O_Token_Value(token);
}


static inline O_Token
__read_number__(Lexer * self)
{
    Token token = Token(TokenID_Integer, 0, __ref__(self));

    if(__char__(self) == '+' || __char__(self) == '-')
    {
        token.length ++;
        __advance__(self);
    }

    while(isdigit(__char__(self)) 
        || __char__(self) == '.' 
        || tolower(__char__(self)) == 'e')
    {
        if(__char__(self) == '.' )
        {
            if(token.id == TokenID_Decimal || token.id == TokenID_Exp)
                return O_Token_Nothing;
            else
                token.id = TokenID_Decimal;
        }
        if(tolower(__char__(self)) == 'e')
        {
            if(token.id == TokenID_Exp)
                return O_Token_Nothing;
            else
                token.id = TokenID_Exp;
        }

        token.length++;
        __advance__(self);
    }   

    return O_Token_Value(token);
}


static inline O_Token
__read_symbol__(Lexer * self)
{
    Token token = Token(TokenID_Symbol, 1, __ref__(self));
    __advance__(self);

    return O_Token_Value(token);
}


static inline O_Token
__read_keyword__(Lexer * self)
{
    Token token = Token(TokenID_Keyword, 0, __ref__(self));

    while(__char__(self) != 0
            && isspace(__char__(self)) == false
            && __char__(self) != '{' 
            && __char__(self) != '}' 
            && __char__(self) != '[' 
            && __char__(self) != ']' 
            && __char__(self) != ':'
            && __char__(self) != '"'
            && __char__(self) != '\''
            && __char__(self) != ',')
    {
        token.length ++;
        __advance__(self);
    }

    return O_Token_Value(token);
}


static O_Token
__next_token__(Lexer * self)
{
    while(__char__(self) != '\0')
    {
        if(isspace(__char__(self)))
        {
            __skip_whitespace__(self);
            continue;
        }
        else if(__char__(self) == '"')
            return __read_string__(self);
        else if(isdigit(__char__(self)))
            return __read_number__(self);
        else if(__char__(self) == '+' && isdigit(__peek__(self)))
            return __read_number__(self);
        else if(__char__(self) == '-' && isdigit(__peek__(self)))
            return __read_number__(self);
        else if(__char__(self) == '{')
            return __read_symbol__(self);
        else if(__char__(self) == '}')
            return __read_symbol__(self);
        else if(__char__(self) == '[')
            return __read_symbol__(self);
        else if(__char__(self) == ']')
            return __read_symbol__(self);
        else if(__char__(self) == ',')
            return __read_symbol__(self);
        else if(__char__(self) == ':')
            return __read_symbol__(self);
        else 
            return __read_keyword__(self);
    }

    return O_Token_Value(Token(TokenID_EOF, 0, NULL));
}


static inline Json *
json_new(enum JsonID id)
{
    Json * json = malloc(sizeof(Json));
    json->id    = id;

    return json;
}


#define O_Json_Nothing (O_Json){.is_value = false}
#define O_Json_Value(T)(O_Json){.is_value = true, .json = T}


static O_Json
__parse__(Lexer * lexer);


static O_Json
__parse_json_object__(Lexer * lexer)
{
    Json * json = json_new(JsonObject);
    json->object = NULL;
    
    for(size_t i = 0;; i++)
    {
        O_Token name = __next_token__(lexer);
                        
        if(name.is_value == false)
        {
            json_delete(json);
            return O_Json_Nothing;
        }
        if(name.token.id == TokenID_Symbol 
            && strncmp(name.token.value, "}", 1) == 0)
        {
            return O_Json_Value(json);
        }
        else if(name.token.id != TokenID_String)
            return O_Json_Nothing;
        
        O_Token colon = __next_token__(lexer);
           
        if(colon.is_value == false 
            || colon.token.id != TokenID_Symbol 
            || strncmp(colon.token.value, ":", colon.token.length) != 0)
        {
            json_delete(json);
            return O_Json_Nothing;
        }

        O_Json value = __parse__(lexer);

        if(value.is_value == true)
        {
            if(json->object == NULL)
                json->object = vector_new(sizeof(JsonPair), 1);
            else
                json->object = vector_resize(VECTOR(json->object), i+1);

            json->object[i].name  = strndup(name.token.value, name.token.length);
            json->object[i].key   = __hash__(json->object[i].name);
            json->object[i].value = value.json;
        }
        else
        {
            json_delete(json);
            return O_Json_Nothing;
        }
        
        O_Token delimiter = __next_token__(lexer);

        if(delimiter.is_value == false 
            || delimiter.token.id != TokenID_Symbol)
        {
            json_delete(json);
            return O_Json_Nothing;
        }

        if(strncmp(delimiter.token.value, ",", 1) == 0)
            continue;
        else if(strncmp(delimiter.token.value, "}", 1) == 0)
            break;
        else
        {
            json_delete(json);
            return O_Json_Nothing;
        }
    }

    return O_Json_Value(json);
}


static O_Json
__parse_json_array__(Lexer * lexer)
{
    Json * json = json_new(JsonArray);
    json->array = NULL;            

    for(size_t i = 0;; i++)
    {
        LexerState store = __store__(lexer);
        O_Json value = __parse__(lexer);

        if(value.is_value == true)
        {
            if(json->array == NULL)
                json->array = vector_new(sizeof(Json*), 1);
            else
                json->array = vector_resize(VECTOR(json->array), i+1);
            
            json->array[i] = value.json;
        }
        else if(value.is_value == false && i > 0)
            return O_Json_Nothing;
        else
            __restore__(lexer, store);

        O_Token delimiter = __next_token__(lexer);
        
        if(delimiter.is_value == false 
            || delimiter.token.id != TokenID_Symbol)
        {
            return O_Json_Nothing;
        }

        if(strncmp(delimiter.token.value, "]", 1) == 0)
            break;
        else if(strncmp(delimiter.token.value, ",", 1) == 0)
            continue;
        else
        {
            json_delete(json);
            return O_Json_Nothing;
        }
    }

    return O_Json_Value(json);
}


static inline O_Json
__parse_json_bool__(bool value)
{
    Json * json   = json_new(JsonBool);
    json->boolean = value;

    return O_Json_Value(json);
}


static O_Json
__parse__(Lexer * lexer)
{
    O_Token t = __next_token__(lexer);

    if(t.is_value == false)
        return O_Json_Nothing;
    else if(t.token.id == TokenID_EOF)
        return O_Json_Nothing;
    else if(t.token.id == TokenID_Symbol)
    {
        if(strncmp(t.token.value, "{", 1) == 0)
            return __parse_json_object__(lexer);
        else if(strncmp(t.token.value, "[", 1) == 0)
            return __parse_json_array__(lexer);
    }
    else if(t.token.id == TokenID_String)
    {
        Json * json  = json_new(JsonString);
        json->string = strndup(t.token.value, t.token.length);

        return O_Json_Value(json);
    }
    else if(t.token.id == TokenID_Integer)
    {
        Json * json   = json_new(JsonInteger);
        json->integer = atoi(t.token.value);

        return O_Json_Value(json);
    }
    else if(t.token.id == TokenID_Decimal || t.token.id == TokenID_Exp)
    {
        Json * json = json_new(JsonFrac);
        json->frac  = atof(t.token.value);

        return O_Json_Value(json);
    }
    else if(t.token.id == TokenID_Keyword)
    {
        if(strncmp(t.token.value, "true", t.token.length) == 0)
            return __parse_json_bool__(true);
        else if(strncmp(t.token.value, "false", t.token.length) == 0)
            return __parse_json_bool__(false);
        else if(strncmp(t.token.value, "null", t.token.length) == 0)
        {
            Json * json = json_new(JsonNull);
            return O_Json_Value(json);
        }
        else
            return O_Json_Nothing;
    }
            
    return O_Json_Nothing;
}


O_Json 
json_load_string(char * code)
{
    Lexer lexer = Lexer(code);
    return __parse__(&lexer);
}


O_Json
json_load_file(FILE * file)
{
    fseek(file, 0, SEEK_END);
    size_t lenght = ftell(file);
    fseek(file, 0, SEEK_SET); 

    char * code = malloc(sizeof(char) * lenght + 1);
    fread(code, lenght, 1, file);

    O_Json json = json_load_string(code);
    free(code);

    return json;
}


O_Json 
json_lookup(
    Json * self
    , char * key)
{
    if(self == NULL 
        || self->id != JsonObject 
        || self->object == NULL)
    {
        return O_Json_Nothing;
    }

    size_t hash = __hash__(key);

    for(size_t i = 0; i < VECTOR(self->object)->length; i++)
    {
        if(self->object[i].key == hash)
            return O_Json_Value(self->object[i].value);
    }
   
    return O_Json_Nothing;
}


void
json_show(
    Json * self
    , FILE * stream)
{
    if(stream == NULL)
        return;
    else if(self == NULL)
        fprintf(stream, "null");

    switch(self->id)
    {
        case JsonString:
            if(self->string != NULL)
                fprintf(stream, "%s", self->string);
            break;
        case JsonArray:
            fprintf(stream, "[");
            
            if(self->array != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->array)->length; i++)
                {
                   if(i == 0)
                        json_show(self->array[i], stream);
                   else
                   {
                        fprintf(stream, ", ");
                        json_show(self->array[i], stream);
                   }

                }
            }

            fprintf(stream, "]");
            break;
        case JsonObject:
            fprintf(stream, "{\n");

            if(self->object != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->object)->length; i++)
                {
                    if(i == 0)
                    {
                        fprintf(stream, "\t%s: ", self->object[i].name);
                        json_show(self->object[i].value, stream);
                    }
                    else
                    {
                        fprintf(stream, ",\n\t%s: ", self->object[i].name);
                        json_show(self->object[i].value, stream);
                    }
                }
            }
            else
                fprintf(stream, "null");

            fprintf(stream, "\n}");
            break;
        case JsonInteger:
            fprintf(stream, "%d", self->integer);
            break;
        case JsonBool:
            fprintf(stream, "%s", self->boolean ? "true" : "false");
            break;
        case JsonFrac:
            fprintf(stream, "%f", self->frac);
            break;
        case JsonNull:
            fprintf(stream, "null");
            break;
    }
}


void
json_delete(Json * self)
{
    if(self == NULL)
        return;

    switch(self->id)
    {
        case JsonString:
            if(self->string != NULL)
                free(self->string);
            break;
        case JsonArray:
            if(self->array != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->array)->length; i++)
                    json_delete(self->array[i]);
            }
            break;
        case JsonObject:
            if(self->object != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->object)->length; i++)
                {
                    free(self->object[i].name);
                    json_delete(self->object[i].value);
                }
            }
            break;
        case JsonInteger:
        case JsonBool:
        case JsonFrac:
        case JsonNull:
            break;
    }

    free(self);
}


