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
#include <stdbool.h>
#include <unistd.h>
#define BASE 147
#define debug printf
int poolSize = 256 * 1024;
char *idName;
int token;
int token_val;
char *src, *old_src;
int poolSize;
int line;
int *text,
    *old_text,
    *stack;
char *data;
int *pc, *bp, *sp, ax, cycle;
int *current_id,
    *symbols;
int *idmain;

enum {
  Num = 128, Fun, Sys, Glo, Loc, Id,
  Char, Else, Enum, If, Int, Return, Sizeof, While,
  Assign, Cond, Lor, Lan, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge, Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

enum {Token, Hash, Name, Type, Class, Value, BType, BClass, BValue, IdSize};

enum { CHAR, INT, PTR }; //basetype = = type % base

int basetype;
int expr_type;
int index_of_bp;

struct quad {
    int addr;
    char *op;
    char *arg1;
    char *arg2;
    char *result;
};
struct quad *q,*qStart;
int quadSize = 0;
int quadPos = 0;
int place,offset;
int loc_place,loc_offset;
char *arg1,*arg2,*result;
bool isArray = false;
void next() {

    char *last_pos;
    int hash;

    while (token = *src) {
        ++src;

        // parse token here
        if (token == '\n') {
            ++line;
        }
        else if (token == '#') {
            while (*src != 0 && *src != '\n') {
                src++;
            }
        }
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {

            free(idName);
            idName = NULL;
            idName = malloc(1024);

            // parse identifier
            last_pos = src - 1;
            hash = token;

            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z') || (*src >= '0' && *src <= '9') || (*src == '_')) {
                hash = hash * BASE + *src;
                src++;
            }

            memcpy(idName,last_pos,src - last_pos);
            idName[src - last_pos] = '\0';

            // look for existing identifier, linear search
            current_id = symbols;
            while (current_id[Token]) {
                if (current_id[Hash] == hash && !memcmp((char *)current_id[Name], last_pos, src - last_pos)) {
                    //found one, return
                    token = current_id[Token];
                    return;
                }
                current_id = current_id + IdSize;
            }


            // store new ID
            current_id[Name] = (int)last_pos;
            current_id[Hash] = hash;
            token = current_id[Token] = Id;
            return;
        }
        else if (token >= '0' && token <= '9') {
            // parse number, three kinds: dec(123) hex(0x123) oct(017)
            token_val = token - '0';
            if (token_val > 0) {
                // dec, starts with [1-9]
                while (*src >= '0' && *src <= '9') {
                    token_val = token_val*10 + *src++ - '0';
                }
            } else {
                // starts with 0
                if (*src == 'x' || *src == 'X') {
                    //hex
                    token = *++src;
                    while ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f') || (token >= 'A' && token <= 'F')) {
                        token_val = token_val * 16 + (token & 15) + (token >= 'A' ? 9 : 0);
                        token = *++src;
                    }
                } else {
                    // oct
                    while (*src >= '0' && *src <= '7') {
                        token_val = token_val*8 + *src++ - '0';
                    }
                }
            }

            token = Num;
            return;
        }
        else if (token == '"' || token == '\'') {
            // parse string literal, currently, the only supported escape
            // character is '\n', store the string literal into data.
            last_pos = data;
            while (*src != 0 && *src != token) {
                token_val = *src++;
                if (token_val == '\\') {
                    // escape character
                    token_val = *src++;
                    if (token_val == 'n') {
                        token_val = '\n';
                    }
                }

                if (token == '"') {
                    *data++ = token_val;
                }
            }

            src++;
            // if it is a single character, return Num token
            if (token == '"') {
                token_val = (int)last_pos;
            } else {
                token = Num;
            }

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
        }
        else if (token == '=') {
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
                token = Ne;
            }
            return;
        }
        else if (token == '<') {
            // parse '<=', '<<' or '<'
            if (*src == '=') {
                src ++;
                token = Le;
            } else if (*src == '<') {
                src ++;
                token = Shl;
            } else {
                token = Lt;
            }
            return;
        }
        else if (token == '>') {
            // parse '>=', '>>' or '>'
            if (*src == '=') {
                src ++;
                token = Ge;
            } else if (*src == '>') {
                src ++;
                token = Shr;
            } else {
                token = Gt;
            }
            return;
        }
        else if (token == '|') {
            // parse '|' or '||'
            if (*src == '|') {
                src ++;
                token = Lor;
            } else {
                token = Or;
            }
            return;
        }
        else if (token == '&') {
            // parse '&' and '&&'
            if (*src == '&') {
                src ++;
                token = Lan;
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

void match(int tk) {
    if (token == tk) {
        next();
    } else {
        debug("%d: expected token: %d\n", line, tk);
        debug("get token: %d instead\n", token);
        exit(-1);
    }
}
void advance(int tk) {
    match(tk);
}
void start();
void expr();
void term();
void rest5();
void unary();
void rest6();
void factor();

void bool_();
void equality();
void rest4();
void rel();
void rop_expr();
void stmts();
void rest0();
void stmt();
void loc();
void resta();
void elist();
void rest1();
int numTemp = 0;
int newTemp(){
    return ++numTemp;
}
void program() {
    // get next token
    next();
    while (token > 0) {
        start();
    }
}

int main(int argc, char **argv)
{
    int i, fd;
    int *tmp;

    argc--;
    argv++;

    line = 1;

    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    // allocate memory for virtual machine
    if (!(text = old_text = malloc(poolSize))) {
        printf("could not malloc(%d) for text area\n", poolSize);
        return -1;
    }
    if (!(data = malloc(poolSize))) {
        printf("could not malloc(%d) for data area\n", poolSize);
        return -1;
    }
    if (!(stack = malloc(poolSize))) {
        printf("could not malloc(%d) for stack area\n", poolSize);
        return -1;
    }
    if (!(symbols = malloc(poolSize))) {
        printf("could not malloc(%d) for symbol table\n", poolSize);
        return -1;
    }
    if (!(q = malloc(poolSize))) {
        printf("could not malloc(%d) for quad\n", poolSize);
        return -1;
    }
    qStart = q;
    memset(q,0,poolSize);
    memset(text, 0, poolSize);
    memset(data, 0, poolSize);
    memset(stack, 0, poolSize);
    memset(symbols, 0, poolSize);
    bp = sp = (int *)((int)stack + poolSize);
    ax = 0;

    src = "char else enum if int return sizeof while "
          "open read close printf malloc memset memcmp exit void main";

     // add keywords to symbol table
    i = Char;
    while (i <= While) {
        next();
        current_id[Token] = i++;
    }

    // add library to symbol table
    i = OPEN;
    while (i <= EXIT) {
        next();
        current_id[Class] = Sys;
        current_id[Type] = INT;
        current_id[Value] = i++;
    }

    next(); current_id[Token] = Char; // handle void type
    next(); idmain = current_id; // keep track of main


    // read the source file
    if ((fd = open(*argv, 0)) < 0) {
        printf("could not open(%s)\n", *argv);
        return -1;
    }

    if (!(src = old_src = malloc(poolSize))) {
        printf("could not malloc(%d) for source area\n", poolSize);
        return -1;
    }
    // read the source file
    if ((i = read(fd, src, poolSize-1)) <= 0) {
        printf("read() returned %d\n", i);
        return -1;
    }
    src[i] = 0; // add EOF character
    close(fd);

    program();

    for(int i = 1;i <= quadSize;i++){
        printf("%d : %s %s %s %s\n",qStart[i].addr,qStart[i].op,qStart[i].arg1,qStart[i].arg2,qStart[i].result);
    }

    return 0;
}

void start() {
    debug("start -> stmts\n");
    stmts();

}
void expr() {
    debug("expr -> term rest5\n");
    term();
    rest5();
}
void term() {
    debug("term -> unary rest6\n");
    unary();
    rest6();
}
void rest5() {
    if (token == Add) {
        debug("rest5 -> + term rest5\n");

        // free(result);
        // result = NULL;
        // result = malloc(1024);

        struct quad q1;
        {

            q1.op = "+";
            // q1.arg1 = "arg1";
            // q1.arg2 = "arg2";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = malloc(1024);

            q1.result = malloc(1024);

            quadSize++;
        }

        advance(token);
        term();
        {
            *q1.result = 't';
            q1.result[1]= '\0';
            char *tmp = malloc(1024);
            strcat(q1.result,itoa(newTemp(),tmp,10));
            q1.result[strlen(tmp)+1]= '\0';

            free(tmp);
            tmp = NULL;
            q1.addr = ++quadPos;
            *++q = q1;
            memcpy(q1.arg2,result,strlen(result));
            q1.arg2[strlen(result)]= '\0';


        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q1.result,strlen(q1.result));
            result[strlen(q1.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back
        rest5();

    }
    else if (token == Sub) {
        debug("rest5 -> - term rest5\n");

        // free(result);
        // result = NULL;
        // result = malloc(1024);

        char *tmp = malloc(1024);

        struct quad q1;
        {

            q1.op = "-";
            // q1.arg1 = "arg1";
            // q1.arg2 = "arg2";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = malloc(1024);

            q1.result = malloc(1024);

            quadSize++;
        }
        advance(token);
        term();
        {
            *q1.result = 't';
            q1.result[1]= '\0';
            strcat(q1.result,itoa(newTemp(),tmp,10));
            q1.result[strlen(tmp)+1]= '\0';
            q1.addr = ++quadPos;
            *++q = q1;

            memcpy(q1.arg2,result,strlen(result));
            q1.arg2[strlen(result)]= '\0';
            free(tmp);
            tmp = NULL;
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q1.result,strlen(q1.result));
            result[strlen(q1.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back
        rest5();
    }
    else {
        debug("rest5 -> epsilon\n");
    }
}
void unary() {
    debug("unary -> factor\n");
    factor();
}
void rest6() {
    if (token == Mul) {
        debug("rest6 -> * unary rest6\n");

        // free(result);
        // result = NULL;
        // result = malloc(1024);

        struct quad q1;
        {

            q1.op = "*";
            // q1.arg1 = "arg1";
            // q1.arg2 = "arg2";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';
            q1.arg2 = malloc(1024);
            q1.result = malloc(1024);

            quadSize++;
        }

        advance(token);
        unary();
        {
           *q1.result = 't';
            q1.result[1]= '\0';
            char *tmp = malloc(1024);
            strcat(q1.result,itoa(newTemp(),tmp,10));
            q1.result[strlen(tmp)+1]= '\0';
            free(tmp);
            tmp = NULL;
            q1.addr = ++quadPos;
            *++q = q1;
            memcpy(q1.arg2,result,strlen(result));
            q1.arg2[strlen(result)]= '\0';
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q1.result,strlen(q1.result));
            result[strlen(q1.result)]= '\0';
        }//write back
        rest6();


    }else if (token == Div) {
        debug("rest6 -> / unary rest6\n");

        // free(result);
        // result = NULL;
        // result = malloc(1024);

        struct quad q1;
        {

            q1.op = "/";
            // q1.arg1 = "arg1";
            // q1.arg2 = "arg2";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = malloc(1024);
            q1.result = malloc(1024);



            quadSize++;
        }
        advance(token);
        unary();
        {
            *q1.result = 't';
            q1.result[1]= '\0';
            char *tmp = malloc(1024);
            strcat(q1.result,itoa(newTemp(),tmp,10));
            q1.result[strlen(tmp)+1]= '\0';
            free(tmp);
            tmp = NULL;
            q1.addr = ++quadPos;
            *++q = q1;
            memcpy(q1.arg2,result,strlen(result));
            q1.arg2[strlen(result)]= '\0';
        }
        {
            free (result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q1.result,strlen(q1.result));
            result[strlen(q1.result)]= '\0';
        }//write back

        rest6();
    }
    else {
        debug("rest6 -> epsilon\n");
    }
}
void factor() {
    if (token == '(') {
        debug("factor -> ( expr )\n");
        advance(token);
        expr();
        advance(')');
    }
    else if (token == Num) {
        debug("factor -> num\n");
        advance(Num);

        free(result);
        result = NULL;
        result = malloc(1024);
        itoa(token_val,result,10);
        result[strlen(result)]= '\0';

    }
    else {
        debug("factor -> loc\n");
        loc();
    }
}
void bool_() {
    debug("bool -> equality\n");
    equality();
}
void equality() {
    debug("equality -> rel rest4\n");
    rel();
    rest4();
}
void rest4() {
    if (token == Eq) {
        debug("rest4 -> == rel rest4\n");
        advance(Eq);
        rel();

    }
    else if (token == Ne) {
        debug("rest4 -> != rel rest4\n");
        advance(Ne);
        rel();

    }
    else {
        debug("rest4 -> epsilon\n");
    }
}
void rel() {
    debug("rel -> expr rop_expr\n");
    expr();
    rop_expr();
}
void rop_expr() {
    if (token == Lt) {
        debug("rop_expr -> < expr\n");
        /*
        *           j< arg1 , arg2 , L1
        *           = ,0  , _ , t1
        *           j , _ , _ , L2
        *    L1 :   = ,1  , _ , t1
        *    L2 :   next_expr
        **/

        struct quad q1;
        {

            q1.op = "j<";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = malloc(1024);
            q1.result = malloc(1024);

            quadSize++;
        }
        advance(Lt);
        expr();
        {
            q1.addr = ++quadPos;
            *++q = q1;
            memcpy(q1.arg2,result,strlen(result));
            q1.arg2[strlen(result)]= '\0';
            int q1Addr = quadPos + 3;
            itoa(q1Addr, q1.result, 10);
        }

        struct quad q2;
        {
            q2.op = "=";
            q2.arg1 = "0";
            q2.arg2 = "_";
            q2.result = malloc(1024);

            quadSize++;
        }

        int ans = newTemp();
        {
            *q2.result = 't';
            q2.result[1]= '\0';
            char *tmp = malloc(1024);
            strcat(q2.result,itoa(ans,tmp,10));
            q2.result[strlen(tmp)+1]= '\0';

            free(tmp);
            tmp = NULL;
            q2.addr = ++quadPos;
            *++q = q2;
        }
        struct quad q3;
        {
            q3.op = "j";
            q3.arg1 = "_";
            q3.arg2 = "_";
            q3.result = malloc(1024);
            int q3Addr = quadPos + 3;
            itoa(q3Addr, q3.result, 10);
            quadSize++;
            q3.addr = ++quadPos;
            *++q = q3;
        }

        struct quad q4;
        {
            q4.op = "=";
            q4.arg1 = "1";

            q4.arg2 = "_";
            quadSize++;

        }
        {
            q4.result=malloc(1024);
            *q4.result = 't';
            q4.result[1]= '\0';
            char *tmp = malloc(1024);
            strcat(q4.result,itoa(ans,tmp,10));
            q4.result[strlen(tmp)+1]= '\0';

            free(tmp);
            tmp = NULL;
            q4.addr = ++quadPos;
            *++q = q4;
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q2.result,strlen(q2.result));
            result[strlen(q2.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back

    } else if (token == Le) {
        debug("rop_expr -> <= expr\n");
        advance(Le);
        expr();

    } else if (token == Gt) {
        debug("rop_expr -> > expr\n");
        advance(Gt);
        expr();

    } else if (token == Ge) {
        debug("rop_expr -> >= expr\n");
        advance(Ge);
        expr();

    } else {
        debug("rop_expr -> epsilon\n");
    }
}
void stmts() {
    debug("stmts -> stmt rest0\n");
    stmt();
    rest0();
}
void rest0() { //somthing wrong
    if (token == ';' || token == Id || token == If || token == While) {
        debug("rest0 -> ; stmt rest0\n");
        if(token==';')advance(';');
        stmt();
        rest0();

    } else {
        debug("rest0 -> epsilon\n");
    }
}
void stmt() {
    free(result);
    result = NULL;
    result = malloc(1024);
    int tmp=0;
    // int *L1,*L2;
    if (token == Id) {

        debug("stmt -> loc = expr ;\n");
        bool flag = false;

        char *_result = malloc(1024);
        memcpy(_result, idName, strlen(idName));
        _result[strlen(idName)]= '\0';
        struct quad q1;
        {

            q1.op = "=";
            // q1.arg1 = "arg1";
            q1.arg1 = malloc(1024);


            q1.arg2 = "_";
            q1.result = _result;


            quadSize++;
        }

        loc();
        int _place = loc_place;
        int _offset = loc_offset;
        if(isArray){
            flag = true;
            q1.op = "[]=";

        }
        advance(Assign);
        expr();
        {

            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';
            if(flag){

                char * brakResult = malloc(1024);
                *brakResult = 't';
                char *tmp = malloc(1024);
                itoa(_place,tmp,10);
                strcat(brakResult,tmp);
                brakResult[strlen(tmp)+1] = '\0';
                strcat(brakResult,"[t");
                tmp = itoa(_offset,tmp,10);
                strcat(brakResult,tmp);
                strcat(brakResult,"]");
                memcpy(q1.result,brakResult,strlen(brakResult));
                q1.result[strlen(brakResult)+1] = '\0';
            }
            q1.addr = ++quadPos;
            *++q = q1;
        }
        isArray = false;

        advance(';');

    }
    else if (token == If) {
        /*if (<cond>)           <cond>
         *                      JZ L1
         *<true statement>      <true statement>
         *else:                  JMP L2
         *L1:                   L1:
         *<false statement>     <false statement>
         *L2:                   L2:
         */
        debug("stmt -> if ( bool ) m stmt n else m stmt\n");
        advance(If);

        advance('(');
        bool_();
        advance(')');

        struct quad q1;
        {
            q1.op = "JZ";
            // q1.arg1 = "arg";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = "_";
            q1.result = malloc(1024);

            q1.addr = ++quadPos;
            *++q = q1;
            quadSize++;
        }
        stmt();

        advance(Else);
        struct quad q2;
        {

            q2.op = "JMP";
            q2.arg1 = "_";
            q2.arg2 = "_";
            q2.result=malloc(1024);

            int q1Addr = quadPos + 2;
            itoa(q1Addr, q1.result, 10);

            q2.addr = ++quadPos;
            *++q = q2;
            quadSize++;
        }

        stmt();

        int q2Addr = quadPos + 1;
        itoa(q2Addr, q2.result, 10);

    }
    else if (token == While) {

        /*L1:                L1:
         *while(<cond>)      <cond>
         *                   JZ L2
         *<statement>        <statement>
         *                   JMP L1
         *L2:                L2:
         */

        debug("stmt -> while ( m bool ) m stmt\n");
        advance(While);

        tmp = quadPos+1;
        advance('(');
        bool_();
        advance(')');

        struct quad q1;
        {

            q1.op = "JZ";
            // q1.arg1 = "arg1";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';

            q1.arg2 = "_";
            q1.result = malloc(1024);

            q1.addr = ++quadPos;
            *++q = q1;
            quadSize++;
        }

        stmt();
        struct quad q2;
        {
            q2.addr = ++quadPos;
            q2.op = "JMP";
            q2.arg1 = "_";
            q2.arg2 = "_";
            q2.result = malloc(1024);
            itoa(tmp, q2.result, 10);
            *++q = q2;
            quadSize++;
        }
        // tmp = ++quadPos;
        itoa(quadPos + 1, q1.result, 10);


    }
    else {
        debug("%d: syntax error1\n", line);
        exit(-1);
    }
    // L1 = NULL;
    // L2 = NULL;
}
char* arrName;
void loc() {
    if (token == Id) {
        debug("loc -> id resta\n");
        advance(Id);
        bool flag = false;
        if(token == Brak){
            flag = true;
        }
        if(flag) {
            isArray = true;
            free(arrName);
            arrName = NULL;
            arrName = malloc(1024);
            memcpy(arrName,idName,strlen(idName));
            arrName[strlen(idName)]= '\0';
        }
        int glo_place = place;
        int glo_offset = offset;
        resta();
        if(flag) {

            struct quad q1;
            {
                q1.addr = ++quadPos;
                q1.op = "=[]";
                q1.arg1 = malloc(1024);
                *q1.arg1 = 't';
                q1.arg1[1] = '\0';
                char *tmp = malloc(1024);
                itoa(place,tmp,10);
                strcat(q1.arg1,tmp);
                q1.arg1[strlen(tmp)+1] = '\0';
                strcat(q1.arg1,"[t");
                tmp = itoa(offset,tmp,10);
                strcat(q1.arg1,tmp);
                strcat(q1.arg1,"]");

                q1.arg2 = "_";
                q1.result = malloc(1024);
                *q1.result = 't';
                q1.result[1] = '\0';
                tmp = itoa(newTemp(),tmp,10);
                strcat(q1.result,tmp);
                q1.result[strlen(tmp)+1] = '\0';

                *++q = q1;
                quadSize++;

                loc_place = place;
                loc_offset = offset;
            }
            {
                free(result);
                result = NULL;
                result = malloc(1024);
                memcpy(result,q1.result,strlen(q1.result));
                result[strlen(q1.result)]= '\0';
            }//write back
        }
        place = glo_place;
        offset = glo_offset;
        flag = false;

    } else {
        printf("%d: syntax error2\n", line);
        exit(-1);
    }
}
void resta() {
    if (token == Brak) {
        debug("resta -> [ elist ]\n");
        advance(Brak);
        elist();
        char* id = malloc(1024);
        memcpy(id,arrName,strlen(arrName));
        id[strlen(arrName)]= '\0';
        struct quad q1;// - arrName C result
        {
            q1.op = "-";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,id,strlen(id));
            q1.arg1[strlen(id)]= '\0';
            q1.arg2 = "C";

            q1.result = malloc(1024);
            q1.result[0] = 't';
            q1.result[1] = '\0';
            char* tmp = malloc(1024);

            place = newTemp();
            itoa(place,tmp,10);
            strcat(q1.result,tmp);
            q1.result[strlen(tmp)+1] = '\0';

            q1.addr = ++quadPos;
            *++q = q1;
            quadSize++;
        }
       // {
       //      free(result);
       //      result = NULL;
       //      result = malloc(1024);
       //      memcpy(result,q1.result,strlen(q1.result));
       //      result[strlen(q1.result)]= '\0';
       //      // debug("write result: %s\n",result);
       //  }//write back

        struct quad q2;// * result offset result
        {
            q2.op = "*";
            q2.arg1 = malloc(1024);
            memcpy(q2.arg1,result,strlen(result));
            q2.arg1[strlen(result)]= '\0';
            q2.arg2 = "w";

            q2.result = malloc(1024);
            q2.result[0] = 't';
            q2.result[1] = '\0';
            char* tmp = malloc(1024);

            offset = newTemp();
            itoa(offset,tmp,10);
            strcat(q2.result,tmp);
            q2.result[strlen(tmp)+1] = '\0';

            q2.addr = ++quadPos;
            *++q = q2;
            quadSize++;
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q2.result,strlen(q2.result));
            result[strlen(q2.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back

        advance(']');


    } else {
        debug("resta -> epsilon\n");

        free(result);
        result = NULL;
        result = malloc(1024);
        memcpy(result,idName,strlen(idName));
        result[strlen(idName)]= '\0';
    }
}
void elist() {
    debug("elist -> expr rest1\n");
    int ndim = 1;
    expr();
    rest1(ndim);
}
void rest1(int ndim) {
    if (token == ',') {
        debug("rest1 -> , expr rest1\n");
        advance(',');
        struct quad q1;// * result limit(result,ndim) result
        {
            q1.op = "*";
            q1.arg1 = malloc(1024);
            memcpy(q1.arg1,result,strlen(result));
            q1.arg1[strlen(result)]= '\0';
            q1.arg2 = malloc(1024);
            memcpy(q1.arg2,"n",6);
            q1.arg2[1] = '\0';
            char* tmp = malloc(1024);
            itoa(++ndim,tmp,10);
            strcat(q1.arg2,tmp);
            q1.arg2[strlen(tmp)+1] = '\0';

            q1.result = malloc(1024);
            q1.result[0] = 't';
            q1.result[1] = '\0';
            char* tmp1 = malloc(1024);
            itoa(newTemp(),tmp1,10);
            strcat(q1.result,tmp1);
            q1.result[strlen(tmp1)+1] = '\0';

            q1.addr = ++quadPos;
            *++q = q1;
            quadSize++;
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q1.result,strlen(q1.result));
            result[strlen(q1.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back
        expr();
        struct quad q2;//+ result expr result
        {
            q2.op = "+";
            q2.arg1 = malloc(1024);
            memcpy(q2.arg1,q1.result,strlen(q1.result));
            q2.arg1[strlen(q1.result)]= '\0';
            q2.arg2 = malloc(1024);
            memcpy(q2.arg2,result,strlen(result));
            q2.arg2[strlen(result)]= '\0';

            q2.result = malloc(1024);
            memcpy(q2.result,q1.result,strlen(q1.result));
            q2.result[strlen(q1.result)]= '\0';

            q2.addr = ++quadPos;
            *++q = q2;
            quadSize++;
        }
        {
            free(result);
            result = NULL;
            result = malloc(1024);
            memcpy(result,q2.result,strlen(q2.result));
            result[strlen(q2.result)]= '\0';
            // debug("write result: %s\n",result);
        }//write back
        rest1(ndim);

    } else {
        debug("rest1 -> epsilon\n");
    }
}
