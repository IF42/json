/*
** @file json.c
** @author Petr Horáček
*/

#include "json.h"
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>


#define INIT_SIZE 10


size_t 
__hash(char * str) {
    if(str == NULL)
        return 0;

    size_t hash = 0;

    while(*str != 0) {
        hash += *str;
        str++;
    }

    return hash;
}


typedef struct {
    size_t index;
    uint32_t row;
    uint32_t column;
    char character;
}LexerState;


typedef struct {
    char * code;
    LexerState state;
}Lexer;


#define Lexer(code) \
    (Lexer) {code, (LexerState){0, 1, 1, *code}}


#define __char(self)\
    ((int)self->state.character)

/*
static char *
strndup(
    const char * str
    , size_t size)
{
    char * dup = malloc(sizeof(char) * size+1);

    memcpy(dup, str, size);
    dup[size] = '\0';

    return dup;
}
*/

static int
__peek(Lexer * self) {
    if(__char(self) == '\0')
        return '\0';
    else
        return self->code[self->state.index + 1];
}


static void
__advance(Lexer * self) {
    if(__char(self) == '\0')
        return;
    else if(__char(self) == '\n') {
        self->state.row ++;
        self->state.column = 1;
    }
    else
        self->state.column ++;

    self->state.character = self->code[++self->state.index];
}


typedef enum {
    TokenID_Keyword
    , TokenID_String
    , TokenID_Integer
    , TokenID_Decimal
    , TokenID_Exp
    , TokenID_Symbol
    , TokenID_Undefined
    , TokenID_EOF
}TokenID;


typedef struct {
    TokenID id;
    size_t  size;
    char *  value;    
}Token;


#define Token(...) (Token){__VA_ARGS__}
 

static inline char *
__ref(Lexer * self) {
    return &self->code[self->state.index];
}


static inline void
__skip_whitespace__(Lexer * self) {
    while(isspace(__char(self)))
        __advance(self);
}


/*
 * TODO: treat excape sequences
 */
static inline Token
__read_string(Lexer * self) {
    __advance(self);

    Token token = Token(TokenID_String, self->state.index, __ref(self));

    while(__char(self) != '"') {
        if(__char(self) == '\\')
            __advance(self);

        __advance(self);
    }

    token.size = self->state.index - token.size;

    __advance(self);

    return token;
}


static inline Token
__read_number(Lexer * self) {
    Token token = Token(TokenID_Integer, self->state.index, __ref(self));

    if(__char(self) == '+' || __char(self) == '-')
        __advance(self);

    while(isdigit(__char(self)) 
        || __char(self) == '.' 
        || tolower(__char(self)) == 'e') {
        if(__char(self) == '.' ) {
            if(token.id == TokenID_Decimal || token.id == TokenID_Exp) {
                token.id = TokenID_Undefined;
                return token;
            }
            else
                token.id = TokenID_Decimal;
        }
        if(tolower(__char(self)) == 'e') {
            if(token.id == TokenID_Exp) {
                token.id = TokenID_Undefined;
                return token;
            }
            else
                token.id = TokenID_Exp;
        }

        __advance(self);
    }   

    token.size = self->state.index - token.size;

    return token;
}


static inline Token
__read_symbol(Lexer * self) {
    Token token = Token(TokenID_Symbol, 1, __ref(self));
    __advance(self);

    return token;
}


static inline Token
__read_keyword(Lexer * self) {
    Token token = Token(TokenID_Keyword, self->state.index, __ref(self));

    while(isalnum(__char(self)))
        __advance(self);

    token.size = self->state.index - token.size;

    return token;
}


static Token
__next_token(Lexer * self) {
    while(__char(self) != '\0') {
        if(isspace(__char(self))) {
            __skip_whitespace__(self);
            continue;
        }
        else if(__char(self) == '"')
            return __read_string(self);
        else if(isdigit(__char(self))
                || ((__char(self) == '+' || __char(self) == '-') && isdigit(__peek(self))))
            return __read_number(self);
        else if(__char(self) == '{')
            return __read_symbol(self);
        else if(__char(self) == '}')
            return __read_symbol(self);
        else if(__char(self) == '[')
            return __read_symbol(self);
        else if(__char(self) == ']')
            return __read_symbol(self);
        else if(__char(self) == ',')
            return __read_symbol(self);
        else if(__char(self) == ':')
            return __read_symbol(self);
        else 
            return __read_keyword(self);
    }

    return Token(TokenID_EOF, 0, NULL);
}


static inline Json *
json_new(enum JsonType type) {
    Json * json = malloc(sizeof(Json));
    json->type    = type;

    return json;
}


static Json *
parse(Lexer * lexer);


static Json *
__parse_json_object(Lexer * lexer) {
    Json * json  = json_new(JsonObject);
    size_t i     = 0;
    
    json->object.size = 0;
    json->object.pair   = NULL;    

    while(true) {
        Token name = __next_token(lexer);
                        
        if(name.id == TokenID_Symbol 
            && strncmp(name.value, "}", 1) == 0) {
            return json;
        }
        else if(name.id != TokenID_String) {
            json_delete(json);
            return NULL;
        }

        Token colon = __next_token(lexer);
           
        if(colon.id != TokenID_Symbol 
            || strncmp(colon.value, ":", colon.size) != 0) {
            json_delete(json);
            return NULL;
        }

        Json * value = parse(lexer);

        if(value != NULL) {
            if(json->object.size == 0) {
                json->object.pair = malloc(sizeof(struct JsonObjectPair)*INIT_SIZE);
                json->object.size = INIT_SIZE;
            }
            else if(i >= json->object.size) {
                json->object.pair = realloc(json->object.pair, sizeof(struct JsonObjectPair)*i*2);
                json->object.size = i*2;
            }

            json->object.pair[i].name  = strndup(name.value, name.size);
            json->object.pair[i].key   = __hash(json->object.pair[i].name);
            json->object.pair[i].value = value;
            
            i++;
        }
        else {
            json_delete(json);
            return NULL;
        }
        
        Token delimiter = __next_token(lexer);

        if(strncmp(delimiter.value, ",", 1) == 0)
            continue;
        else if(strncmp(delimiter.value, "}", 1) == 0)
            break;
        else {
            json_delete(json);
            return NULL;
        }
    }
	
    if(json->object.pair != NULL) {
        json->object.pair   = realloc(json->object.pair, sizeof(struct JsonObjectPair) * i);
        json->object.size = i;
    }

    return json;
}


static Json *
__parse_json_array(Lexer * lexer) {
    Json * json = json_new(JsonArray);    
    size_t i = 0;
    
    json->array.value  = NULL;
    json->array.size = 0;

    while(true) {
        LexerState store = lexer->state;
        Json * value = parse(lexer);
        
        if(value != NULL) {
            if(json->array.size == 0) {
                json->array.value = malloc(sizeof(Json*) * INIT_SIZE);
                json->array.size = INIT_SIZE;
            }
            else if(i >= json->array.size) {
                json->array.value = realloc(json->array.value, sizeof(Json*)*i*2);
                json->array.size = i*2;
            }

            json->array.value[i] = value;
            i++;
        }
        else if(value == NULL && i > 0)
            return NULL;
        else
            lexer->state = store;
        
        Token delimiter = __next_token(lexer);
        
        if(strncmp(delimiter.value, "]", 1) == 0)
            break;
        else if(strncmp(delimiter.value, ",", 1) == 0)
            continue;
        else {
            json_delete(json);
            return NULL;
        }
    }

    if(json->array.value != NULL) {
        json->array.value  = realloc(json->array.value, sizeof(Json*) * i);
        json->array.size = i; 
    }

    return json;
}


static Json *
parse(Lexer * lexer)
{
    Token token = __next_token(lexer);

    if(token.id == TokenID_EOF)
        return NULL;
    else if(token.id == TokenID_Symbol) {
        if(strncmp(token.value, "{", 1) == 0)
            return __parse_json_object(lexer);
        else if(strncmp(token.value, "[", 1) == 0)
            return __parse_json_array(lexer);
    }
    else if(token.id == TokenID_String) {
        Json * json  = json_new(JsonString);
        json->string = strndup(token.value, token.size);

        return json;
    }
    else if(token.id == TokenID_Integer) {
        Json * json   = json_new(JsonInteger);
        json->string = strndup(token.value, token.size);

        return json;
    }
    else if(token.id == TokenID_Decimal || token.id == TokenID_Exp) {
        Json * json = json_new(JsonFrac);
        json->string = strndup(token.value, token.size);

        return json;
    }
    else if(token.id == TokenID_Keyword) {
        if(strncmp(token.value, "true", token.size) == 0
            || strncmp(token.value, "false", token.size) == 0) {
            Json * json = json_new(JsonBool);
            json->string = strndup(token.value, token.size);

            return json;
        }
        else if(strncmp(token.value, "null", token.size) == 0){
            Json * json = json_new(JsonNull);
            return json;
        }
        else
            return NULL;
    }
            
    return NULL;
}


Json *
json_parse_string(char * code) {
    Lexer lexer = Lexer(code);
    return parse(&lexer);
}


Json *
json_parse_file(FILE * file) {
    fseek(file, 0, SEEK_END);
    size_t lenght = ftell(file);
    fseek(file, 0, SEEK_SET); 

    char * code = malloc(sizeof(char) * lenght + 1);
    fread(code, lenght, 1, file);

    Json * json = json_parse_string(code);
    free(code);

    return json;
}


Json *
json_lookup(
    Json * self
    , char * key) {
    if(self == NULL 
        || self->type != JsonObject) {
        return NULL;
    }

    size_t hash = __hash(key);

    for(size_t i = 0; i < self->object.size; i ++) {
        if(self->object.pair[i].key == hash)
            return self->object.pair[i].value;
    }
   
    return NULL;
}


static void
__json_show(
    Json * self
    , FILE * stream) {
    if(self == NULL) {
        fprintf(stream, "null");
        return;
    }

    switch(self->type) {
        case JsonArray:
            fprintf(stream, "[");
            
            for(size_t i = 0; i < self->array.size; i++) {
               if(i > 0)
                    fprintf(stream, ", ");

                __json_show(self->array.value[i], stream);
            }

            fprintf(stream, "]");
            break;
        case JsonObject:
            fprintf(stream, "{");
            
            if(self->object.pair != NULL) {
                for(size_t i = 0; i < self->object.size; i++) {
                    fprintf(stream, i == 0 ? "\'%s\': " : ", \'%s\': ", self->object.pair[i].name);
                    __json_show(self->object.pair[i].value, stream);
                }
            }

            fprintf(stream, "}");
            break;
        case JsonInteger:
        case JsonBool:
        case JsonFrac:
        case JsonString:
            fprintf(stream, "%s", self->string);
            break;
        case JsonNull:
            fprintf(stream, "null");
            break;
    }
}


void
json_show(
    Json * self
    , FILE * stream) {
    if(stream == NULL)
        return;

    __json_show(self, stream);
    putchar('\n');
}


Json * json_string_new(char * string) {
    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonString
        , .string = strdup(string)
    };

    return self;
}


Json * json_bool_new(bool value) {
    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonBool
        , .string = value == true ? strdup("true") : strdup("false")
    };

    return self;
}


Json * json_integer_new(long value) {
    int size = snprintf(NULL, 0, "%ld", value);

    if(size <= 0)
        size = 1;

    char * integer = malloc(sizeof(char) * (size + 1));
    
    snprintf(integer, size, "%ld", value);

    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonInteger
        , .string = integer
    };

    return self;
}


Json * json_frac_new(double value) {
    int size = snprintf(NULL, 0, "%f", value);

    if(size <= 0)
        size = 1;

    char * frac = malloc(sizeof(char) * (size + 1));
    
    snprintf(frac, size, "%f", value);

    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonFrac
        , .string = frac
    };

    return self;
}


Json * json_null_new(void) {
    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonNull
    };

    return self;
}


Json * json_array_new(size_t size) {
    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonArray
        , .array.size = size
        , .array.value = size == 0 ? NULL : malloc(sizeof(Json *) * size)
    };

    memset(self->array.value, 0, sizeof(Json *) * size);

    return self;
}


bool json_array_append(Json * self, Json * value) {
    if(self->type == JsonArray) {
        self->array.value = realloc(self->array.value, sizeof(Json *) * self->array.size + 1);
        self->array.value[self->array.size] = value;
        self->array.size ++;
        return true;
    }

    return false;
}


Json * json_object_new(size_t size) {
    Json * self = malloc(sizeof(Json));

    *self = (Json) {
        .type = JsonObject
        , .object.size = size
        , .object.pair = size == 0 ? NULL : malloc(sizeof(struct JsonObjectPair) * size)
    };

    memset(self->object.pair, 0, sizeof(struct JsonObjectPair) * size);

    return self;
}


bool json_object_append(Json * self, char * name, Json * value) {
    if(self->type == JsonObject) {
        self->object.pair = realloc(self->object.pair, sizeof(struct JsonObjectPair) * self->object.size + 1);

        self->object.pair[self->object.size].name = strdup(name);
        self->object.pair[self->object.size].key = __hash(name);
        self->object.pair[self->object.size].value = value;

        self->object.size ++;
        return true;
    }

    return false;
}


bool json_object_set_record(Json * self, size_t index, char * name, Json * value) {
    if(self->type == JsonObject 
        && index < self->object.size) {
        
        if(self->object.pair[index].name != NULL)
            free(self->object.pair[index].name);

        json_delete(self->object.pair[index].value);

        self->object.pair[index].name = strdup(name);
        self->object.pair[index].key = __hash(name);
        self->object.pair[index].value = value;

        return true;
    }

    return false;
}


bool json_object_set(Json * self, char * name, Json * value) {
    if(self == NULL 
        || self->type != JsonObject) {
        return NULL;
    }

    size_t hash = __hash(name);

    for(size_t i = 0; i < self->object.size; i ++) {
        if(self->object.pair[i].key == hash) {
            self->object.pair[i].value = value;
            return true;
        }
    }

    return false;
}


Json *
json_clone(Json * self) {
    (void) self;
    Json * json = NULL;

    return json;
}


void
json_delete(Json * self) {
    if(self == NULL)
        return;

    switch(self->type) {
        case JsonString:
        case JsonInteger:
        case JsonBool:
        case JsonFrac:
            if(self->string != NULL)
                free(self->string);
            break;
        case JsonArray:
            if(self->array.value != NULL) {
                for(size_t i = 0; i < self->array.size; i++)
                    json_delete(self->array.value[i]);
             
                free(self->array.value);   
            }
            break;
        case JsonObject:
			if(self->object.pair != NULL) {
                for(size_t i = 0; i < self->object.size; i++) {
                    if(self->object.pair[i].name != NULL)
                        free(self->object.pair[i].name);

                    json_delete(self->object.pair[i].value);
                }

                free(self->object.pair);
            }
            
            break;
        case JsonNull:
            break;
    }

    free(self);
}


