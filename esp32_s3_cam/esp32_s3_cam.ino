/*
 * ESP32-S3-CAM — กล้อง Wi-Fi รุ่น S3 ภาพชัดกว่า ลื่นกว่า ESP32-CAM ตัวเดิม
 * K iTech Review (TikTok) • Kham iTech Lab (YouTube / Facebook)
 *
 * ทำอะไรได้
 *   - ดูภาพสด (MJPEG) ผ่านเบราว์เซอร์ในมือถือ/คอม ใน Wi-Fi บ้าน
 *   - ถ่ายภาพนิ่ง ความละเอียดสูงสุด 1600x1200 (UXGA)
 *   - เปิด/ปิดไฟ LED (ถ้าบอร์ดมี)
 *   - ปรับความละเอียด กลับหัวภาพ จากหน้าเว็บ
 *   - บันทึกภาพลง microSD กดเอง หรือถ่ายอัตโนมัติทุก 10 วิ - 30 นาที (timelapse)
 *     ชื่อไฟล์เป็นวันเวลาจริง การ์ดใกล้เต็มจะลบรูปเก่าสุดให้เอง + หน้าดูรูปย้อนหลัง
 *     (การ์ด microSD ไม่เกิน 32GB ฟอร์แมต FAT32 — ไม่ใส่การ์ดก็ใช้งานอื่นได้ปกติ)
 *
 * ทำไม S3 ดีกว่า ESP32-CAM: CPU เร็วกว่า, PSRAM 8MB แบบ OPI (เร็วกว่าเดิมมาก),
 * มี USB ในตัว เสียบสายอัปโหลดได้เลย ไม่ต้องต่อ IO0 / USB-TTL
 *
 * ตั้งค่า Arduino IDE (สำคัญ!)
 *   ESP32-S3-CAM / Freenove : Board "ESP32S3 Dev Module", PSRAM "OPI PSRAM",
 *                             Flash Size "16MB" (หรือตามบอร์ด), USB CDC On Boot "Enabled"
 *   XIAO ESP32S3 Sense      : Board "XIAO_ESP32S3", PSRAM "OPI PSRAM"
 *
 * ไม่ต้องติดตั้งไลบรารีเพิ่ม (esp_camera มากับ ESP32 board package แล้ว)
 */

#include <Arduino.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "esp_http_server.h"

// ------------------------------------------------------------------ เลือกบอร์ด (เปิดบรรทัดเดียว)
#define BOARD_S3_EYE        // ESP32-S3-CAM N16R8 ทั่วไป, Freenove ESP32-S3 WROOM CAM, ESP32-S3-EYE
// #define BOARD_XIAO_SENSE // Seeed Studio XIAO ESP32S3 Sense

// ------------------------------------------------------------------ ตั้งค่า Wi-Fi
const char* WIFI_SSID = "ชื่อ Wi-Fi บ้าน";
const char* WIFI_PASS = "รหัส Wi-Fi";
const char* TIME_ZONE = "ICT-7";       // เวลาประเทศไทย ใช้ตั้งชื่อไฟล์รูป

// ------------------------------------------------------------------ ขากล้องตามบอร์ด
#if defined(BOARD_S3_EYE)
  #define PWDN_GPIO_NUM   -1
  #define RESET_GPIO_NUM  -1
  #define XCLK_GPIO_NUM   15
  #define SIOD_GPIO_NUM    4
  #define SIOC_GPIO_NUM    5
  #define Y9_GPIO_NUM     16
  #define Y8_GPIO_NUM     17
  #define Y7_GPIO_NUM     18
  #define Y6_GPIO_NUM     12
  #define Y5_GPIO_NUM     10
  #define Y4_GPIO_NUM      8
  #define Y3_GPIO_NUM      9
  #define Y2_GPIO_NUM     11
  #define VSYNC_GPIO_NUM   6
  #define HREF_GPIO_NUM    7
  #define PCLK_GPIO_NUM   13
  #define LED_PIN          2    // ไฟ LED บนบอร์ด (ถ้าบอร์ดไม่มี ตั้งเป็น -1)
  // ช่อง microSD แบบ SD_MMC 1-bit
  #define SD_MMC_CMD      38
  #define SD_MMC_CLK      39
  #define SD_MMC_D0       40
#elif defined(BOARD_XIAO_SENSE)
  #define PWDN_GPIO_NUM   -1
  #define RESET_GPIO_NUM  -1
  #define XCLK_GPIO_NUM   10
  #define SIOD_GPIO_NUM   40
  #define SIOC_GPIO_NUM   39
  #define Y9_GPIO_NUM     48
  #define Y8_GPIO_NUM     11
  #define Y7_GPIO_NUM     12
  #define Y6_GPIO_NUM     14
  #define Y5_GPIO_NUM     16
  #define Y4_GPIO_NUM     18
  #define Y3_GPIO_NUM     17
  #define Y2_GPIO_NUM     15
  #define VSYNC_GPIO_NUM  38
  #define HREF_GPIO_NUM   47
  #define PCLK_GPIO_NUM   13
  #define LED_PIN         -1    // LED บน XIAO ใช้ขา 21 ร่วมกับ SD จึงปิดไว้
  // ช่อง microSD บนบอร์ดขยาย Sense ต่อแบบ SPI
  #define SD_USE_SPI
  #define SD_CS_PIN       21
#else
  #error "เลือกบอร์ดก่อน: เปิด #define BOARD_S3_EYE หรือ BOARD_XIAO_SENSE"
#endif

#include "sd_storage.h"   // บันทึกลง microSD (ไฟล์อยู่ในโฟลเดอร์เดียวกัน)

httpd_handle_t webServer = NULL;     // หน้าเว็บ + คำสั่ง (พอร์ต 80)
httpd_handle_t streamServer = NULL;  // ภาพสด (พอร์ต 81)
bool ledOn = false;

void setLed(bool on) {
  if (LED_PIN < 0) return;
#if defined(BOARD_XIAO_SENSE)
  digitalWrite(LED_PIN, on ? LOW : HIGH);
#else
  digitalWrite(LED_PIN, on ? HIGH : LOW);
#endif
}

// ------------------------------------------------------------------ หน้าเว็บ
static const char INDEX_TOP[] PROGMEM = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP32-S3-CAM</title><style>
body{font-family:sans-serif;background:#0b1530;color:#fff;margin:0;padding:12px;text-align:center}
h1{font-size:20px;margin:6px 0 10px}
img{width:100%;max-width:900px;border-radius:12px;background:#000}
.r{display:flex;gap:8px;justify-content:center;flex-wrap:wrap;margin:12px 0}
button,select{background:#1c2b55;border:2px solid #3cc8ff;color:#fff;border-radius:12px;padding:12px 16px;font-size:16px}
.s{color:#9ab;font-size:13px}
</style></head><body>
<h1>ESP32-S3-CAM</h1>
<img id="v" alt="กำลังโหลดภาพ...">
<div class="r">
  <button onclick="location.href='/capture'">ถ่ายภาพ</button>
  <button onclick="fetch('/led').then(r=>r.text()).then(t=>this.innerText='ไฟ: '+t)">ไฟ: ปิด</button>
  <button onclick="fetch('/flip')">กลับหัวภาพ</button>
  <select onchange="fetch('/res?v='+this.value)">
    <option value="5">QVGA 320x240</option>
    <option value="8">VGA 640x480</option>
    <option value="9" selected>SVGA 800x600</option>
    <option value="11">HD 1280x720</option>
    <option value="13">UXGA 1600x1200</option>
  </select>
</div>)HTML";
// (ปุ่ม SD จาก sd_storage.h แทรกตรงนี้)
static const char INDEX_BOTTOM[] PROGMEM = R"HTML(
<div class="s" id="i"></div>
<script>
document.getElementById('v').src=location.protocol+'//'+location.hostname+':81/stream';
fetch('/info').then(r=>r.text()).then(t=>document.getElementById('i').innerText=t);
</script>
</body></html>)HTML";

static esp_err_t indexHandler(httpd_req_t* req) {
  httpd_resp_set_type(req, "text/html; charset=utf-8");
  httpd_resp_send_chunk(req, INDEX_TOP, strlen(INDEX_TOP));
  httpd_resp_send_chunk(req, sdcam::HTML_CONTROLS, strlen(sdcam::HTML_CONTROLS));
  httpd_resp_send_chunk(req, INDEX_BOTTOM, strlen(INDEX_BOTTOM));
  return httpd_resp_send_chunk(req, NULL, 0);
}

static esp_err_t infoHandler(httpd_req_t* req) {
  sensor_t* s = esp_camera_sensor_get();
  const char* name = "กล้องไม่รู้จัก";
  switch (s->id.PID) {
    case OV2640_PID: name = "OV2640 (2MP)"; break;
    case OV3660_PID: name = "OV3660 (3MP)"; break;
    case OV5640_PID: name = "OV5640 (5MP)"; break;
  }
  char buf[128];
  snprintf(buf, sizeof(buf), "กล้อง %s • PSRAM %u MB • ภาพสดพอร์ต 81",
           name, (unsigned)(ESP.getPsramSize() / (1024 * 1024)));
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_send(req, buf, strlen(buf));
}

// ------------------------------------------------------------------ ถ่ายภาพนิ่ง
static esp_err_t captureHandler(httpd_req_t* req) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) { httpd_resp_send_500(req); return ESP_FAIL; }
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return res;
}

// ------------------------------------------------------------------ คำสั่งจากหน้าเว็บ
static esp_err_t ledHandler(httpd_req_t* req) {
  const char* t;
  if (LED_PIN < 0) {
    t = "บอร์ดนี้ไม่มีไฟ";
  } else {
    ledOn = !ledOn;
    setLed(ledOn);
    t = ledOn ? "เปิด" : "ปิด";
  }
  httpd_resp_set_type(req, "text/plain; charset=utf-8");
  return httpd_resp_send(req, t, strlen(t));
}

static esp_err_t flipHandler(httpd_req_t* req) {
  sensor_t* s = esp_camera_sensor_get();
  static bool flipped = false;
  flipped = !flipped;
  s->set_vflip(s, flipped);
  s->set_hmirror(s, flipped);
  return httpd_resp_send(req, "OK", 2);
}

static esp_err_t resHandler(httpd_req_t* req) {
  char q[32], v[8];
  if (httpd_req_get_url_query_str(req, q, sizeof(q)) == ESP_OK &&
      httpd_query_key_value(q, "v", v, sizeof(v)) == ESP_OK) {
    int f = atoi(v);
    if (f >= FRAMESIZE_QVGA && f <= FRAMESIZE_UXGA) {
      sensor_t* s = esp_camera_sensor_get();
      s->set_framesize(s, (framesize_t)f);
    }
  }
  return httpd_resp_send(req, "OK", 2);
}

// ------------------------------------------------------------------ ภาพสด MJPEG
#define BOUNDARY "frame"
static const char* STREAM_TYPE = "multipart/x-mixed-replace;boundary=" BOUNDARY;
static const char* STREAM_SEP  = "\r\n--" BOUNDARY "\r\n";
static const char* STREAM_HDR  = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static esp_err_t streamHandler(httpd_req_t* req) {
  esp_err_t res = httpd_resp_set_type(req, STREAM_TYPE);
  if (res != ESP_OK) return res;
  char hdr[64];
  while (true) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { res = ESP_FAIL; break; }
    size_t n = snprintf(hdr, sizeof(hdr), STREAM_HDR, fb->len);
    res = httpd_resp_send_chunk(req, STREAM_SEP, strlen(STREAM_SEP));
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, hdr, n);
    if (res == ESP_OK) res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
    esp_camera_fb_return(fb);
    if (res != ESP_OK) break;   // ผู้ชมปิดหน้าเว็บแล้ว
  }
  return res;
}

void startServers() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 80;
  cfg.max_uri_handlers = 16;     // ค่าเริ่มต้นได้แค่ 8 ไม่พอสำหรับเมนู SD
  httpd_uri_t uris[] = {
    {"/",        HTTP_GET, indexHandler,   NULL},
    {"/info",    HTTP_GET, infoHandler,    NULL},
    {"/capture", HTTP_GET, captureHandler, NULL},
    {"/led",     HTTP_GET, ledHandler,     NULL},
    {"/flip",    HTTP_GET, flipHandler,    NULL},
    {"/res",     HTTP_GET, resHandler,     NULL},
  };
  if (httpd_start(&webServer, &cfg) == ESP_OK) {
    for (auto& u : uris) httpd_register_uri_handler(webServer, &u);
    sdcam::registerHandlers(webServer);
  }

  cfg.server_port = 81;
  cfg.ctrl_port += 1;            // แต่ละเซิร์ฟเวอร์ต้องใช้ ctrl_port ไม่ซ้ำกัน
  httpd_uri_t stream = {"/stream", HTTP_GET, streamHandler, NULL};
  if (httpd_start(&streamServer, &cfg) == ESP_OK) {
    httpd_register_uri_handler(streamServer, &stream);
  }
}

// ------------------------------------------------------------------ setup / loop
void setup() {
  Serial.begin(115200);
  delay(1500);   // รอ USB Serial พร้อม
  if (LED_PIN >= 0) { pinMode(LED_PIN, OUTPUT); setLed(false); }
  Serial.println(sdcam::begin() ? "พบการ์ด SD" : "ไม่พบการ์ด SD — ใช้งานต่อได้ แต่บันทึกภาพไม่ได้");

  if (!psramFound()) {
    Serial.println("ไม่เจอ PSRAM — ไปที่ Tools > PSRAM แล้วเลือก \"OPI PSRAM\" แล้วอัปโหลดใหม่");
  }

  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer   = LEDC_TIMER_0;
  c.pin_d0 = Y2_GPIO_NUM;  c.pin_d1 = Y3_GPIO_NUM;  c.pin_d2 = Y4_GPIO_NUM;  c.pin_d3 = Y5_GPIO_NUM;
  c.pin_d4 = Y6_GPIO_NUM;  c.pin_d5 = Y7_GPIO_NUM;  c.pin_d6 = Y8_GPIO_NUM;  c.pin_d7 = Y9_GPIO_NUM;
  c.pin_xclk = XCLK_GPIO_NUM;   c.pin_pclk = PCLK_GPIO_NUM;
  c.pin_vsync = VSYNC_GPIO_NUM; c.pin_href = HREF_GPIO_NUM;
  c.pin_sccb_sda = SIOD_GPIO_NUM; c.pin_sccb_scl = SIOC_GPIO_NUM;
  c.pin_pwdn = PWDN_GPIO_NUM;   c.pin_reset = RESET_GPIO_NUM;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.grab_mode = CAMERA_GRAB_LATEST;   // เอาภาพล่าสุดเสมอ ภาพสดไม่ดีเลย์
  if (psramFound()) {
    // จองบัฟเฟอร์ไว้ที่ UXGA เพื่อให้สลับความละเอียดสูงสุดได้ แล้วค่อยเริ่มที่ SVGA
    c.frame_size = FRAMESIZE_UXGA; c.jpeg_quality = 10; c.fb_count = 2; c.fb_location = CAMERA_FB_IN_PSRAM;
  } else {
    c.frame_size = FRAMESIZE_QVGA; c.jpeg_quality = 15; c.fb_count = 1; c.fb_location = CAMERA_FB_IN_DRAM;
  }

  if (esp_camera_init(&c) != ESP_OK) {
    Serial.println("เปิดกล้องไม่สำเร็จ — เช็กว่าเลือกบอร์ดถูก (BOARD_...) และสายแพกล้องเสียบแน่น");
    while (true) delay(1000);
  }

  sensor_t* s = esp_camera_sensor_get();
  if (psramFound()) s->set_framesize(s, FRAMESIZE_SVGA);
  if (s->id.PID == OV3660_PID) { s->set_vflip(s, 1); s->set_brightness(s, 1); }  // OV3660 ภาพกลับหัวมาจากโรงงาน

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);              // ปิดโหมดประหยัดไฟ Wi-Fi ภาพสดลื่นขึ้น
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("กำลังต่อ Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) { delay(400); Serial.print("."); }

  configTzTime(TIME_ZONE, "pool.ntp.org", "time.google.com");   // ดึงเวลาจริงจากอินเทอร์เน็ต
  startServers();
  Serial.printf("\nเปิดในมือถือ: http://%s\n", WiFi.localIP().toString().c_str());
}

void loop() {
  sdcam::loop();   // ถ่ายอัตโนมัติลง SD ตามรอบเวลาที่ตั้งไว้
  delay(50);
}
