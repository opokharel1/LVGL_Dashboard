/* Using LVGL with Arduino and TFT_eSPI
   Display label and button with touch support
   Works with GT911 I2C touch
*/
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <Wire.h>
#include <GT911.h>

/* Screen resolution */
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/* LVGL draw buffer size (1/10 of screen) */
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))

/* I2C touch pins */
#define TOUCH_SDA  33
#define TOUCH_SCL  32
#define TOUCH_INT  21
#define TOUCH_RST  25

/* Global objects */
void *draw_buf;
unsigned long lastTickMillis = 0;
GT911 ts = GT911();

/* Button event callback - must be defined before use */
void button_event_cb(lv_event_t * e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_CLICKED) {
    Serial.println("Button Pressed!");
  }
}

/* Touch read callback */
void my_touch_read(lv_indev_t * indev, lv_indev_data_t * data) {
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
  if (touches) {
    GTPoint* p = ts.getPoints();
    data->point.x = TFT_HOR_RES - p->y;  // flip if needed
    data->point.y = p->x;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Starting LVGL Example...");
  
  /* Initialize LVGL */
  lv_init();
  
  /* Initialize I2C for touch */
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  
  /* Initialize GT911 touch */
  ts.begin(TOUCH_INT, TOUCH_RST);
  
  /* Allocate LVGL draw buffer */
  draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!draw_buf) {
    Serial.println("Failed to allocate LVGL draw buffer!");
    while(1);
  }
  
  /* Initialize TFT display for LVGL */
  lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  
  /* Setup touch input device for LVGL */
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  
  /* Create label */
  lv_obj_t *label = lv_label_create(lv_scr_act());
  lv_label_set_text(label, "Hello Arduino, I'm LVGL!");
  lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
  
  /* Create a button */
  lv_obj_t *btn = lv_btn_create(lv_scr_act());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 60);
  
  /* Create label on button */
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Press Me");
  lv_obj_center(btn_label);
  
  /* Add button event callback - FIXED: Added NULL as 4th parameter */
  lv_obj_add_event_cb(btn, button_event_cb, LV_EVENT_CLICKED, NULL);
  
  Serial.println("Setup done!");
}

void loop() {
  /* Calculate elapsed time for LVGL tick */
  unsigned long tickPeriod = millis() - lastTickMillis;
  lastTickMillis = millis();
  
  /* Update LVGL tick */
  lv_tick_inc(tickPeriod);
  
  /* Handle LVGL tasks */
  lv_timer_handler();  // Changed from lv_task_handler() to lv_timer_handler()
  
  delay(5); // Small delay for stability
}