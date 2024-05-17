/*
 *author: zzydanny
 *time: 2024/5/7
 */
#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#define BASE 147
#define debug printf
int token;
char *src;
int poolSize;
int line;

char *DATASEGMENT;
int *TEXTSEGMENT;
int *STACK;
int *HEAP;
int *pc, *bp, *sp, ax, cycle; // virtual machine registers
int BP; //base pointer

int *idmain;
char* LP;  //last position (debug)
int baseType;
int exprType;
enum { LEA ,IMM ,JMP ,CALL,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PUSH,
       OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
       OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };

//lexical
enum {
    Num = 128, Fun, Sys, Id, Glo, Loc,
    Char, Else, Enum, If, Int, Return, Sizeof, While,
    Assign,Cond,LAnd,LOr,And,Xor,Or,Eq,Neq,Lss,Leq,Gtr,Geq,Lshf,Rshf,Add,Sub,Mul,Div,Mod,Inc,Dec,Not,Brak
    // Assign,Div,Mul,Add,Sub,Lss,Leq,Gtr,Geq,Eq,Neqq,Inc,Dec,Not,And,Or,Lshf,Rshf,Mod,Brak,Cond
};
enum{INT,CHAR,PTR};  // type : exp (***int) = INT + PTR*3
int tokenVal;
int *currentId;
int *symbols; //start of symbol table
enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, Size};//indentifier table
int indexBP;
void next() {
    printf("call next()\n");
    char *last_pos;
    int hash;

    while (token = *src) {
        printf("----\n");
        ++src;

        // parse token here
        if (token == '\n') {
            ++line;
        }
        else if (token == '#') {
            // skip macro, because we will not support it
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
            printf("get id\n");
            // parse identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * 147 + *src;
                src++;
            }
            printf("match\n");
            int l1 = (int)last_pos;
            int l2 = (int)src;
            char *str1 =malloc(poolSize);
            int tmp = (int)last_pos;
            memcpy(str1,(char*)tmp,src - last_pos);
            printf("str1:%s\n",str1);

            // look for existing identifier, linear search
            currentId = symbols;
            while (currentId[Token]) {
                if (currentId[Hash] == hash && !memcmp((char *)currentId[Name], last_pos, src - last_pos)) {
                    //found one, return
                    debug("already exists\n");
                    token = currentId[Token];
                    return;
                }
                currentId = currentId + Size;
            }


            // store new ID
            currentId[Name] = (int)last_pos;
            currentId[Hash] = hash;
            token = currentId[Token] = Id;
            return;
        }
        else if (token >= '0' && token <= '9') {
            printf("get number\n");
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            tokenVal = token - '0';
            if (tokenVal > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    tokenVal = tokenVal*10 + *src++ - '0';
                }
            } else {
                // starts with 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        tokenVal = tokenVal * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        tokenVal = tokenVal*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }
        else if (token == '"' || token == '\'') {
            printf("get string\n");
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into DATASEGMENT.
            last_pos = DATASEGMENT;
            while (*src != 0 && *src != token) {
                tokenVal = *src++;
                if (tokenVal == '\\') {
                    // escape character
                    tokenVal = *src++;
                    if (tokenVal == 'n') {
                        tokenVal = '\n';
                    }
                }

                putchar(tokenVal);
                if (token == '"') {
                    *DATASEGMENT++ = tokenVal;
                }
            }

            src++;
            // if it is a single character, return Num token
            if (token == '"') {
                tokenVal = (int)last_pos;
            } else {
                token = Num;
            }
            printf("end string\n");
            putchar('\n');
            return;
        }
        else if (token == '/') {
            if (*src == '/') {
                // skip comments
                while (*src != 0 && *src != '\n') {
                    ++src;
                }
            } else {
                // divide operator
                token = Div;
                return;
            }
            return;
        }
        printf("-%c\n",token);
        if (token == '=') {
            // parse '==' and '='
            if (*src == '=') {
                src ++;
                token = Eq;
            } else {
                token = Assign;
            }
            return;
        }
        else if (token == '+') {
            // parse '+' and '++'
            if (*src == '+') {
                src ++;
                token = Inc;
            } else {
                token = Add;
            }
            return;
        }
        else if (token == '-') {
            // parse '-' and '--'
            if (*src == '-') {
                src ++;
                token = Dec;
            } else {
                token = Sub;
            }
            return;
        }
        else if (token == '!') {
            // parse '!='
            if (*src == '=') {
                src++;
                token = Neq;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                token = Leq;
            } else if (*src == '<') {
                src ++;
                token = Lshf;
            } else {
                token = Lss;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Geq;
            } else if (*src == '>') {
                src ++;
                token = Rshf;
            } else {
                token = Gtr;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                token = LOr;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                token = LAnd;
            } else {
                token = And;
            }
            return;
        }
        else if (token == '^') {
            token = Xor;
            return;
        }
        else if (token == '%') {
            token = Mod;
            return;
        }
        else if (token == '*') {
            token = Mul;
            return;
        }
        else if (token == '[') {
            token = Brak;
            return;
        }
        else if (token == '?') {
            token = Cond;
            return;
        }
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') {
            // directly return the character as token;
            return;
        }
    }
    return;
}
/**
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
            // printf("Get Var: %c",token);
            lastPos = src - 1;
            LP = lastPos;
            hash = token;
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                // putchar(*src);
                hash = hash * BASE + *src;
                src++;
            }
            //check wehter is existing in indetifier table
            //ToDo :(2024/5/8:done)
            currentId = symbols;
            while(currentId[Token]) {
                if(currentId[Hash] == hash && !memcmp((char* )currentId[Name],lastPos,src - lastPos)) {
                    token = currentId[Token];
                    // printf("already exist\n");
                    return;
                }
                currentId = currentId + Size; //next postion
            }
            //add new identifier
            currentId[Name] = (int)lastPos;
            currentId[Hash] = hash;
            token = currentId[Token] = Id;
            // printf("add new identifier\n");
            return;
        }else
        if( token >= '0' && token <='9' ) {
            //Number
            //ToDo : Neqgative number
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
            // printf("Get token(number) : %d \n", tokenVal);
            token = Num;
            return;
        }else

        //String && char
        if( token == '"' || token == '\'' ) {
            // printf("Get token: str%c",token);

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
                        // printf("\\n");
                        *src++;
                    }else {
                        tokenVal = *src;
                        // printf("%c",tokenVal);
                        *src++;
                    }
                }
            }
            src++;
            if(token == '"') {
                // printf("\"\n");
                tokenVal = (int)*lastPos;
                *DATASEGMENT++ = lastPos;
            }else {
                printf("Missing \' or \" \n");
                exit(-1);
                // token = Num;
            }
            return;
        }else
        //operator
        // printf("Get operator: ");
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
                token = LAnd;
            }else {
                token = And;
            }
            return;
        }else
        if(token == '|') {
            //parser ||
            if(*src == '|') {
                src++;
                token = LOr;
            }else {
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
*/
void match(int tk) {
    if(token==tk)next();
    else{
        printf("Error at line %d: expect token %d\n",line,tk);
        printf("get token %d instead\n",token);
        exit(-1);
    }
}
//expr
void expression(int level) {
    printf("call expression\n");
    /*
    *expression := assign {',' assign}
    *assign := equality ['=' assign]
    *equality := relational {'==' relational | '!=' relational}
    *relational := add {'<' add | '<=' add | '>' add | '>=' add}
    *add := mul {'+' mul | '-' mul}
    *mul := unary {'*' unary | '/' unary}
    *unary := primary | '*' unary | '&' unary | '!' unary | '-' unary
    *primary := num | id | '(' expr ')'
    */
    int *id;
    int tmp;
    int *addr;
    if(!token) {
        printf("Error at line %d: unexpected token EOF\n",line);
        exit(-1);
    }
    if(token == Num) {
        match(Num);
        *++TEXTSEGMENT = IMM;
        *++TEXTSEGMENT = tokenVal;
        exprType = INT;
    }else if(token == '"') {
        /*string*/
        *++TEXTSEGMENT = IMM;
        *++TEXTSEGMENT = tokenVal;
        match('"');
        while(token == '"') {
            match('"');
        }
        //ToDo : string like "abc"'\n'"abc" should be treated as "abcabc"
    }else if(token == Sizeof) {
        /*sizeof(int) sizeof(char) sizeof(*int) sizeof(*char)*/
        match(Sizeof);
        match('(');
        exprType = INT;
        if(token == Int) {
            match(Int);
        }else if(token == Char) {
            match(Char);
            exprType = CHAR;
        }
        while(token == Mul) {
            match(Mul);
            exprType = exprType + PTR;
        }
        match(')');
        *++TEXTSEGMENT = IMM;
        *++TEXTSEGMENT = (exprType == CHAR) ? sizeof(char) : sizeof(int);
        exprType = INT;
    }else if(token == Id) {
        /*1.Global or local variable
         *2.Function call
         *3.Enum
         */
        match(Id);
        id = currentId;
        if(token == '(') {
            /*function call*/
            match('(');
            tmp = 0;
            while(token != ')') {
                expression(Assign);
                *++TEXTSEGMENT = PUSH;
                tmp++;
                if(token == ',') {
                    match(',');
                }
            }
            match(')');
            if(id[Class] == Sys) {
                *++TEXTSEGMENT = id[Value];
            }else if(id[Class] == Fun) {
                *++TEXTSEGMENT = CALL;
                *++TEXTSEGMENT = id[Value];
            }else {
                printf("Error at line %d: call non-function\n",line);
                exit(-1);
            }
            if(tmp) {
                *++TEXTSEGMENT = ADJ;
                *++TEXTSEGMENT = tmp;
            }
            exprType = id[Type];
        }else if(id[Class] == Num) {
            /*enum*/
            *++TEXTSEGMENT = IMM;
            *++TEXTSEGMENT = id[Value];
            exprType = INT;
        }else {
            /*variable*/
            if (id[Class] == Loc) {
                *++TEXTSEGMENT = LEA;
                *++TEXTSEGMENT = indexBP - id[Value];
            }else if (id[Class] == Glo) {
                *++TEXTSEGMENT = IMM;
                *++TEXTSEGMENT = id[Value];
            }
            exprType = id[Type];
            *++TEXTSEGMENT = (exprType == CHAR) ? LC : LI;
        }
    }
    else if (token == '(') {
        /* cast or parenthesis */
        match('(');
        if(token == Int || token == Char) {
            /* cast */
            tmp = (token == Int) ? INT : CHAR;
            match(token);
            while(token == Mul) {
                match(Mul);
                tmp = tmp + PTR;
            }
            match(')');
            expression(Inc);
            exprType = tmp;
        }else {
            /*parenthesis */
            expression(Assign);
            exprType = INT;
        }
    }
    else if (token == Mul) {
        /* dereference *a */
        match(Mul);
        expression(Inc);
        if (exprType >= PTR) {
            exprType = exprType - PTR;
        }else {
            printf("Bad dereference at line %d:\n",line);
            exit(-1);
        }
        *++TEXTSEGMENT = (exprType == CHAR) ? LC : LI;
    }
    else if (token == AND) {
        /* address of (&)a */
        match(AND);
        expression(Inc);
        if (*TEXTSEGMENT == LC || *TEXTSEGMENT == LI) {
            TEXTSEGMENT--;
        }else {
            printf("Bad address of at line %d:\n",line);
            exit(-1);
        }
    }
    else if(token == '!') {
        /* bool not */
        match('!');
        expression(Inc);
        /*<expr == 0>*/
        *++TEXTSEGMENT = PUSH;
        *++TEXTSEGMENT = IMM;
        *++TEXTSEGMENT = 0;
        *++TEXTSEGMENT = EQ;
        exprType = INT;
    }
    else if(token == ADD) {
        /* +a */
        match(ADD);
        expression(Inc);
    }
    else if (token == SUB) {
        /* -a */
        match(SUB);
        expression(Inc);
        if (token == Num) {
            *++TEXTSEGMENT = IMM;
            *++TEXTSEGMENT = -tokenVal;
            match(Num);
        }
        else {
            /* -1 * a : a maybe a return val of a function*/
            *++TEXTSEGMENT = IMM;
            *++TEXTSEGMENT = -1;
            expression(Inc);
            *++TEXTSEGMENT = MUL;
        }
        exprType = INT;
    }
    else if (token == Inc || token == Dec) {
        tmp = token;
        match(token);
        expression(Inc);
        if (*TEXTSEGMENT == LC) {
            *TEXTSEGMENT = PUSH;
            *++TEXTSEGMENT = LC;
        }else if (*TEXTSEGMENT == LI) {
            *TEXTSEGMENT = PUSH;
            *++TEXTSEGMENT = LI;
        }else {
            printf("Error at line %d: lvalue required in increment/decrement\n",line);
            exit(-1);
        }
        *++TEXTSEGMENT = PUSH;
        *++TEXTSEGMENT = IMM;
        *++TEXTSEGMENT = (exprType > PTR) ? sizeof(int) : sizeof(char);
        *++TEXTSEGMENT = (tmp == Inc) ? ADD : SUB;
        *++TEXTSEGMENT = (exprType == CHAR) ? SC : SI;
    }
    //binary operator
    while(token>=level) {
        tmp = exprType;
        if(token == Assign) {
            match(Assign);
            printf("matched assign\n");
            if(*TEXTSEGMENT == LC || *TEXTSEGMENT == LI) {
                *TEXTSEGMENT = PUSH;
            }else {
                printf("Error at line %d: lvalue required in assignment\n",line);
                exit(-1);
            }
            expression(Assign);
            exprType = tmp;
            *++TEXTSEGMENT = (exprType == CHAR) ? SC : SI;  //store
        }else if(token == Cond) {
            //expr?expr:expr
            match(Cond);
            *++TEXTSEGMENT = JZ;
            addr = ++TEXTSEGMENT;
            expression(Assign);
            if(token == ':') {
                match(':');
            }else {
                printf("%d: missing colon in conditional\n", line);
                exit(-1);
            }
            *addr = (int)(TEXTSEGMENT + 3);
            *++TEXTSEGMENT = JMP;
            addr = ++TEXTSEGMENT;
            expression(Cond);
            *addr = (int)(TEXTSEGMENT + 1);
        }
        else if (token == LOr) {
            // logic or
            match(LOr);
            *++TEXTSEGMENT = JNZ;
            addr = ++TEXTSEGMENT;
            expression(LAnd);
            *addr = (int)(TEXTSEGMENT + 1);
            exprType = INT;
        }
        else if (token == LAnd) {
            // logic and
            match(LAnd);
            *++TEXTSEGMENT = JZ;
            addr = ++TEXTSEGMENT;
            expression(Or);
            *addr = (int)(TEXTSEGMENT + 1);
            exprType = INT;
        }
        else if (token == Or) {
            // bitwise or
            match(Or);
            *++TEXTSEGMENT = PUSH;
            expression(Xor);
            *++TEXTSEGMENT = OR;
            exprType = INT;
        }
        else if (token == Xor) {
            // bitwise xor
            match(Xor);
            *++TEXTSEGMENT = PUSH;
            expression(And);
            *++TEXTSEGMENT = XOR;
            exprType = INT;
        }
        else if (token == And) {
            // bitwise and
            match(And);
            *++TEXTSEGMENT = PUSH;
            expression(Eq);
            *++TEXTSEGMENT = AND;
            exprType = INT;
        }
        else if (token == Eq) {
            // equal ==
            match(Eq);
            *++TEXTSEGMENT = PUSH;
            expression(Neq);
            *++TEXTSEGMENT = EQ;
            exprType = INT;
        }
        else if (token == Neq) {
            // not equal !=
            match(Neq);
            *++TEXTSEGMENT = PUSH;
            expression(Lss);
            *++TEXTSEGMENT = NE;
            exprType = INT;
        }
        else if (token == Lss) {
            // less than
            match(Lss);
            *++TEXTSEGMENT = PUSH;
            expression(Lshf);
            *++TEXTSEGMENT = LT;
            exprType = INT;
        }
        else if (token == Gtr) {
            // greater than
            match(Gtr);
            *++TEXTSEGMENT = PUSH;
            expression(Lshf);
            *++TEXTSEGMENT = GT;
            exprType = INT;
        }
        else if (token == Leq) {
            // less than or equal to
            match(Leq);
            *++TEXTSEGMENT = PUSH;
            expression(Lshf);
            *++TEXTSEGMENT = LE;
            exprType = INT;
        }
        else if (token == Geq) {
            // greater than or equal to
            match(Geq);
            *++TEXTSEGMENT = PUSH;
            expression(Lshf);
            *++TEXTSEGMENT = GE;
            exprType = INT;
        }
        else if (token == Lshf) {
            // shift left
            match(Lshf);
            *++TEXTSEGMENT = PUSH;
            expression(Add);
            *++TEXTSEGMENT = SHL;
            exprType = INT;
        }
        else if (token == Rshf) {
            // shift right
            match(Rshf);
            *++TEXTSEGMENT = PUSH;
            expression(Add);
            *++TEXTSEGMENT = SHR;
            exprType = INT;
        }
        else if (token == Add) {
            // add
            match(Add);
            *++TEXTSEGMENT = PUSH;
            expression(Mul);

            exprType = tmp;
            if (exprType > PTR) {
                // pointer type, and not `char *`
                *++TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = IMM;
                *++TEXTSEGMENT = sizeof(int);
                *++TEXTSEGMENT = MUL;
            }
            *++TEXTSEGMENT = ADD;
        }
        else if (token == Sub) {
            // sub
            match(Sub);
            *++TEXTSEGMENT = PUSH;
            expression(Mul);
            if (tmp > PTR && tmp == exprType) {
                // pointer subtraction
                *++TEXTSEGMENT = SUB;
                *++TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = IMM;
                *++TEXTSEGMENT = sizeof(int);
                *++TEXTSEGMENT = DIV;
                exprType = INT;
            } else if (tmp > PTR) {
                // pointer movement
                *++TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = IMM;
                *++TEXTSEGMENT = sizeof(int);
                *++TEXTSEGMENT = MUL;
                *++TEXTSEGMENT = SUB;
                exprType = tmp;
            } else {
                // numeral subtraction
                *++TEXTSEGMENT = SUB;
                exprType = tmp;
            }
        }
        else if (token == Mul) {
            // multiply
            match(Mul);
            *++TEXTSEGMENT = PUSH;
            expression(Inc);
            *++TEXTSEGMENT = MUL;
            exprType = tmp;
        }
        else if (token == Div) {
            // divide
            match(Div);
            *++TEXTSEGMENT = PUSH;
            expression(Inc);
            *++TEXTSEGMENT = DIV;
            exprType = tmp;
        }
        else if (token == Mod) {
            // Modulo
            match(Mod);
            *++TEXTSEGMENT = PUSH;
            expression(Inc);
            *++TEXTSEGMENT = MOD;
            exprType = tmp;
        }
        else if (token == Inc || token == Dec) {
            // postfix inc(++) and dec(--)
            // we will increase the value to the variable and decrease it
            // on `ax` to get its original value.
            if (*TEXTSEGMENT == LI) {
                *TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = LI;
            }
            else if (*TEXTSEGMENT == LC) {
                *TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = LC;
            }
            else {
                printf("%d: bad value in increment\n", line);
                exit(-1);
            }

            *++TEXTSEGMENT = PUSH;
            *++TEXTSEGMENT = IMM;
            *++TEXTSEGMENT = (exprType > PTR) ? sizeof(int) : sizeof(char);
            *++TEXTSEGMENT = (token == Inc) ? ADD : SUB;
            *++TEXTSEGMENT = (exprType == CHAR) ? SC : SI;
            *++TEXTSEGMENT = PUSH;
            *++TEXTSEGMENT = IMM;
            *++TEXTSEGMENT = (exprType > PTR) ? sizeof(int) : sizeof(char);
            *++TEXTSEGMENT = (token == Inc) ? SUB : ADD;
            match(token);
        }
        else if (token == Brak) {
            // array access var[xx]
            match(Brak);
            *++TEXTSEGMENT = PUSH;
            expression(Assign);
            match(']');

            if (tmp > PTR) {
                // pointer, `not char *`
                *++TEXTSEGMENT = PUSH;
                *++TEXTSEGMENT = IMM;
                *++TEXTSEGMENT = sizeof(int);
                *++TEXTSEGMENT = MUL;
            }
            else if (tmp < PTR) {
                printf("%d: pointer type expected\n", line);
                exit(-1);
            }
            exprType = tmp - PTR;
            *++TEXTSEGMENT = ADD;
            *++TEXTSEGMENT = (exprType == CHAR) ? LC : LI;
        }
        else {
            printf("%d: compiler error, token = %d\n", line, token);
            exit(-1);
        }
    }
}

void enum_declaration() {
    printf("call enum_declarration\n");
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

void function_parameter() {
    printf("call function_parameter\n");
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
    printf("call statement\n");
    /*
    *statement := non_empty_statement | empty_statement
    *non_empty_statement := if_statement | while_statement | '{' statement '}'
    *                    | 'return' expression
    *                    | expression ';'
    *if_statement := 'if' '(' expression ')' statement ['else' non_empty_statement]
    *while_statement := 'while' '(' expression ')' non_empty_statement
    */

    /*if (...)<statement> [else <statement>]*/
    /*if (<cond>)           <cond>
     *                      JZ L1
     *<true statement>      <true statement>
     *else:                  JMP L2
     *L1:                   L1:
     *<false statement>     <false statement>
     *L2:                   L2:
     */
     int *L1,*L2;
     if (token == If ) {
         match(If);
         match('(');
            expression(Assign);
         match(')');
         *++TEXTSEGMENT = JZ;
         L2 = ++TEXTSEGMENT;
         statement();
         if(token == Else) {
             match(Else);
             *L2 = (int)(TEXTSEGMENT + 3);
             *++TEXTSEGMENT = JMP;
             L2 = ++TEXTSEGMENT;
             statement();

         }
        *L2 = (int)(TEXTSEGMENT + 1);
     }


    /*while (<cond>) <statement>*/
    /*L1:                L1:
     *while(<cond>)      <cond>
     *                   JZ L2
     *<statement>        <statement>
     *                   JMP L1
     *L2:                L2:
     */
    else if (token == While) {
        match(While);
        L1 = TEXTSEGMENT + 1;
        match('(');
        expression(Assign);
        match(')');
        *++TEXTSEGMENT = JZ;
        L2 = TEXTSEGMENT + 1;
        statement();
        *++TEXTSEGMENT = JMP;
        *++TEXTSEGMENT = (int)L1;
        *L2 = (int)(TEXTSEGMENT + 1);
    }
    else if (token == Return) {
        //return <expr>
        match(Return);
        if (token != ';') {
            expression(Assign);
        }
        match(';');
        *++TEXTSEGMENT = LEV;
    }
    else if (token == '{') {
        // { <statement> ... }
        match('{');
        while (token != '}') {
            statement();
        }
        match('}');
    }
    else if (token == ';') {
        //empty statement
        match(';');
    }
    else {
        //a = b; or function_call();
        expression(Assign);
        printf("test\n");
        match(';');
    }
}

void function_body() {
    printf("call function_body\n");
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
    posLocal = indexBP;

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
            debug("match id \n");
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
        match(';');
    }
    *++TEXTSEGMENT = ENT; //entrance
    *++TEXTSEGMENT = posLocal - indexBP;

    //statements
    while (token != '}') {
        statement();
    }
    *++TEXTSEGMENT = LEV; //
    debug("end call function_body\n");
    return;
}

void function_declaration() {
    printf("call function_delaration\n");
    // type func (param) {body}
    match('(');
    function_parameter();
    match(')');
    match('{');
    function_body();


    // match('}');
    currentId = symbols;
    int cnt = 0;
    while(currentId[Token]) {
        if (currentId[Class] == Loc) {
            currentId[Class] = currentId[BClass];
            currentId[Type] = currentId[BType];
            currentId[Value] = currentId[BValue];
            //recover the symbol table
        }
        currentId = currentId + Size;
    }
    debug("end call function_declaration\n");
}

/*
 *globel_declaration := enum_declaration | variable_declaration | function_declaration
 *enum_declaration := 'enum' [id] '{' id ['=' 'num'] {',' id ['=' 'num']} '}'
 *variable_declaration := type {'*'} id {',' {'*'} id } ';'
 *function_declaration := type {'*'} id '(' parameter_declaration ')' '{' body_declaration '}'
 */
void global_declaration() {
    printf("call global_declaration");
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
    // printf("token:%d| %c\n",token,token);
    //variable or function declaration
    if(token == Int) {
        // printf("get int\n");
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
    debug("end call global_declaration\n");
}
int cnt = 0;
//entrence
void program(){
    debug("call program\n");
    next();
    while(token > 0 ) {
        printf("cnt: %d\n",++cnt);
        printf("line: %d\n",line);
        // next();
        global_declaration();
    }
    debug("end call program\n");
}

//Assembly Language
int eval() {
    int op, *tmp;
    while (1) {
        op = *pc++; // get next operation code

        if (op == IMM)       {ax = *pc++;}                                     // load immediate value to ax
        else if (op == LC)   {ax = *(char *)ax;}                               // load character to ax, address in ax
        else if (op == LI)   {ax = *(int *)ax;}                                // load integer to ax, address in ax
        else if (op == SC)   {ax = *(char *)*sp++ = ax;}                       // save character to address, value in ax, address on stack
        else if (op == SI)   {*(int *)*sp++ = ax;}                             // save integer to address, value in ax, address on stack
        else if (op == PUSH) {*--sp = ax;}                                     // push the value of ax onto the stack
        else if (op == JMP)  {pc = (int *)*pc;}                                // jump to the address
        else if (op == JZ)   {pc = ax ? pc + 1 : (int *)*pc;}                   // jump if ax is zero
        else if (op == JNZ)  {pc = ax ? (int *)*pc : pc + 1;}                   // jump if ax is zero
        else if (op == CALL) {*--sp = (int)(pc+1); pc = (int *)*pc;}           // call subroutine
        //else if (op == RET)  {pc = (int *)*sp++;}                              // return from subroutine;
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}      // make new stack frame
        else if (op == ADJ)  {sp = sp + *pc++;}                                // add esp, <size>
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;}  // restore call frame and PC
        else if (op == ENT)  {*--sp = (int)bp; bp = sp; sp = sp - *pc++;}      // make new stack frame
        else if (op == ADJ)  {sp = sp + *pc++;}                                // add esp, <size>
        else if (op == LEV)  {sp = bp; bp = (int *)*sp++; pc = (int *)*sp++;}  // restore call frame and PC
        else if (op == LEA)  {ax = (int)(bp + *pc++);}                         // load address for arguments.

        else if (op == OR)  ax = *sp++ | ax;
        else if (op == XOR) ax = *sp++ ^ ax;
        else if (op == AND) ax = *sp++ & ax;
        else if (op == EQ)  ax = *sp++ == ax;
        else if (op == NE)  ax = *sp++ != ax;
        else if (op == LT)  ax = *sp++ < ax;
        else if (op == LE)  ax = *sp++ <= ax;
        else if (op == GT)  ax = *sp++ >  ax;
        else if (op == GE)  ax = *sp++ >= ax;
        else if (op == SHL) ax = *sp++ << ax;
        else if (op == SHR) ax = *sp++ >> ax;
        else if (op == ADD) ax = *sp++ + ax;
        else if (op == SUB) ax = *sp++ - ax;
        else if (op == MUL) ax = *sp++ * ax;
        else if (op == DIV) ax = *sp++ / ax;
        else if (op == MOD) ax = *sp++ % ax;


        else if (op == EXIT) { printf("exit(%d)", *sp); return *sp;}
        else if (op == OPEN) { ax = open((char *)sp[1], sp[0]); }
        else if (op == CLOS) { ax = close(*sp);}
        else if (op == READ) { ax = read(sp[2], (char *)sp[1], *sp); }
        else if (op == PRTF) { tmp = sp + pc[1]; ax = printf((char *)tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5], tmp[-6]); }
        else if (op == MALC) { ax = (int)malloc(*sp);}
        else if (op == MSET) { ax = (int)memset((char *)sp[2], sp[1], *sp);}
        else if (op == MCMP) { ax = memcmp((char *)sp[2], (char *)sp[1], *sp);}
        else {
            // printf("unknown instruction:%d\n", op);
            return 0;
        }
    }
    return 0;
}

int main(int argc,char **argv) {
    int *tmp;
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
    if (!(STACK = malloc(poolSize))) {
        printf("could not malloc(%d) for stack\n", poolSize);
        return -1;
    }
    memset(STACK,0,poolSize);
    memset(TEXTSEGMENT,0,poolSize);
    memset(DATASEGMENT,0,poolSize);
    memset(symbols,0,poolSize);
    bp = sp = (int *)((int)STACK + poolSize);
    ax = 0;

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
        //ToDo : error (编译器问题？？？，不用cmake用gcc就解决了） 64位机的指针是8位，32位机是4位，转换为int（4位）会出现问题
        // // printf("match\n");
        // char *str1 =malloc(poolSize);
        // int tmp = (int)LP;
        // memcpy(str1,(char*)tmp,src - LP);
        // printf("str1:%s\n",str1);

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

    src[fileLength] = 0;
    close(file);
    program();

    if (!(pc = (int *)idmain[Value])) {
        printf("main() not defined\n");
        return -1;
    }
    debug("init pc\n");
    // setup stack
    if(!(sp = (int *)((int)STACK + poolSize))) {
        printf("could not malloc(%d) for stack\n", poolSize);
        return -1;
    }

    *--sp = EXIT;
    *--sp = PUSH;
    tmp = sp;
    *--sp = argc;
    *--sp = (int)argv;
    *--sp = (int)tmp;

    debug("call eval()\n");
    debug("-----------------\n");
    return eval();
}
