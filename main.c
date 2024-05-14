/*
 *author: zzydanny
 *time: 2024/5/7
 */
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
int *TEXTSEGMENT;
int *STACK;
int *HEAP;
int *idmain;
char* LP;  //last position (debug)
int baseType;
int exprType;
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
enum{INT,CHAR,PTR};  // type : exp (***int) = INT + PTR*3
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

void match(int tk) {
    if(token==tk)next();
    else{
        printf("Error at line %d: expect token %d\n",line,tk);
        exit(-1);
    }
}
//expr
void expression(int level) {

}

void enum_declaration() {
    //enum_declaration := 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num']} '}'
    //parse enum [id] {id [= num] , id [= num], ...}
    int i;
    i = 0;
    while (token != '}') {
        if(token != Id) {
            printf("Error at line %d: Expect identifier, but %d instead\n",line,token);
            exit(-1);
        }
        i = tokenVal;
        next();
        if(token == Assign) {
            next();
            if(token != Num) {
                printf("Error at line %d: Bad assign,should be initialized\n",line);
                exit(-1);
            }
            i = tokenVal;
            next();
        }
        currentId[Class] = Num;
        currentId[Type] = INT;
        currentId[Value] = i++; //default value ascending
        if (token == ',') {
            next();
        }
    }

}

int BP; //base pointer

void function_parameter() {
    //parameter_declaration := type {'*'} id {',' type {'*'} id}
    int type;
    int params;
    params = 0;
    while (token != ')') {
        type = INT;
        if (token == Int) {
            match(Int);
        }
        else if (token == Char) {
            match(Char);
            type = CHAR;
        }
        //pointer type
        while (token == Mul) {
            match(Mul);
            type = type + PTR;
        }
        //parameter name
        if (token != Id) {
            printf("Error at line %d: expect parameter name\n",line);
            exit(-1);
        }
        if (currentId[Class] == Loc) {
            printf("Error at line %d: duplicate parameter name\n",line);
            exit(-1);
        }
        match(Id);
        currentId[BClass] = currentId[Class];
        currentId[Class] = Loc;
        currentId[BType] = currentId[Type];
        currentId[Type] = type;
        currentId[BValue] = currentId[Value];
        currentId[Value] = params++; //index of parameter offset of BP
        if (token == ',') {
            match(',');
        }
    }
    BP = params + 1;
}

void statement() {

}

void function_body() {
    /*
     *type func (param) {body}
     *in body:
     *{
     *  1. local variable declaration
     *  2. statement
     * }
     */
    int posLocal; // postion of local variables on the stack
    int type;
    posLocal = BP;

    //local variable declaration
    while(token == Int || token ==Char) {
        baseType = (token == Int) ? INT : CHAR;
        match(token);
        while (token != ';') {
            type = baseType;
            while (token == Mul) {
                match(Mul);
                type = type + PTR;
            }
            if (token != Id) {
                printf("Error at line %d: expect variable name\n",line);
                exit(-1);
            }
            if (currentId[Class] == Loc) {
                printf("Error at line %d: duplicate variable name\n",line);
                exit(-1);
            }
            match(Id);
            currentId[BClass] = currentId[Class];
            currentId[Class] = Loc;
            currentId[BType] = currentId[Type];
            currentId[Type] = type;
            currentId[BValue] = currentId[Value];
            currentId[Value] = ++posLocal;
            if (token == ',') {
                match(',');
            }
        }
        match(";");
    }
    *++TEXTSEGMENT = ENT; //entrance
    *++TEXTSEGMENT = posLocal - BP;

    //statements
    while (token != '}') {
        statement();
    }
    *++TEXTSEGMENT = LEV; //
}
void function_declaration() {
    // type func (param) {body}
    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();


    // match('}');
    currentId = symbols;
    while(currentId[Token]) {
        if (currentId[Class] == Loc) {
            currentId[Class] = currentId[BClass];
            currentId[Type] = currentId[BType];
            currentId[Value] = currentId[BValue];
            //recover the symbol table
        }
    }
}

/*
 *globel_declaration := enum_declaration | variable_declaration | function_declaration
 *enum_declaration := 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num']} '}'
 *variable_declaration := type {'*'} id {',' {'*'} id } ';'
 *function_declaration := type {'*'} id '(' parameter_declaration ')' '{' body_declaration '}'
 */
void globel_declaration() {
    int type;
    int i;
    baseType = INT;
    //enum declaration
    if(token == Enum) {
        match(Enum);
        if(token != '{') {
            match(Id);
        }
        if(token=='{') {
            match('{');
            enum_declaration();
            match('}');
        }
        match(';');
        return;
    }
    printf("token:%d| %c\n",token,token);
    //variable or function declaration
    if(token == Int) {
        printf("get int\n");
        match(Int);
        baseType = INT;
    }
    else if(token == Char) {
        match(Char);
        baseType = CHAR;
    }
    while (token != ';' && token != '}') {
        type = baseType;
        //check for pointer like int **a;
        while(token == Mul) {
            match(Mul);
            type = type + PTR;//pointer base type = type % number of base type;
        }
        if(token != Id) {
            printf("Error at line %d: expect variable name\n",line);
            printf("token:%d\n",token);
            exit(-1);
        }
        if(currentId[Class]) {
            printf("Error at line %d: duplicate variable name\n",line);
            exit(-1);
        }
        match(Id);
        currentId[Type] = type;
        if(token == '(') {
            currentId[Class] = Fun;
            currentId[Value] = (int)(TEXTSEGMENT + 1);
            function_declaration();
        }else {
            currentId[Class] = Glo;
            currentId[Value] = (int)DATASEGMENT;
            DATASEGMENT = DATASEGMENT + sizeof(int);
            //ToDo : change sizeof int to sizeof type?
            //ToDo : pointers should be stored in the heap when malloc() is called
        }
        if(token == ',') {
            match(',');
        }
    }
    next();
}


//entrence
void program(){
    next();
    while(token >0 ) {
        printf("----\n");
        globel_declaration();
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
    if (!(TEXTSEGMENT = malloc(poolSize))) {
        printf("could not malloc(%d) for TEXTSEGMENT table\n", poolSize);
        return -1;
    }
    memset(TEXTSEGMENT,0,poolSize);
    memset(DATASEGMENT,0,poolSize);
    memset(symbols,0,poolSize);

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
