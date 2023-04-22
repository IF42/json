/**
** @file json.c
** @author Petr Horáček
** @brief 
** json = object | array | value | null ;
** object = "{" [ members ] "}" ;
** members = pair { "," pair } ;
** pair = string ":" value ;
** array = "[" [ elements ] "]" ;
** elements = value { "," value } ;
** value = string | number | object | array | "true" | "false" | "null" ;
** string = '"' { character } '"' ;
** number = [ "-" ] int [ frac ] [ exp ] ;
** int = "0" | digit1-9 { digit } ;
** frac = "." digit { digit } ;
** exp = ( "e" | "E" ) [ "+" | "-" ] digit { digit } ;
** character = unescaped | escape ( "\" / "/" / "\"" / "b" / "f" / "n" / "r" / "t" / "u" hex hex hex hex ) ;
** unescaped = utf8-character - '"' - '\' ;
** escape = '\' ;
** hex = digit / "A" / "B" / "C" / "D" / "E" / "F" / "a" / "b" / "c" / "d" / "e" / "f" ;
** digit1-9 = "1" / "2" / "3" / "4" / "5" / "6" / "7" / "8" / "9" ;
** digit = "0" / digit1-9 ;
*/

#include "json.h"
