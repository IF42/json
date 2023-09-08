/*
** @file json.c
** @author Petr Horáček
*/

#include "json.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#define VECTOR_INIT_SIZE 10


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


#define __char__(self)\
    ((int)self->state.character)


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
    , TokenID_Undefined
    , TokenID_EOF
}TokenID;


typedef struct
{
    TokenID id;
    size_t  length;
    char *  value;    
}Token;


#define Token(...) (Token){__VA_ARGS__}
 

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
static inline Token
__read_string__(Lexer * self)
{
    __advance__(self);

    size_t start = self->state.index;
    Token token = Token(TokenID_String, 0, __ref__(self));

    while(__char__(self) != '"')
    {
        if(__char__(self) == '\\')
            __advance__(self);

        __advance__(self);
    }

    token.length = self->state.index - start;

    __advance__(self);

    return token;
}


static inline Token
__read_number__(Lexer * self)
{
    size_t start = self->state.index;
    Token token = Token(TokenID_Integer, 0, __ref__(self));

    if(__char__(self) == '+' || __char__(self) == '-')
        __advance__(self);

    while(isdigit(__char__(self)) 
        || __char__(self) == '.' 
        || tolower(__char__(self)) == 'e')
    {
        if(__char__(self) == '.' )
        {
            if(token.id == TokenID_Decimal || token.id == TokenID_Exp)
            {
                token.id = TokenID_Undefined;
                return token;
            }
            else
                token.id = TokenID_Decimal;
        }
        if(tolower(__char__(self)) == 'e')
        {
            if(token.id == TokenID_Exp)
            {
                token.id = TokenID_Undefined;
                return token;
            }
            else
                token.id = TokenID_Exp;
        }

        __advance__(self);
    }   

    token.length = self->state.index - start;

    return token;
}


static inline Token
__read_symbol__(Lexer * self)
{
    Token token = Token(TokenID_Symbol, 1, __ref__(self));
    __advance__(self);

    return token;
}

static inline bool
__elem__(
    char * array
    , char c)
{
    while(*array != 0)
    {
        if(*(++array) == c)
            return true;
    }

    return false;
}


static char symbol[] = "{}[]:\'\",";


static inline Token
__read_keyword__(Lexer * self)
{
    size_t start = self->state.index;
    Token token = Token(TokenID_Keyword, 0, __ref__(self));

    while(__char__(self) != 0
            && isspace(__char__(self)) == false
            && __elem__(symbol, __char__(self)) == false)
    {
        __advance__(self);
    }

    token.length = self->state.index - start;

    return token;
}


static Token
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

    return Token(TokenID_EOF, 0, NULL);
}


static inline Json *
json_new(enum JsonID id)
{
    Json * json = malloc(sizeof(Json));
    json->id    = id;

    return json;
}


static Json *
__parse__(Lexer * lexer);


static Json *
__parse_json_object__(Lexer * lexer)
{
    Json * json  = json_new(JsonObject);
    json->object = NULL;
    size_t i     = 0;
    
    while(true)
    {
        Token name = __next_token__(lexer);
                        
        if(name.id == TokenID_Symbol 
            && strncmp(name.value, "}", 1) == 0)
        {
            return json;
        }
        else if(name.id != TokenID_String)
        {
            json_delete(json);
            return NULL;
        }

        Token colon = __next_token__(lexer);
           
        if(colon.id != TokenID_Symbol 
            || strncmp(colon.value, ":", colon.length) != 0)
        {
            json_delete(json);
            return NULL;
        }

        Json * value = __parse__(lexer);

        if(value != NULL)
        {
            if(json->object == NULL)
                 json->object = vector(JsonPair, VECTOR_INIT_SIZE);
            else if(i >= VECTOR(json->object)->length)
                json->object = vector_resize(VECTOR(json->object), i*2);

            json->object[i].name  = strndup(name.value, name.length);
            json->object[i].key   = __hash__(json->object[i].name);
            json->object[i].value = value;
            i++;
        }
        else
        {
            json_delete(json);
            return NULL;
        }
        
        Token delimiter = __next_token__(lexer);

        if(delimiter.id != TokenID_Symbol)
        {
            json_delete(json);
            return NULL;
        }

        if(strncmp(delimiter.value, ",", 1) == 0)
            continue;
        else if(strncmp(delimiter.value, "}", 1) == 0)
            break;
        else
        {
            json_delete(json);
            return NULL;
        }
    }
	
    if(json->object != NULL)
        json->object = vector_resize(VECTOR(json->object), i);

    return json;
}


static Json *
__parse_json_array__(Lexer * lexer)
{
    Json * json = json_new(JsonArray);
    json->array = NULL;
    size_t i = 0;

    while(true)
    {
        LexerState store = lexer->state;
        Json * value = __parse__(lexer);

        if(value != NULL)
        {
            if(json->array == NULL)
                json->array = vector_new(sizeof(Json*), VECTOR_INIT_SIZE); 
            else if(i >= VECTOR(json->array)->length)
                json->array = vector_resize(VECTOR(json->array), i*2);
            
            json->array[i] = value;
            i++;
        }
        else if(value == NULL && i > 0)
            return NULL;
        else
            lexer->state = store;

        Token delimiter = __next_token__(lexer);
        
        if(delimiter.id != TokenID_Symbol)
            return NULL;

        if(strncmp(delimiter.value, "]", 1) == 0)
            break;
        else if(strncmp(delimiter.value, ",", 1) == 0)
            continue;
        else
        {
            json_delete(json);
            return NULL;
        }
    }

    if(json->array != NULL)
        json->array = vector_resize(VECTOR(json->array), i);

    return json;
}


static inline Json *
__parse_json_bool__(bool value)
{
    Json * json   = json_new(JsonBool);
    json->boolean = value;

    return json;
}


static Json *
__parse__(Lexer * lexer)
{
    Token token = __next_token__(lexer);

    if(token.id == TokenID_EOF)
        return NULL;
    else if(token.id == TokenID_Symbol)
    {
        if(strncmp(token.value, "{", 1) == 0)
            return __parse_json_object__(lexer);
        else if(strncmp(token.value, "[", 1) == 0)
            return __parse_json_array__(lexer);
    }
    else if(token.id == TokenID_String)
    {
        Json * json  = json_new(JsonString);
        json->string = strndup(token.value, token.length);

        return json;
    }
    else if(token.id == TokenID_Integer)
    {
        Json * json   = json_new(JsonInteger);
        json->integer = atoi(token.value);

        return json;
    }
    else if(token.id == TokenID_Decimal || token.id == TokenID_Exp)
    {
        Json * json = json_new(JsonFrac);
        json->frac  = atof(token.value);

        return json;
    }
    else if(token.id == TokenID_Keyword)
    {
        if(strncmp(token.value, "true", token.length) == 0)
            return __parse_json_bool__(true);
        else if(strncmp(token.value, "false", token.length) == 0)
            return __parse_json_bool__(false);
        else if(strncmp(token.value, "null", token.length) == 0)
        {
            Json * json = json_new(JsonNull);
            return json;
        }
        else
            return NULL;
    }
            
    return NULL;
}


Json *
json_load_string(char * code)
{
    Lexer lexer = Lexer(code);
    return __parse__(&lexer);
}


Json *
json_load_file(FILE * file)
{
    fseek(file, 0, SEEK_END);
    size_t lenght = ftell(file);
    fseek(file, 0, SEEK_SET); 

    char * code = malloc(sizeof(char) * lenght + 1);
    fread(code, lenght, 1, file);

    Json * json = json_load_string(code);
    free(code);

    return json;
}


Json *
json_lookup(
    Json * self
    , char * key)
{
    if(self == NULL 
        || self->id != JsonObject)
    {
        return NULL;
    }

    size_t hash = __hash__(key);

    for(size_t i = 0; i < VECTOR(self->object)->length; i ++)
    {
        if(self->object[i].key == hash)
            return self->object[i].value;
    }
   
    return NULL;
}


static void
__json_show__(
    Json * self
    , FILE * stream)
{
    if(self == NULL)
    {
        fprintf(stream, "null");
        return;
    }

    switch(self->id)
    {
        case JsonString:
            if(self->string != NULL)
                fprintf(stream, "\'%s\'", self->string);
            break;
        case JsonArray:
            fprintf(stream, "[");
            
            for(size_t i = 0; i < VECTOR(self->array)->length; i++)
            {
               if(i == 0)
                    __json_show__(self->array[i], stream);
               else
               {
                    fprintf(stream, ", ");
                    __json_show__(self->array[i], stream);
               }
            }

            fprintf(stream, "]");
            break;
        case JsonObject:
            fprintf(stream, "{");
            
            if(self->object != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->object)->length; i++)
                {
                    if(i == 0)
                    {
                        fprintf(stream, "\'%s\': ", self->object[i].name);
                        __json_show__(self->object[i].value, stream);
                    }
                    else
                    {
                        fprintf(stream, ", \'%s\': ", self->object[i].name);
                        __json_show__(self->object[i].value, stream);
                    }
                }
            }
            else
                fprintf(stream, "null");

            fprintf(stream, "}");
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
json_show(
    Json * self
    , FILE * stream)
{
    if(stream == NULL)
        return;

    __json_show__(self, stream);
    putchar('\n');
}


Json *
json_clone(Json * self)
{
    Json * json = NULL;

    switch(self->id)
    {
    case JsonNull:
        json = json_new(JsonNull);
        break;
    case JsonInteger:
        json          = json_new(JsonInteger);
        json->integer = self->integer; 
        break;
    case JsonBool:
        json = json_new(JsonBool);
        json->boolean = self->boolean;
        break;
    case JsonFrac:
        json = json_new(JsonFrac);
        json->frac = self->frac;
        break;
    case JsonString:
        json = json_new(JsonString);
        json->string = strdup(self->string);
        break;
    case JsonArray:
        break;
    case JsonObject:
        break;
    }
    
    return json;
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
                
                vector_delete(VECTOR(self->array));
            }
            break;
        case JsonObject:
			if(self->object != NULL)
            {
                for(size_t i = 0; i < VECTOR(self->object)->length; i++)
                {
                    if(self->object[i].name != NULL)
                        free(self->object[i].name);

                    json_delete(self->object[i].value);
                }

                vector_delete(VECTOR(self->object));
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


