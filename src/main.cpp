/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <GT911.h>


/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
// uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// this is for the I2C touch
// DEFINE YOUR PINS HERE
#define TOUCH_SDA  33
#define TOUCH_SCL  32
#define TOUCH_INT  21   // or whatever pin you're using
#define TOUCH_RST  25   // or whatever pin you're using


void *draw_buf;

unsigned long lastTickMillis = 0;

GT911 ts = GT911();

void my_touch_read(lv_indev_t * indev, lv_indev_data_t * data) {
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
  if (ts.touched()) {
    GTPoint* p = ts.getPoints();
    data->point.x = TFT_HOR_RES - p->y;  // flip x if needed
    data->point.y = p->x;
    Serial.printf("Touch at %d,%d\n", data->point.x, data->point.y);
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();

    Serial.begin(115200);
    delay(2000); /*Wait for serial monitor to open*/
    Serial.println( LVGL_Arduino );

    lv_init();

    // Initialize I2C 
    Wire.begin(TOUCH_SDA, TOUCH_SCL);

    // Then initialize GT911 with INT and RST pins
    ts.begin(TOUCH_INT, TOUCH_RST);  

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL); 

    lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    // create input device for lvgl
    // for general touch screen, choose pointer type
    lv_indev_t * indev = lv_indev_create();

    // for encoder or physical button, choose other type
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_read);
    // implement callback function for touch events (called by lvgl systems and checks if there are touch events)
    

    lv_obj_t *label = lv_label_create( lv_scr_act() );
    lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    Serial.println( "Setup done" );
}

void loop()
{
  unsigned long tickPeriod = millis() - lastTickMillis;
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
    if (touches) {
    GTPoint* tp = ts.getPoints();
    for (uint8_t i = 0; i < touches; i++) {
      Serial.printf("#%d  %d,%d s:%d\n", tp[i].trackId, tp[i].x, tp[i].y, tp[i].area);
    }
  }
  lastTickMillis = millis();
  lv_tick_inc(tickPeriod); /*Tell LVGL how many milliseconds has elapsed*/
  lv_task_handler(); /* let the GUI do its work */
  delay(5); /* let this time pass */
}