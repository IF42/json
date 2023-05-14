/*
** @file json.c
** @author Petr Horáček
*/

#include "json.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#define JSON_NO_EXPORT static
#define JSON_INLINE_NO_EXPORT static inline

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


JSON_INLINE_NO_EXPORT LexerState
__store__(Lexer * self)
{
    return self->state;
}


JSON_INLINE_NO_EXPORT void
__restore__(Lexer * self, LexerState state)
{
    self->state = state;
}


JSON_INLINE_NO_EXPORT char
__char__(Lexer * self)
{
    return self->state.character;
}

    
JSON_NO_EXPORT void
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


JSON_NO_EXPORT char
__peek__(Lexer * self)
{
    if(__char__(self) == '\0')
        return '\0';
    else
        return self->code[self->state.index + 1];
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


#define O_Token_Nothing (O_Token){false}
#define O_Token_Value(T) (O_Token){true, T}


JSON_INLINE_NO_EXPORT char *
__ref__(Lexer * self)
{
    return &self->code[self->state.index];
}


JSON_INLINE_NO_EXPORT void
__skip_whitespace__(Lexer * self)
{
    while(isspace(__char__(self)))
        __advance__(self);
}


/*
** TODO: treat excape sequences
*/
JSON_INLINE_NO_EXPORT O_Token
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


JSON_INLINE_NO_EXPORT O_Token
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


JSON_INLINE_NO_EXPORT O_Token
__read_symbol__(Lexer * self)
{
    Token token = Token(TokenID_Symbol, 1, __ref__(self));
    __advance__(self);

    return O_Token_Value(token);
}


JSON_INLINE_NO_EXPORT O_Token
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


JSON_NO_EXPORT O_Token
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

    return O_Token_Value(Token(TokenID_EOF, 0, '\0'));
}


JSON_INLINE_NO_EXPORT Json *
json_new(enum JsonID id)
{
    Json * json = malloc(sizeof(Json));
    json->id    = id;

    return json;
}


#define O_Json_Nothing (O_Json){.is_value = false}
#define O_Json_Value(T)(O_Json){.is_value = true, .json = T}


O_Json
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
        {
            Json * json = json_new(JsonObject);
            json->object = NULL;
            
            for(size_t i = 0;; i++)
            {
                O_Token name     = __next_token__(lexer);
                                
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

                    json->object[i].name = strndup(name.token.value, name.token.length);
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
        else if(strncmp(t.token.value, "[", 1) == 0)
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
        {
            Json * json   = json_new(JsonBool);
            json->boolean = true;

            return O_Json_Value(json);
        }
        else if(strncmp(t.token.value, "false", t.token.length) == 0)
        {
            Json * json   = json_new(JsonBool);
            json->boolean = false;        

            return O_Json_Value(json);
        }   
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


void
json_delete(Json * self)
{
    if(self == NULL)
        return;

    switch(self->id)
    {
        case JsonString:
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


