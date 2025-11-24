#include "oarm.h"
#include "ostd.h"

/*#define LOG_VERBOSE*/
#define LOG_NONE

ResultState entry(int argc, char** argv) {
#ifndef LOG_NONE
  printf("oarm v0.1\n____\n\n");
#endif

  ResultState r;

  FILE* input_stream = NULL;
  if (argc <= 1) {
    print_help();
    r.return_val = 0;
    return r;
  } else {
    s8 arg1 = s8_from(malloc, argv[1]);
    if (s8_eq(s8_from(malloc, "--help"), arg1)) {
      print_help();
      r.return_val = 0;
      return r;
    } else if (s8_eq(s8_from(malloc, "--docs"), arg1)) {
      print_docs();
      r.return_val = 0;
      return r;
    } else {
      input_stream = fopen(argv[1], "r");
      if (input_stream == NULL) {
        perror("Error opening file");
        r.return_val = 1;
        return r;
      }
    }
  }

  /*Copy the file into memory*/
  /*fun fact, there is a race condition between getting size of file and
   * allocating memory. Whatever though, don't run the assembler while editing
   * the file.*/
  s8 program;
  fseek(input_stream, 0, SEEK_END);
  i64 fsize = ftell(input_stream);
  fseek(input_stream, 0, SEEK_SET);
  program.str = malloc((u64)(fsize + 1));
  fread(program.str, (u64)fsize, 1, input_stream);
  fclose(input_stream);
  program.str[fsize] = EOF;
  program.len = (int)fsize + 1;

  TokenizedProgram program_tokens = tokenize(program);

#ifdef LOG_VERBOSE
  log_tokenized_program(program_tokens);
#endif

  State s;
  memset(s.memory, 0, sizeof(int) * MEM_BYTES);
  memset(s.registers, 0, sizeof(int) * NUM_REGISTERS);
  s.pc = 0;
  s.cont = true;
  s.cmp = 0;
  s.labels = resolve_labels(program_tokens);
  program_tokens = resolve_register_labels(program_tokens);
  while (s.cont) {
    s = tick(s, program_tokens.lines[s.pc]);
    if (s.pc > program_tokens.len || s.pc < 0) {
      s.cont = false;
    }
  }

  /*This is a short lived program, so I purposefully am not freeing anything.
   * The OS can do that for me.*/
  r.return_val = 0;
  r.state = s;
  return r;
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
      "  --help              Show this help message and exit\n"
      "  --docs              Show documentation\n");
}

void print_docs(void) {
  printf(
      "oarm (Orion's subset of ARM assembly) documentation\n"
      "\n"
      "Arguments:\n"
      "  constant: ex: \'#10\' is the integer 10, \'#-33\' is negative 33\n"
      "  register: ex: \'x0\' is the first register, \'x9\' is the last "
      "register\n"
      "  memory address: ex: \'[#1]\' is memory address 1, \'[x2]\' is the "
      "address of the value in register x2\n"
      "\n"
      "Debugging:\n"
      "  reg - print all registers\n"
      "  mem - print all memory\n"
      "  rpc - print the program counter\n"
      "  rcb - print the comparison byte\n"
      "\n"
      "Registers + Memory:\n"
      "  mov - move a constant or register value to a register ex: \'mov x0, "
      "x0, #1\'\n"
      "  ldr - load value at memory address into register ex: \'ldr x0, "
      "[#1]\'\n"
      "  str - store the value from register into memory ex: \'str x0, [#1]\'\n"
      "\n"
      "Arithmetic:\n"
      "  add - add two register or constant values and store in register ex: "
      "\'add x0, x0, #1\' increments x0 by 1\n"
      "  sub - subtract\n"
      "  lsl - bitwise shift left ex: \'lsl x0, x0, #1\' shifts the value in "
      "x0 left 1\n"
      "  lsr - bitwise shift right\n"
      "\n"
      "Branches:\n"
      "  cmp - compare two register or constant values, sets the sign byte to "
      "-1, 0, or 1 ex: \'cmp x0, #1\'\n"
      "  <label name>: - labels are arbitrary strings with a colon ex: "
      "\'exit:\' declares the exit label\n"
      "  b - branch (jump) to the label specified ex: \'b exit\' jumps the "
      "exit label\n"
      "  beq - branch if equal, jumps if the cmp byte is 0 ex: \'beq exit\'\n"
      "  bne - branch if not equal\n"
      "  blt - branch if less than\n"
      "  ble - branch if less than or equal\n"
      "  bgt - branch if greater than\n"
      "  bge - branch if greater than or equal\n"
      "\n"
      "Register Labels:\n"
      "  .reg <label_name> <register> - give pretty name to register ex: "
      "\'.reg counter x0\' lets you use the word \'counter\' in place of \'x0\'"
      "\n\n"
      "");
}

TokenizedProgram tokenize(s8 s) {
  int program_size = 2;
  TokenizedProgram program;
  program.len = 0;
  program.lines = (Line*)malloc((size_t)program_size * sizeof(Line));
  memset(program.lines, 0, (size_t)program_size * sizeof(Line));
  s8 t;
  t.len = 0;
  t.str = malloc(sizeof(u8) * MAX_IDENT_LEN);
  memset(t.str, 0, sizeof(char) * MAX_IDENT_LEN);

  int i = 0;
  for (; i < s.len; i++) {
    char c = s.str[i];
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
      case '\0':
      case EOF:
      case '\n':
        if (is_token_empty && num_tokens == 0) {
          break;
        }
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
          t.str[t.len] = c;
          t.len++;
        }
        /*Push token onto program struct.*/
        program.lines[li].tokens[num_tokens] = t;
        program.lines[li].len = num_tokens + 1;

        /* reset token*/
        t.len = 0;
        t.str = malloc(sizeof(u8) * MAX_IDENT_LEN);
        memset(t.str, 0, sizeof(char) * MAX_IDENT_LEN);
        break;
      default:
        if (t.len >= MAX_IDENT_LEN) {
          /*just grow the token and get rid of max identifier*/
          printf("Warning: max identifier length of %i exceeded",
                 MAX_IDENT_LEN);
          break;
        }
        t.str[t.len] = c;
        t.len++;
    }
  }
  return program;
}

Map resolve_labels(TokenizedProgram p) {
  /*Find all label declarations and store line number.*/
  Map labels = map_init(malloc, 10);
  int ln = 0;
  for (; ln < p.len; ln++) {
    Line line = p.lines[ln];
    if (line.len == 1) {
      s8 t = line.tokens[0];
      if (t.len > 0 && ':' == t.str[t.len - 1]) {
        t.len--;
        labels = map_set(malloc, labels, t, ln);
      }
    }
  }

  return labels;
}

TokenizedProgram resolve_register_labels(TokenizedProgram p) {
  /*Find all register label declarations and replace references to them with the
   * register they point too.*/

  Map register_labels = map_init(malloc, 10);
  s8 reg_keyword = s8_from(malloc, ".reg");
  int ln = 0;
  for (; ln < p.len; ln++) {
    Line line = p.lines[ln];
    bool is_register_label_decl =
        line.len == 3 && s8_eq(line.tokens[0], reg_keyword);
    if (is_register_label_decl) {
      s8 reg_str;
      reg_str.str = line.tokens[2].str + 1;
      reg_str.len = line.tokens[2].len - 1;
      ResultInt r = parse_int(reg_str);
      if (!r.ok) {
        printf("warning register label failed to parse\n");
      }
      register_labels = map_set(malloc, register_labels, line.tokens[1], r.val);
      continue;
    }
    int j = 0;
    for (; j < line.len; j++) {
      /*register label tokens could only be bare, or wrapped in []

      if it starts with square [], strip those before checking map
      */
      s8 t = line.tokens[j];

      if (t.len > 2 && t.str[0] == '[') {
        t.len -= 1;
        t.str++;
      }

      ResultInt r = map_get(register_labels, t);
      if (r.ok) {
        char ascii_num = (char)(r.val + '0');

        char buf[2];
        buf[0] = ascii_num;
        buf[1] = '\0';

        line.tokens[j] =
            s8_concat(malloc, s8_from(malloc, "x"), s8_from(malloc, buf));
      }
    }
  }

  return p;
}

State tick(State s, Line line) {
/*Evaluate one line of asm.*/
#ifndef LOG_NONE
  log_line(line);
#endif
  if (line.len < 1) {
    printf("warning:tick: empty line\n");
    s.cont = false;
    return s;
  }
  s8 t = line.tokens[0];
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
    case CMP:
      s = cmp(s, line);
      break;
    case RCB:
      printf("cmp: %i\n", s.cmp);
      break;
    case UNKNOWN:
      printf("Error could not parse statement identifier: %c%c%c\n", t.str[0],
             t.str[1], t.str[2]);
      break;
    case REG_LABEL:
    case LABEL_DECL:
      /*Label declarations dont do anything. They can be jumped too.*/
      break;
  }
  s.pc++;
  return s;
}

CMD identify_cmd(s8 t) {
  if (t.str[t.len - 1] == ':') {
    return LABEL_DECL;
  }
  int key = (t.str[0] << 16) | (t.str[1] << 8) | t.str[2];
  switch (key) {
    case ('m' << 16) | ('e' << 8) | 'm':
      return MEM;
    case ('m' << 16) | ('o' << 8) | 'v':
      return MOV;
    case ('a' << 16) | ('d' << 8) | 'd':
      return ADD;
    case ('r' << 16) | ('e' << 8) | 'g':
      return REG;
    case ('r' << 16) | ('e' << 8) | 't':
      return RET;
    case ('l' << 16) | ('s' << 8) | 'l':
      return LSL;
    case ('l' << 16) | ('s' << 8) | 'r':
      return LSR;
    case ('s' << 16) | ('u' << 8) | 'b':
      return SUB;
    case ('l' << 16) | ('d' << 8) | 'r':
      return LDR;
    case ('s' << 16) | ('t' << 8) | 'r':
      return STR;
    case ('b' << 16) | ('e' << 8) | 'q':
      return BEQ;
    case ('b' << 16) | ('n' << 8) | 'e':
      return BNE;
    case ('b' << 16) | ('l' << 8) | 't':
      return BLT;
    case ('b' << 16) | ('l' << 8) | 'e':
      return BLE;
    case ('b' << 16) | ('g' << 8) | 't':
      return BGT;
    case ('b' << 16) | ('g' << 8) | 'e':
      return BGE;
    case ('b' << 16) | (0 << 8) | 0:
      return BRANCH;
    case ('r' << 16) | ('p' << 8) | 'c':
      return RPC;
    case ('c' << 16) | ('m' << 8) | 'p':
      return CMP;
    case ('r' << 16) | ('c' << 8) | 'b':
      return RCB;
    case ('.' << 16) | ('r' << 8) | 'e':
      return REG_LABEL;
  }
  return UNKNOWN;
}

ResultInt parse_int(s8 s) {
  /*Convert s8 char array to int.*/
  ResultInt r;

  if (s.len > 8) {
    printf(
        "Integer overflow detected in parse int. Max int is 8 digits. "
        "Truncating digits.\n");
    r.ok = false;
    r.val = 0;
    return r;
  }

  i32 result = 0;
  i32 place = 1;
  i32 i = s.len - 1;
  i32 sign = 1;
  i32 end = 0;
  if (s.str[0] == '-') {
    sign = -1;
    end = 1;
  }

  for (; i >= end; i--) {
    if (s.str[i] < (int)'0' || s.str[i] > (int)'9') {
      printf("Non digit detected in parse int string: %x (%i)\n", s.str[i],
             s.str[i]);
      r.ok = false;
      r.val = 0;
      return r;
    }
    result += place * (s.str[i] - (int)'0');
    place = place * 10;
  }
  r.ok = true;
  r.val = result * sign;
  return r;
}

Args parse_args(Line line) {
  Args args;
  args.count = 0;
  args.is_valid = true;
  int i = 1;
  for (; i < line.len; i++) {
    s8 t = line.tokens[i];
    Arg a;

    /*Address argument */
    if (t.str[0] == '[') {
      a.tag = ADDRESS;
      if (t.str[1] == 'x') {
        a.addr.type = A_REGISTER;
      } else if (t.str[1] == '#') {
        a.addr.type = A_CONSTANT;
      } else {
        printf(
            "Argument %i, unsupported address type, must be register or "
            "constant.",
            args.count + 1);
        args.is_valid = false;
        return args;
      }

      t.str = t.str + 2;
      t.len = t.len - 3;
      if (t.len < 1) {
        printf(
            "Argument %i, expected numerical value for memory "
            "address argument, got empty string.\n",
            args.count + 1);
        args.is_valid = false;
        return args;
      }
      ResultInt r = parse_int(t);
      if (!r.ok) {
        args.is_valid = false;
        return args;
      }
      a.addr.val = r.val;

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
    else if (t.str[0] == 'x') {
      a.tag = REGISTER;
      t.str += 1;
      t.len -= 1;
      ResultInt r = parse_int(t);
      if (!r.ok) {
        args.is_valid = false;
        return args;
      }
      a.reg = r.val;
      if (a.reg > NUM_REGISTERS || a.reg < 0) {
        printf(
            "Argument %i register is out of range, must be between 0 and "
            "%i\n",
            args.count + 1, NUM_REGISTERS);
        args.is_valid = false;
        return args;
      }

      /*constant argument*/
    } else if (t.str[0] == '#') {
      a.tag = CONSTANT;
      t.str++;
      t.len--;
      ResultInt r = parse_int(t);
      if (!r.ok) {
        args.is_valid = false;
        return args;
      }
      a.constant = r.val;
      /*label argument*/
    } else {
      a.tag = LABEL_ARG;
      a.label = t;
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
    s.registers[a1.reg] = val1 << val2;
  } else {
    s.registers[a1.reg] = val1 >> val2;
  }
  return s;
}

State cmp(State s, Line line) {
  Args args = parse_args(line);
  if (!args.is_valid) {
    s.cont = false;
    return s;
  }

  ArgValidations v;
  v.expected_arg_count = 2;
  v.cmd_pretty_str = "cmp";
  ArgValidation a1;
  a1.expected_arg_type = REGISTER_OR_CONSTANT;
  ArgValidation a2;
  a2.expected_arg_type = REGISTER_OR_CONSTANT;
  v.validations[0] = a1;
  v.validations[1] = a2;

  if (!validate_args(args, v)) {
    s.cont = false;
    return s;
  }

  int val1 = get_register_or_constant(s, args.args[0]);
  int val2 = get_register_or_constant(s, args.args[1]);
  if (val1 < val2) {
    s.cmp = -1;
  } else if (val1 > val2) {
    s.cmp = 1;
  } else {
    s.cmp = 0;
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

  s8 label = args.args[0].label;
  ResultInt jmp = map_get(s.labels, label);
  if (!jmp.ok) {
    s.cont = false;
    printf("label declaration not found for label: %s", s8_to_c(malloc, label));
    return s;
  }

  switch (command) {
    case BRANCH:
      s.pc = jmp.val;
      break;
    case BLE:
      if (s.cmp <= 0) {
        s.pc = jmp.val;
      }
      break;
    case BLT:
      if (s.cmp < 0) {
        s.pc = jmp.val;
      }
      break;
    case BGE:
      if (s.cmp >= 0) {
        s.pc = jmp.val;
      }
      break;
    case BGT:
      if (s.cmp > 0) {
        s.pc = jmp.val;
      }
      break;
    case BNE:
      if (s.cmp != 0) {
        s.pc = jmp.val;
      }
      break;
    case BEQ:
      if (s.cmp == 0) {
        s.pc = jmp.val;
      }
      break;
    default:
      printf("this should never happen");
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
    s8 t = line.tokens[i];
    int j = 0;
    for (; j < t.len; j++) {
      putchar(t.str[j]);
    }
    putchar(' ');
  }
  putchar('\n');
}

void log_tokenized_program(TokenizedProgram p) {
#ifndef LOG_NONE
  printf("\n\nTokenized Program:\n");
  printf("Line count: %i\n", p.len);
#endif
  int i = 0;
  for (; i < p.len; i++) {
#ifndef LOG_NONE
    putchar('\n');
#endif
    int j = 0;
    Line line = p.lines[i];
#ifndef LOG_NONE
    printf("%i. (Toks: %i):", i, line.len);
#endif
    for (; j < line.len; j++) {
      int t = 0;
#ifndef LOG_NONE
      putchar(' ');
#endif
      s8 token = line.tokens[j];
      for (; t < token.len; t++) {
#ifndef LOG_NONE
        putchar(token.str[t]);
#endif
      }
    }
  }
#ifndef LOG_NONE
  printf("\n\n");
#endif
}