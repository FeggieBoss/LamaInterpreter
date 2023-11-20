#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <cassert>
#include <algorithm>
#include <stack>
#include <cstring>

extern "C" {
  #include "runtime/runtime.h"
}
void *__start_custom_data;
void *__stop_custom_data;

struct Value;

typedef unsigned char Byte;
typedef int Number;
typedef char* String;
typedef Value* RefValue;
struct Array {
    unsigned int sz;
    Value* ptr;
};
struct Sexp {
    unsigned int sz;
    const char *name;
    Value *ptr;
};
struct Fun {
  unsigned int addr;
  unsigned int n;
  Value *ptr;
};

struct Value {
    Value() {
      type = "Byte";
    }
    Byte byte;
    Number number;
    String string;
    Array array;
    Sexp sexp;
    RefValue ref_value;
    Fun fun;
    const char* type;
};

class Stack {
public:
  unsigned int global_ct;
  Value *globals;
  char *ptr;
  char *sp, *lp, *fp, *cp;

  Stack(const unsigned int mx_size, const unsigned int _global_ct) {
    global_ct = _global_ct;
    cp = sp = lp = fp = ptr = new char[mx_size];
    globals = new Value[_global_ct];
  }

  void setup(const unsigned int args_ct, const unsigned int locals_ct, const unsigned int captured_ct = 0) {
    char *prev_sp = sp-(captured_ct + args_ct + 1)*sizeof(Value), *prev_lp = lp, *prev_fp = fp, *prev_cp = cp;
    
    char* cur = prev_sp;

    // CAPTURED
    cur += captured_ct*sizeof(Value);
    cp = cur;

    // PREV_IP
    cur += sizeof(Value);

    // ARGS
    cur += args_ct * sizeof(Value);
    // for(unsigned int i=0;i<args_ct/2;++i) { // ARGS reverse
    //   auto dop = Value();
    //   memcpy(reinterpret_cast<void*>(&dop), reinterpret_cast<void*>(prev_sp + i*sizeof(Value)), sizeof(Value));
    //   memcpy(reinterpret_cast<void*>(prev_sp + i*sizeof(Value)), reinterpret_cast<void*>(sp - (i+1)*sizeof(Value)), sizeof(Value));
    //   memcpy(reinterpret_cast<void*>(sp - (i+1)*sizeof(Value)), reinterpret_cast<void*>(&dop), sizeof(Value));
    // }

    // LOCALS
    lp = cur;
    for(unsigned int i=0;i<locals_ct;++i) { 
      auto dop = Value();
      memcpy(reinterpret_cast<void*>(cur), reinterpret_cast<void*>(&dop), sizeof(Value));
      cur += sizeof(Value);
    }

    *reinterpret_cast<char**>(cur) = prev_cp; // prevCP
    cur += sizeof(char*);

    *reinterpret_cast<char**>(cur) = prev_lp; // prevLP
    cur += sizeof(char*);

    *reinterpret_cast<char**>(cur) = prev_fp; // prevFP
    cur += sizeof(char*);

    fp = cur;
    
    *reinterpret_cast<char**>(cur) = prev_sp; // prevSP
    cur += sizeof(char*);
    
    sp = cur;
  }

  ~Stack() {
    delete[] ptr;
    delete[] globals;
  }

  Value& top() {
    return *reinterpret_cast<Value*>(sp-sizeof(Value));
  }

  Value* get_cpt(unsigned int x) {
    return reinterpret_cast<Value*>(cp-(x+1)*sizeof(Value));
  }

  Value* get_arg(unsigned int x) {
    return reinterpret_cast<Value*>(lp-(x+1)*sizeof(Value));
  }

  Value* get_loc(unsigned int x) {
    return reinterpret_cast<Value*>(lp+x*sizeof(Value));
  }

  void push(Value &&v) {
    auto dop = v;
    memcpy(reinterpret_cast<void*>(sp), reinterpret_cast<void*>(&dop), sizeof(Value));
    sp += sizeof(Value);
  }

  void push(Value &v) {
    auto dop = v;
    memcpy(reinterpret_cast<void*>(sp), reinterpret_cast<void*>(&dop), sizeof(Value));
    sp += sizeof(Value);
  }

  void pop(unsigned int _sizeof = sizeof(Value)) {
    sp-=_sizeof;
  }
};

void SET_VALUE(Value *v, Value &x) {
  if(x.type == "RefValue") {
      *v = *x.ref_value;
  } else {
      *v = x;
  }
}

void debug_value(Value *const v) {
  printf("type is %s, value : ", v->type);
  
  if(v->type=="Byte") {
      printf("Byte=%u", (unsigned int)v->byte);
  } else if (v->type=="Number") {
      printf("Number=%d", v->number);
  } else if (v->type=="String") {
      printf("String=%s", v->string);
  } else if (v->type=="Array") {
      printf("Array=[");
      for(int i=0;i<v->array.sz;++i) {
          debug_value(v->array.ptr+i);
          if(i!=v->array.sz-1) printf(" , ");
      }
      printf("]");
  } else if (v->type=="Sexp") {
      printf("Sexp(%s)=[", v->sexp.name);
      for(int i=0;i<v->sexp.sz;++i) {
          debug_value(v->sexp.ptr+i);
          if(i!=v->sexp.sz-1) printf(" , ");
      }
      printf("]");
  } else if (v->type=="RefValue") {
      printf("(RefValue)");
      debug_value(v->ref_value);
  } else if (v->type=="Fun") {
      printf("Fun(%d) Captured=[", v->fun.addr);
      for(int i=0;i<v->fun.n;++i) {
          debug_value(v->fun.ptr+i);
          if(i!=v->fun.n-1) printf(" , ");
      }
      printf("]");
  } else {
      printf("ERROR\n");
      exit(1);
  }
}

void debug_stack(const Stack &st) {
  printf("\tGLOBALS: [");
  int it=0;
  while(it<st.global_ct) {
    debug_value(&st.globals[it]);
    if(it!=st.global_ct-1) printf(" , ");
    ++it;
  }
  printf("]\n");

  char *sp=st.sp,*lp=st.lp,*fp=st.fp, *cp=st.cp;

  char *prev_fp = nullptr, *prev_sp = nullptr, *prev_lp = nullptr, *prev_cp = nullptr;
  while(prev_fp != st.ptr) {
    prev_cp = *reinterpret_cast<char**>(fp-3*sizeof(char*));
    prev_lp = *reinterpret_cast<char**>(fp-2*sizeof(char*));
    prev_fp = *reinterpret_cast<char**>(fp-sizeof(char*));
    prev_sp = *reinterpret_cast<char**>(fp);

    char *cur = prev_sp;
    printf("\tCAPTURED: [");
    while(cur!=cp) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur+=sizeof(Value);
    }
    printf("]\n");

    cur = lp;
    printf("\tARGS: [");
    while(cur!=cp+sizeof(Value)) {
      cur-=sizeof(Value);
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
    }
    printf("]\n");

    cur = lp;
    printf("\tLOCALS: [");
    while(cur!=(fp-3*sizeof(char*))) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur+=sizeof(Value);
    }
    printf("]\n");


    cur = fp;
    cur += sizeof(char*);
  
    printf("\tOPS: [");
    while(cur!=sp) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur+=sizeof(Value);
    }
    printf("]\n");

    sp = prev_sp;
    fp = prev_fp;
    lp = prev_lp;
    cp = prev_cp;
  }
}

Value POP_VALUE(Stack &st) {
    Value v = st.top();
    st.pop();
    if(v.type=="RefValue") v = *v.ref_value;
    return v;
}

RefValue POP_REFVALUE(Stack &st) {
    Value v = st.top();
    st.pop();
    if(v.type!="RefValue") {
        printf("ERROR POP_REFVALUE");
        exit(1);
    }
    return v.ref_value;
}

void PUSH_BYTE(Stack &st, Byte b) {
    st.push(Value());
    st.top().type = "Byte";
    st.top().byte = b;
}

void PUSH_NUMBER(Stack &st, Number n) {
    st.push(Value());
    st.top().type = "Number";
    st.top().number = n;
}

void PUSH_STRING(Stack &st, String s) {
    st.push(Value());
    st.top().type = "String";
    st.top().string = s;
}

void PUSH_ARRAY(Stack &st, Array a) {
    st.push(Value());
    st.top().type = "Array";
    st.top().array = a;
}

void PUSH_SEXP(Stack &st, Sexp s) {
    st.push(Value());
    st.top().type = "Sexp";
    st.top().sexp = s;
}

void PUSH_REFVALUE(Stack &st, RefValue r) {
    if(r->type=="RefValue") {
      st.push(*r);
    } else {
      st.push(Value());
      st.top().type = "RefValue";
      st.top().ref_value = r;
    }
}

void PUSH_FUN(Stack &st, Fun fun) {
    st.push(Value());
    st.top().type = "Fun";
    st.top().fun = fun;
}

/* The unpacked representation of bytecode file */
typedef struct {
  char *string_ptr;              /* A pointer to the beginning of the string table */
  int  *public_ptr;              /* A pointer to the beginning of publics table    */
  char *code_ptr;                /* A pointer to the bytecode itself               */
  char *end_code_ptr;            /* A pointer to the end of bytecode               */
  int  *global_ptr;              /* A pointer to the global area                   */
  int   stringtab_size;          /* The size (in bytes) of the string table        */
  int   global_area_size;        /* The size (in words) of global area             */
  int   public_symbols_number;   /* The number of public symbols                   */
  char  *buffer;               
} bytefile;


char* get_string (bytefile *f, int pos) {
  return &f->string_ptr[pos];
}

void read_file (char *name, bytefile &bf) {
  std::ifstream f(name, std::ios::binary | std::ios::ate);

  long size = 1ULL*f.tellg() - 3*sizeof(int);
  f.seekg(0, f.beg);
  
  bf = bytefile{};
  bf.buffer = new char[size];

  f.read(reinterpret_cast<char*>(&bf.stringtab_size), sizeof(bf.stringtab_size));
  f.read(reinterpret_cast<char*>(&bf.global_area_size), sizeof(bf.global_area_size));
  f.read(reinterpret_cast<char*>(&bf.public_symbols_number), sizeof(bf.public_symbols_number));
  f.read(bf.buffer, size);
  
  bf.string_ptr  = &bf.buffer [bf.public_symbols_number * 2 * sizeof(int)];
  bf.public_ptr  = (int*) bf.buffer;
  bf.code_ptr    = &bf.string_ptr [bf.stringtab_size];
  bf.global_ptr  = new int[bf.global_area_size];
  bf.end_code_ptr = bf.buffer + size;

  f.close();
}

template< typename... Args >
std::string string_sprintf(const char* format, Args... args ) {
  int length = std::snprintf(nullptr, 0, format, args...);
  assert( length >= 0 );

  char* buf = new char[length + 1];
  std::snprintf(buf, length + 1, format, args...);

  std::string str( buf );
  delete[] buf;
  return str;
}

void parse(bytefile *bf, FILE *f = stdout) {
# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (bf, INT)
# define FAIL   fprintf (stderr, "ERROR: invalid opcode %d-%d\n", h, l)
  
  Stack st = Stack(100000, bf->global_area_size);

  st.setup(0, 0);

  PUSH_NUMBER(st, (bf->end_code_ptr - bf->code_ptr) - 1); // sizeof(STOP) = 1

  st.push(Value());
  st.push(Value());

  char *ip = bf->code_ptr;
  do {
    char x = BYTE,
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

                        int ______;
                        //fscanf(stdin, "%d", &______);
    fprintf (f, "0x%.8x:\t", ip-bf->code_ptr-1);

    auto switch_lambda = [&st, &bf, &ip, h, l]() {
      static const char* ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
      static const char* pats[] = {"=str", "#str", "#array", "#sexp", "#box", "#val", "#fun"};
      static const char* lds [] = {"LD", "LDA", "ST"};

      switch (h) {
      case 15: { // STOP
        return std::string("STOP");
      }
        
      case 0: { // BINOP
        Number b, a, c;
        b = POP_VALUE(st).number;
        a = POP_VALUE(st).number;

        switch (l) {
        case 1: { // +
            c = a + b;
            break;
        }
        case 2: { // -
            c = a - b;
            break;

        }
        case 3: { // *
            c = a * b;
            break;

        }
        case 4: { // /
            c = a / b;
            break;

        }
        case 5: { // %
            c = a % b;
            break;

        }
        case 6: { // <
            c = int(a < b);
            break;

        }
        case 7: { // <=
            c = int(a <= b);
            break;

        }
        case 8: { // >
            c = int(a > b);
            break;

        }
        case 9: { // >=
            c = int(a >= b);
            break;
        }
        case 10: { // ==
            c = int(a == b);
            break;
        }
        case 11: { // !=
            c = int(a != b);
            break;

        }
        case 12: { // &&
            c = int((a!=0) && (b!=0));
            break;

        }
        case 13: { // ||
            c = int((a!=0) || (b!=0));
            break;

        }
        
        default: FAIL;
        }

        PUSH_NUMBER(st, c);

        return string_sprintf("BINOP\t%s", ops[l-1]);
      }
        
      case 1:
        switch (l) {
        case  0: { // CONST
          Number n = INT;
          PUSH_NUMBER(st, n);
          return string_sprintf("CONST\t%d", (int)n);
          //fprintf (f, "CONST\t%d", INT);
        }          
          
        case  1: { // STRING
          char* s = STRING;
          PUSH_STRING(st, s);
          return string_sprintf("STRING\t%s", s);
        }
                      
        case  2: { // SEXP
          char *name = STRING;
          int n = INT;

          Value* ptr = new Value[n];
          for(int i=0;i<n;++i) {
            ptr[n-i-1] = POP_VALUE(st);
          }

          Sexp sexp{};
          sexp.name = name;
          sexp.ptr = ptr;
          sexp.sz = n;
          PUSH_SEXP(st, sexp);

          return string_sprintf("SEXP\t%s %d", name, n);
          //fprintf (f, "SEXP\t%s ", STRING);
          //fprintf (f, "%d", INT);
          
        }
                    
        case  3: { // STI
          Value v = POP_VALUE(st);
          RefValue r = POP_REFVALUE(st);
          SET_VALUE(r, v);
          return std::string("STI");
        }
          
          
        case  4: { // STA
          Value val = POP_VALUE(st);
          int id = POP_VALUE(st).number;
          Value v = POP_VALUE(st);

          if(v.type=="String") {
            String s = v.string;
            s[id] = (char)val.number;
            PUSH_NUMBER(st, Number(val.number));
          } else if (v.type=="Array") {
            Array a = v.array;

            if(a.sz <= id) {
              printf("bad index %d >= %d\n", id, a.sz);
              exit(1);
            }

            a.ptr[id] = val;
            st.push(val);
          } else {
            printf("STA ERROR\n");
            exit(1);
          }
          return std::string("STA");
        }
                    
        case  5: { // JMP
          int addr = INT;
          ip = bf->code_ptr + addr;
          return string_sprintf("JMP\t0x%.8x", addr);
          //fprintf (f, "JMP\t0x%.8x", INT);
        }
                    
        case  6: { // END
          char *prev_cp = *reinterpret_cast<char**>(st.fp-3*sizeof(char*));
          char *prev_lp = *reinterpret_cast<char**>(st.fp-2*sizeof(char*));
          char *prev_fp = *reinterpret_cast<char**>(st.fp-sizeof(char*));
          char *prev_sp = *reinterpret_cast<char**>(st.fp);

          auto ret = POP_VALUE(st);

          st.sp = st.cp + sizeof(Value);
          int prev_ip = (int)st.top().number;  st.pop();

          st.lp = prev_lp;
          st.fp = prev_fp;
          st.sp = prev_sp;
          st.cp = prev_cp;

          ip = bf->code_ptr + prev_ip;

          st.push(ret);

          return std::string("END");
        }
                    
        case  7: {// RET
          printf("RET bytecode not supported anymore");
          exit(1);
          return std::string("RET");
        }
                    
        case  8: { // DROP
          auto _ = POP_VALUE(st);
          return std::string("DROP");
        }
                    
        case  9: { // DUP
          st.push(st.top());
          return std::string("DUP");
        }
          
        case 10: { // SWAP
          auto a = st.top(); st.pop();
          auto b = st.top(); st.pop();
          st.push(a);
          st.push(b);
          return std::string("SWAP");
        }

        case 11: { // ELEM
          int id = POP_VALUE(st).number;
          Value v = POP_VALUE(st);

          if(v.type=="String") {
            String s = v.string;
            PUSH_NUMBER(st, Number((int)*(s+id)));
          } else if (v.type=="Array") {
            Array a = v.array;
            if(a.sz <= id) {
              printf("bad index array %d >= %d\n", id, a.sz);
              exit(1);
            }
            PUSH_REFVALUE(st, &a.ptr[id]);
          } else if (v.type=="Sexp") {
            Sexp sexp = v.sexp;
            if(sexp.sz <= id) {
              printf("bad index sexp %d >= %d\n", id, sexp.sz);
              exit(1);
            }
            PUSH_REFVALUE(st, &sexp.ptr[id]);
          } else {
            printf("ELEM ERROR\n");
            exit(1);
          }
          return std::string("ELEM");
        }
          
          
        default:
          FAIL;
        }
        
        
      case 2: { // LD
        switch (l) {
            case 0: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)&st.globals[x]);
                return string_sprintf("LD\tG(%d)", x);
            }
            
            case 1: { 
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_loc(x));
                return string_sprintf("LD\tL(%d)", x);
            }
            
            case 2: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_arg(x));
                return string_sprintf("LD\tA(%d)", x);
            }
            
            case 3: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_cpt(x));
                return string_sprintf("LD\tC(%d)", x);
            }
            
            default: FAIL;
        }
      }
  
      case 3: { // LDA 
        switch (l) {
            case 0: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)&st.globals[x]);
                return string_sprintf("LDA\tG(%d)", x);
            }
            
            case 1: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_loc(x));
                return string_sprintf("LDA\tL(%d)", x);
            }
            
            case 2: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_arg(x));
                return string_sprintf("LDA\tA(%d)", x);
            }
            
            case 3: {
                int x = INT;
                PUSH_REFVALUE(st, (RefValue)st.get_cpt(x));
                return string_sprintf("LDA\tC(%d)", INT);
            }
            
            default: FAIL;
        }
      }
  
      case 4: { // ST
        switch (l) {
            case 0: {
                int x = INT;
                Value v = POP_VALUE(st);
                st.push(v);
                SET_VALUE(st.globals+x, v);
                return string_sprintf("ST\tG(%d)", x);
            }

            case 1: {
                int x = INT;
                Value v = POP_VALUE(st);
                st.push(v);
                SET_VALUE(st.get_loc(x), v);
                return string_sprintf("ST\tL(%d)", x);
            }
            
            case 2: { 
                int x = INT;
                Value v = POP_VALUE(st);
                st.push(v);
                SET_VALUE(st.get_arg(x), v);
                return string_sprintf("ST\tA(%d)", x);
            }
            
            case 3: {
                int x = INT;
                Value v = POP_VALUE(st);
                st.push(v);
                SET_VALUE(st.get_cpt(x), v);
                return string_sprintf("ST\tC(%d)", x);
            }
            
            default: FAIL;
        }
      }
        
        
      case 5:
        switch (l) {
        case  0: { // CJMPz
          int addr = INT;
          Number n = POP_VALUE(st).number;
          if(n==0) {
            ip = bf->code_ptr + addr;
          }
          return string_sprintf("CJMPz\t0x%.8x", addr);
        }
                    
        case  1: { // CJMPnz
          int addr = INT;
          Number n = POP_VALUE(st).number;
          if(n!=0) {
            ip = bf->code_ptr + addr;
          }
          return string_sprintf("CJMPnz\t0x%.8x", addr);
        }
                    
        case  2: { // BEGIN
          int args_ct = INT;
          int locals_ct = INT;

          st.setup(args_ct, locals_ct);

          return string_sprintf("BEGIN\t%d %d", args_ct, locals_ct);          
        }
          
        case  3: { // CBEGIN
          int args_ct = INT;
          int locals_ct = INT;
          int n = (int)POP_VALUE(st).number;

          st.setup(args_ct, locals_ct, n);

          return string_sprintf("CBEGIN\t%d %d", args_ct, locals_ct);          
        }
          
        case  4: { // CLOSURE 
          int addr = INT;
          std::string s = string_sprintf("CLOSURE\t0x%.8x", addr);
          
          int n = INT;

          Fun fun; 
          fun.addr = addr;          
          fun.n = n;
          fun.ptr = new Value[n];

          for (int i = 0; i<n; ++i) {
            switch (BYTE) {
              case 0: {
                int x = INT;
                fun.ptr[i] = st.globals[x];

                s += string_sprintf(" G(%d)", x);
                break;
              }
              case 1: { 
                int x = INT;
                fun.ptr[i] = *st.get_loc(x);

                s += string_sprintf(" L(%d)", x);
                break;
              }
              case 2: {
                int x = INT;
                fun.ptr[i] = *st.get_arg(x);

                s += string_sprintf(" A(%d)", x);
                break;
              }
              case 3: {
                int x = INT;
                fun.ptr[i] = *st.get_cpt(x);

                s += string_sprintf(" C(%d)", x);
                break;
              }
              default: FAIL;
            }
          }
          PUSH_FUN(st, fun);
          return s;
        };

        case  5: { // CALLC
          int args_ct = INT;

          Value args[args_ct];
          for(int i=0;i<args_ct;++i) {
            args[i] = POP_VALUE(st);
          }

          Fun fun = POP_VALUE(st).fun;
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + fun.addr;

          for(int i=0;i<fun.n;++i) {
            st.push(fun.ptr[fun.n-i-1]);
          }
          PUSH_NUMBER(st, prev_ip);
          for(int i=0;i<args_ct;++i) {
            st.push(args[i]);
          }
          if(fun.n) PUSH_NUMBER(st, fun.n);

          return string_sprintf("CALLC\t%d", args_ct);
        }
                    
        case  6: { // CALL
          int addr = INT;
          int args_ct = INT;

          Value args[args_ct];
          for(int i=0;i<args_ct;++i) {
            args[i] = POP_VALUE(st);
          }
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + addr;

          PUSH_NUMBER(st, prev_ip);
          for(int i=0;i<args_ct;++i) {
            st.push(args[i]);
          }

          return string_sprintf("CALL\t0x%.8x %d", addr, args_ct);         
        }
                    
        case  7: { // TAG
          char* name = STRING;
          int n = INT;

          Value v = POP_VALUE(st);
          Sexp sexp = v.sexp;
          PUSH_NUMBER(st, Number(int(v.type == "Sexp" && strcmp(name, sexp.name) == 0 && n == sexp.sz)));

          return string_sprintf("TAG\t%s %d", name, n);
          
        }
          
        case  8: { // ARRAY
          int n = INT;

          Value v = POP_VALUE(st);
          Array a = v.array;
          PUSH_NUMBER(st, Number(int(v.type == "Array" && a.sz == n)));

          return string_sprintf("ARRAY\t%d", n);
        }
                   
        case  9: { // FAIL
          int a = INT;
          int b = INT;
          return string_sprintf("FAIL\t%d %d", a, b);
        }
          
        case 10: { // LINE
          return string_sprintf("LINE\t%d", INT);
        }
          

        default:
          FAIL;
        }
                
      case 6: { // PATT
        switch (l) {
          case 0: { // =str
            String s2 = POP_VALUE(st).string;
            Value v = POP_VALUE(st);
            String s1 = v.string;
            PUSH_NUMBER(st, Number(v.type == "String" && strcmp(s1,s2) == 0));
            break;
          }
          case 1: { // #str
            Value v = POP_VALUE(st);
            PUSH_NUMBER(st, Number(v.type == "String"));
            break;
          }
          case 2: { // #array
            Value v = POP_VALUE(st);
            PUSH_NUMBER(st, Number(v.type == "Array"));
            break;
          }
          case 3: { // #sexp
            Value v = POP_VALUE(st);
            PUSH_NUMBER(st, Number(v.type == "Sexp"));
            break;
          }
          case 4: { // #box
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, Number(v.type == "RefValue"));
            break;
          }
          case 5: { // #val
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, Number(v.type != "RefValue"));
            break;
          }
          case 6: { // #fun
            Value v = POP_VALUE(st);
            PUSH_NUMBER(st, Number(v.type == "Fun"));
            break;
          }
          default: {
            printf("Unexpected error PATT\n");
            exit(1);
          }
        } 
        return string_sprintf("PATT\t%s", pats[l]);
      }
        

      case 7: {
        switch (l) {
        case 0: { // READ
          int n;
          fscanf(stdin, "%d", &n);
          PUSH_NUMBER(st, n);

          return std::string("CALL\tLread");
        }
                    
        case 1: { // WRITE
          Number n = POP_VALUE(st).number;
          PUSH_NUMBER(st, n);
          fprintf(stdout, "\n[stdout]%d\n\n", (int)n);

          return std::string("CALL\tLwrite");
        }

        case 2: { // Llength
          Value v = POP_VALUE(st);

          int sz = 0;
          if (v.type=="String") {
            sz = strlen(v.string);
          } else if (v.type=="Array") {
            sz = v.array.sz;
          } else if (v.type=="Sexp") {
            sz = v.sexp.sz; 
          } else {
            printf("Llength ERROR\n");
            exit(1);
          }

          PUSH_NUMBER(st, Number(sz));

          return std::string("CALL\tLlength");
        }
          
        case 3: { // Lstring
          return std::string("CALL\tLstring");
        }
          
        case 4: { // Barray
          int n = INT;
          Value* ptr = new Value[n];
          for(int i=0;i<n;++i) {
            ptr[n-i-1] = POP_VALUE(st);
          }
          
          Array a{};
          a.ptr = ptr;
          a.sz = n;
          PUSH_ARRAY(st, a);
          return string_sprintf("CALL\tBarray %d", n);
        }
          
        default:
          FAIL;
        }
        
      }  
          
      default:
        FAIL;
      }

      return std::string("");
    };
    
    auto print = switch_lambda();
    printf("%s\n",print.c_str());

    printf("\t\t=====debug=====\n");
    debug_stack(st);
    printf("\t\t===============\n");

    if(h==5 && l==9) {
      fprintf(stdout, "[stdout] *** FAILURE: match failure at {file}:{a}:{b}, value \'{str}\'\n");
      break;
    }
    if(h==15) break;
    //fprintf (f, "\n");
  }
  while (1);
}

int main (int argc, char* argv[]) {
  bytefile bf;
  read_file(argv[1], bf);
  parse(&bf);

  delete[] bf.buffer;
  delete[] bf.global_ptr;
  return 0;
}