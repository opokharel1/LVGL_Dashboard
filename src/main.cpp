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
lv_display_t *disp;

/* Buffer to store image data in RAM */
uint8_t *image_data = NULL;
uint32_t image_size = 0;

/* LVGL UI objects for serial display */
lv_obj_t *textarea;
lv_obj_t *counter_label;
int message_count = 0;

/* Touch callback */
void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
  uint8_t touches = ts.touched(GT911_MODE_POLLING);
  if (touches)
  {
    GTPoint *p = ts.getPoints();
    data->point.x = TFT_HOR_RES - p->y;
    data->point.y = p->x;
    data->state = LV_INDEV_STATE_PRESSED;
  }
  else
  {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/* Load image from SD card into RAM */
bool load_image_to_ram(const char *path)
{
  Serial.printf("Loading image: %s\n", path);

  File file = SD.open(path);
  if (!file)
  {
    Serial.println("ERROR: Failed to open image file!");
    return false;
  }

  image_size = file.size();
  Serial.printf("Image size: %u bytes\n", image_size);

  image_data = (uint8_t *)malloc(image_size);
  if (!image_data)
  {
    Serial.println("ERROR: Failed to allocate memory for image!");
    file.close();
    return false;
  }

  size_t bytes_read = file.read(image_data, image_size);
  file.close();

  if (bytes_read != image_size)
  {
    Serial.printf("ERROR: Read %u bytes, expected %u\n", bytes_read, image_size);
    free(image_data);
    image_data = NULL;
    return false;
  }

  Serial.println("Image loaded into RAM successfully!");
  return true;
}

/* Add message to display - KEEP ONLY THIS ONE */
void add_message_to_display(const String &msg)
{
  Serial.println("=== add_message_to_display called ===");
  Serial.printf("Message: '%s'\n", msg.c_str());
  
  // Check if textarea exists
  if (textarea == NULL) {
    Serial.println("ERROR: textarea is NULL!");
    return;
  }
  
  message_count++;
  Serial.printf("Message count: %d\n", message_count);

  // Update counter
  char counter_text[32];
  snprintf(counter_text, sizeof(counter_text), "%d messages", message_count);
  lv_label_set_text(counter_label, counter_text);
  Serial.printf("Counter updated: %s\n", counter_text);

  // Add timestamp
  unsigned long time_sec = millis() / 1000;
  char full_msg[256];
  snprintf(full_msg, sizeof(full_msg), "[%02lu:%02lu] %s\n", 
           time_sec / 60, time_sec % 60, msg.c_str());
  
  Serial.printf("Full message with timestamp: %s", full_msg);

  // Add message to textarea
  lv_textarea_add_text(textarea, full_msg);
  Serial.println("Message added to textarea");
  
  // Scroll to bottom
  lv_obj_scroll_to_y(textarea, LV_COORD_MAX, LV_ANIM_OFF);
  Serial.println("Scrolled to bottom");
  
  // Force refresh
  lv_refr_now(disp);
  Serial.println("Display refreshed");
  Serial.println("=================================");
}

/* Create UI for Serial Display */
void create_serial_display_ui()
{
  Serial.println("Creating serial display UI...");
  
  lv_obj_t *scr = lv_scr_act();
  
  // Clean ALL objects from screen
  lv_obj_clean(scr);
  Serial.println("Screen cleaned");
  
  // Set background color
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

  /* Title */
  lv_obj_t *title = lv_label_create(scr);
  lv_label_set_text(title, "Serial Input Monitor");
  lv_obj_set_style_text_color(title, lv_color_black(), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  /* Counter */
  counter_label = lv_label_create(scr);
  lv_label_set_text(counter_label, "0 messages");
  lv_obj_set_style_text_color(counter_label, lv_color_hex(0x555555), 0);
  lv_obj_align(counter_label, LV_ALIGN_TOP_RIGHT, -10, 12);

  /* Text area for displaying messages */
  textarea = lv_textarea_create(scr);
  lv_obj_set_size(textarea, TFT_HOR_RES - 20, TFT_VER_RES - 60);
  lv_obj_align(textarea, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_textarea_set_text(textarea, "Waiting for serial input...\n");

  lv_obj_set_style_bg_color(textarea, lv_color_hex(0xeeeeee), 0);
  lv_obj_set_style_text_color(textarea, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_color(textarea, lv_color_hex(0x888888), 0);
  lv_obj_set_style_border_width(textarea, 2, 0);
  lv_obj_set_style_radius(textarea, 6, 0);
  lv_obj_set_style_pad_all(textarea, 8, 0);

  lv_obj_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_AUTO);

  Serial.println("Serial display UI created");
  
  // CRITICAL: Force immediate screen refresh
  lv_refr_now(disp);
  Serial.println("Screen refreshed");
}

// *** REMOVED DUPLICATE add_message_to_display() HERE ***

void setup()
{
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== LVGL SD Card Image Display ===");

  /* Initialize SD Card */
  Serial.println("Initializing SD Card...");
  SPIClass spi = SPIClass(VSPI);
  spi.begin(18, 19, 23, SD_CS);

  if (!SD.begin(SD_CS, spi))
  {
    Serial.println("ERROR: SD Card mount failed!");
    while (1)
      delay(1000);
  }
  Serial.println("SD Card mounted!");

  /* List files */
  Serial.println("\nFiles in /lvgl:");
  File root = SD.open("/lvgl");
  if (root)
  {
    File file = root.openNextFile();
    while (file)
    {
      Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
      file = root.openNextFile();
    }
    root.close();
  }

  /* Load image into RAM */
  if (!load_image_to_ram("/lvgl/logo1.bin"))
  {
    Serial.println("ERROR: Failed to load image!");
    while (1)
      delay(1000);
  }

  /* Close SD card */
  SD.end();
  Serial.println("SD card closed, image is in RAM");

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

  if (!draw_buf)
  {
    Serial.println("ERROR: Draw buffer allocation failed!");
    while (1)
      delay(1000);
  }
  Serial.println("Draw buffer allocated");

  /* Create display */
  disp = lv_tft_espi_create(
      TFT_HOR_RES,
      TFT_VER_RES,
      draw_buf,
      TFT_HOR_RES * 40 * (LV_COLOR_DEPTH / 8));
  Serial.println("Display created");

  /* Setup touch input */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  Serial.println("Touch input configured");

  /* ===== SPLASH SCREEN WITH LOGO ===== */
  Serial.println("\n=== Creating splash screen ===");
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);

  /* Text */
  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Charge Into The Future");
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -64);

  /* Create image descriptor */
  static lv_image_dsc_t img_dsc;
  img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
  img_dsc.header.w = 148;
  img_dsc.header.h = 148;
  img_dsc.data_size = image_size;
  img_dsc.data = image_data;

  /* Display image */
  Serial.println("Displaying logo...");
  lv_obj_t *img = lv_image_create(scr);
  lv_image_set_src(img, &img_dsc);
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 4);

  // Force render
  lv_refr_now(disp);
  
  Serial.println("Logo displayed!");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  
  // Show for 3 seconds
  Serial.println("Waiting 3 seconds...");
  delay(3000);

  /* ===== CLEANUP AND SWITCH TO SERIAL UI ===== */
  Serial.println("\n=== Switching to serial monitor UI ===");
  
  // Free image memory
  if (image_data)
  {
    free(image_data);
    image_data = NULL;
    Serial.println("Image data freed from RAM");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  }

  // Create serial display UI
  create_serial_display_ui();
  
  Serial.println("\n=== Setup Complete ===");
  Serial.println("Send messages via Serial Monitor!");
}

void loop()
{
  // Handle LVGL tasks
  lv_timer_handler();

  // Read Serial input
  if (Serial.available())
  {
    String msg = Serial.readStringUntil('\n');
    msg.trim();
    if (msg.length() > 0)
    {
      Serial.printf("Received: %s\n", msg.c_str());
      add_message_to_display(msg);
    }
  }
  
  delay(5);
}
