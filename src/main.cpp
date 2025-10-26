#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>

#define TFT_HOR_RES 480
#define TFT_VER_RES 320
#define DRAW_BUF_LINES 10

TFT_eSPI tft = TFT_eSPI();
lv_display_t *display;

// UI Elements
lv_obj_t *textarea;
lv_obj_t *status_label;
lv_obj_t *counter_label;

int message_count = 0;

// Flush callback for LVGL v9
void my_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)px_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void create_ui() {
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x1a1a1a), 0);

    // Top Status Bar
    lv_obj_t *top_panel = lv_obj_create(scr);
    lv_obj_set_size(top_panel, TFT_HOR_RES, 50);
    lv_obj_align(top_panel, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(top_panel, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(top_panel, 0, 0);
    lv_obj_set_style_radius(top_panel, 0, 0);

    status_label = lv_label_create(top_panel);
    lv_label_set_text(status_label, "Serial Monitor Ready");
    lv_obj_set_style_text_color(status_label, lv_color_white(), 0);
    lv_obj_align(status_label, LV_ALIGN_LEFT_MID, 10, 0);

    counter_label = lv_label_create(top_panel);
    lv_label_set_text(counter_label, "0 msgs");
    lv_obj_set_style_text_color(counter_label, lv_color_white(), 0);
    lv_obj_align(counter_label, LV_ALIGN_RIGHT_MID, -10, 0);

    // Main Text Area
    textarea = lv_textarea_create(scr);
    lv_obj_set_size(textarea, TFT_HOR_RES - 20, TFT_VER_RES - 70);
    lv_obj_align(textarea, LV_ALIGN_TOP_MID, 0, 60);
    lv_textarea_set_text(textarea, "Waiting for data...\n");
    
    lv_obj_set_style_bg_color(textarea, lv_color_hex(0x2d2d2d), 0);
    lv_obj_set_style_text_color(textarea, lv_color_hex(0x00ff00), 0);
    lv_obj_set_style_border_color(textarea, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_border_width(textarea, 2, 0);
    lv_obj_set_style_radius(textarea, 8, 0);
    lv_obj_set_style_pad_all(textarea, 10, 0);
    
    lv_textarea_set_one_line(textarea, false);
    lv_obj_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_AUTO);
    
    Serial.println("UI created successfully");
}

void add_message_to_display(String msg) {
    Serial.println("=== add_message_to_display called ===");
    Serial.printf("Message: %s\n", msg.c_str());
    
    message_count++;
    
    // Update counter
    char counter_text[32];
    snprintf(counter_text, sizeof(counter_text), "%d msgs", message_count);
    lv_label_set_text(counter_label, counter_text);
    Serial.printf("Counter updated: %s\n", counter_text);
    
    // Get current text
    const char* current_text = lv_textarea_get_text(textarea);
    Serial.printf("Current textarea text length: %d\n", strlen(current_text));
    
    // Build new message with timestamp
    unsigned long time_sec = millis() / 1000;
    char full_message[256];
    snprintf(full_message, sizeof(full_message), "[%02lu:%02lu] %s\n", 
             time_sec / 60, time_sec % 60, msg.c_str());
    
    Serial.printf("Adding message: %s", full_message);
    
    // Add to textarea
    lv_textarea_add_text(textarea, full_message);
    
    // Scroll to bottom
    lv_obj_scroll_to_y(textarea, LV_COORD_MAX, LV_ANIM_OFF);
    
    // Update status
    lv_label_set_text(status_label, "New Message!");
    
    Serial.println("Message added to display");
    Serial.println("=================================");
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== LVGL v9 Serial Display ===");
    
    // Initialize TFT
    Serial.println("Initializing TFT...");
    tft.begin();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    Serial.println("TFT initialized");

    // Initialize LVGL
    Serial.println("Initializing LVGL...");
    lv_init();
    Serial.println("LVGL initialized");

    // Allocate buffer
    size_t buffer_size = TFT_HOR_RES * DRAW_BUF_LINES * sizeof(lv_color_t);
    lv_color_t *buf = (lv_color_t *)malloc(buffer_size);
    
    if (buf == NULL) {
        Serial.println("ERROR: Buffer allocation failed!");
        while(1) delay(1000);
    }
    
    Serial.printf("Buffer: %u bytes allocated\n", buffer_size);
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());

    // Create display
    Serial.println("Creating LVGL display...");
    display = lv_display_create(TFT_HOR_RES, TFT_VER_RES);
    lv_display_set_flush_cb(display, my_flush_cb);
    lv_display_set_buffers(display, buf, NULL, buffer_size, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("Display created");

    // Create UI
    Serial.println("Creating UI...");
    create_ui();
    
    // Force initial render
    Serial.println("Forcing initial render...");
    lv_refr_now(display);
    
    Serial.println("\n=== Setup Complete ===");
    Serial.println("Send messages via Serial Monitor\n");
}

unsigned long last_status_reset = 0;
unsigned long last_handler_time = 0;

void loop() {
    // Handle LVGL with timing debug
    unsigned long now = millis();
    if (now - last_handler_time >= 5) {
        lv_timer_handler();
        last_handler_time = now;
    }

    // Reset status label
    if (now - last_status_reset > 2000 && last_status_reset > 0) {
        lv_label_set_text(status_label, "Serial Monitor Ready");
        last_status_reset = 0;
    }

    // Handle serial input
    if (Serial.available()) {
        String serialInput = Serial.readStringUntil('\n');
        serialInput.trim();

        if (serialInput.length() > 0) {
            Serial.printf("Received: %s\n", serialInput.c_str());
            
            // Add to display
            add_message_to_display(serialInput);
            
            last_status_reset = millis();
            
            // Force immediate update
            lv_refr_now(display);
        }
    }
}
