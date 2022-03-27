/*
 * LVGL Hardware abstraction layer
 */

void lvgl_setup()
{

  tft.begin(); /* TFT init */
  tft.setRotation(3);

  ts_begin();

  lv_init();

  // LVGL display drawing buffer allocation
  lv_disp_buf_init(&disp_buf, buf, NULL, LV_HOR_RES_MAX * 10);

  // Initialize the display
  lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = disp_flush;
  disp_drv.buffer = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  // Initialize the touchscreen driver
  ts_begin();

  // Initialize the input device driver
  lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_tp_read;
  lv_indev_drv_register(&indev_drv);

  //Set the theme..
  lv_theme_t * th = lv_theme_material_init(LV_THEME_DEFAULT_COLOR_PRIMARY, LV_THEME_DEFAULT_COLOR_SECONDARY, LV_THEME_MATERIAL_FLAG_DARK , LV_THEME_DEFAULT_FONT_SMALL , LV_THEME_DEFAULT_FONT_NORMAL, LV_THEME_DEFAULT_FONT_SUBTITLE, LV_THEME_DEFAULT_FONT_TITLE);     
  lv_theme_set_act(th);
}


void disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint16_t c;
  uint32_t len;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, (area->x2 - area->x1 + 1), (area->y2 - area->y1 + 1));

  len = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);
  tft.pushPixels((uint16_t*) color_p, len);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}
