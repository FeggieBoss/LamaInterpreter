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

//#define DEBUG
//#define DEBUG_WAIT_ON_STEP

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
/*
    | operand  |
    |  stack   |<---sp / __gc_stack_top
    +----------+
    |prev_sp   |
    +----------+<---fp
    |prev_fp   |
    +----------+
    |prev_lp   |
    +----------+
    |prev_cp   |
    +----------+
    |local_n   |
    |...       |
    |local_0   |
    +----------+<---lp
    |arg_0     |
    |...       |(reversed)
    |arg_n     |
    +----------+
    |prev_ip   |
    +----------+    
    |captured_n|
    |...       |
    |captured_0|
    +----------+<---cp
    |capturedsz|
    +----------+         stack section №i
    {
      stack sections
    }
    +----------+  
    |global_0  |<---globals
    |...       |
    |global_n  |<---ptr_start / __gc_stack_bottom
    +----------+
*/
public:
  unsigned int global_ct;
  Value *globals;
  char *ptr_end, *ptr_start;
  char *lp, *fp, *cp;

  Stack(const unsigned int mx_size_, const unsigned int _global_ct) {
    mx_size = mx_size_;
    global_ct = _global_ct;

    ptr_end = new char[mx_size_ + _global_ct*sizeof(Value)];
    ptr_start = ptr_end + mx_size_ + _global_ct*sizeof(Value);

    globals = reinterpret_cast<Value*>(ptr_end + mx_size_);
    for(int i=0;i<_global_ct;++i) globals[i] = BOX(0);

    lp = fp = ptr_end + mx_size_;
    cp = ptr_end + mx_size_ - sizeof(Value);

    __gc_stack_bottom = (size_t)ptr_start;
    __gc_stack_top = (size_t)(cp);

    this->push(BOX(0)); // captured_sz
    this->push(BOX(0)); // prev_ip
    
    this->setup(0, 0);
  }

  ~Stack() {
    delete[] ptr_end;
  }

  void setup(const unsigned int args_ct, const unsigned int locals_ct, const unsigned int captured_ct = 0) {
    char* sp = (char*)(__gc_stack_top);

    unsigned int free_space = (sp - ptr_end) + sizeof(Value), need = (locals_ct+5)*sizeof(Value);
    if(free_space < need) {
      std::cerr<<"Setup: no more space avaible - free space is " << free_space << " but need " << need << "\n";
      exit(1);
    }

    char *prev_sp = sp+(captured_ct + 1 + args_ct + 1)*sizeof(Value), *prev_lp = lp, *prev_fp = fp, *prev_cp = cp;
    
    char* cur = prev_sp;

    // CAPTURED
    cp = cur;
    cur -= captured_ct*sizeof(Value);

    // PREV_IP
    cur -= sizeof(Value);

    // ARGS
    char* arg_end = cur;
    cur -= args_ct * sizeof(Value);
    for(unsigned int i=0;i<args_ct/2;++i) { // ARGS reverse
      auto dop = Value();
      memcpy(reinterpret_cast<void*>(&dop), reinterpret_cast<void*>(cur + i*sizeof(Value)), sizeof(Value));
      memcpy(reinterpret_cast<void*>(cur + i*sizeof(Value)), reinterpret_cast<void*>(arg_end - (i+1)*sizeof(Value)), sizeof(Value));
      memcpy(reinterpret_cast<void*>(arg_end - (i+1)*sizeof(Value)), reinterpret_cast<void*>(&dop), sizeof(Value));
    }

    // LOCALS
    lp = cur;
    for(unsigned int i=0;i<locals_ct;++i) { 
      auto dop = BOX(Value());
      cur -= sizeof(Value);
      memcpy(reinterpret_cast<void*>(cur), reinterpret_cast<void*>(&dop), sizeof(Value));
    }

    cur -= sizeof(char*);
    *reinterpret_cast<char**>(cur) = prev_cp; // prevCP

    cur -= sizeof(char*);
    *reinterpret_cast<char**>(cur) = prev_lp; // prevLP

    cur -= sizeof(char*);
    *reinterpret_cast<char**>(cur) = prev_fp; // prevFP

    fp = cur;
    
    cur -= sizeof(char*);
    *reinterpret_cast<char**>(cur) = prev_sp; // prevSP
    
    sp = cur - sizeof(char*);

    __gc_stack_top = (size_t)sp;
  }

  Value& top() {
    char* sp = (char*)__gc_stack_top;
    if(sp+sizeof(char*)==fp-sizeof(char*)) {
      std::cerr<<"Unexpected stack.top(): stack is empty\n";
      exit(1);
    }
    return *reinterpret_cast<Value*>(sp+sizeof(Value));
  }

  Value* get_cpt(unsigned int x) {
    return reinterpret_cast<Value*>(cp-(x+1)*sizeof(Value));
  }

  Value* get_arg(unsigned int x) {
    return reinterpret_cast<Value*>(lp+(x)*sizeof(Value));
  }

  Value* get_loc(unsigned int x) {
    return reinterpret_cast<Value*>(lp-(x+1)*sizeof(Value));
  }

  void push(Value &&v) {
    char* sp = (char*)__gc_stack_top;
    if(sp-ptr_end<sizeof(Value)) {
      std::cerr<<"Unexpected stack.push(): no more space avaible - free space is " << sp-ptr_end << " but need " << sizeof(Value) << "\n";
      exit(1);
    }

    auto dop = v;
    memcpy(reinterpret_cast<void*>(sp), reinterpret_cast<void*>(&dop), sizeof(Value));
    sp -= sizeof(Value);
    __gc_stack_top = (size_t)sp;
  }

  void push(Value &v) {
    char* sp = (char*)__gc_stack_top;
    if(sp-ptr_end<sizeof(Value)) {
      std::cerr<<"Unexpected stack.push(): no more space avaible - free space is " << sp-ptr_end << " but need " << sizeof(Value) << "\n";
      exit(1);
    }

    auto dop = v;
    memcpy(reinterpret_cast<void*>(sp), reinterpret_cast<void*>(&dop), sizeof(Value));
    sp -= sizeof(Value);
    __gc_stack_top = (size_t)sp;
  }

  void pop(unsigned int _sizeof = sizeof(Value)) {
    char* sp = (char*)__gc_stack_top;
    if((fp-sizeof(char*))-sp < _sizeof) {
      std::cerr<<"Unexpected stack.pop(): stack is empty\n";
      exit(1);
    }
    sp += sizeof(char*);
    __gc_stack_top = (size_t)sp;
  }
private:
  unsigned int mx_size;
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

  char *sp = (char*)__gc_stack_top, *lp=st.lp,*fp=st.fp, *cp=st.cp;

  char *prev_fp = nullptr, *prev_sp = nullptr, *prev_lp = nullptr, *prev_cp = nullptr;
  while(prev_fp != (char*)st.globals) {
    prev_cp = *reinterpret_cast<char**>(fp+2*sizeof(char*));
    prev_lp = *reinterpret_cast<char**>(fp+1*sizeof(char*));
    prev_fp = *reinterpret_cast<char**>(fp);
    prev_sp = *reinterpret_cast<char**>(fp-1*sizeof(char*));

    char *cur = fp - 2*sizeof(char*);

    printf("\tOPS: [");
    while(cur!=sp) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur-=sizeof(Value);
    }
    printf("]\n");


    cur = lp - sizeof(Value);
    printf("\tLOCALS: [");
    while(cur!=(fp+2*sizeof(char*))) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur-=sizeof(Value);
    }
    printf("]\n");

    unsigned int captured_sz = UNBOX(*reinterpret_cast<Value*>(cp));

    cur = lp;
    printf("\tARGS: [");
    while(cur!=cp-(captured_sz+1)*sizeof(Value)) {
      debug_value(reinterpret_cast<Value*>(cur));
      printf(" , ");
      cur+=sizeof(Value);
    }
    printf("]\n");

    cur += sizeof(Value);

    printf("\tCAPTURED: [");
    while(cur!=cp) {
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

void parse(bytefile *bf, char* fname) {
# define INT    (ip += sizeof (int), *(int*)(ip - sizeof (int)))
# define BYTE   *ip++
# define STRING get_string (bf, INT)
# define FAIL   fprintf (stderr, "ERROR: invalid opcode %d-%d\n", h, l)
  
  Stack st = Stack(10000000, bf->global_area_size);

  PUSH_NUMBER(st, 0); // captured_sz
  PUSH_NUMBER(st, (bf->end_code_ptr - bf->code_ptr) - 1); // prev_ip // sizeof(STOP) = 1
  PUSH_NUMBER(st, 80085); 
  PUSH_NUMBER(st, 555);   

  char *ip = bf->code_ptr;
  do {
    char x = BYTE,
         h = (x & 0xF0) >> 4,
         l = x & 0x0F;

    #ifdef DEBUG_WAIT_ON_STEP
      int ______;
      fscanf(stdin, "%d", &______);
    #endif

    #ifdef DEBUG
      printf("0x%.8x:\t", ip-bf->code_ptr-1);
    #endif

    auto switch_lambda = [&st, &bf, &ip, fname, h, l]() {
      static const char* ops [] = {"+", "-", "*", "/", "%", "<", "<=", ">", ">=", "==", "!=", "&&", "!!"};
      static const char* pats[] = {"=str", "#string", "#array", "#sexp", "#ref", "#val", "#fun"};
      static const char* lds [] = {"LD", "LDA", "ST"};

      switch (h) {
      case 15: { // STOP
        #ifndef DEBUG
          return 0;
        #else 
          return std::string("STOP");
        #endif
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

        #ifndef DEBUG
          return 0;
        #else 
          return string_sprintf("BINOP\t%s", ops[l-1]);
        #endif
      }
        
      case 1:
        switch (l) {
        case  0: { // CONST
          Number n = INT;
          PUSH_NUMBER(st, n);
          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CONST\t%d", (int)n);
          #endif
        }          
          
        case  1: { // STRING
          char* s = STRING;
          String str = Bstring___(s);
          PUSH_STRING(st, str);
          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("STRING\t%s", s);
          #endif
        }
                      
        case  2: { // SEXP
          char *name = STRING;
          int n = INT;

          Sexp s = Bsexp___(BOX(n+1), UNBOX(LtagHash(name)));
          
          for(int i=n-1;i>=0;--i) {
            reinterpret_cast<int*>(s)[i] = st.top();
            st.pop();
          }

          PUSH_SEXP(st, s);

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("SEXP\t%s %d", name, n);
          #endif
        }
                    
        case  3: { // STI
          // Value v = POP_VALUE(st);
          // RefValue r = POP_REFVALUE(st);
          // SET_VALUE(r, v);
          printf("STI bytecode not supported anymore");
          exit(1);
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("STI");
          #endif
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
          
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("STA");
          #endif
        }
                    
        case  5: { // JMP
          int addr = INT;
          ip = bf->code_ptr + addr;

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("JMP\t0x%.8x", addr);
          #endif
        }
                    
        case  6: { // END
          char *prev_cp = *reinterpret_cast<char**>(st.fp+2*sizeof(char*));
          char *prev_lp = *reinterpret_cast<char**>(st.fp+1*sizeof(char*));
          char *prev_fp = *reinterpret_cast<char**>(st.fp+0*sizeof(char*));
          char *prev_sp = *reinterpret_cast<char**>(st.fp-1*sizeof(char*));

          auto ret = st.top(); st.pop();

          unsigned int captured_sz = UNBOX(*reinterpret_cast<Value*>(st.cp));
          int prev_ip = (size_t)UNBOX(*reinterpret_cast<Value*>(st.cp-(captured_sz+1)*sizeof(Value)));

          // shrinking stack
          st.lp = prev_lp;
          st.fp = prev_fp;
          st.cp = prev_cp;
          __gc_stack_top = (size_t)prev_sp;
          //__gc_root_scan_stack();

          ip = bf->code_ptr + prev_ip;

          st.push(ret);


          #ifndef DEBUG
            return 0;
          #else 
            return std::string("END");
          #endif
        }
                    
        case  7: { // RET
          printf("RET bytecode not supported anymore");
          exit(1);

          #ifndef DEBUG
            return 0;
          #else 
            return std::string("RET");
          #endif
        }
                    
        case  8: { // DROP
          st.pop();

          #ifndef DEBUG
            return 0;
          #else 
            return std::string("DROP");
          #endif
        }
                    
        case  9: { // DUP
          st.push(st.top());

          #ifndef DEBUG
            return 0;
          #else 
            return std::string("DUP");
          #endif
        }
          
        case 10: { // SWAP
          auto a = st.top(); st.pop();
          auto b = st.top(); st.pop();
          st.push(a);
          st.push(b);


          #ifndef DEBUG
            return 0;
          #else 
            return std::string("SWAP");
          #endif
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


          #ifndef DEBUG
            return 0;
          #else 
            return std::string("ELEM");
          #endif
        }
          
        default:
          FAIL;
        }
        
      case 2: { // LD
        switch (l) {
            case 0: {
                int x = INT;
                st.push(st.globals[x]);

                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("LD\tG(%d)", x);
                #endif
            }
            
            case 1: { 
                int x = INT;
                st.push(*st.get_loc(x));


                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("LD\tL(%d)", x);
                #endif
            }
            
            case 2: {
                int x = INT;
                st.push(*st.get_arg(x));

                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("LD\tA(%d)", x);
                #endif
            }
            
            case 3: {
                int x = INT;
                st.push(*st.get_cpt(x));


                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("LD\tC(%d)", x);
                #endif
            }
            
            default: FAIL;
        }
      }
  
      case 3: { // LDA 
        #ifdef DEBUG
          std::string s;
        #endif
        Value* v;
        switch (l) {
            case 0: {
                int x = INT;
                v = (&st.globals[x]);
                #ifdef DEBUG
                  s = string_sprintf("LDA\tG(%d)", x);
                #endif
                break;
            }
            
            case 1: {
                int x = INT;
                v = (st.get_loc(x));
                #ifdef DEBUG
                  s = string_sprintf("LDA\tL(%d)", x);
                #endif
                break;
            }
            
            case 2: {
                int x = INT;
                v = (st.get_arg(x));
                #ifdef DEBUG
                  s = string_sprintf("LDA\tA(%d)", x);
                #endif
                break;
            }
            
            case 3: {
                int x = INT;
                v = (st.get_cpt(x));
                #ifdef DEBUG
                  s = string_sprintf("LDA\tC(%d)", INT);
                #endif
                break;
            }
            
            default: FAIL;
        }

        if(UNBOXED(*v)) {
          st.push(int(v));
        } else {
          st.push(*v);
        }


        #ifndef DEBUG
          return 0;
        #else 
          return s;
        #endif
      }
  
      case 4: { // ST
        switch (l) {
            case 0: {
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.globals+x, v);
                
                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("ST\tG(%d)", x);
                #endif
            }

            case 1: {
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.get_loc(x), v);


                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("ST\tL(%d)", x);
                #endif
            }
            
            case 2: { 
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.get_arg(x), v);


                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("ST\tA(%d)", x);
                #endif
            }
            
            case 3: {
                int x = INT;
                Value v = st.top();
                SET_VALUE(st.get_cpt(x), v);


                #ifndef DEBUG
                  return 0;
                #else 
                  return string_sprintf("ST\tC(%d)", x);
                #endif
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
          
          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CJMPz\t0x%.8x", addr);
          #endif
        }
                    
        case  1: { // CJMPnz
          int addr = INT;
          Number n = POP_UNBOXED(st);
          if(n!=0) {
            ip = bf->code_ptr + addr;
          }

          #ifndef DEBUG
            return 0;
          #else
            return string_sprintf("CJMPnz\t0x%.8x", addr); 
          #endif
        }
                    
        case  2: { // BEGIN
          int args_ct = INT;
          int locals_ct = INT;

          st.setup(args_ct, locals_ct);

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("BEGIN\t%d %d", args_ct, locals_ct);    
          #endif      
        }
          
        case  3: { // CBEGIN
          int args_ct = INT;
          int locals_ct = INT;
          int n = POP_UNBOXED(st);

          st.setup(args_ct, locals_ct, n);

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CBEGIN\t%d %d", args_ct, locals_ct); 
          #endif         
        }
          
        case  4: { // CLOSURE 
          int addr = INT;

          #ifdef DEBUG
            std::string s = string_sprintf("CLOSURE\t0x%.8x", addr);
          #endif
          
          int n = INT;

          Fun fun = Bclosure___(BOX(n), (void*)addr);

          for (int i = 0; i<n; ++i) {
            switch (BYTE) {
              case 0: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = st.globals[x];

                #ifdef DEBUG
                  s += string_sprintf(" G(%d)", x);
                #endif
                break;
              }
              case 1: { 
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_loc(x);

                #ifdef DEBUG
                  s += string_sprintf(" L(%d)", x);
                #endif
                break;
              }
              case 2: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_arg(x);
                
                #ifdef DEBUG
                  s += string_sprintf(" A(%d)", x);
                #endif
                break;
              }
              case 3: {
                int x = INT;
                reinterpret_cast<int*>(fun)[i+1] = *st.get_cpt(x);

                #ifdef DEBUG
                  s += string_sprintf(" C(%d)", x);
                #endif
                break;
              }
              default: FAIL;
            }
          }

          PUSH_FUN(st, fun);


          #ifndef DEBUG
            return 0;
          #else 
            return s;
          #endif
        };

        case  5: { // CALLC
          int args_ct = INT;

          char* sp = (char*)__gc_stack_top;
          sp += args_ct * sizeof(Value);
          __gc_stack_top = (size_t)sp;
          
          Fun fun = (void*)st.top(); st.pop();
          data* fundata = TO_DATA(fun);
          unsigned int n = LEN(fundata->tag) - 1;
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + reinterpret_cast<int*>(fun)[0];

          sp += sizeof(Value);
          if(sp-st.ptr_end<(args_ct+1+n+1)*sizeof(Value)) {
            std::cerr<<"Unexpected CALLC: no more space avaible - free space is " << sp-st.ptr_end << " but need " << (args_ct+1+n+1)*sizeof(Value) << "\n";
            exit(1);
          }
          memcpy((void*)(sp - (args_ct+1+n)*sizeof(Value)), (void*)(sp-args_ct*sizeof(Value)), args_ct*sizeof(Value));


          PUSH_NUMBER(st, n);
          
          for(unsigned int i=0;i<n;++i) {
            st.push(reinterpret_cast<int*>(fun)[n-i]);
          }
          PUSH_NUMBER(st, prev_ip);

          __gc_stack_top = (size_t)(sp-(args_ct+1+n+1)*sizeof(Value));

          if(n) PUSH_NUMBER (st, n);
          
          
          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CALLC\t%d", args_ct);
          #endif
        }
                    
        case  6: { // CALL
          int addr = INT;
          int args_ct = INT;
          
          Number prev_ip = Number(ip-(bf->code_ptr));
          ip = bf->code_ptr + addr;

          char* sp = (char*)__gc_stack_top;
          sp += args_ct*sizeof(Value);
          __gc_stack_top = (size_t)sp;
          sp += sizeof(Value);

          if(sp-st.ptr_end<(args_ct+1+1)*sizeof(Value)) {
            std::cerr<<"Unexpected CALLC: no more space avaible - free space is " << sp-st.ptr_end << " but need " << (args_ct+1+1)*sizeof(Value) << "\n";
            exit(1);
          }
          memcpy((void*)(sp-(args_ct+1+1)*sizeof(Value)), (void*)(sp - args_ct*sizeof(Value)), args_ct*sizeof(Value));

          PUSH_NUMBER(st, 0);
          PUSH_NUMBER(st, prev_ip);

          __gc_stack_top = (size_t)(sp-(args_ct+1+1+1)*sizeof(Value));

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CALL\t0x%.8x %d", addr, args_ct);    
          #endif     
        }
                    
        case  7: { // TAG
          char* name = STRING;
          int n = INT;

          Value v = st.top(); st.pop();
          data* sdata = TO_DATA(v);
          sexp* s = TO_SEXP(v);
          PUSH_NUMBER(st, int(!UNBOXED(v) && TAG(sdata->tag)==SEXP_TAG && LEN(sdata->tag) == n && s->tag == UNBOX(LtagHash(name))));

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("TAG\t%s %d", name, n);
          #endif
        }
          
        case  8: { // ARRAY
          int n = INT;

          Value v = st.top(); st.pop();
          data* adata = TO_DATA(v);
          PUSH_NUMBER(st, int(TAG(adata->tag) == ARRAY_TAG && LEN(adata->tag) == n));

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("ARRAY\t%d", n);
          #endif
        }
                   
        case  9: { // FAIL
          int a = INT;
          int b = INT;
          Value v = st.top(); st.pop();
          Bmatch_failure((void*)v, fname, a, b);

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("FAIL\t%d %d", a, b);
          #endif
        }
          
        case 10: { // LINE
          int a = INT;
          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("LINE\t%d", a);
          #endif
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


        #ifndef DEBUG
          return 0;
        #else 
          return string_sprintf("PATT\t%s", pats[l]);
        #endif
      }
        

      case 7: {
        switch (l) {
        case 0: { // READ
          int n;
          fscanf(stdin, "%d", &n);
          PUSH_NUMBER(st, n);
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("CALL\tLread");
          #endif
        }
                    
        case 1: { // WRITE
          Number n = POP_UNBOXED(st);
          PUSH_NUMBER(st, n);
          fprintf(stdout, "%d\n", (int)n);
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("CALL\tLwrite");
          #endif
        }

        case 2: { // Llength
          data* vdata = TO_DATA(POP_BOXED(st));

          int sz = LEN(vdata->tag);
          PUSH_NUMBER(st, Number(sz));
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("CALL\tLlength");
          #endif
        }
          
        case 3: { // Lstring
          Value v = st.top(); st.pop();
          v = (int)Bstringval((void*)v);
          st.push(v);
          #ifndef DEBUG
            return 0;
          #else 
            return std::string("CALL\tLstring");
          #endif
        }
          
        case 4: { // Barray
          int n = INT;
          
          Array a = (void*)Barray___(BOX(n));
          for(int i=n-1;i>=0;--i) {
            reinterpret_cast<int*>(a)[i] = st.top(); 
            st.pop();
          }

          PUSH_ARRAY(st, a);

          #ifndef DEBUG
            return 0;
          #else 
            return string_sprintf("CALL\tBarray %d", n);
          #endif
        }
          
        default:
          FAIL;
        }
        
      }  
          
      default:
        FAIL;
      }

      #ifndef DEBUG
        return 0;
      #else 
        return std::string("");
      #endif
    };

    auto print = switch_lambda();

  #ifdef DEBUG
    printf("%s\n",print.c_str());

    printf("\t\t=====debug=====\n");
    debug_stack(st);
    printf("\t\t===============\n");
  #endif

    if(h==15) break;
  }
  while (1);
}

int main (int argc, char* argv[]) {
  bytefile bf;
  read_file(argv[1], bf);
  parse(&bf, argv[1]);

  delete[] bf.buffer;
  return 0;
}