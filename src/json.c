/*
** @file json.c
** @author Petr Horáček
**
** json      ::= object | array
** object    ::= "{" (pair ("," pair)*)? "}"
** pair      ::= string ":" value
** array     ::= "[" (value ("," value)*)? "]"
** value     ::= string | number | object | array | "true" | "false" | "null"
** string    ::= '"' char* '"'
** number    ::= int frac? exp?
** int       ::= digit | digit1-9 digits
** frac      ::= "." digits
** exp       ::= ("e" | "E") ("+" | "-")? digits
** digits    ::= digit*
** digit     ::= "0" | digit1-9
*/

#include "json.h"
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>


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


#define Lexer(code)                 \
    (Lexer) {code, (LexerState){0, 1, 1, *code}}


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

    if(__char__(self) == '\n')
    {
        self->state.row ++;
        self->state.column = 1;
    }
    else
        self->state.column ++;

    self->state.character = self->code[self->state.index++];
}


JSON_NO_EXPORT char
__peek__(Lexer * self)
{
    if(__char__(self) == '\0')
        return '\0';
    else
        return self->code[self->state.index + 1];
}


JSON_INLINE_NO_EXPORT LexerState
__store__(Lexer * self)
{
    return self->state;
}


JSON_INLINE_NO_EXPORT void
__restore__(
    Lexer * self
    , LexerState state)
{
    self->state = state;
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


#define TOKEN_ID(T)                             \
    T == TokenID_Keyword ? "TokenID_Keyword":   \
    T == TokenID_String  ? "TokenID_String":    \
    T == TokenID_Integer ? "TokenID_Decimal":   \
    T == TokenID_Exp     ? "TokenID_Exp":       \
    T == TokenID_Symbol  ? "TokenID_Symbol":    \
    T == TokenID_EOF     ? "TokenID_EOF":       \
                           "Unknown"    

typedef struct
{
    TokenID id;
    size_t length;
    char * value;    
}Token;


#define Token(...) (Token){__VA_ARGS__}
 

typedef enum
{
    LexerError_OK
    , LexerError_UnexpectedToken
    , LexerError_DecimaFormat
}LexerError_ID;


typedef struct
{
    LexerError_ID error;
    uint32_t row;
    uint32_t column;
}LexerError;


#define LexerError(...) {__VA_ARGS__}


typedef struct
{
    enum{E_TokenID_Left, E_TokenID_Right}id;

    union
    {
        Token left;
        LexerError right;
    };
}E_Token;


#define E_Token_Left(token) \
    (E_Token){.id = E_TokenID_Left, .left = token}


#define E_Token_Right(error) \
    (E_Token){.id = E_TokenID_Right, .right = error}


JSON_INLINE_NO_EXPORT void
__skip_whitespace__(Lexer * self)
{
    while(isspace(__char__(self)))
        __advance__(self);
}


JSON_INLINE_NO_EXPORT E_Token
__read_string__(Lexer * self)
{
    __advance__(self);

    Token token = Token(TokenID_String, 0, &self->code[self->state.index]);

    while(__char__(self) != '"')
    {
        token.length++;
        __advance__(self);
    }

    __advance__(self);

    return E_Token_Left(token);
}


JSON_INLINE_NO_EXPORT E_Token
__read_number__(Lexer * self)
{
    Token token = Token(.length = 0, .value = &self->code[self->state.index]);

    if(tolower(__char__(self)) == 'e')
    {
        token.id = TokenID_Exp;
        __advance__(self);
    }
    else if(__char__(self) == '.')
    {
        token.id = TokenID_Decimal;
        __advance__(self);
    }
    else
        token.id = TokenID_Integer;

    if(__char__(self) == '+' || __char__(self) == '-')
        __advance__(self);

    while(isdigit(__char__(self)) == true || __char__(self) == '.')
    {
        if(__char__(self) == '.')
        {
            if(token.id == TokenID_Decimal || token.id == TokenID_Exp)
            {
                return E_Token_Right(
                            LexerError(
                                LexerError_DecimaFormat
                                , self->state.row
                                , self->state.column));
            }
            else
                token.id = TokenID_Decimal;
        }

        token.length++;
        __advance__(self);
    }   

    return E_Token_Left(token);
}


JSON_INLINE_NO_EXPORT E_Token
__read_symbol__(Lexer * self)
{
    Token token = Token(TokenID_Symbol, 1, &self->code[self->state.index]);
    __advance__(self);

    return E_Token_Left(token);
}


JSON_INLINE_NO_EXPORT E_Token
__read_keyword(Lexer * self)
{
    Token token = Token(TokenID_Keyword, 0, &self->code[self->state.index]);
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

    return E_Token_Left(token);
}


JSON_NO_EXPORT E_Token
__next_token__(Lexer * self)
{
    while(__char__(self) != '\0')
    {
        if(isspace(__char__(self)))
        {
            __skip_whitespace__(self);
            continue;
        }

        if(__char__(self) == '"')
            return __read_string__(self);
        else if(isdigit(__char__(self))
            || (__char__(self) == '+' 
                && (isdigit(__peek__(self)) || __peek__(self) == '.'))
            || (__char__(self) == '-'
                && (isdigit(__peek__(self)) || __peek__(self) == '.'))
            || (tolower(__char__(self)) == 'e'
                && (isdigit(__peek__(self)) || __peek__(self) == '.'))
            || (__char__(self) == '.' 
                && isdigit(__peek__(self))))
        {
            return __read_number__(self);
        }
        else if(__char__(self) == '{')
        {
            return __read_symbol__(self);
        }
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
            return __read_keyword(self);
    }

    return E_Token_Left(Token(TokenID_EOF, 0, '\0'));
}


typedef struct
{
    Lexer lexer;
}JsonParser;


void
json_show_tokens(char * code)
{
    JsonParser parser = (JsonParser){.lexer=Lexer(code)};

    for(E_Token t = __next_token__(&parser.lexer);; t = __next_token__(&parser.lexer))
    {
        if(t.id == E_TokenID_Right)
        {
            printf("%d:%d: lexer eeror %d\n", t.right.row, t.right.column, t.right.error);
            break;
        }
        else
        {
            if(t.left.id == TokenID_EOF)
                break;
            else
            {
                printf("Token {id=%s, length=%lld, value=\"", TOKEN_ID(t.left.id), t.left.length);
                fwrite(t.left.value, sizeof(char), t.left.length, stdout);
                puts("\"}");
            }
        }
    }
}




