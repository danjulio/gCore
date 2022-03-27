/*
 * Implement the graphical user interface
 */

// Button array definition
static const char* btn_map[] = {"AND", "OR", "D", "E", "F", "AC", "C", "\n",
  "NOR", "XOR", "A", "B", "C", "-", "MC", "\n",
  "<<", ">>", "7", "8", "9", "+", "M+", "\n",
  "RoL", "RoR", "4", "5", "6", "x", "MR", "\n",
  "X<<Y", "X>>Y", "1", "2", "3", "/", "END", "\n",
  "2's", "1's", "FF", "0", "00", "=", "BKSP", ""};

// Button map indicies
#define BTN_AND  0
#define BTN_OR   1
#define BTN_D    2
#define BTN_E    3
#define BTN_F    4
#define BTN_AC   5
#define BTN_CLR  6

#define BTN_NOR  7
#define BTN_XOR  8
#define BTN_A    9
#define BTN_B    10
#define BTN_C    11
#define BTN_SUB  12
#define BTN_MC   13

#define BTN_L1   14
#define BTN_R1   15
#define BTN_7    16
#define BTN_8    17
#define BTN_9    18
#define BTN_ADD  19
#define BTN_MADD 20

#define BTN_ROL  21
#define BTN_ROR  22
#define BTN_4    23
#define BTN_5    24
#define BTN_6    25
#define BTN_MUL  26
#define BTN_MR   27

#define BTN_XLY  28
#define BTN_XRY  29
#define BTN_1    30
#define BTN_2    31
#define BTN_3    32
#define BTN_DIV  33
#define BTN_END  34

#define BTN_2S   35
#define BTN_1S   36
#define BTN_FF   37
#define BTN_0    38
#define BTN_00   39
#define BTN_EQ   40
#define BTN_BKSP 41

const uint16_t hex_digit_indicies[7] = {BTN_A, BTN_B, BTN_C, BTN_D, BTN_E, BTN_F, BTN_FF};


// Background color for switch and drop-down selected item
#define SELECTED_COLOR LV_COLOR_MAKE(0x30, 0x30, 0x50)


// ================================================
// LVGL Objects
// ================================================

// Screen
lv_obj_t* scr_main;

// Number of bits drop-down menu
lv_obj_t* dd_num_bits;

// Display
lv_obj_t* lbl_disp;

// Hex/Decimal switch
lv_obj_t* sw_base;

// Keypad array
lv_obj_t* btnm_keypad;



// ================================================
// GUI API Routines
// ================================================

void gui_init()
{
  scr_main = lv_obj_create(NULL, NULL);

  // Override the focus ring (get rid of it)
  static lv_style_t style_page_override;
  lv_style_init(&style_page_override);
  lv_style_set_outline_width(&style_page_override, LV_STATE_DEFAULT, 0);
  lv_style_set_outline_width(&style_page_override, LV_STATE_FOCUSED, 0);
  lv_style_set_border_width(&style_page_override, LV_STATE_DEFAULT, 0);
  lv_style_set_border_width(&style_page_override, LV_STATE_FOCUSED, 0);

  // Create on-screen widgets
  dd_num_bits = lv_dropdown_create(scr_main, NULL);
  lv_obj_add_style(dd_num_bits, LV_PAGE_PART_BG, &style_page_override);
  lv_obj_set_style_local_bg_color(dd_num_bits, LV_DROPDOWN_PART_SELECTED, LV_STATE_DEFAULT, SELECTED_COLOR);
  lv_obj_set_pos(dd_num_bits, 5, 5);
  lv_obj_set_width(dd_num_bits, 60);
  lv_dropdown_set_options(dd_num_bits, "8\n16\n24\n32\n40\n48\n56\n64");
  lv_dropdown_set_max_height(dd_num_bits, 300);
  lv_obj_set_event_cb(dd_num_bits, gui_cb_num_bits);

  lbl_disp = lv_label_create(scr_main, NULL);
  lv_label_set_long_mode(lbl_disp, LV_LABEL_LONG_BREAK);
  lv_label_set_align(lbl_disp, LV_LABEL_ALIGN_RIGHT);
  lv_obj_set_style_local_text_font(lbl_disp, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_montserrat_28);
  lv_obj_set_style_local_text_color(lbl_disp, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, READOUT_COLOR);
  lv_obj_set_pos(lbl_disp, 70, 10);
  lv_obj_set_width(lbl_disp, 330);
  lv_label_set_text_static(lbl_disp, "0");

  sw_base = lv_switch_create(scr_main, NULL);
  lv_obj_add_style(sw_base, LV_SWITCH_PART_BG, &style_page_override);
  lv_obj_set_style_local_bg_color(sw_base, LV_SWITCH_PART_INDIC, LV_STATE_CHECKED, SELECTED_COLOR);
  lv_obj_set_style_local_bg_color(sw_base, LV_SWITCH_PART_BG, LV_STATE_DEFAULT, SELECTED_COLOR);
  lv_obj_set_style_local_bg_color(sw_base, LV_SWITCH_PART_INDIC, LV_STATE_FOCUSED, SELECTED_COLOR);
  lv_obj_set_pos(sw_base, 410, 10);
  lv_obj_set_width(sw_base, 60);
  lv_obj_set_event_cb(sw_base, gui_cb_sw_base);

  btnm_keypad = lv_btnmatrix_create(scr_main, NULL);
  lv_obj_add_style(btnm_keypad, LV_PAGE_PART_BG, &style_page_override);
  lv_btnmatrix_set_map(btnm_keypad, btn_map);
  lv_obj_set_pos(btnm_keypad, 5, 50);
  lv_obj_set_width(btnm_keypad, 470);
  lv_obj_set_height(btnm_keypad, 270);
  lv_obj_set_event_cb(btnm_keypad, gui_cb_keypad);

  lv_disp_load_scr(scr_main);

  // Get initial values
  switch(calc_get_bits()) {
    case 8:
      lv_dropdown_set_selected(dd_num_bits, 0);
      break;
    case 16:
      lv_dropdown_set_selected(dd_num_bits, 1);
      break;
    case 24:
      lv_dropdown_set_selected(dd_num_bits, 2);
      break;
    case 32:
      lv_dropdown_set_selected(dd_num_bits, 3);
      break;
    case 40:
      lv_dropdown_set_selected(dd_num_bits, 4);
      break;
    case 48:
      lv_dropdown_set_selected(dd_num_bits, 5);
      break;
    case 56:
      lv_dropdown_set_selected(dd_num_bits, 6);
      break;
    default:
      lv_dropdown_set_selected(dd_num_bits, 7);
  }
  
  if (calc_get_base16()) {
    lv_switch_on(sw_base, false);
    gui_enable_hex_btns(true);
  } else {
    lv_switch_off(sw_base, false);
    gui_enable_hex_btns(false);
  }

  calc_update_display();
}


void gui_update_display(uint64_t v)
{
  static char buf[21];                      // Sized to hold the longest (base10) number "18446744073709551615"
                                            // Static for LVGL
  int dig[20];                              // Base10 digits in reverse order (lowest first)
  int i;
  int n;
  bool leading_zero;

  // ESP32 Arduino doesn't print 64-bit numbers so we have to piece together a result
  if (calc_get_base16()) {
    // Break the number down into individual hexadecimal digit values
    for (i=0; i<16; i++) {
      n = v & 0xF;
      v = v >> 4;
      dig[i] = n;
    }
    
    // Then "print" the digits to the character array, blanking leading zeros
    leading_zero = true;
    n = 0;
    for (i=15; i>=0; i--) {
      if (dig[i] != 0) {
        leading_zero = false;
        if (dig[i] > 9) {
          buf[n++] = 'A' + (dig[i] - 10);
        } else {
          buf[n++] = '0' + dig[i];
        }
      } else if ((!leading_zero) || (i == 0)) {
        buf[n++] = '0';
      }
    }
  } else {
    // Break the number down into individual decimal digit values
    for (i=0; i<20; i++) {
      n = v % 10;
      v = v / 10;
      dig[i] = n;
    }

    // Then "print" the digits to the character array, blanking leading zeros
    leading_zero = true;
    n = 0;
    for (i=19; i>=0; i--) {
      if (dig[i] != 0) {
        leading_zero = false;
        buf[n++] = '0' + dig[i];
      } else if ((!leading_zero) || (i == 0)) {
        buf[n++] = '0';
      }
    }
  }
  buf[n] = 0;  // terminate the string

  // Update the display
  lv_label_set_text_static(lbl_disp, buf);
}



// ================================================
// GUI Object callbacks
// ================================================
void gui_enable_hex_btns(bool en)
{
  int i;

  for (i=0; i<7; i++) {
    if (en) {
      lv_btnmatrix_clear_btn_ctrl(btnm_keypad, hex_digit_indicies[i], LV_BTNMATRIX_CTRL_DISABLED);
    } else {
      lv_btnmatrix_set_btn_ctrl(btnm_keypad, hex_digit_indicies[i], LV_BTNMATRIX_CTRL_DISABLED);
    }
  }
}


void gui_cb_sw_base(lv_obj_t* obj, lv_event_t event)
{
  bool sw_val;
  
  if (event == LV_EVENT_VALUE_CHANGED) {
    sw_val = lv_switch_get_state(obj);
    calc_set_base16(sw_val);
    gui_enable_hex_btns(sw_val);
    note_activity();
  }
}


void gui_cb_num_bits(lv_obj_t* obj, lv_event_t event)
{
  uint16_t n;
  
  if (event == LV_EVENT_VALUE_CHANGED) {
    n = lv_dropdown_get_selected(obj);
    switch (n) {
      case 0:
        calc_set_bits(8);
        break;
      case 1:
        calc_set_bits(16);
        break;
      case 2:
        calc_set_bits(24);
        break;
      case 3:
        calc_set_bits(32);
        break;
      case 4:
        calc_set_bits(40);
        break;
      case 5:
        calc_set_bits(48);
        break;
      case 6:
        calc_set_bits(56);
        break;
      default:
        calc_set_bits(64);
    }
    note_activity();
  }
}


void gui_cb_keypad(lv_obj_t* obj, lv_event_t event)
{
  uint16_t n;
  
  if (event == LV_EVENT_VALUE_CHANGED) {
    n = lv_btnmatrix_get_active_btn(obj);
    switch (n) {
      case BTN_0:
        calc_btn_VAL(0, calc_get_base16() ? 16 : 10);
        break;
      case BTN_00:
        calc_btn_VAL(0, calc_get_base16() ? 256 : 100 );
        break;
      case BTN_1:
        calc_btn_VAL(1, 0);
        break;
      case BTN_2:
        calc_btn_VAL(2, 0);
        break;
      case BTN_3:
        calc_btn_VAL(3, 0);
        break;
      case BTN_4:
        calc_btn_VAL(4, 0);
        break;
      case BTN_5:
        calc_btn_VAL(5, 0);
        break;
      case BTN_6:
        calc_btn_VAL(6, 0);
        break;
      case BTN_7:
        calc_btn_VAL(7, 0);
        break;
      case BTN_8:
        calc_btn_VAL(8, 0);
        break;
      case BTN_9:
        calc_btn_VAL(9, 0);
        break;
      case BTN_A:
        calc_btn_VAL(0xA, 0);
        break;
      case BTN_B:
        calc_btn_VAL(0xB, 0);
        break;
      case BTN_C:
        calc_btn_VAL(0xC, 0);
        break;
      case BTN_D:
        calc_btn_VAL(0xD, 0);
        break;
      case BTN_E:
        calc_btn_VAL(0xE, 0);
        break;
      case BTN_F:
        calc_btn_VAL(0xF, 0);
        break;
      case BTN_FF:
        calc_btn_VAL(0xFF, 0);
        break;
        
      case BTN_AC:
        calc_btn_AC();
        break;
      case BTN_CLR:
        calc_btn_CLR();
        break;
      case BTN_BKSP:
        calc_btn_BKSP();
        break;

      case BTN_SUB:
        calc_btn_op(CALC_OP_SUB);
        break;
      case BTN_ADD:
        calc_btn_op(CALC_OP_ADD);
        break;
      case BTN_MUL:
        calc_btn_op(CALC_OP_MUL);
        break;
      case BTN_DIV:
        calc_btn_op(CALC_OP_DIV);
        break;
      case BTN_EQ:
        calc_btn_imm(CALC_OP_EQ);
        break;
      case BTN_MC:
        calc_btn_MC();
        break;
      case BTN_MADD:
        calc_btn_MADD();
        break;
      case BTN_MR:
        calc_btn_MR();
        break;
      case BTN_END:
        calc_btn_imm(CALC_OP_END);
        break;

      case BTN_AND:
        calc_btn_op(CALC_OP_AND);
        break;
      case BTN_OR:
        calc_btn_op(CALC_OP_OR);
        break;
      case BTN_NOR:
        calc_btn_op(CALC_OP_NOR);
        break;
      case BTN_XOR:
        calc_btn_op(CALC_OP_XOR);
        break;
      case BTN_L1:
        calc_btn_imm(CALC_OP_L1);
        break;
      case BTN_R1:
        calc_btn_imm(CALC_OP_R1);
        break;
      case BTN_ROL:
        calc_btn_imm(CALC_OP_ROL);
        break;
      case BTN_ROR:
        calc_btn_imm(CALC_OP_ROR);
        break;
      case BTN_XLY:
        calc_btn_op(CALC_OP_XLY);
        break;
      case BTN_XRY:
        calc_btn_op(CALC_OP_XRY);
        break;
      case BTN_2S:
        calc_btn_imm(CALC_OP_2S);
        break;
      case BTN_1S:
        calc_btn_imm(CALC_OP_1S);
        break;
    }

    note_activity();
  }
}
