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

enum {//if token is smaller then 128 ,then it isn't a token
    Num = 128 ,Fun,Op,Id,GLo,Loc,
    Int,Char,If,Else,While,Return,Brak,Cond,
    Assign,Div,Mul,Add,Sub,Lss,Leq,Gtr,Geq,Eq,Neq,Inc,Dec,Not,And,Or,Lshf,Rshf,Mod
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
            //Number
            //ToDo : Negative number
            tokenVal = token - '0';
            if(tokenVal > 0) {
                while (*src >= '0' && *src <= '9' ) {
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
            printf("Get token(number) : %d \n", tokenVal);
            token = Num;
            return;
        }

        //String && char
        if( token == '"' || token == '\'' ) {
            printf("Get token: str%c",token);
            lastPos = src;
            if( *src == EOF ) {
                printf("Error: Missing \" or \' \n");
                return;
            }
            while(*src != EOF && *src != token ) {
                tokenVal = *src;
                *src++;
                if(tokenVal == '\\'){
                    if(*src == 'n') {
                        tokenVal = '\n';
                        printf("\\n");
                        *src++;
                    }else {
                        tokenVal = *src;
                        printf("%c",tokenVal);
                        *src++;
                    }
                }
            }
            src++;
            if(token == '"') {
                printf("\"\n");
                tokenVal = (int)*lastPos;
            }else {
                token = Num;
            }
            return;
        }
        //operator
        printf("Get operator: ");
        if(token == '/') {
            if (*src == '/') {
                //skip comments
                while (*src != EOF && *src != '\n') {
                    src++;
                }
                return;
            }else {
                //op divide
                token = Div;
                return;
            }
        }
        if(token == '=') {
            //parser == or =
            if(*src == '=') {
                src++;
                token = Eq;
            }else {
                token = Assign;
            }
            return;
        }
        if(token == '+') {
            //parser ++ or +
            if(*src == '+') {
                src++;
                token = Inc;
            }else {
                token = Add;
            }
            return;
        }
        if(token == '-') {
            //parser -- or -
            if(*src == '-') {
                src++;
                token = Dec;
            }else {
                token = Sub;
            }
            return;
        }
        if(token == '*') {
            token = Mul;
            return;
        }
        if(token == '<') {
            //parser <= or < or <<
            if(*src == '=') {
                src++;
                token = Leq;
            }else if(*src == '<') {
                token = Lshf;
            }else {
                token = Lss;
            }
            return;
        }
        if (token == '>') {
            //parser >= or >
            if(*src == '=') {
                src++;
                token = Geq;
            }else if(*src == '>') {
                token = Rshf;
            }else {
                token = Gtr;
            }
            return;
        }
        if(token == '!') {
            //parser != or !
            if(*src == '=') {
                src++;
                token = Neq;
            }else {
                token = Not;
            }
            return;
        }
        //Todo : Bitwise operator & | ^ ~
        if(token == '&') {
            //parser &&
            if(*src == '&') {
                src++;
                token = And;
            }
            return;
        }
        if(token == '|') {
            //parser ||
            if(*src == '|') {
                src++;
                token = Or;
            }
            return;
        }
        if(token == '%') {
            token = Mod;
            return;
        }
        if(token == '[') {
            token = Brak;
            return;
        }
        if (token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
}

//expr
void expression(int level) {

}

//entrence
void program(){
    printf("------\n");
    next();
    while(token >0 ) {
        printf("token is: %d ( %c )\n",token,token);
        printf("------\n");
        next();
    }
    printf("------\n");
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
    system("pause");
    return eval();
}
