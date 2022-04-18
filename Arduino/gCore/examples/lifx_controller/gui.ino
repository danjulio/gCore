/*
 * GUI related routines
 *   - Initialization
 *   - Display configuration
 *   - Top line status updates
 *   - Control callbacks
 */

// ================================================
// Constants
// ================================================

// Vertical alignment
#define GUI_ROW1_Y 8
#define GUI_ROW2_Y 60
#define GUI_ROW3_Y 120

// Kelvin scaling
#define KELVIN_MIN 2600
#define KELVIN_MAX 9200

// Touch control scaling (provides a better "landing" zone at the ends)
// Position within the 256 pixel touch control distance to start/end
// changing output value
#define TOUCH_MIN 12
#define TOUCH_MAX 244



// ================================================
// External images
// ================================================
LV_IMG_DECLARE(brightness_selector);
LV_IMG_DECLARE(color_selector);
LV_IMG_DECLARE(temp_selector);
LV_IMG_DECLARE(temp_selector_R);



// ================================================
// LVGL Objects
// ================================================

// Screen
lv_obj_t* scr_main;

// Screen header objects (always visible)
lv_obj_t* lbl_wifi_status;
lv_obj_t* lbl_batt_status;

// Setup items (shown while connecting to wifi or device discovery)
lv_obj_t* lbl_setup_status;
lv_obj_t* sp_setup;

// Device selector (displayed after devices found)
lv_obj_t* dd_selector;

// Device controls (conditionally displayed based on the selected device capabilities)
//
// Color device
lv_obj_t* im_color_picker;
lv_obj_t* im_c_warmth_picker;
lv_obj_t* im_c_brightness_picker;

// Adjustable white device
lv_obj_t* im_w_warmth_picker;
lv_obj_t* im_w_brightness_picker;

// Non-adjustable white device
lv_obj_t* im_w_switch;

// Status update task
lv_task_t* task_status;



// ================================================
// GUI Variables
// ================================================
Device* sel_lifx_device = NULL;



// ================================================
// GUI API Routines
// ================================================

void gui_init()
{
  scr_main = lv_cont_create(NULL, NULL);

  lbl_wifi_status = lv_label_create(scr_main, NULL);
  lv_obj_set_pos(lbl_wifi_status, 10, GUI_ROW1_Y);
  lv_label_set_recolor(lbl_wifi_status, true);
  lv_label_set_static_text(lbl_wifi_status, "#404040 " LV_SYMBOL_WIFI "#");

  lbl_batt_status = lv_label_create(scr_main, NULL);
  lv_obj_set_pos(lbl_batt_status, 290, GUI_ROW1_Y);
  lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_EMPTY);

  // All other objects start off hidden

  lbl_setup_status = lv_label_create(scr_main, NULL);
  lv_obj_set_pos(lbl_setup_status, 0, GUI_ROW2_Y);
  lv_label_set_long_mode(lbl_setup_status, LV_LABEL_LONG_BREAK);
  lv_label_set_align(lbl_setup_status, LV_LABEL_ALIGN_CENTER);
  lv_obj_set_width(lbl_setup_status, 320);
  lv_obj_set_hidden(lbl_setup_status, true);

  sp_setup = lv_spinner_create(scr_main, NULL);
  lv_obj_set_pos(sp_setup, 128, GUI_ROW3_Y);
  lv_obj_set_width(sp_setup, 64);
  lv_obj_set_height(sp_setup, 64);
  lv_obj_set_hidden(sp_setup, true);

  dd_selector = lv_dropdown_create(scr_main, NULL);
  lv_obj_set_pos(dd_selector, 80, GUI_ROW2_Y);
  lv_obj_set_width(dd_selector, 160);
  lv_obj_set_hidden(dd_selector, true);
  lv_obj_set_event_cb(dd_selector, cb_selector);

  im_color_picker = lv_img_create(scr_main, NULL);
  lv_obj_set_pos(im_color_picker, 12, GUI_ROW3_Y);
  lv_img_set_src(im_color_picker, &color_selector);
  lv_obj_set_hidden(im_color_picker, true);
  lv_obj_set_click(im_color_picker, true);
  lv_obj_set_event_cb(im_color_picker, cb_color_picker);

  im_c_warmth_picker = lv_img_create(scr_main, NULL);
  lv_obj_set_pos(im_c_warmth_picker, 12 + 256 + 8, GUI_ROW3_Y);
  lv_img_set_src(im_c_warmth_picker, &temp_selector_R);
  lv_obj_set_hidden(im_c_warmth_picker, true);
  lv_obj_set_click(im_c_warmth_picker, true);
  lv_obj_set_event_cb(im_c_warmth_picker, cb_warmth_picker);

  im_c_brightness_picker = lv_img_create(scr_main, NULL);
  lv_obj_set_pos(im_c_brightness_picker, 12, GUI_ROW3_Y + 256 + 8);
  lv_img_set_src(im_c_brightness_picker, &brightness_selector);
  lv_obj_set_hidden(im_c_brightness_picker, true);
  lv_obj_set_click(im_c_brightness_picker, true);
  lv_obj_set_event_cb(im_c_brightness_picker, cb_brightness_picker);

  im_w_warmth_picker = lv_img_create(scr_main, NULL);
  lv_obj_set_pos(im_w_warmth_picker, 32, GUI_ROW3_Y);
  lv_img_set_src(im_w_warmth_picker, &temp_selector);
  lv_obj_set_hidden(im_w_warmth_picker, true);
  lv_obj_set_click(im_w_warmth_picker, true);
  lv_obj_set_event_cb(im_w_warmth_picker, cb_warmth_picker);

  im_w_brightness_picker = lv_img_create(scr_main, NULL);
  lv_obj_set_pos(im_w_brightness_picker, 32, GUI_ROW3_Y + 32 + 16);
  lv_img_set_src(im_w_brightness_picker, &brightness_selector);
  lv_obj_set_hidden(im_w_brightness_picker, true);
  lv_obj_set_click(im_w_brightness_picker, true);
  lv_obj_set_event_cb(im_w_brightness_picker, cb_brightness_picker);

  im_w_switch = lv_switch_create(scr_main, NULL);
  lv_obj_set_pos(im_w_switch, 144, GUI_ROW3_Y);
  lv_obj_set_hidden(im_w_switch, true);
  lv_obj_set_event_cb(im_w_switch, cb_switch);

  task_status = lv_task_create(cb_status_udpate_task, 1000, LV_TASK_PRIO_MID, NULL);

  lv_disp_load_scr(scr_main);
}


void gui_display_mode(int m)
{
  switch (m) {
    case GUI_WIFI_SETUP:
      lv_label_set_static_text(lbl_setup_status, "Connecting to Wifi");
      lv_obj_set_hidden(lbl_setup_status, false);
      lv_obj_set_hidden(sp_setup, false);
      break;
    case GUI_NO_WIFI:
      lv_label_set_static_text(lbl_setup_status, "Could not connect to Wifi");
      lv_obj_set_hidden(sp_setup, true);
      break;
    case GUI_DEV_SCAN:
      lv_label_set_static_text(lbl_setup_status, "Scanning for devices");
      break;
    case GUI_NO_DEV:
      lv_label_set_static_text(lbl_setup_status, "Could not find any devices");
      lv_obj_set_hidden(sp_setup, true);
      break;
    case GUI_CTRL_DEV:
      lv_obj_set_hidden(lbl_setup_status, true);
      lv_obj_set_hidden(sp_setup, true);
      lv_obj_set_hidden(dd_selector, false);
      gui_setup_device_list();
      gui_setup_device_controls();
      break;
  }
}



// ================================================
// GUI internal subroutines
// ================================================
void gui_setup_device_list()
{
  char* buf;
  int l = 0;
  uint16_t i;
  uint16_t n;
  Device* dev;
  
  // Compute the length of all device names
  n = lifx.DeviceCount();
  for (i=0; i<n; i++) {
    dev = lifx.GetIndexedDevice(i);
    l += strlen(dev->Label) + 1;
  }

  // Get buffer and fill with device name list for drop-down menu
  buf = (char*) malloc(l);
  if (buf != NULL) {
    l = 0;  // Used as index into buf
    for (i=0; i<n; i++) {
      dev = lifx.GetIndexedDevice(i);
      if (i < (n-1)) {
        sprintf(&buf[l], "%s\n", dev->Label);
      } else {
        // Last entry doesn't have trailing "\n"
        sprintf(&buf[l], "%s", dev->Label);
      }
      l = strlen(buf);
    }

    lv_dropdown_set_options(dd_selector, buf);
  }
  
  // Set first entry
  lv_dropdown_set_selected(dd_selector, 0);
  sel_lifx_device = lifx.GetIndexedDevice(0);
}


void gui_setup_device_controls()
{
  lifx_light_type l_type;
  uint32_t i;
  uint32_t pid;

  pid = sel_lifx_device->Product;
  i = lifx_find_pid_index(pid);
  if (i >= 0) {
    l_type = lifx_types[i].color;
    switch (l_type) {
      case COLOR:
        lv_obj_set_hidden(im_color_picker, false);
        lv_obj_set_hidden(im_c_warmth_picker, false);
        lv_obj_set_hidden(im_c_brightness_picker, false);
        lv_obj_set_hidden(im_w_warmth_picker, true);
        lv_obj_set_hidden(im_w_brightness_picker, true);
        lv_obj_set_hidden(im_w_switch, true);
        break;
      case ADJ_WHITE:
        lv_obj_set_hidden(im_color_picker, true);
        lv_obj_set_hidden(im_c_warmth_picker, true);
        lv_obj_set_hidden(im_c_brightness_picker, true);
        lv_obj_set_hidden(im_w_warmth_picker, false);
        lv_obj_set_hidden(im_w_brightness_picker, false);
        lv_obj_set_hidden(im_w_switch, true);
        break;
      case WHITE:
        lv_obj_set_hidden(im_color_picker, true);
        lv_obj_set_hidden(im_c_warmth_picker, true);
        lv_obj_set_hidden(im_c_brightness_picker, true);
        lv_obj_set_hidden(im_w_warmth_picker, true);
        lv_obj_set_hidden(im_w_brightness_picker, false);
        lv_obj_set_hidden(im_w_switch, true);
        break;
      default:
        lv_obj_set_hidden(im_color_picker, true);
        lv_obj_set_hidden(im_c_warmth_picker, true);
        lv_obj_set_hidden(im_c_brightness_picker, true);
        lv_obj_set_hidden(im_w_warmth_picker, true);
        lv_obj_set_hidden(im_w_brightness_picker, true);
        lv_obj_set_hidden(im_w_switch, false);
    }
  }
}


uint16_t get_touch_point(uint16_t raw, uint16_t start)
{
  uint16_t coord;

  // Compute the real coordinate value in the control
  coord = raw - start;

  if (coord < TOUCH_MIN) {
    coord = 0;
  } else if (coord > TOUCH_MAX) {
    coord = 255;
  } else {
    // Scale the coordinate for 0-255 within the legal range
    coord = (coord - TOUCH_MIN) * 255 / (TOUCH_MAX - TOUCH_MIN + 1);
  }

  return coord;
}



// ================================================
// GUI Object callbacks
// ================================================

void cb_status_udpate_task(lv_task_t* task)
{
  bool wifi_connected;
  int batt_level;                           // 0: Empty, 1: 25%, 2: 50%, 3: 75%, 4: 100%
  uint16_t batt_mv;
  static bool prev_wifi_connected = false;
  static int prev_batt_level = 0;
  static char wifi_buf[13];                 // Recolor + icon + terminating null

  // Get current state
  (void) gc.gcore_get_reg16(GCORE_REG_VB, &batt_mv);
  wifi_connected = (WiFi.status() == WL_CONNECTED);

  // Compute battery level - roughly based on curve in https://www.richtek.com/battery-management/en/designing-liion.html
  // for 0.2C load
  if (batt_mv >= 3870) batt_level = 4;
  else if (batt_mv >= 3720) batt_level = 3;
  else if (batt_mv >= 3680) batt_level = 2;
  else if (batt_mv >= 3600) batt_level = 1;
  else batt_level = 0;

  // Update battery level if necessary
  if (batt_level != prev_batt_level) {
    prev_batt_level = batt_level;

    switch (batt_level) {
      case 0:
        lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_EMPTY);
        break;
      case 1:
        lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_1);
        break;
      case 2:
        lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_2);
        break;
      case 3:
        lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_3);
        break;
      default:
        lv_label_set_static_text(lbl_batt_status, LV_SYMBOL_BATTERY_FULL);
    }
  }

  // Update wifi status if necessary
  if (wifi_connected != prev_wifi_connected) {
    prev_wifi_connected = wifi_connected;
    
    if (wifi_connected) {
      sprintf(wifi_buf, "#D0D0D0 " LV_SYMBOL_WIFI "#");
    } else {
      sprintf(wifi_buf, "#404040 " LV_SYMBOL_WIFI "#");
    }
    wifi_buf[12] = 0;
    lv_label_set_static_text(lbl_wifi_status, wifi_buf);
  }
}


void cb_selector(lv_obj_t* obj, lv_event_t event)
{
  int n;

  if (event == LV_EVENT_VALUE_CHANGED) {
    n = lv_dropdown_get_selected(obj);
    sel_lifx_device = lifx.GetIndexedDevice(n);
    gui_setup_device_controls();
    note_activity();
  }
}


void cb_color_picker(lv_obj_t* obj, lv_event_t event)
{
  lv_indev_t* touch;
  lv_point_t cur_point;
  uint16_t x, y;
  uint32_t h, s;
  uint16_t b, k;
  uint16_t left, top;

  if (event == LV_EVENT_PRESSING) {
    touch = lv_indev_get_act();
    lv_indev_get_point(touch, &cur_point);

    // Get the starting coordinates of this object
    left = lv_obj_get_x(obj);
    top = lv_obj_get_y(obj);

    // Compute the touch point
    x = get_touch_point(cur_point.x, left);
    y = get_touch_point(cur_point.y, top);

    // Compute the hue and saturation
    h = y * 65535 / 255;
    s = 65535 - (x * 65535 / 255);

    // Get the current brightness and kelvin values
    b = sel_lifx_device->Brightness;
    k = sel_lifx_device->Kelvin;

    lifx.SetDeviceColor(sel_lifx_device, (uint16_t) h, (uint16_t) s, b, k);
    note_activity();
  }
}


void cb_warmth_picker(lv_obj_t* obj, lv_event_t event)
{
  lv_indev_t* touch;
  lv_point_t cur_point;
  uint16_t p;
  uint32_t k;
  uint16_t h, s, b;
  uint16_t left, top;

  if (event == LV_EVENT_PRESSING) {
    touch = lv_indev_get_act();
    lv_indev_get_point(touch, &cur_point);

    // Get the starting coordinates of this object
    left = lv_obj_get_x(obj);
    top = lv_obj_get_y(obj);

    // Compute the kelvin with coordinates based on the control
    if (obj == im_c_warmth_picker) {
      // Color warmth picker is vertical (warm at top / min y)
      p = get_touch_point(cur_point.y, top);
      k = KELVIN_MIN + (p * (KELVIN_MAX - KELVIN_MIN) / 255);
    } else {
      // Adjustable white picker is horizantal (warm at right / max x)
      p = get_touch_point(cur_point.x, left);
      k = KELVIN_MAX - (p * (KELVIN_MAX - KELVIN_MIN) / 255);
    }

    // Get the current hue, saturation and brightness values
    h = sel_lifx_device->Hue;
    s = sel_lifx_device->Saturation;
    b = sel_lifx_device->Brightness;

    lifx.SetDeviceColor(sel_lifx_device, h, s, b, (uint16_t) k);
    note_activity();
  }
}


void cb_brightness_picker(lv_obj_t* obj, lv_event_t event)
{
  lv_indev_t* touch;
  lv_point_t cur_point;
  uint16_t x;
  uint32_t b;
  uint16_t p;
  uint16_t left;

  if (event == LV_EVENT_PRESSING) {
    touch = lv_indev_get_act();
    lv_indev_get_point(touch, &cur_point);

    // Get the starting coordinates of this object
    left = lv_obj_get_x(obj);

    // Compute the brightness
    x = get_touch_point(cur_point.x, left);
    b = x * 65535 / 256;

    // Set the brightness
    lifx.SetDeviceBrightness(sel_lifx_device, (uint16_t) b);

    // We have to turn the bulb on/off if the brightness should change the power state
    if ((b == 0) && (sel_lifx_device->Power != 0)) {
      lifx.SetDevicePower(sel_lifx_device, 0);
    } else if ((b != 0) && (sel_lifx_device->Power == 0)) {
      lifx.SetDevicePower(sel_lifx_device, 65535);
    }
    note_activity();
  }
}


void cb_switch(lv_obj_t* obj, lv_event_t event)
{
  uint16_t p;

  if (event == LV_EVENT_VALUE_CHANGED) {
    if (lv_switch_get_state(obj)) {
      p = 65535;
    } else {
      p = 0;
    }

    lifx.SetDevicePower(sel_lifx_device, p);
    note_activity();
  }
}
