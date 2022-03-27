/*
 * Implement the calculator functionality
 */

// Calculator logic states
#define CALC_ST_ENT_A  0
#define CALC_ST_ENT_OP 1
#define CALC_ST_ENT_B  2
#define CALC_ST_RES    3

// Calculator operations
#define CALC_OP_NUL 0
#define CALC_OP_ADD 1
#define CALC_OP_SUB 2
#define CALC_OP_MUL 3
#define CALC_OP_DIV 4
#define CALC_OP_EQ  5
#define CALC_OP_END 6
#define CALC_OP_AND 7
#define CALC_OP_OR  8
#define CALC_OP_NOR 9
#define CALC_OP_XOR 10
#define CALC_OP_L1  11
#define CALC_OP_R1  12
#define CALC_OP_ROL 13
#define CALC_OP_ROR 14
#define CALC_OP_XLY 15
#define CALC_OP_XRY 16
#define CALC_OP_2S  17
#define CALC_OP_1S  18


//
// Calculator state
//
int calc_state;
int calc_num_bits;
int calc_op_val;
bool calc_base16;
uint64_t calc_valid_mask;
uint64_t calc_op_A;
uint64_t calc_op_B;
uint64_t calc_mem;



// ================================================
// Calculator logic API
// ================================================

void calc_init()
{
  calc_state = nv_get_state();
  calc_op_val = nv_get_op();
  calc_base16 = nv_get_base16();
  calc_num_bits = nv_get_num_bits();
  calc_valid_mask = calc_bits_to_mask(calc_num_bits);
  calc_op_A = nv_get_val(0);
  calc_op_B = nv_get_val(1);
  calc_mem = nv_get_val(2);
}


void calc_save_state()
{
  nv_set_state(calc_state);
  nv_set_op(calc_op_val);
  nv_set_num_bits(calc_num_bits);
  nv_set_base16(calc_base16);
  nv_set_val(0, calc_op_A);
  nv_set_val(1, calc_op_B);
  nv_set_val(2, calc_mem);

  nv_save();
}


void calc_set_bits(int n)
{
  calc_num_bits = n;

  calc_valid_mask = calc_bits_to_mask(calc_num_bits);

  calc_op_A &= calc_valid_mask;
  calc_op_B &= calc_valid_mask;
  calc_update_display();  
}


int calc_get_bits()
{
  return calc_num_bits;
}


void calc_set_base16(bool b16)
{
  calc_base16 = b16;
  calc_update_display();
}


bool calc_get_base16()
{
  return calc_base16;
}


uint64_t calc_get_op_val()
{
  return ((calc_state == CALC_ST_ENT_A) || (calc_state == CALC_ST_RES)) ? calc_op_A : calc_op_B;
}


void calc_put_op_val(uint64_t v)
{
  if ((calc_state == CALC_ST_ENT_A) || (calc_state == CALC_ST_RES)) {
    calc_op_A = v;
  } else {
    calc_op_B = v;
  }
}


void calc_update_display()
{
  gui_update_display(calc_get_op_val());
}


void calc_btn_AC()
{
  // All clear
  calc_state = CALC_ST_ENT_A;
  calc_op_val = CALC_OP_NUL;
  calc_op_A = 0;
  calc_op_B = 0;
  gui_update_display(0);
}


void calc_btn_BKSP()
{
  // Backspace one digit
  if ((calc_state == CALC_ST_ENT_A) || (calc_state == CALC_ST_RES)) {
    calc_op_A /= (calc_base16) ? 16 : 10;
    gui_update_display(calc_op_A);
  } else {
    calc_op_B /= (calc_base16) ? 16 : 10;
    gui_update_display(calc_op_B);
  }
}


void calc_btn_CLR()
{
  // Clear last entry
  if (calc_state == CALC_ST_ENT_A) {
    calc_op_A = 0;
    gui_update_display(0);
  } else if (calc_state == CALC_ST_ENT_B) {
    calc_op_B = 0;
    gui_update_display(0);
  } else if (calc_state == CALC_ST_RES) {
    calc_btn_AC();
  }
}


void calc_btn_VAL(uint8_t v, uint16_t alt_v)
{
  uint16_t add_val;
  uint16_t mult_val;
  uint64_t t;

  // Handle states
  if (calc_state == CALC_ST_RES) {
    // User starts entering a number while displaying a result
    t = 0;
    calc_state = CALC_ST_ENT_A;
  } else {
    if (calc_state == CALC_ST_ENT_OP) {
      calc_state = CALC_ST_ENT_B;
    }
    
    // Get the current operand
    t = calc_get_op_val();
  }

  // Determine if we are using the normal value or an alternate value (used for '0' and '00' when
  // the value is currently not 0)
  if ((t != 0) && (alt_v != 0)) {
    add_val = 0;
    mult_val = alt_v;
  } else {
    add_val = v;
    if (v == 0xFF) {
      // Handle the special case of FF
      mult_val = 256;
    } else {
      mult_val = (calc_base16) ? 16 : 10;
    }
  }

  // Compute the new value
  t = ((t * mult_val) + add_val) & calc_valid_mask;

  // Finally copy back the [possibly] updated operand and update the display
  calc_put_op_val(t);
  gui_update_display(t);
}


// Operand for two argument calculation
void calc_btn_op(uint8_t op)
{
  // Immediately perform the previous op if we're currently entering the second operand
  if (calc_state == CALC_ST_ENT_B) {
    calc_btn_imm(CALC_OP_EQ);
  }

  // Setup for another operand
  calc_op_val = op;
  calc_state = CALC_ST_ENT_OP;
}


// Operand for immediate calculation
void calc_btn_imm(uint8_t op)
{
  uint64_t t;

  t = calc_get_op_val();
  
  switch (op) {
    case CALC_OP_EQ:
      switch (calc_op_val) {
        case CALC_OP_ADD:
          t = (calc_op_A + calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_SUB:
          t = (calc_op_A - calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_MUL:
          t = (calc_op_A * calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_DIV:
          // Like the Apple calculator we just return 0 for a division by 0
          if (calc_op_B == 0) {
            t = 0;
          } else {
            t = (calc_op_A / calc_op_B) & calc_valid_mask;
          }
         break;
        case CALC_OP_AND:
          t = (calc_op_A & calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_OR:
          t = (calc_op_A | calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_NOR:
          t = ~(calc_op_A | calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_XOR:
          t = (calc_op_A ^ calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_XLY:
          t = (calc_op_A << calc_op_B) & calc_valid_mask;
          break;
        case CALC_OP_XRY:
          t = (calc_op_A >> calc_op_B) & calc_valid_mask;
          break;
      }
      calc_state = CALC_ST_RES;
      break;
    case CALC_OP_END:
      t = calc_swap_endian(t);
      break;
    case CALC_OP_L1:
      t = (t << 1) & calc_valid_mask;
      break;
    case CALC_OP_R1:
      t = (t >> 1) & calc_valid_mask;
      break;
    case CALC_OP_ROL:
      if (t & ((uint64_t) 1 << (calc_num_bits - 1))) {
        // Rotate MSbit into LSbit
        t = ((t << 1) & calc_valid_mask) + 1;
      } else {
        t = (t << 1) & calc_valid_mask;
      }
      break;
    case CALC_OP_ROR:
      if (t & 0x1) {
        // Rotate LSbit into MSbit
        t = ((t >> 1) & calc_valid_mask) | ((uint64_t) 1 << (calc_num_bits - 1));
      } else {
        t = (t >> 1) & calc_valid_mask;
      }
      break;
    case CALC_OP_2S:
      t = (~t + 1) & calc_valid_mask;
      break;
    case CALC_OP_1S:
      t = ~t & calc_valid_mask;
      break;
  }

  // Also perform the previous op if we've just modified the second operand
  if (calc_state == CALC_ST_ENT_B) {
    calc_op_B = t;
    calc_btn_imm(CALC_OP_EQ);
    t = calc_op_A; // Get the result of the recursive call
  }

  // Prepare this result to be the first operand of a subsequent calculation
  calc_op_A = t;
  calc_op_B = 0;
  calc_state = CALC_ST_RES;
  gui_update_display(t);
}


// Memory functions don't affect calculator state
void calc_btn_MC()
{
  calc_mem = 0;
}


void calc_btn_MADD()
{
  calc_mem = (calc_mem + calc_get_op_val()) & calc_valid_mask;
}


void calc_btn_MR()
{
  uint64_t t;

  t = calc_mem & calc_valid_mask;
  calc_put_op_val(t);
  gui_update_display(t);
}



// ================================================
// Calc module private routines
// ================================================
uint64_t calc_bits_to_mask(int n)
{
  uint64_t m;
  
  switch (n) {
    case 8:
      m = 0xFF;
      break;
    case 16:
      m = 0xFFFF;
      break;
    case 24:
      m = 0xFFFFFF;
      break;
    case 32:
      m = 0xFFFFFFFF;
      break;
    case 40:
      m = 0xFFFFFFFFFF;
      break;
    case 48:
      m = 0xFFFFFFFFFFFF;
      break;
    case 56:
      m = 0xFFFFFFFFFFFFFF;
      break;
    default:
      m = 0xFFFFFFFFFFFFFFFF;
  }

  return m;
}


uint64_t calc_swap_endian(uint64_t v)
{
  uint64_t t;
  
  // Swapping depends on number of bits
  switch (calc_num_bits) {
    case 8:
      // Nothing happens for 8-bits
      t = v;
      break;
    case 16:
      t = (v >> 8) & 0xFF;
      t |= (v << 8) & 0xFF00;
      break;
    case 24:
      // Odd byte configurations like this don't really make sense but we try something anyway
      t = (v >> 16) & 0xFF;
      t |= (v & 0xFF00);
      t |= (v << 16) & 0xFF0000;
      break;
    case 32:
      t = (v >> 24) & 0xFF;
      t |= (v >> 8) & 0xFF00;
      t |= (v << 8) & 0xFF0000;
      t |= (v << 24) & 0xFF000000;
      break;
    case 40:
      t = (v >> 32) & 0xFF;
      t |= (v >> 16) & 0xFF00;
      t |= (v & 0xFF0000);
      t |= (v << 16) & 0xFF000000;
      t |= (v << 32) & 0xFF00000000;
      break;
    case 48:
      t = (v >> 40) & 0xFF;
      t |= (v >> 24) & 0xFF00;
      t |= (v >> 8) & 0xFF0000;
      t |= (v << 8) & 0xFF000000;
      t |= (v << 24) & 0xFF00000000;
      t |= (v << 40) & 0xFF0000000000;
      break;
    case 56:
      t = (v >> 48) & 0xFF;
      t |= (v >> 32) & 0xFF00;
      t |= (v >> 16) & 0xFF0000;
      t |= (v & 0xFF000000);
      t |= (v << 16) & 0xFF00000000;
      t |= (v << 32) & 0xFF0000000000;
      t |= (v << 48) & 0xFF000000000000;
      break;
    default:
      t = (v >> 56) & 0xFF;
      t |= (v >> 40) & 0xFF00;
      t |= (v >> 24) & 0xFF0000;
      t |= (v >> 8) & 0xFF000000;
      t |= (v << 8) & 0xFF00000000;
      t |= (v << 24) & 0xFF0000000000;
      t |= (v << 40) & 0xFF000000000000;
      t |= (v << 56) & 0xFF00000000000000;
  }

  return t & calc_valid_mask;
}
