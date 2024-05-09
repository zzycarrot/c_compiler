#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#define BASE 147
int token;
char *src;
int poolSize;
int line;
char *DATASEGMENT;
int *idmain;
char* LP;
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

//lexical
/*
enum {//if token is smaller then 128 ,then it isn't a token
    Num = 128 ,Fun,Sys,Id,GLo,Loc,
    Int,Char,If,Else,While,Return,Sizeof,Enum,
    Assign,Div,Mul,Add,Sub,Lss,Leq,Gtr,Geq,Eq,Neq,Inc,Dec,Not,And,Or,Lshf,Rshf,Mod,Brak,Cond
};//token
*/
enum {
    Num = 128, Fun, Sys, Id, Glo, Loc,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign,Div,Mul,Add,Sub,Lss,Leq,Gtr,Geq,Eq,Neq,Inc,Dec,Not,And,Or,Lshf,Rshf,Mod,Brak,Cond
  };
int tokenVal;
int *currentId;
int *symbols; //start of symbol table
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, Size};//indentifier table
void next() {
    char * lastPos;
    int hash;
    while(token = *src) {
        ++src; //LL(1);
        //next line
        if(token == '\n') {
            line++;
            return;
        }else

        //ToDo : define && include
        if(token == '#' ) {
            while(*src != EOF && *src !='\n') {
                src++;
            }
            return;
        }else


        //Varible or Function
        // if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
        //
        //     // parse identifier
        //     lastPos = src - 1;
        //     hash = token;
        //
        //     while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
        //         hash = hash * 147 + *src;
        //         src++;
        //     }
        //
        //     // look for existing identifier, linear search
        //     currentId = symbols;
        //     while (currentId[Token]) {
        //         if (currentId[Hash] == hash && !memcmp((char *)currentId[Name], lastPos, src - lastPos)) {
        //             //found one, return
        //             token = currentId[Token];
        //             return;
        //         }
        //         currentId = currentId + Size;
        //     }
        //
        //
        //     // store new ID
        //     currentId[Name] = (int)lastPos;
        //     currentId[Hash] = hash;
        //     token = currentId[Token] = Id;
        //     return;
        // }
        if( (token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || token == '_') {
            printf("Get Var: %c",token);
            lastPos = src - 1;
            LP = lastPos;
            hash = token;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                putchar(*src);
                hash = hash * BASE + *src;
                src++;
            }
            putchar('\n');
            //check wehter is existing in indetifier table
            //ToDo :(2024/5/8:done)
            currentId = symbols;
            while(currentId[Token]) {
                // if(currentId[Hash] == hash) {
                //     printf("match\n");
                //     char *str1 =malloc(poolSize);
                //     memcpy(str1,lastPos,src - lastPos);
                //     printf("str1:%s\n",str1);
                // }
                if(currentId[Hash] == hash && !memcmp((char* )currentId[Name],lastPos,src - lastPos)) {
                    printf("--if\n");
                    token = currentId[Token];
                    printf("already exist\n");
                    return;
                }
                currentId = currentId + Size; //next postion
            }
            //add new identifier
            currentId[Name] = (int)lastPos;
            currentId[Hash] = hash;
            token = currentId[Token] = Id;
            printf("add new identifier\n");
            return;
        }else
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
        }else

        //String && char
        if( token == '"' || token == '\'' ) {
            printf("Get token: str%c",token);

            lastPos = DATASEGMENT;
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
                *DATASEGMENT++ = lastPos;
            }else {
                token = Num;
            }
            return;
        }else
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
        }else
        if(token == '=') {
            //parser == or =
            if(*src == '=') {
                src++;
                token = Eq;
            }else {
                token = Assign;
            }
            return;
        }else
        if(token == '+') {
            //parser ++ or +
            if(*src == '+') {
                src++;
                token = Inc;
            }else {
                token = Add;
            }
            return;
        }else
        if(token == '-') {
            //parser -- or -
            if(*src == '-') {
                src++;
                token = Dec;
            }else {
                token = Sub;
            }
            return;
        }else
        if(token == '*') {
            token = Mul;
            return;
        }else
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
        }else
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
        }else
        if(token == '!') {
            //parser != or !
            if(*src == '=') {
                src++;
                token = Neq;
            }else {
                token = Not;
            }
            return;
        }else
        //Todo : Bitwise operator & | ^ ~
        if(token == '&') {
            //parser &&
            if(*src == '&') {
                src++;
                token = And;
            }
            return;
        }else
        if(token == '|') {
            //parser ||
            if(*src == '|') {
                src++;
                token = Or;
            }
            return;
        }else
        if(token == '%') {
            token = Mod;
            return;
        }else
        if(token == '[') {
            token = Brak;
            return;
        }else
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
    next();
    while(token >0 ) {
        next();
    }

}

//Assembly Lang
int eval() {

}

int main(int argc,char **argv) {
    int file;
    int fileLength;
    poolSize = 1024*256;
    int i;
    argc--;
    argv++;
    if((file = open(*argv , 0)) < 0) {
        printf("cannot open file:%s\n", *argv);
        return -1;
    }
    if (!(DATASEGMENT = malloc(poolSize))) {
        printf("could not malloc(%d) for DATASEGMENT table\n", poolSize);
        return -1;
    }
    if (!(symbols = malloc(poolSize))) {
        printf("could not malloc(%d) for symbol table\n", poolSize);
        return -1;
    }
    memset(DATASEGMENT,0,sizeof(poolSize));
    memset(symbols,0,sizeof(poolSize));

    // add keywords to symbol table
    // src = "int char if else while return sizeof enum "
    //       "open read close printf malloc memset memcmp exit void main";
    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";
    i = Char;
    while (i <= While) {
        next();
        currentId[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) {
        next();
        currentId[Class] = Sys;
        currentId[Type] = Int;
        currentId[Value] = i++;
        //ToDo : error (编译器问题？？？，不用cmake用gcc就解决了）
        printf("match\n");
        char *str1 =malloc(poolSize);
        int tmp = (int)LP;
        memcpy(str1,(char*)tmp,src - LP);
        printf("str1:%s\n",str1);

    }

    next(); currentId[Token] = Char; // handle void type
    next(); idmain = currentId; // keep track of main
    if(!(src = malloc (poolSize))) {
        printf("couldn't malloc size of pool\n");
        return -1;
    }

    if((fileLength = read(file , src , poolSize - 1)) <= 0) {
        printf(("file empty"));
        return -1;
    }

    src[fileLength] = EOF;
    close(file);
    program();
    system("pause");
    return eval();
}
