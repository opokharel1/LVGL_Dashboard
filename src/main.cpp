/*Using LVGL with Arduino requires some extra steps:
 *Be sure to read the docs here: https://docs.lvgl.io/master/get-started/platforms/arduino.html  */

#include <lvgl.h>

#if LV_USE_TFT_ESPI
#include <TFT_eSPI.h>
#endif

/*Set to your screen resolution*/
#define TFT_HOR_RES   480
#define TFT_VER_RES   320

/*LVGL draw into this buffer, 1/10 screen size usually works well. The size is in bytes*/
#define DRAW_BUF_SIZE (TFT_HOR_RES * TFT_VER_RES / 10 * (LV_COLOR_DEPTH / 8))
// uint32_t draw_buf[DRAW_BUF_SIZE / 4];
void *draw_buf;
unsigned long lastTickMillis = 0;


void setup()
{
    String LVGL_Arduino = "Hello Arduino! ";
    LVGL_Arduino += String('V') + lv_version_major() + "." + lv_version_minor() + "." + lv_version_patch();
    /*Set a tick source so that LVGL will know how much time elapsed. */
    // lv_tick_set_cb(lvgl_millis_wrapper);
    Serial.begin(115200);
    delay(2000); /*Wait for serial monitor to open*/
    Serial.println( LVGL_Arduino );

    lv_init();

    draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL); 

  lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);

    lv_obj_t *label = lv_label_create( lv_scr_act() );
    lv_label_set_text( label, "Hello Arduino, I'm LVGL!" );
    lv_obj_align( label, LV_ALIGN_CENTER, 0, 0 );

    Serial.println( "Setup done" );
}

void loop()
{
  unsigned long tickPeriod = millis() - lastTickMillis;
  lastTickMillis = millis();
  lv_tick_inc(tickPeriod); /*Tell LVGL how many milliseconds has elapsed*/
  lv_task_handler(); /* let the GUI do its work */
  delay(5); /* let this time pass */
}