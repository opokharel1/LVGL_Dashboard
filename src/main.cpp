#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>
#include <GT911.h>

#define SD_CS 5
#define TFT_HOR_RES 480
#define TFT_VER_RES 320

/* Touch pins */
#define TOUCH_SDA 33
#define TOUCH_SCL 32
#define TOUCH_INT 21
#define TOUCH_RST 25

GT911 ts = GT911();
void *draw_buf;

/* Buffer to store image data in RAM */
uint8_t *image_data = NULL;
uint32_t image_size = 0;

/* Dashboard UI Elements */
lv_obj_t *speed_label;
lv_obj_t *range_label;
lv_obj_t *avg_wh_label;
lv_obj_t *trip_label;
lv_obj_t *odo_label;
lv_obj_t *avg_kmh_label;
lv_obj_t *motor_temp_label;
lv_obj_t *battery_temp_label;
lv_obj_t *mode_label;
lv_obj_t *status_label;
lv_obj_t *time_label;

/* Dashboard data */
int speed = 0;
int range = 130;
int avg_wh = 40;
int trip = 130;
int odo = 1300;
int avg_kmh = 40;
int motor_temp = 30;
int battery_temp = 30;
String mode = "Eco";
String status = "DISARMED";

/* Touch callback */
void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
  if (touches) {
    GTPoint *p = ts.getPoints();
    data->point.x = TFT_HOR_RES - p->y;
    data->point.y = p->x;
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* Load image from SD card into RAM */
bool load_image_to_ram(const char *path) {
  Serial.printf("Loading image: %s\n", path);

  File file = SD.open(path);
  if (!file) {
    Serial.println("ERROR: Failed to open image file!");
    return false;
  }

  image_size = file.size();
  Serial.printf("Image size: %u bytes\n", image_size);

  image_data = (uint8_t *)malloc(image_size);
  if (!image_data) {
    Serial.println("ERROR: Failed to allocate memory for image!");
    file.close();
    return false;
  }

  size_t bytes_read = file.read(image_data, image_size);
  file.close();

  if (bytes_read != image_size) {
    Serial.printf("ERROR: Read %u bytes, expected %u\n", bytes_read, image_size);
    free(image_data);
    image_data = NULL;
    return false;
  }

  Serial.println("Image loaded into RAM successfully!");
  return true;
}

/* Update time display */
void update_time_display() {
  unsigned long now = millis() / 1000;
  int hours = (now / 3600) % 24;
  int minutes = (now / 60) % 60;
  
  char time_str[16];
  snprintf(time_str, sizeof(time_str), "%d:%02d AM", hours == 0 ? 12 : hours, minutes);
  lv_label_set_text(time_label, time_str);
}

/* Update dashboard values */
void update_dashboard() {
  char buf[32];
  
  // Speed
  snprintf(buf, sizeof(buf), "%d", speed);
  lv_label_set_text(speed_label, buf);
  
  // Range
  snprintf(buf, sizeof(buf), "Range %d km", range);
  lv_label_set_text(range_label, buf);
  
  // Avg W/km
  snprintf(buf, sizeof(buf), "Avg. %d W/km", avg_wh);
  lv_label_set_text(avg_wh_label, buf);
  
  // Trip
  snprintf(buf, sizeof(buf), "TRIP %d km", trip);
  lv_label_set_text(trip_label, buf);
  
  // ODO
  snprintf(buf, sizeof(buf), "ODO %d km", odo);
  lv_label_set_text(odo_label, buf);
  
  // Avg km/h
  snprintf(buf, sizeof(buf), "AVG. %d km/h", avg_kmh);
  lv_label_set_text(avg_kmh_label, buf);
  
  // Motor temp
  snprintf(buf, sizeof(buf), "Motor %d°C", motor_temp);
  lv_label_set_text(motor_temp_label, buf);
  
  // Battery temp
  snprintf(buf, sizeof(buf), "Battery %d°C", battery_temp);
  lv_label_set_text(battery_temp_label, buf);
  
  // Mode
  lv_label_set_text(mode_label, mode.c_str());
  
  // Status
  lv_label_set_text(status_label, status.c_str());
  
  // Time
  update_time_display();
}

/* Create EV Dashboard UI */
void create_ev_dashboard_ui() {
  Serial.println("Creating EV dashboard UI...");
  
  lv_obj_t *scr = lv_scr_act();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_hex(0xe5e5e5), 0);

  /* Top bar */
  lv_obj_t *top_bar = lv_obj_create(scr);
  lv_obj_set_size(top_bar, TFT_HOR_RES, 60);
  lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(top_bar, lv_color_white(), 0);
  lv_obj_set_style_border_width(top_bar, 0, 0);
  lv_obj_set_style_radius(top_bar, 0, 0);
  lv_obj_set_style_pad_all(top_bar, 0, 0);

  // Time
  time_label = lv_label_create(top_bar);
  lv_label_set_text(time_label, "9:41 AM");
  lv_obj_set_style_text_color(time_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_18, 0);
  lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 0);

  // Menu button (left)
  lv_obj_t *menu_btn = lv_label_create(top_bar);
  lv_label_set_text(menu_btn, "≡ Menu");
  lv_obj_set_style_text_font(menu_btn, &lv_font_montserrat_16, 0);
  lv_obj_align(menu_btn, LV_ALIGN_LEFT_MID, 10, 0);

  // Map button (right)
  lv_obj_t *map_btn = lv_label_create(top_bar);
  lv_label_set_text(map_btn, "⚲ Map");
  lv_obj_set_style_text_font(map_btn, &lv_font_montserrat_16, 0);
  lv_obj_align(map_btn, LV_ALIGN_RIGHT_MID, -10, 0);

  /* Status badge */
  lv_obj_t *status_badge = lv_obj_create(scr);
  lv_obj_set_size(status_badge, 140, 40);
  lv_obj_align(status_badge, LV_ALIGN_TOP_MID, 0, 70);
  lv_obj_set_style_bg_color(status_badge, lv_color_hex(0x333333), 0);
  lv_obj_set_style_radius(status_badge, 20, 0);
  lv_obj_set_style_border_width(status_badge, 0, 0);

  status_label = lv_label_create(status_badge);
  lv_label_set_text(status_label, "DISARMED");
  lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_16, 0);
  lv_obj_center(status_label);

  /* Main speed display (center) */
  speed_label = lv_label_create(scr);
  lv_label_set_text(speed_label, "0");
  lv_obj_set_style_text_color(speed_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(speed_label, &lv_font_montserrat_48, 0);
  lv_obj_align(speed_label, LV_ALIGN_CENTER, 0, -20);

  // "Km/h" label
  lv_obj_t *kmh_label = lv_label_create(scr);
  lv_label_set_text(kmh_label, "Km/h");
  lv_obj_set_style_text_color(kmh_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(kmh_label, &lv_font_montserrat_16, 0);
  lv_obj_align(kmh_label, LV_ALIGN_CENTER, 0, 20);

  /* Mode selector (center bottom) */
  lv_obj_t *mode_container = lv_obj_create(scr);
  lv_obj_set_size(mode_container, 200, 60);
  lv_obj_align(mode_container, LV_ALIGN_CENTER, 0, 80);
  lv_obj_set_style_bg_color(mode_container, lv_color_white(), 0);
  lv_obj_set_style_radius(mode_container, 10, 0);
  lv_obj_set_style_border_width(mode_container, 0, 0);

  lv_obj_t *mode_text = lv_label_create(mode_container);
  lv_label_set_text(mode_text, "Mode");
  lv_obj_set_style_text_color(mode_text, lv_color_black(), 0);
  lv_obj_set_style_text_font(mode_text, &lv_font_montserrat_14, 0);
  lv_obj_align(mode_text, LV_ALIGN_TOP_MID, 0, 5);

  mode_label = lv_label_create(mode_container);
  lv_label_set_text(mode_label, "Eco");
  lv_obj_set_style_text_color(mode_label, lv_color_hex(0x00cc00), 0);
  lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_20, 0);
  lv_obj_align(mode_label, LV_ALIGN_CENTER, 0, 5);

  /* Left side info */
  range_label = lv_label_create(scr);
  lv_label_set_text(range_label, "Range 130 km");
  lv_obj_set_style_text_color(range_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(range_label, &lv_font_montserrat_16, 0);
  lv_obj_align(range_label, LV_ALIGN_LEFT_MID, 10, -60);

  avg_wh_label = lv_label_create(scr);
  lv_label_set_text(avg_wh_label, "Avg. 40 W/km");
  lv_obj_set_style_text_color(avg_wh_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(avg_wh_label, &lv_font_montserrat_16, 0);
  lv_obj_align(avg_wh_label, LV_ALIGN_LEFT_MID, 10, -20);

  /* Right side info */
  motor_temp_label = lv_label_create(scr);
  lv_label_set_text(motor_temp_label, "Motor 30°C");
  lv_obj_set_style_text_color(motor_temp_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(motor_temp_label, &lv_font_montserrat_16, 0);
  lv_obj_align(motor_temp_label, LV_ALIGN_RIGHT_MID, -10, -60);

  battery_temp_label = lv_label_create(scr);
  lv_label_set_text(battery_temp_label, "Battery 30°C");
  lv_obj_set_style_text_color(battery_temp_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(battery_temp_label, &lv_font_montserrat_16, 0);
  lv_obj_align(battery_temp_label, LV_ALIGN_RIGHT_MID, -10, -20);

  /* Bottom bar (trip/odo/avg) */
  lv_obj_t *bottom_bar = lv_obj_create(scr);
  lv_obj_set_size(bottom_bar, TFT_HOR_RES, 50);
  lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(bottom_bar, lv_color_white(), 0);
  lv_obj_set_style_border_width(bottom_bar, 0, 0);
  lv_obj_set_style_radius(bottom_bar, 0, 0);

  trip_label = lv_label_create(bottom_bar);
  lv_label_set_text(trip_label, "TRIP 130 km");
  lv_obj_set_style_text_color(trip_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(trip_label, &lv_font_montserrat_14, 0);
  lv_obj_align(trip_label, LV_ALIGN_LEFT_MID, 20, 0);

  odo_label = lv_label_create(bottom_bar);
  lv_label_set_text(odo_label, "ODO 1300 km");
  lv_obj_set_style_text_color(odo_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(odo_label, &lv_font_montserrat_14, 0);
  lv_obj_align(odo_label, LV_ALIGN_CENTER, 0, 0);

  avg_kmh_label = lv_label_create(bottom_bar);
  lv_label_set_text(avg_kmh_label, "AVG. 40 km/h");
  lv_obj_set_style_text_color(avg_kmh_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(avg_kmh_label, &lv_font_montserrat_14, 0);
  lv_obj_align(avg_kmh_label, LV_ALIGN_RIGHT_MID, -20, 0);

  Serial.println("EV dashboard UI created!");
}

/* Parse serial input and update values */
void parse_serial_input(String input) {
  // Format: "speed:99,range:130,avg_wh:40,trip:130,odo:1300,avg_kmh:40,motor:30,battery:30,mode:Eco,status:ARMED"
  
  int idx;
  
  if ((idx = input.indexOf("speed:")) >= 0) {
    speed = input.substring(idx + 6, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("range:")) >= 0) {
    range = input.substring(idx + 6, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("avg_wh:")) >= 0) {
    avg_wh = input.substring(idx + 7, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("trip:")) >= 0) {
    trip = input.substring(idx + 5, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("odo:")) >= 0) {
    odo = input.substring(idx + 4, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("avg_kmh:")) >= 0) {
    avg_kmh = input.substring(idx + 8, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("motor:")) >= 0) {
    motor_temp = input.substring(idx + 6, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("battery:")) >= 0) {
    battery_temp = input.substring(idx + 8, input.indexOf(',', idx)).toInt();
  }
  if ((idx = input.indexOf("mode:")) >= 0) {
    int end = input.indexOf(',', idx);
    if (end < 0) end = input.length();
    mode = input.substring(idx + 5, end);
    mode.trim();
  }
  if ((idx = input.indexOf("status:")) >= 0) {
    int end = input.indexOf(',', idx);
    if (end < 0) end = input.length();
    status = input.substring(idx + 7, end);
    status.trim();
  }
  
  update_dashboard();
  Serial.println("Dashboard updated");
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== EV Dashboard ===");

  /* Initialize SD Card */
  Serial.println("Initializing SD Card...");
  SPIClass spi = SPIClass(VSPI);
  spi.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS, spi)) {
    Serial.println("ERROR: SD Card mount failed!");
    while (1) delay(1000);
  }
  Serial.println("SD Card mounted!");

  /* Load splash image */
  if (!load_image_to_ram("/lvgl/logo1.bin")) {
    Serial.println("ERROR: Failed to load image!");
    while (1) delay(1000);
  }

  SD.end();
  Serial.println("SD card closed");

  /* Initialize LVGL */
  lv_init();
  Serial.println("LVGL initialized");

  /* Initialize touch */
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  ts.begin(TOUCH_INT, TOUCH_RST);
  Serial.println("Touch initialized");

  /* Allocate draw buffer */
  draw_buf = heap_caps_malloc(
      TFT_HOR_RES * 40 * (LV_COLOR_DEPTH / 8),
      MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);

  if (!draw_buf) {
    Serial.println("ERROR: Draw buffer allocation failed!");
    while (1) delay(1000);
  }

  /* Create display */
  lv_display_t *disp = lv_tft_espi_create(
      TFT_HOR_RES, TFT_VER_RES, draw_buf,
      TFT_HOR_RES * 40 * (LV_COLOR_DEPTH / 8));

  /* Setup touch input */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);

  /* Show splash screen */
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Charge Into The Future");
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -64);

  static lv_image_dsc_t img_dsc;
  img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  img_dsc.header.w = 148;
  img_dsc.header.h = 148;
  img_dsc.data_size = image_size;
  img_dsc.data = image_data;

  lv_obj_t *img = lv_image_create(scr);
  lv_image_set_src(img, &img_dsc);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 4);

  lv_refr_now(disp);
  delay(3000);

  /* Cleanup splash */
  lv_obj_delete(img);
  lv_obj_delete(label);
  if (image_data) {
    free(image_data);
    image_data = NULL;
  }

  /* Create dashboard */
  create_ev_dashboard_ui();
  lv_refr_now(disp);

  Serial.println("\n=== Setup Complete ===");
  Serial.println("Send data in format:");
  Serial.println("speed:99,range:130,avg_wh:40,trip:130,odo:1300,avg_kmh:40,motor:30,battery:30,mode:Sport,status:ARMED");
}

unsigned long last_time_update = 0;

void loop() {
  lv_timer_handler();
  
  // Update time every second
  if (millis() - last_time_update > 1000) {
    update_time_display();
    last_time_update = millis();
  }
  
  // Read serial input
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    if (input.length() > 0) {
      Serial.printf("Received: %s\n", input.c_str());
      parse_serial_input(input);
    }
  }
  
  delay(5);
}