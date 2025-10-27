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
#define TOUCH_SDA  33
#define TOUCH_SCL  32
#define TOUCH_INT  21
#define TOUCH_RST  25

GT911 ts = GT911();
void *draw_buf;

/* Buffer to store image data in RAM */
uint8_t *image_data = NULL;
uint32_t image_size = 0;

/* Touch callback */
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
  
  // Allocate memory for image
  image_data = (uint8_t *)malloc(image_size);
  if (!image_data) {
    Serial.println("ERROR: Failed to allocate memory for image!");
    file.close();
    return false;
  }
  
  // Read entire file into RAM
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

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== LVGL SD Card Image Display ===");
  
  /* Initialize SD Card FIRST, before display */
  Serial.println("Initializing SD Card...");
  SPIClass spi = SPIClass(VSPI);
  spi.begin(18, 19, 23, SD_CS);  // SCK, MISO, MOSI, CS
  
  if (!SD.begin(SD_CS, spi)) {
    Serial.println("ERROR: SD Card mount failed!");
    while (1) delay(1000);
  }
  Serial.println("SD Card mounted!");
  
  /* List files */
  Serial.println("\nFiles in /lvgl:");
  File root = SD.open("/lvgl");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      Serial.printf("  - %s (%d bytes)\n", file.name(), file.size());
      file = root.openNextFile();
    }
    root.close();
  }
  
  /* Load image into RAM before initializing display */
  if (!load_image_to_ram("/lvgl/logo1.bin")) {
    Serial.println("ERROR: Failed to load image!");
    while (1) delay(1000);
  }
  
  /* Now we can close SD card - image is in RAM */
  SD.end();
  Serial.println("SD card closed, image is now in RAM");
  
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
    MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL
  );
  
  if (!draw_buf) {
    Serial.println("ERROR: Draw buffer allocation failed!");
    while (1) delay(1000);
  }
  Serial.println("Draw buffer allocated");
  
  /* Create display */
  lv_display_t *disp = lv_tft_espi_create(
    TFT_HOR_RES, 
    TFT_VER_RES, 
    draw_buf, 
    TFT_HOR_RES * 40 * (LV_COLOR_DEPTH / 8)
  );
  Serial.println("Display created");
  
  /* Setup touch input */
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touch_read);
  Serial.println("Touch input configured");
  
  /* Create UI */
  lv_obj_t *scr = lv_scr_act();
  lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
  
  /* Title */
  lv_obj_t *label = lv_label_create(scr);
  lv_label_set_text(label, "Charge Into The Future");
  lv_obj_set_style_text_color(label, lv_color_black(), 0);
  lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, -64);
  
  /* Create image descriptor pointing to RAM data */
  static lv_image_dsc_t img_dsc;
  img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;  // FIXED: Changed for LVGL v9
  img_dsc.header.w = 148;   // Change to your image width  //240
  img_dsc.header.h = 148;   // Change to your image height  //148
  img_dsc.data_size = image_size;
  img_dsc.data = image_data;
  
  /* Display image from RAM */
  Serial.println("Creating image widget...");
  lv_obj_t *img = lv_image_create(scr);  // FIXED: lv_image_create for LVGL v9
  lv_image_set_src(img, &img_dsc);       // FIXED: lv_image_set_src for LVGL v9
  lv_obj_align(img, LV_ALIGN_CENTER, 0, 4);
  
  Serial.println("\n=== Setup Complete ===");
  Serial.println("Image should now be visible!");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

void loop() {
  lv_timer_handler();
  delay(5);
}