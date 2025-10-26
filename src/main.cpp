/* LVGL with Arduino - Two Screen Navigation
   Screen 1: Button to navigate
   Screen 2: Serial input display with back button
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

/* Screen objects */
lv_obj_t *screen1;  // Home screen
lv_obj_t *screen2;  // Serial display screen

/* Screen 2 UI elements */
lv_obj_t *textarea;
lv_obj_t *status_label;
lv_obj_t *counter_label;
int message_count = 0;

/* Touch read callback */
void my_touch_read(lv_indev_t * indev, lv_indev_data_t * data) {
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
  if (touches) {
    GTPoint* p = ts.getPoints();
    data->point.x = TFT_HOR_RES - p->y;
    data->point.y = p->x;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* Navigate to Serial Monitor Screen */
void goto_serial_screen(lv_event_t * e) {
  Serial.println("Navigating to Serial Monitor screen...");
  lv_scr_load(screen2);
}

/* Navigate back to Home Screen */
void goto_home_screen(lv_event_t * e) {
  Serial.println("Navigating to Home screen...");
  lv_scr_load(screen1);
}

/* Add message to serial display */
void add_message_to_display(String msg) {
  Serial.println("Adding message to display...");
  
  message_count++;
  
  // Update counter
  char counter_text[32];
  snprintf(counter_text, sizeof(counter_text), "%d msgs", message_count);
  lv_label_set_text(counter_label, counter_text);
  
  // Build message with timestamp
  unsigned long time_sec = millis() / 1000;
  char full_message[256];
  snprintf(full_message, sizeof(full_message), "[%02lu:%02lu] %s\n", 
           time_sec / 60, time_sec % 60, msg.c_str());
  
  // Add to textarea
  lv_textarea_add_text(textarea, full_message);
  
  // Scroll to bottom
  lv_obj_scroll_to_y(textarea, LV_COORD_MAX, LV_ANIM_OFF);
  
  // Update status
  lv_label_set_text(status_label, "New Message!");
  
  Serial.println("Message added successfully");
}

/* Create Screen 1 - Home Screen */
void create_screen1() {
  screen1 = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen1, lv_color_hex(0x1a1a1a), 0);
  
  /* Title label */
  lv_obj_t *label = lv_label_create(screen1);
  lv_label_set_text(label, "LVGL Navigation Demo");
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
  lv_obj_align(label, LV_ALIGN_CENTER, 0, -60);
  
  /* Instruction label */
  lv_obj_t *inst_label = lv_label_create(screen1);
  lv_label_set_text(inst_label, "Press button to view Serial Monitor");
  lv_obj_set_style_text_color(inst_label, lv_color_hex(0xaaaaaa), 0);
  lv_obj_align(inst_label, LV_ALIGN_CENTER, 0, 0);
  
  /* Button to go to serial screen */
  lv_obj_t *btn = lv_btn_create(screen1);
  lv_obj_set_size(btn, 200, 60);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 80);
  lv_obj_set_style_bg_color(btn, lv_color_hex(0x2196F3), 0);
  
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Serial Monitor");
  lv_obj_set_style_text_font(btn_label, &lv_font_montserrat_18, 0);
  lv_obj_center(btn_label);
  
  lv_obj_add_event_cb(btn, goto_serial_screen, LV_EVENT_CLICKED, NULL);
  
  Serial.println("Screen 1 created");
}

/* Create Screen 2 - Serial Display Screen */
void create_screen2() {
  screen2 = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen2, lv_color_hex(0x1a1a1a), 0);
  
  /* Top Status Bar */
  lv_obj_t *top_panel = lv_obj_create(screen2);
  lv_obj_set_size(top_panel, TFT_HOR_RES, 50);
  lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(top_panel, lv_color_hex(0x2196F3), 0);
  lv_obj_set_style_border_width(top_panel, 0, 0);
  lv_obj_set_style_radius(top_panel, 0, 0);
  
  status_label = lv_label_create(top_panel);
  lv_label_set_text(status_label, "Serial Monitor");
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_18, 0);
  lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 10, 0);
  
  counter_label = lv_label_create(top_panel);
  lv_label_set_text(counter_label, "0 msgs");
  lv_obj_set_style_text_color(counter_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(counter_label, &lv_font_montserrat_14, 0);
  lv_obj_align(counter_label, LV_ALIGN_RIGHT_MID, -10, 0);
  
  /* Main Text Area */
  textarea = lv_textarea_create(screen2);
  lv_obj_set_size(textarea, TFT_HOR_RES - 20, TFT_VER_RES - 130);
  lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 60);
  lv_textarea_set_text(textarea, "Waiting for serial data...\n");
  
  lv_obj_set_style_bg_color(textarea, lv_color_hex(0x2d2d2d), 0);
  lv_obj_set_style_text_color(textarea, lv_color_hex(0x00ff00), 0);
  lv_obj_set_style_border_color(textarea, lv_color_hex(0x2196F3), 0);
  lv_obj_set_style_border_width(textarea, 2, 0);
  lv_obj_set_style_radius(textarea, 8, 0);
  lv_obj_set_style_text_font(textarea, &lv_font_montserrat_14, 0);
  lv_obj_set_style_pad_all(textarea, 10, 0);
  
  lv_textarea_set_one_line(textarea, false);
  lv_obj_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_AUTO);
  
  /* Back Button */
  lv_obj_t *back_btn = lv_btn_create(screen2);
  lv_obj_set_size(back_btn, 120, 50);
  lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x666666), 0);
  
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, "< Back");
  lv_obj_set_style_text_font(back_label, &lv_font_montserrat_16, 0);
  lv_obj_center(back_label);
  
  lv_obj_add_event_cb(back_btn, goto_home_screen, LV_EVENT_CLICKED, NULL);
  
  Serial.println("Screen 2 created");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== LVGL Two Screen Navigation ===");
  Serial.println("Send messages via Serial Monitor");
  Serial.println("They will appear on Screen 2\n");
  
  /* Initialize LVGL */
  lv_init();
  
  /* Initialize I2C for touch */
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  
  /* Initialize GT911 touch */
  ts.begin(TOUCH_INT, TOUCH_RST);
  Serial.println("Touch controller initialized");
  
  /* Allocate LVGL draw buffer */
  draw_buf = heap_caps_malloc(DRAW_BUF_SIZE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
  if (!draw_buf) {
    Serial.println("Failed to allocate LVGL draw buffer!");
    while(1);
  }
  Serial.printf("Draw buffer allocated: %d bytes\n", DRAW_BUF_SIZE);
  
  /* Initialize TFT display for LVGL */
  lv_display_t * disp = lv_tft_espi_create(TFT_HOR_RES, TFT_VER_RES, draw_buf, DRAW_BUF_SIZE);
  Serial.println("Display initialized");
  
  /* Setup touch input device for LVGL */
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  Serial.println("Touch input configured");
  
  /* Create both screens */
  create_screen1();
  create_screen2();
  
  /* Load home screen first */
  lv_scr_load(screen1);
  
  Serial.println("=== Setup Complete ===");
  Serial.println("Touch the button to navigate!\n");
}

unsigned long last_status_reset = 0;

void loop() {
  /* Calculate elapsed time for LVGL tick */
  unsigned long tickPeriod = millis() - lastTickMillis;
  lastTickMillis = millis();
  
  /* Update LVGL tick */
  lv_tick_inc(tickPeriod);
  
  /* Handle LVGL tasks */
  lv_timer_handler();
  
  /* Reset status label on screen 2 */
  if (millis() - last_status_reset > 2000 && last_status_reset > 0) {
    lv_label_set_text(status_label, "Serial Monitor");
    last_status_reset = 0;
  }
  
  /* Handle serial input */
  if (Serial.available()) {
    String serialInput = Serial.readStringUntil('\n');
    serialInput.trim();
    
    if (serialInput.length() > 0) {
      Serial.printf("Received: %s\n", serialInput.c_str());
      
      // Add message to screen 2
      add_message_to_display(serialInput);
      
      last_status_reset = millis();
    }
  }
  
  delay(5);
}