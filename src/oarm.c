#include "oarm.h"

#define LOG_VERBOSE
int entry(int argc, char** argv) {
  printf("oarm v0.1\n____\n\n");

  FILE* input_stream = NULL;
  if (argc <= 1) {
    print_help();
  } else {
    if (strcmp("--help", argv[1]) == 0) {
      print_help();
      return 0;
    } else {
      input_stream = fopen(argv[1], "r");
      if (input_stream == NULL) {
        perror("Error opening file");
        return 1;
      }
    }
  }

  /*Copy the file into memory*/
  /*fun fact, there is a race condition between getting size of file and
   * allocating memory. Whatever though, don't run the assembler while editing
   * the file.*/
  fseek(input_stream, 0, SEEK_END);
  long fsize = ftell(input_stream);
  fseek(input_stream, 0, SEEK_SET);
  char* program = malloc((unsigned long)(fsize + 1));
  fread(program, (unsigned long)fsize, 1, input_stream);
  fclose(input_stream);
  program[fsize] = EOF;

  TokenizedProgram program_tokens = tokenize(program, (int)fsize);

#ifdef LOG_VERBOSE
  log_tokenized_program(program_tokens);
#endif

  State s;
  memset(s.memory, 0, sizeof(int) * MEM_BYTES);
  memset(s.registers, 0, sizeof(int) * NUM_REGISTERS);
  s.pc = 0;
  s.cont = true;
  s.cmp = 0;
  while (s.cont) {
    s = tick(s, program_tokens.lines[s.pc]);
    if (s.pc > program_tokens.len || s.pc < 0) {
      s.cont = false;
    }
  }

  free(program_tokens.lines);
  free(program);
  return 0;
}

void print_help(void) {
  /*Write a little tutorial of the commands available*/
  printf(
      "Usage: oarm [FILE]\n"
      "\n"
      "Examples:\n"
      "  oarm program.s      Assemble and run program.s\n"
      "\n"
      "Options:\n"
      "  --help              Show this help message and exit\n");
}

TokenizedProgram tokenize(char* str, int length) {
  int program_size = 2;
  TokenizedProgram program;
  program.len = 0;
  program.lines = (Line*)malloc((size_t)program_size * sizeof(Line));
  memset(program.lines, 0, (size_t)program_size * sizeof(Line));
  Token t;
  t.len = 0;

  int i = 0;
  for (; i < length; i++) {
    char c = str[i];
    int li = program.len;
    int num_tokens = program.lines[li].len;
    bool is_token_empty = t.len == 0;
    if (num_tokens >= MAX_TOKENS_PER_LINE) {
      printf(
          "parsing failed, max tokens exceeded on line %i more than %i tokens "
          "detected\n",
          i, MAX_TOKENS_PER_LINE);
      break;
    }
    switch (c) {
      case EOF:
      case '\n':
        program.len++;
        if (program.len == program_size) {
          int old_program_size = program_size;
          program_size = program_size * 2;
          program.lines =
              realloc(program.lines, (size_t)program_size * sizeof(Line));
          memset(program.lines + old_program_size, 0,
                 (size_t)(program_size - old_program_size) * sizeof(Line));
        }
      case ' ':
      case ',':
      case ':':
        if (is_token_empty) {
          break;
        }
        if (c == ':') {
          if (t.len >= MAX_IDENT_LEN) {
            printf("Warning: max identifier length of %i exceeded",
                   MAX_IDENT_LEN);
            break;
          }
          t.tok[t.len] = c;
          t.len++;
        }
        /*Push token onto program struct.*/
        program.lines[li].tokens[num_tokens] = t;
        program.lines[li].len = num_tokens + 1;

        /* reset token*/
        t.len = 0;
        memset(t.tok, 0, sizeof(char) * MAX_IDENT_LEN);
        break;
      default:
        if (t.len >= MAX_IDENT_LEN) {
          printf("Warning: max identifier length of %i exceeded",
                 MAX_IDENT_LEN);
          break;
        }
        t.tok[t.len] = c;
        t.len++;
    }
  }
  return program;
}

State tick(State s, Line line) {
/*Evaluate one line of asm.*/
  log_line(line);
  if (line.len < 1) {
    printf("warning:tick: empty line\n");
    s.cont = false;
    return s;
  }
  Token t = line.tokens[0];
  CMD cmd = identify_cmd(t);
  switch (cmd) {
    case ADD:
      s = add_or_sub(s, line, true);
      break;
    case BRANCH:
    case BEQ:
    case BNE:
    case BLE:
    case BLT:
    case BGE:
    case BGT:
      s = branch(s, line, cmd);
      break;
    case LDR:
      s = ldr(s, line);
      break;
    case LSL:
      s = lsl_or_lsr(s, line, true);
      break;
    case LSR:
      s = lsl_or_lsr(s, line, false);
      break;
    case MEM:
      log_mem(s);
      break;
    case MOV:
      s = mov(s, line);
      break;
    case REG:
      log_registers(s);
      break;
    case RET:
      s.cont = false;
      break;
    case NL:
      s.cont = false;
    case STR:
      s = str(s, line);
      break;
    case SUB:
      s = add_or_sub(s, line, false);
      break;
    case RPC:
      printf("pc: %i\n", s.pc);
      break;
    case UNKNOWN:

      printf("Error could not parse statement identifier: %c%c%c\n", t.tok[0],
             t.tok[1], t.tok[2]);
      break;
    case LABEL:
      break;
  }
  s.pc++;
  return s;
}

CMD identify_cmd(Token t) {
  if (t.tok[t.len - 1] == ':') {
    return LABEL;
  }
  int key = (t.tok[0] << 16) | (t.tok[1] << 8) | t.tok[2];
  switch (key) {
    case ('m' << 16) | ('e' << 8) | 'm':
      return MEM;
      break;
    case ('m' << 16) | ('o' << 8) | 'v':
      return MOV;
      break;
    case ('a' << 16) | ('d' << 8) | 'd':
      return ADD;
      break;
    case ('r' << 16) | ('e' << 8) | 'g':
      return REG;
      break;
    case ('r' << 16) | ('e' << 8) | 't':
      return RET;
      break;
    case ('l' << 16) | ('s' << 8) | 'l':
      return LSL;
      break;
    case ('l' << 16) | ('s' << 8) | 'r':
      return LSR;
      break;
    case ('s' << 16) | ('u' << 8) | 'b':
      return SUB;
      break;
    case ('s' << 16) | ('t' << 8) | 'r':
      return STR;
      break;
    case ('b' << 16) | ('e' << 8) | 'q':
      return BEQ;
      break;
    case ('b' << 16) | ('n' << 8) | 'e':
      return BNE;
      break;
    case ('b' << 16) | ('l' << 8) | 't':
      return BLT;
      break;
    case ('b' << 16) | ('l' << 8) | 'e':
      return BLE;
      break;
    case ('b' << 16) | ('g' << 8) | 't':
      return BGT;
      break;
    case ('b' << 16) | ('g' << 8) | 'e':
      return BGE;
      break;
    case ('b' << 16) | (0 << 8) | 0:
      return BRANCH;
      break;
    case ('r' << 16) | ('p' << 8) | 'c':
      return RPC;
      break;
  }
  return UNKNOWN;
}

int parse_int(const char* num, int len) {
  /*Given an array of chars, return an int.*/
  if (len > 8) {
    printf(
        "Integer overflow detected in parse int. Max int is 8 digits. "
        "Truncating digits.\n");
    len = 8;
  }

  int result = 0;
  int place = 1;
  int i = len - 1;
  int sign = 1;
  int end = 0;
  if(num[0]=='-'){
      sign = -1;
      end = 1;
  }

  for (; i >= end; i--) {
    if (num[i] < (int)'0' || num[i] > (int)'9') {
      printf("Non digit detected in parse int string: %x (%i)\n", num[i],
             num[i]);
      return 0;
    }
    result += place * (num[i] - (int)'0');
    place = place * 10;
  }

  return result*sign;
}

Args parse_args(Line line) {
  Args args;
  args.count = 0;
  args.is_valid = true;
  int i = 1;
  for (; i < line.len; i++) {
    Token t = line.tokens[i];
    Arg a;

    /*Label argument*/
    if (t.tok[t.len - 1] == ':') {
      a.tag = LABEL_ARG;
      a.label = parse_int((const char*)t.tok, t.len - 1);
      /*TODO replace this with looking up the label in the label jump table?
      or change label to just be a char array and return the label name and the
      caller can handle looking it up
      */
    }
    /*Address argument */
    else if (t.tok[0] == '[') {
      a.tag = ADDRESS;
      if (t.tok[1] == 'x') {
        a.addr.type = A_REGISTER;
      } else if (t.tok[1] == '#') {
        a.addr.type = A_CONSTANT;
      } else {
        printf(
            "Argument %i, unsupported address type, must be register or "
            "constant.",
            args.count + 1);
        args.is_valid = false;
        return args;
      }

      const char* num_start = (const char*)t.tok + 2;
      int num_len = t.len - 3;
      if (num_len < 1) {
        printf(
            "Argument %i, expected numerical value for memory "
            "address argument, got empty string.\n",
            args.count + 1);
        args.is_valid = false;
        return args;
      }
      a.addr.val = parse_int((const char*)num_start, num_len);

      if (a.addr.type == A_CONSTANT) {
        if (a.addr.val > MEM_BYTES || a.addr.val < 0) {
          printf(
              "Argument %i memory address is out of range, must be between 0 "
              "and %i\n",
              args.count + 1, MEM_BYTES);
          args.is_valid = false;
          return args;
        }
      }
    }
    /*Register argument*/
    else if (t.tok[0] == 'x') {
      a.tag = REGISTER;
      a.reg = parse_int(t.tok + 1, t.len - 1);
      if (a.reg > NUM_REGISTERS || a.reg < 0) {
        printf(
            "Argument %i register is out of range, must be between 0 and "
            "%i\n",
            args.count + 1, NUM_REGISTERS);
        args.is_valid = false;
        return args;
      }

      /*constant argument*/
    } else if (t.tok[0] == '#') {
      a.tag = CONSTANT;
      a.constant = parse_int(t.tok + 1, t.len - 1);
    } else {
      args.is_valid = false;
      printf(
          "Error parsing args, couldn't recognize arg type. tok %i last 3 "
          "chars: %c%c%c\n",
          i, t.tok[0], t.tok[1], t.tok[2]);
      return args;
    }
    args.args[args.count] = a;
    args.count++;
  }

  return args;
}

State mov(State s, Line line) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  v.cmd_pretty_str = "mov";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  int val = get_register_or_constant(s, a2);

  s.registers[a1.reg] = val;
  return s;
}

State ldr(State s, Line line) {
  Args args = parse_args(line);

  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  v.cmd_pretty_str = "ldr";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = ADDRESS;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a2.addr.type == A_REGISTER) {
    int addr = s.registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("ldr: out of bounds memory access at address %i\n", addr);
      s.cont = false;
      return s;
    }
    s.registers[a1.reg] = s.memory[addr];
  } else if (a2.addr.type == A_CONSTANT) {
    s.registers[a1.reg] = s.memory[a2.addr.val];
  }

  return s;
}

State str(State s, Line line) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  v.cmd_pretty_str = "str";
  v.expected_arg_count = 2;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = ADDRESS;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];

  if (a2.addr.type == A_REGISTER) {
    int addr = s.registers[a2.addr.val];
    if (addr < 0 || addr >= MEM_BYTES) {
      printf("str: out of bounds memory access at address %i\n", addr);
      s.cont = false;
      return s;
    }
    s.memory[addr] = s.registers[a1.reg];
  } else if (a2.addr.type == A_CONSTANT) {
    if (a2.addr.val < 0 || a2.addr.val >= MEM_BYTES) {
      printf("str: out of bounds memory access at address %i\n", a2.addr.val);
      s.cont = false;
      return s;
    }
    s.memory[a2.addr.val] = s.registers[a1.reg];
  }
  return s;
}

State add_or_sub(State s, Line line, bool is_add) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  if (is_add) {
    v.cmd_pretty_str = "add";
  } else {
    v.cmd_pretty_str = "sub";
  }
  v.expected_arg_count = 3;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  ArgValidation third_arg;
  third_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  v.validations[2] = third_arg;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];
  Arg a3 = args.args[2];

  int val1 = get_register_or_constant(s, a2);
  int val2 = get_register_or_constant(s, a3);

  if (!is_add) {
    val2 = val2 * -1;
  }
  s.registers[a1.reg] = val1 + val2;
  return s;
}

State lsl_or_lsr(State s, Line line, bool is_left) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  if (is_left) {
    v.cmd_pretty_str = "lsl";
  } else {
    v.cmd_pretty_str = "lsr";
  }
  v.expected_arg_count = 3;
  ArgValidation first_arg;
  first_arg.expected_arg_type = REGISTER;
  ArgValidation second_arg;
  second_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  ArgValidation third_arg;
  third_arg.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = first_arg;
  v.validations[1] = second_arg;
  v.validations[2] = third_arg;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  Arg a1 = args.args[0];
  Arg a2 = args.args[1];
  Arg a3 = args.args[2];

  int val1 = get_register_or_constant(s, a2);
  int val2 = get_register_or_constant(s, a3);

  if (is_left) {
    s.registers[a1.reg] = val2 << val1;
  } else {
    s.registers[a1.reg] = val2 >> val1;
  }
  return s;
}

State branch(State s, Line line, CMD command) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  v.expected_arg_count = 1;
  v.cmd_pretty_str = "branch";

  ArgValidation v1;
  v1.expected_arg_type = LABEL_ARG;
  v.validations[0] = v1;
  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  switch (command) {
    case BRANCH:
      s.pc += (int)args.args[0].label;
      break;
    default:
      break;
  }
  return s;
}

void log_registers(State s) {
  int i = 0;
  printf("registers: [");
  for (; i < NUM_REGISTERS; i++) {
    printf("%i, ", s.registers[i]);
  }
  printf("]\n");
}

void log_mem(State s) {
  int i = 0;
  printf("mem: [");
  for (; i < MEM_BYTES; i++) {
    if (i % 48 == 0) {
      printf("\n");
    }
    printf("%i, ", s.memory[i]);
  }
  printf("]\n");
}

bool validate_args(Args args, ArgValidations validations) {
  if (args.count != validations.expected_arg_count) {
    printf("%s: expected %i arguments, got %i\n", validations.cmd_pretty_str,
           validations.expected_arg_count, args.count);
    return false;
  }
  int i = 0;
  for (; i < args.count; i++) {
    ArgType cmd_type = args.args[i].tag;
    ArgType expected = validations.validations[i].expected_arg_type;

    if (expected == REGISTER_OR_CONSTANT && !(cmd_type == REGISTER) &&
        !(cmd_type == CONSTANT)) {
      printf("%s: expected arg %i to be a register or a constant.\n",
             validations.cmd_pretty_str, i + 1);
      return false;
    }
    if (expected != REGISTER_OR_CONSTANT && expected != cmd_type) {
      const char* err_msg = "";
      if (expected == REGISTER) {
        err_msg = "register";
      } else if (expected == CONSTANT) {
        err_msg = "constant";
      }
      if (expected == ADDRESS) {
        err_msg = "address";
      }
      printf("%s: expected arg %i to be a %s.\n", validations.cmd_pretty_str,
             i + 1, err_msg);
      return false;
    }
  }
  return true;
}

int get_register_or_constant(State s, Arg a) {
  int val = 0;
  if (a.tag == REGISTER) {
    val = s.registers[a.reg];
  } else if (a.tag == CONSTANT) {
    val = a.constant;
  }
  return val;
}

void log_line(Line line) {
  int i = 0;
  printf("> ");
  for (; i < line.len; i++) {
    Token t = line.tokens[i];
    int j = 0;
    for (; j < t.len; j++) {
      putchar(t.tok[j]);
    }
    putchar(' ');
  }
  putchar('\n');
}

void log_tokenized_program(TokenizedProgram p) {
  printf("\n\nTokenized Program:\n");
  printf("Line count: %i\n", p.len);

  int i = 0;
  for (; i < p.len; i++) {
    putchar('\n');
    int j = 0;
    Line line = p.lines[i];
    printf("%i. (Toks: %i):", i, line.len);
    for (; j < line.len; j++) {
      int t = 0;
      putchar(' ');
      Token token = line.tokens[j];
      for (; t < token.len; t++) {
        putchar(token.tok[t]);
      }
    }
  }
  printf("\n\n");
}