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

typedef int Number;
typedef void* String;
typedef void* Array;
typedef void* Sexp;
typedef void* Fun;

typedef int Value;

// struct Value {
//     Value() {
//       type = "Byte";
//     }
//     Byte byte;
//     Number number;
//     String string;
//     Array array;
//     Sexp sexp;
//     RefValue ref_value;
//     Fun fun;
//     const char* type;
// };

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
    for(int i=0;i<_global_ct;++i) globals[i] = BOX(0);
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
      auto dop = BOX(Value());
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
  *v = x;
}

void debug_value(Value *const val) {
  Value v = *val;
  if(UNBOXED(v)) {
    printf("type is Number, value : %d", UNBOX(v));
  } else {
    data* vdata = TO_DATA(v);
    switch (TAG(vdata->tag))
    {
      case STRING_TAG: {
        printf("String=%s", (char*)v);
        break;
      }
      case ARRAY_TAG: {
        printf("Array=[");
        int n = LEN(vdata->tag);
        for(int i=0;i<n;++i) {
            debug_value(reinterpret_cast<int*>(v)+i);
            if(i!=n-1) printf(" , ");
        }
        printf("]");
        break;
      }
      case SEXP_TAG: {
        sexp* sexp_ = TO_SEXP(v);
        printf("Sexp(%s)=[", de_hash(sexp_->tag));
        int n = LEN(vdata->tag);
        for(int i=0;i<n;++i) {
            debug_value(reinterpret_cast<int*>(v)+i);
            if(i!=n-1) printf(" , ");
        }
        printf("]");
        break;
      }
      case CLOSURE_TAG: {
        printf("Fun(0x%.8x) Captured=[", reinterpret_cast<int*>(v)[0]);
        int n = LEN(vdata->tag);
        for(int i=1;i<n;++i) {
            debug_value(reinterpret_cast<int*>(v)+i);
            if(i!=n-1) printf(" , ");
        }
        printf("]");
        break;
      }
      default:{
        break;
      }
    }
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

Value POP_UNBOXED(Stack &st) {
    Value v = st.top();
    st.pop();
    if (UNBOXED(v)) {
      v = UNBOX(v);
    } else {
      printf("Error POP_UNBOXED: st.top() = %d\n", v);
    }
    return v;
}

Value POP_BOXED(Stack &st) {
    Value v = st.top();
    st.pop();
    return v;
}

void PUSH_NUMBER(Stack &st, Number n) {
    st.push(BOX(n));
}

void PUSH_STRING(Stack &st, String s) {
    st.push((int)s);
}

void PUSH_ARRAY(Stack &st, Array a) {
    st.push((int)a);
}

void PUSH_SEXP(Stack &st, Sexp s) {
    st.push((int)s);
}

void PUSH_FUN(Stack &st, Fun fun) {
    st.push((int)fun);
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
  
  Stack st = Stack(10000000, bf->global_area_size);
  
  st.setup(0, 0);

  PUSH_NUMBER(st, (bf->end_code_ptr - bf->code_ptr) - 1); // sizeof(STOP) = 1

  PUSH_NUMBER(st, 80085);
  PUSH_NUMBER(st, 555);

  char *ip = bf->code_ptr;
  do {
    char x = BYTE,
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

                        int ______;
                        //fscanf(stdin, "%d", &______);
    //fprintf (f, "0x%.8x:\t", ip-bf->code_ptr-1);

    auto switch_lambda = [&st, &bf, &ip, h, l]() {
      static const char* ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
      static const char* pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
      static const char* lds [] = {"LD", "LDA", "ST"};

      switch (h) {
      case 15: { // STOP
        return std::string("STOP");
      }
        
      case 0: { // BINOP
        Number b, a, c;
        b = POP_UNBOXED(st);
        a = POP_UNBOXED(st);

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
          String str = Bstring___(s);
          PUSH_STRING(st, str);
          return string_sprintf("STRING\t%s", s);
        }
                      
        case  2: { // SEXP
          char *name = STRING;
          int n = INT;

          Value* ptr = new Value[n];
          for(int i=0;i<n;++i) {
            ptr[n-i-1] = st.top(); st.pop();
          }
          Sexp s = Bsexp___(BOX(n+1), LtagHash(name));
          
          for(int i=0;i<n;++i) {
            reinterpret_cast<int*>(s)[i] = ptr[i];
          }

          PUSH_SEXP(st, s);

          return string_sprintf("SEXP\t%s %d", name, n);
          //fprintf (f, "SEXP\t%s ", STRING);
          //fprintf (f, "%d", INT);
          
        }
                    
        case  3: { // STI
          // Value v = POP_VALUE(st);
          // RefValue r = POP_REFVALUE(st);
          // SET_VALUE(r, v);
          printf("STI bytecode not supported anymore");
          exit(1);
          return std::string("STI");
        }
          
        case  4: { // STA
          Value val = st.top(); st.pop();
          Value id = st.top(); st.pop();

          if(!UNBOXED(id)) { // UNBOXED v
            *reinterpret_cast<int*>(id) = val;
            st.push(val);
          } else { // BOXED v
            id = UNBOX(id);
            Value v = st.top(); st.pop();

            data* vdata = TO_DATA(v);
            switch (TAG(vdata->tag))
            {
              case STRING_TAG: {
                String s = (void*)v;
                reinterpret_cast<char*>(v)[id] = UNBOX(val);
                PUSH_NUMBER(st, Number(val));
                break;
              }
              
              case ARRAY_TAG: {
                Array a = (void*)v;

                if(id >= LEN(vdata->tag)) {
                  printf("STA: bad index %d >= %d\n", id, LEN(vdata->tag));
                  exit(1);
                }
                
                reinterpret_cast<int*>(a)[id] = val;
                PUSH_NUMBER(st, val);
                break;
              }

              case SEXP_TAG: {
                Sexp s = (void*)v;

                if(id >= LEN(vdata->tag)) {
                  printf("STA: bad index %d >= %d\n", id, LEN(vdata->tag));
                  exit(1);
                }

                reinterpret_cast<int*>(s)[id] = val;
                st.push(val);
                break;
              }

              default: {
                printf("STA ERROR\n");
                exit(1);
              }
            }
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

          auto ret = st.top(); st.pop();

          st.sp = st.cp + sizeof(Value);
          int prev_ip = POP_UNBOXED(st); st.pop();

          st.lp = prev_lp;
          st.fp = prev_fp;
          st.sp = prev_sp;
          st.cp = prev_cp;

          ip = bf->code_ptr + prev_ip;

          st.push(ret);

          return std::string("END");
        }
                    
        case  7: { // RET
          printf("RET bytecode not supported anymore");
          exit(1);
          return std::string("RET");
        }
                    
        case  8: { // DROP
          st.pop();
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
          int id = POP_UNBOXED(st);
          Value v = POP_BOXED(st);

          data* vdata = TO_DATA(v);
          switch (TAG(vdata->tag))
          {
            case STRING_TAG: {
              String s = (void*)v;
              
              PUSH_NUMBER(st, Number(reinterpret_cast<char*>(v)[id]));
              break;
            }
            
            case ARRAY_TAG: {
              Array a = (void*)v;

              if(id >= LEN(vdata->tag)) {
                printf("ELEM: bad index %d >= %d\n", id, LEN(vdata->tag));
                exit(1);
              }

              st.push(reinterpret_cast<int*>(a)[id]);
              break;
            }

            case SEXP_TAG: {
              Sexp s = (void*)v;

              if(id >= LEN(vdata->tag)) {
                printf("ELEM: bad index %d >= %d\n", id, LEN(vdata->tag));
                exit(1);
              }

              st.push(reinterpret_cast<int*>(s)[id]);
              break;
            }

            default: {
              printf("ELEM ERROR\n");
              exit(1);
            }
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
                st.push(st.globals[x]);
                return string_sprintf("LD\tG(%d)", x);
            }
            
            case 1: { 
                int x = INT;
                st.push(*st.get_loc(x));
                return string_sprintf("LD\tL(%d)", x);
            }
            
            case 2: {
                int x = INT;
                st.push(*st.get_arg(x));
                return string_sprintf("LD\tA(%d)", x);
            }
            
            case 3: {
                int x = INT;
                st.push(*st.get_cpt(x));
                return string_sprintf("LD\tC(%d)", x);
            }
            
            default: FAIL;
        }
      }
  
      case 3: { // LDA 
        std::string s;
        Value* v;
        switch (l) {
            case 0: {
                int x = INT;
                v = (&st.globals[x]);
                s = string_sprintf("LDA\tG(%d)", x);
                break;
            }
            
            case 1: {
                int x = INT;
                v = (st.get_loc(x));
                s = string_sprintf("LDA\tL(%d)", x);
                break;
            }
            
            case 2: {
                int x = INT;
                v = (st.get_arg(x));
                s = string_sprintf("LDA\tA(%d)", x);
                break;
            }
            
            case 3: {
                int x = INT;
                v = (st.get_cpt(x));
                s = string_sprintf("LDA\tC(%d)", INT);
                break;
            }
            
            default: FAIL;
        }

        if(UNBOXED(*v)) {
          st.push(int(v));
        } else {
          st.push(*v);
        }

        return s;
      }
  
      case 4: { // ST
        switch (l) {
            case 0: {
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.globals+x, v);
                return string_sprintf("ST\tG(%d)", x);
            }

            case 1: {
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.get_loc(x), v);
                return string_sprintf("ST\tL(%d)", x);
            }
            
            case 2: { 
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.get_arg(x), v);
                return string_sprintf("ST\tA(%d)", x);
            }
            
            case 3: {
                int x = INT;
                Value v = st.top();
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
          Number n = POP_UNBOXED(st);
          if(n==0) {
            ip = bf->code_ptr + addr;
          }
          return string_sprintf("CJMPz\t0x%.8x", addr);
        }
                    
        case  1: { // CJMPnz
          int addr = INT;
          Number n = POP_UNBOXED(st);
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
          int n = POP_UNBOXED(st);

          st.setup(args_ct, locals_ct, n);

          return string_sprintf("CBEGIN\t%d %d", args_ct, locals_ct);          
        }
          
        case  4: { // CLOSURE 
          int addr = INT;
          std::string s = string_sprintf("CLOSURE\t0x%.8x", addr);
          
          int n = INT;

          Fun fun = Bclosure___(BOX(n), (void*)addr);

          for (int i = 0; i<n; ++i) {
            switch (BYTE) {
              case 0: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = st.globals[x];

                s += string_sprintf(" G(%d)", x);
                break;
              }
              case 1: { 
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_loc(x);

                s += string_sprintf(" L(%d)", x);
                break;
              }
              case 2: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_arg(x);

                s += string_sprintf(" A(%d)", x);
                break;
              }
              case 3: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_cpt(x);

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
          for(unsigned int i=0;i<args_ct;++i) {
            args[i] = st.top(); st.pop();
          }

          Fun fun = (void*)st.top(); st.pop();
          data* fundata = TO_DATA(fun);
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + reinterpret_cast<int*>(fun)[0];

          unsigned int n = LEN(fundata->tag) - 1;
          for(unsigned int i=0;i<n;++i) {
            st.push(reinterpret_cast<int*>(fun)[n-i]);
          }
          PUSH_NUMBER(st, prev_ip);
          for(unsigned int i=0;i<args_ct;++i) {
            st.push(args[i]);
          }
          if(n) PUSH_NUMBER (st, n);

          return string_sprintf("CALLC\t%d", args_ct);
        }
                    
        case  6: { // CALL
          int addr = INT;
          int args_ct = INT;

          Value args[args_ct];
          for(unsigned int i=0;i<args_ct;++i) {
            args[i] = st.top(); st.pop();
          }
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + addr;

          PUSH_NUMBER(st, prev_ip);
          for(unsigned int i=0;i<args_ct;++i) {
            st.push(args[i]);
          }

          return string_sprintf("CALL\t0x%.8x %d", addr, args_ct);         
        }
                    
        case  7: { // TAG
          char* name = STRING;
          int n = INT;

          Value v = st.top(); st.pop();
          data* sdata = TO_DATA(v);
          sexp* s = TO_SEXP(v);
          PUSH_NUMBER(st, int(!UNBOXED(v) && TAG(sdata->tag)==SEXP_TAG && LEN(sdata->tag) == n && s->tag == LtagHash(name)));

          return string_sprintf("TAG\t%s %d", name, n);
          
        }
          
        case  8: { // ARRAY
          int n = INT;

          Value v = st.top(); st.pop();
          data* adata = TO_DATA(v);
          PUSH_NUMBER(st, int(TAG(adata->tag) == ARRAY_TAG && LEN(adata->tag) == n));

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
            String s2 = (void*)st.top(); st.pop();
            String s1 = (void*)st.top(); st.pop();
            data* s1data = TO_DATA(s1); 
            PUSH_NUMBER(st, Number(TAG(s1data->tag)==STRING_TAG && strcmp((char*)s1,(char*)s2) == 0));
            break;
          }
          case 1: { // #string
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, !UNBOXED(v) && TAG(TO_DATA(v)->tag)==STRING_TAG);
            break;
          }
          case 2: { // #array
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, !UNBOXED(v) && TAG(TO_DATA(v)->tag)==ARRAY_TAG);
            break;
          }
          case 3: { // #sexp
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, !UNBOXED(v) && TAG(TO_DATA(v)->tag)==SEXP_TAG);
            break;
          }
          case 4: { // #ref
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, !UNBOXED(v));
            break;
          }
          case 5: { // #val
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, UNBOXED(v));
            break;
          }
          case 6: { // #fun
            Value v = st.top(); st.pop();
            PUSH_NUMBER(st, !UNBOXED(v) && TAG(TO_DATA(v)->tag)==CLOSURE_TAG);
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
          Number n = POP_UNBOXED(st);
          PUSH_NUMBER(st, n);
          fprintf(stdout, "%d\n", (int)n);

          return std::string("CALL\tLwrite");
        }

        case 2: { // Llength
          data* vdata = TO_DATA(POP_BOXED(st));

          int sz = LEN(vdata->tag);
          PUSH_NUMBER(st, Number(sz));

          return std::string("CALL\tLlength");
        }
          
        case 3: { // Lstring
          return std::string("CALL\tLstring");
        }
          
        case 4: { // Barray
          int n = INT;
          Value* ptr = new Value[n];
          for(unsigned int i=0;i<n;++i) {
            ptr[n-i-1] = st.top(); st.pop();
          }
          
          Array a = (void*)Barray___(BOX(n));
          for(unsigned int i=0;i<n;++i) {
            reinterpret_cast<int*>(a)[i] = ptr[i];
          }

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
    //printf("%s\n",print.c_str());

    //printf("\t\t=====debug=====\n");
    //debug_stack(st);
    //printf("\t\t===============\n");

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