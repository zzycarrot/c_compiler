#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define BASE 131
int token,tokenVal;
char *src;
int poolSize;
int line;


//lexical

enum {
    Num,Fun,Sys,Id,GLo,Loc,
    Int,Char,If,Else,While,Return,
    Assign,
};//token
enum identifier {
    Token,
    Hash,
    Name,
    Type,
    Class,
    Value,
    Gvalue
};//indentifier table
void next() {
    char *lastPos;
    int hash;
    while(token = *src) {
        ++src; //LL(1);

        //next line
        if(token == '\n') {
            line++;
            return;
        }

        //ToDo : define && include
        if(token == '#' ) {
            while(*src != EOF && *src !='\n') {
                src++;
            }
            return;
        }

        //Varible or Function
        if( (token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || token == '_') {
            printf("Get token: %c",token);
            lastPos = src - 1;
            hash = token;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                putchar(*src);
                hash = hash * BASE + *src;
                src++;
            }
            putchar('\n');

            //check wehter is existing in indetifier table
            //ToDo
            return;
        }
        if( token >= '0' && token <='9' ) {
            tokenVal = token - '0';
            if(tokenVal > 0) {
                while (*src >= '0' && token <= '9' ) {
                    tokenVal = tokenVal * 10 + *src++ - '0';
                }
            }
            else {
                //ToDo
                if(*src == 'x' || *src == 'X' ) {
                    //HEX


                }else{
                    //OCT

                }
            }
            printf("Get token: %d", tokenVal);
        }
    }
}

//expr
void expression(int level) {

}

//entrence
void program() {

}

//Assembly Lang
int eval() {

}

int main(int argc,char **argv) {
    int file;
    int fileLength;
    poolSize = 10024*256;
    argc--;
    argv++;
    if((file = open(*argv , 0)) < 0) {
        printf("cannot open file:%s\n", *argv);
        return -1;
    }
    if(!(src = malloc (poolSize))) {
        printf("couldn't malloc size of pool\n");
        return -1;
    }
    if((fileLength = read(file , src , poolSize - 1)) <= 0) {
        printf(("file empty"));
    }
    src[fileLength] = EOF;
    close(file);
    program();
    return eval();
}
