#include "json.h"

#include <ctype.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>


Json * json_null_new(Alloc * alloc) {
    Json_Null * self = new(alloc, sizeof(Json_Null));
    
    *self = (Json_Null) {
        .type = JsonType_Null
        , .alloc = alloc
    };

    return JSON(self);
}


static Json * json_value_new(Alloc * alloc, Json type, size_t length, char * c_str) {
    Json_Value * self = new(alloc, sizeof(Json_Value));

    *self = (Json_Value) {
        .type = type
        , .alloc = alloc
        , .length = length
        , .c_str = c_str
    };

    return JSON(self); 
}


Json * json_bool_new(Alloc * alloc, size_t length, char * c_str) {
    return json_value_new(alloc, JsonType_Bool, length, c_str);
}


Json * json_string_new(Alloc * alloc, size_t length, char * c_str) {
    return json_value_new(alloc, JsonType_String, length, c_str);
}


Json * json_number_new(Alloc * alloc, size_t length, char * c_str) {
    return json_value_new(alloc, JsonType_Number, length, c_str);
}


Json * json_array_new(Alloc * alloc) {
    Json_Array * self = new(alloc, sizeof(Json_Array));

    *self = (Json_Array) {
        .type = JsonType_Array
        , .alloc = alloc
    };

    return JSON(self);
}


static void * json_array_next(Iterator * it) {
    if(it->index < JSON_ARRAY(it->context)->size) {
        return  JSON_ARRAY(it->context)->front[it->index];
    } else {
        return NULL;
    }
}


Iterator json_array_iterator(Json_Array * self) {
    return (Iterator) {
        .next = json_array_next
        , .context = self
    }; 
}


size_t json_array_size(Json_Array * self) {
    return self->size;
}


static void json_array_resize(Json_Array * self) {
    self->capacity = (self->capacity + 1) * 3;
    Json ** arr = resize(self->alloc, self->front, sizeof(Json*) * self->capacity);
    assert(arr != NULL);
    self->front = arr;
}


void json_array_push_back(Json_Array * self, Json * child) {
    if(self->size >= self->capacity) {
        json_array_resize(self);
    }

    self->front[self->size++] = child;
}


void json_array_push_front(Json_Array * self, Json * child) {
    if(self->size >= self->capacity) {
        json_array_resize(self);
    }
    memmove(&self->front[1], self->front, sizeof(Json*) * self->size);
    self->front[0] = child;
    self->size++;
}


void json_array_delete_front(Json_Array * self) {

}


void json_array_delete_back(Json_Array * self) {

}


void json_array_delete_at(Json_Array * self, size_t index) {

}


Json * json_array_at(Json_Array * self, size_t index) {
    if(index < self->size) {
        return self->front[index];
    } else {
        return NULL;
    }
}


Json * json_object_new(Alloc * alloc) {
    Json_Object * self = new(alloc, sizeof(Json_Object));

    *self = (Json_Object) {
        .type = JsonType_Object
        , .alloc = alloc
    };

    return JSON(self);
}


static void * json_object_next(Iterator * it) {
    if(it->index < JSON_OBJECT(it->context)->size) {
        return &JSON_OBJECT(it->context)->front[it->index];
    } else {
        return NULL;
    }
}


Iterator json_object_iterator(Json_Object * self) {
    return (Iterator) {
        .next = json_object_next
        , .context = self
    }; 
}


#define INIT_HASH_SEED 5381


// Hash function djb2 
static size_t json_object_hash(size_t size, const char * c_str) {
    size_t hash = INIT_HASH_SEED;
    char c;
    while ((c = *c_str++) && (size --) > 0) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}


static void json_object_resize(Json_Object * self) {
    self->capacity = (self->capacity + 1) * 3;
    Json_Object_Node * arr = resize(self->alloc, self->front, sizeof(Json_Object_Node) * self->capacity);
    assert(arr != NULL);
    self->front = arr;    
}


void json_object_push_back(Json_Object * self, Json_Object_Key key, Json * child) {
    if(self->size >= self->capacity) {
        json_object_resize(self);
    }

    self->front[self->size++] = (Json_Object_Node) {
        .ID = json_object_hash(key.length, key.c_str)
        , .key = key
        , .child = child
    };
}


void json_object_push_front(Json_Object * self, Json_Object_Key key, Json * child) {
    if(self->size >= self->capacity) {
        json_object_resize(self);
    }
    memmove(&self->front[1], self->front, sizeof(Json_Object_Node) * self->size);
    self->front[0] = (Json_Object_Node) {
        .ID = json_object_hash(key.length, key.c_str)
        , .key = key
        , .child = child
    };
    self->size++;
}


void json_object_delete_front(Json_Object * self) {

}


void json_object_delete_back(Json_Object * self) {

}


void json_object_delete_at(Json_Object * self, char * key) {

}


Json * json_object_at(Json_Object * self, char * key) {
    size_t hash = INIT_HASH_SEED;
    char c;
    while ((c = *key++) != '/' && c != '\0') {
        hash = ((hash << 5) + hash) + c;
    }
    
    iterate(json_object_iterator(self), Json_Object_Node*, node, {
        if(node->ID == hash) {
            if(*key == '/') {
                if(*node->child == JsonType_Object) { 
                    return json_object_at(JSON_OBJECT(node->child), key);
                } else {
                    return NULL;
                }
            } else {
                return node->child;
            }
        }
    });

    return NULL;
}


typedef struct {
    size_t index;
    uint32_t row;
    uint32_t column;
    char character;
}Json_LexerState;


typedef struct {
    char * c_str;
    Json_LexerState state;
}Json_Lexer;


#define json_lexer(c_str) \
    (Json_Lexer) {(c_str), (Json_LexerState){0, 1, 1, *(c_str)}}


#define json_lexer_character(self)\
    ((int)(self)->state.character)


static int json_lexer_peek(Json_Lexer * self) {
    if(json_lexer_character(self) == '\0') {
        return '\0';
    }  else {
        return self->c_str[self->state.index + 1];
    }
}


static void json_lexer_advance(Json_Lexer * self) {
    if(json_lexer_character(self) == '\0') {
        return;
    } else if(json_lexer_character(self) == '\n') {
        self->state.row ++;
        self->state.column = 1;
    } else {
        self->state.column ++;
    }

    self->state.character = self->c_str[++self->state.index];
}


typedef enum {
    TokenID_Keyword
    , TokenID_String
    , TokenID_Number
    , TokenID_Symbol
    , TokenID_Undefined
    , TokenID_EOF
} TokenID;


typedef struct {
    TokenID id;
    size_t size;
    char * c_str;    
} Token;


#define Token(...) (Token){__VA_ARGS__}
 

static inline char * json_lexer_ref(Json_Lexer * self) {
    return &self->c_str[self->state.index];
}


static inline void json_lexer_skip_whitespace(Json_Lexer * self) {
    while(isspace(json_lexer_character(self))) {
        json_lexer_advance(self);
    }
}


static inline Token json_lexer_read_string(Json_Lexer * self) {
    json_lexer_advance(self);

    Token token = Token(TokenID_String, self->state.index, json_lexer_ref(self));

    while(json_lexer_character(self) != '"') {
        if(json_lexer_character(self) == '\\') {
            json_lexer_advance(self);
        }

        json_lexer_advance(self);
    }

    token.size = self->state.index - token.size;
    json_lexer_advance(self);
    return token;
}


static inline Token json_lexer_read_number(Json_Lexer * self) {
    Token token = Token(TokenID_Number, self->state.index, json_lexer_ref(self));

    if(json_lexer_character(self) == '+' 
            || json_lexer_character(self) == '-'
            || json_lexer_character(self) == '.') {
        json_lexer_advance(self);
    }

    while(isdigit(json_lexer_character(self)) 
            || json_lexer_character(self) == '.' 
            || tolower(json_lexer_character(self)) == 'e') {
        json_lexer_advance(self);
    }   

    token.size = self->state.index - token.size;

    return token;
}


static inline Token json_lexer_read_symbol(Json_Lexer * self) {
    Token token = Token(TokenID_Symbol, 1, json_lexer_ref(self));
    json_lexer_advance(self);

    return token;
}


static inline Token json_lexer_read_keyword(Json_Lexer * self) {
    Token token = Token(TokenID_Keyword, self->state.index, json_lexer_ref(self));

    while(isalnum(json_lexer_character(self))) {
        json_lexer_advance(self);
    }

    token.size = self->state.index - token.size;

    return token;
}


static Token json_lexer_next_token(Json_Lexer * self) {
    while(json_lexer_character(self) != '\0') {
        if(isspace(json_lexer_character(self))) {
            json_lexer_skip_whitespace(self);
            continue;
        } else if(json_lexer_character(self) == '"') {
            return json_lexer_read_string(self);
        } else if(isdigit(json_lexer_character(self)) 
                || ((json_lexer_character(self) == '+' || json_lexer_character(self) == '-') && isdigit(json_lexer_peek(self)))) {
            return json_lexer_read_number(self);
        } else if(json_lexer_character(self) == '{') {
            return json_lexer_read_symbol(self);
        } else if(json_lexer_character(self) == '}') {
            return json_lexer_read_symbol(self);
        } else if(json_lexer_character(self) == '[') {
            return json_lexer_read_symbol(self);
        } else if(json_lexer_character(self) == ']') {
            return json_lexer_read_symbol(self);
        } else if(json_lexer_character(self) == ',') {
            return json_lexer_read_symbol(self);
        } else if(json_lexer_character(self) == ':') {
            return json_lexer_read_symbol(self);
        } else {
            return json_lexer_read_keyword(self);
        }
    }

    return Token(TokenID_EOF, 0, NULL);
}


typedef struct {
    Alloc * alloc;
    Json_Lexer lexer;
} Json_Parser;


static bool json_parser_expect_token(Json_Parser * self, TokenID ID, size_t size, char * c_str) {
    Json_LexerState store = self->lexer.state;
    Token tok = json_lexer_next_token(&self->lexer);

    if(tok.id == ID && tok.size == size && strncmp(c_str, tok.c_str, size) == 0) {
        return true;
    } else {
        self->lexer.state = store;
        return false;
    }
}


static Json * json_parse(Json_Parser * self);


static inline Json * json_parse_array(Json_Parser * self) {
    Json * array = json_array_new(self->alloc);

    if(json_parser_expect_token(self, TokenID_Symbol, 1, "]") == false) {
        do {
            Json * child = json_parse(self);

            if(child != NULL) {
                json_array_push_back(JSON_ARRAY(array), child);
            } else {
                break;
            }
        } while(json_parser_expect_token(self, TokenID_Symbol, 1, ",") == true);
        assert(json_parser_expect_token(self, TokenID_Symbol, 1, "]") == true);
    }

    return array;
}


static inline Json * json_parse_object(Json_Parser * self) {
    Json * object = json_object_new(self->alloc);

    if(json_parser_expect_token(self, TokenID_Symbol, 1, "}") == false) {
        do {
            Token key = json_lexer_next_token(&self->lexer);
            assert(key.id == TokenID_String);

            if(key.id != TokenID_String) {
                break;
            } 

            assert(json_parser_expect_token(self, TokenID_Symbol, 1, ":") == true);

            Json * child = json_parse(self);
            if(child == NULL) {
                break;
            }

            json_object_push_back(JSON_OBJECT(object), (Json_Object_Key) {.length = key.size, .c_str = key.c_str}, child);
        } while(json_parser_expect_token(self, TokenID_Symbol, 1, ",") == true);

        assert(json_parser_expect_token(self, TokenID_Symbol, 1, "}") == true);
    }

    return object;
}


static Json * json_parse(Json_Parser * self) {
    Token token = json_lexer_next_token(&self->lexer);

    if(token.id == TokenID_EOF) {
        return NULL;
    } else if(token.id == TokenID_Symbol) {
        if(strncmp(token.c_str, "{", 1) == 0) {
            return json_parse_object(self);
            return NULL;
        } else if(strncmp(token.c_str, "[", 1) == 0) {
            return json_parse_array(self);
        }
    } else if(token.id == TokenID_String) {
        return json_string_new(self->alloc, token.size, token.c_str);
    } else if(token.id == TokenID_Number) {
        return json_number_new(self->alloc, token.size, token.c_str);
    } else if(token.id == TokenID_Keyword) {
        if(strncmp(token.c_str, "true", token.size) == 0 || strncmp(token.c_str, "false", token.size) == 0) {
            return json_bool_new(self->alloc, token.size, token.c_str);
        } else if(strncmp(token.c_str, "null", token.size) == 0){
            return json_null_new(self->alloc);
        } else {
            return NULL;
        }
    }
            
    return NULL;
}


Json * json_deserialize(Alloc * alloc, char * c_str) {
    Json_Parser parser = {
        .alloc = alloc
        , .lexer = json_lexer(c_str)
    };
    
    return json_parse(&parser);
}


static void show(Json * self, FILE * stream) {
    if(self != NULL) {
        switch(*self) {
            case JsonType_Null:
                fprintf(stream, "null");
                break;
            case JsonType_Bool:
            case JsonType_Number:
                fprintf(stream, "%.*s", (int) JSON_VALUE(self)->length, JSON_VALUE(self)->c_str);
                break;
            case JsonType_String:
                fprintf(stream, "\"%.*s\"", (int) JSON_VALUE(self)->length, JSON_VALUE(self)->c_str);
                break;
            case JsonType_Array:
                fprintf(stream, "[");
                iterate(json_array_iterator(JSON_ARRAY(self)), Json*, value, {
                    if(iterator.index > 0) {
                        fprintf(stream, ", ");
                    }
                    show(value, stream);
                });
                fprintf(stream, "]");
                break;
            case JsonType_Object:
                fprintf(stream, "{");
                iterate(json_object_iterator(JSON_OBJECT(self)), Json_Object_Node*, node, {
                    if(iterator.index > 0) {
                        fprintf(stream, ", ");
                    }
                    fprintf(stream, "\"%.*s\" = ", (int) node->key.length, node->key.c_str);
                    show(node->child, stream);
                });
                fprintf(stream, "}");
                break;
        } 
    } else {
        fprintf(stream, "null");
    }
}


void json_show(Json * self, FILE * stream) {
    show(self, stream);
    fprintf(stream, "\n");
}


void json_finalize(Json * self) {
    if(self != NULL) {
        switch(*self) {
            case JsonType_Null:
                delete(JSON_NULL(self)->alloc, self);
                break;
            case JsonType_Bool:
            case JsonType_Number:
            case JsonType_String:
                delete(JSON_VALUE(self)->alloc, self);
                break;
            case JsonType_Array:
                iterate(json_array_iterator(JSON_ARRAY(self)), Json*, value, {
                    json_finalize(value);
                });

                if(JSON_ARRAY(self)->front != NULL) {
                    delete(JSON_ARRAY(self)->alloc, JSON_ARRAY(self)->front);
                }
                delete(JSON_ARRAY(self)->alloc, self);
                break;
            case JsonType_Object:
                iterate(json_object_iterator(JSON_OBJECT(self)), Json_Object_Node*, node, {
                    json_finalize(node->child);
                });
                
                if(JSON_OBJECT(self)->front != NULL) {
                    delete(JSON_OBJECT(self)->alloc, JSON_OBJECT(self)->front);
                }
                delete(JSON_OBJECT(self)->alloc, self);
                break;
        } 
    }
}


