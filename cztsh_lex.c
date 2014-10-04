#include <stdio.h>
#include "czt.h"

typedef enum {T_WORD, T_BAR, T_AMP, T_SEMI, T_GT, T_GTGT, T_LT, T_EQUAL,  
  T_NL, T_EOF, T_VARIABLE, T_SUBST, T_ERROR} TOKEN_TYPE;


static TOKEN_TYPE nextToken(FILE *fd, char *word, int maxword){
    int word_count = 0;
    char c;
    typedef enum{NEUTRAL, GTGT, INWORD, INQUOTE, INQUOTE_SLASH, AFTER_DOLLAR_FIRST, AFTER_DOLLAR, INVARIABLE, IN_DOLLAR_SUBST} STATUS;
    STATUS status = NEUTRAL;
    while((c = fgetc(fd)) != EOF){
        switch (status){
        case NEUTRAL:
            switch (c){
            case '|':
                return T_BAR;
            case '&':
                return T_AMP;
            case ';':
                return T_SEMI;
            case '>':
                status = GTGT;
                continue;
            case '<':
                return T_LT;
            case '=':
                return T_EQUAL;
            case '\n':
                return T_NL;
            case '$':
                status = AFTER_DOLLAR_FIRST;
                word_count = 0;
                continue;
            case ' ':
            case '\t':
                continue;
            case '"':
                status = INQUOTE;
                word_count = 0;
                continue;
            case '\\':

                break;
            default:
                status = INWORD;
                word_count = 0;
                word[word_count++] = c;
                continue;
            }
        case GTGT:
            if(c == '>'){
                return T_GTGT;
            }
            else{
                ungetc(c, fd);
                return T_GT;
            }
        case INWORD:
            switch (c) {
            case ';':
            case '&':
            case '|':
            case '<':
            case '>':
            case '\n':
            case ' ':
            case '\t':
            case '=':
                ungetc(c, fd);
                word[word_count++] = '\0';
                return T_WORD;
            default:
                word[word_count++] = c;
                continue;
            }
        case AFTER_DOLLAR_FIRST:
            switch (c) {
            case '{':
                status = INVARIABLE;
                continue;
            case '(':
                status = IN_DOLLAR_SUBST;
                continue;
            default:
                ungetc(c, fd);
                status = AFTER_DOLLAR;
                continue;
            }
        case INVARIABLE:
            switch (c) {
            case '}':
                word[word_count++] = '\0';
                return T_VARIABLE;
            default:
                word[word_count++] = c;
                continue;
            }
        case IN_DOLLAR_SUBST:
            switch (c) {
            case ')':
                word[word_count++] = '\0';
                return T_SUBST;
            default:
                word[word_count++] = c;
                continue;
            }
        case AFTER_DOLLAR:
            switch (c) {
            case ';':
            case '&':
            case '|':
            case '<':
            case '>':
            case '\n':
            case ' ':
            case '\t':
                ungetc(c, fd);
                word[word_count++] = '\0';
                return T_VARIABLE;
            default:
                word[word_count++] = c;
                continue;
            }
        case INQUOTE:
            switch (c){
            case '\\':
                status = INQUOTE_SLASH;
                continue;
            case '"':
                word[word_count++] = '\0';
                return T_WORD;
            default:
                word[word_count++] = c;
                continue;
            }
        case INQUOTE_SLASH:
            switch (c){
            case EOF:
                word[word_count++] = '\0';
                return T_WORD;
            case '\n':
                status = INQUOTE;
                continue;
            case 'a':
                word[word_count++] = '\a';
                status = INQUOTE;
                continue;
            case 'b':
                word[word_count++] = '\b';
                status = INQUOTE;
                continue;
            case 'f':
                word[word_count++] = '\f';
                status = INQUOTE;
                continue;
            case 'n':
                word[word_count++] = '\n';
                status = INQUOTE;
                continue;
            case 'r':
                word[word_count++] = '\r';
                status = INQUOTE;
                continue;
            case 't':
                word[word_count++] = '\t';
                status = INQUOTE;
                continue;
            case 'v':
                word[word_count++] = '\v';
                status = INQUOTE;
                continue;
            case '\'':
                word[word_count++] = '\'';
                status = INQUOTE;
                continue;
            case '"':
                word[word_count++] = '\"';
                status = INQUOTE;
                continue;
            case '$':
                word[word_count++] = '$';
                status = INQUOTE;
                continue;
            case '\\':
                word[word_count++] = '\\';
                status = INQUOTE;
                continue;
            default:
                word[word_count++] = '\\';
                word[word_count++] = c;
                status = INQUOTE;
            }
        default:
            fprintf(stderr, "Error parse tokens\n");
            return T_ERROR;
        }
    }
    return T_ERROR;
}

int main(int argc, char const *argv[])
{
    char str[100];
    TOKEN_TYPE type;
    FILE * fd = fopen("test_sh.txt", "rw");
    while((type = nextToken(fd, str, 100)) != T_ERROR){
        switch (type){  
        case T_WORD:
            printf("T_WORD: %s\n", str);
            break;
        case T_BAR:
            printf("T_BAR\n");
            break;
        case T_AMP:
            printf("T_AMP\n");
            break;
        case T_SEMI:
            printf("T_SEMI\n");
            break;
        case T_GT:
            printf("T_GT\n");
            break;
        case T_GTGT:
            printf("T_GTGT\n");
            break;
        case T_LT:
            printf("T_LT\n");
            break;
        case T_NL:
            printf("T_NL\n");
            break;
        case T_EOF:
            printf("T_EOF\n");
            break;
        case T_VARIABLE:
            printf("T_VARIABLE: %s\n", str);
            break;
        case T_SUBST:
            printf("T_SUBST: %s\n", str);
            break;
        default:
            fprintf(stderr, "error\n");
        }
    }
    return 0;
}