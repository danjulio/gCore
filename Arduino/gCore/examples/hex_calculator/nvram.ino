/*
 * Store/Retrieve calculator state from NVRAM
 * 
 * NVRAM Layout
 *   Offset    Contents
 *   ----------------------------------------------------------
 *     0       Magic Byte 0 - "0xCA" - used to verify legal contents
 *     1       Magic Byte 1 - "0x1C"
 *     2       Number of bits
 *     3       Hex display mode flag
 *     4       Calculator state
 *     5       Current op
 *    6-13     Op A
 *    14-21    Op B
 *    22-29    Mem
 */

// ================================================
// NVRAM MODULE CONSTANTS AND VARIABLES
// ================================================

// Magic byte values
#define NV_MAGIC_BYTE_0 0xCA
#define NV_MAGIC_BYTE_1 0x1C

// Offsets
#define NV_MAGIC_0  0
#define NV_MAGIC_1  (NV_MAGIC_0 + 1)
#define NV_NUM_BITS (NV_MAGIC_1 + 1)
#define NV_MODE_BIT (NV_NUM_BITS + 1)
#define NV_CALC_ST  (NV_MODE_BIT + 1)
#define NV_CALC_OP  (NV_CALC_ST + 1)
#define NV_OP_A     (NV_CALC_OP + 1)
#define NV_OP_B     (NV_OP_A + 8)
#define NV_MEM      (NV_OP_B + 8)

#define NV_LEN      (NV_MEM + 8)

// NVRAM local array
uint8_t nv_array[NV_LEN];



// ================================================
// NVRAM API
// ================================================
void nv_init()
{
  int i;
  
  // Read NVRAM into our local array
  (void) gc.gcore_get_nvram_bytes(0, nv_array, NV_LEN);

  // Look for the Magic Bytes to validate the contents
  if (!((nv_array[NV_MAGIC_0] == NV_MAGIC_BYTE_0) && (nv_array[NV_MAGIC_1] == NV_MAGIC_BYTE_1))) {
    nv_array[NV_MAGIC_0] = NV_MAGIC_BYTE_0;
    nv_array[NV_MAGIC_1] = NV_MAGIC_BYTE_1;
    nv_array[NV_NUM_BITS] = 64;
    nv_array[NV_MODE_BIT] = 0;  // Decimal
    nv_array[NV_CALC_ST] = CALC_ST_ENT_A;
    nv_array[NV_CALC_OP] = CALC_OP_NUL;

    // Set all values to 0
    for (i=NV_OP_A; i<NV_LEN; i++) {
      nv_array[i] = 0;
    }

    Serial.println("Initialized NVRAM");
  }
}


void nv_save()
{
  (void) gc.gcore_set_nvram_bytes(0, nv_array, NV_LEN);
}


int nv_get_num_bits()
{
  return (int) nv_array[NV_NUM_BITS];
}


void nv_set_num_bits(int n)
{
  nv_array[NV_NUM_BITS] = (uint8_t) n;
}


bool nv_get_base16()
{
  return (nv_array[NV_MODE_BIT] == 0) ? false : true;
}


void nv_set_base16(bool f)
{
  nv_array[NV_MODE_BIT] = f ? 1 : 0;
}


int nv_get_state()
{
  return (int) nv_array[NV_CALC_ST];
}


void nv_set_state(int st)
{
  nv_array[NV_CALC_ST] = (uint8_t) st;
}


int nv_get_op()
{
  return (int) nv_array[NV_CALC_OP];
}


void nv_set_op(int op)
{
  nv_array[NV_CALC_OP] = (uint8_t) op;
}


uint64_t nv_get_val(int n)
{
  uint64_t* tP;
  
  switch (n) {
    case 0:
      tP = (uint64_t *) &nv_array[NV_OP_A];
      break;
    case 1:
      tP = (uint64_t *) &nv_array[NV_OP_B];
      break;
    default:
      tP = (uint64_t *) &nv_array[NV_MEM];
  }

  return *tP;
}


void nv_set_val(int n, uint64_t v)
{
  uint64_t* tP;
  
  switch (n) {
    case 0:
      tP = (uint64_t *) &nv_array[NV_OP_A];
      break;
    case 1:
      tP = (uint64_t *) &nv_array[NV_OP_B];
      break;
    default:
      tP = (uint64_t *) &nv_array[NV_MEM];
  }

  *tP = v;
}
