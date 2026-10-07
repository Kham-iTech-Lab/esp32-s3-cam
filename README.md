# ESP32-S3-CAM — กล้อง Wi-Fi รุ่น S3 ภาพชัดกว่า ลื่นกว่า

โค้ดกล้องวงจรปิดสำหรับบอร์ด **ESP32-S3** ที่มีกล้อง
ติดตามได้ที่ TikTok **K iTech Review** · YouTube / Facebook **Kham iTech Lab**

ใช้บอร์ด ESP32-CAM ตัวเดิม (ไม่ใช่ S3)? ดู [esp32-cam-cctv](https://github.com/Kham-iTech-Lab/esp32-cam-cctv)

## S3 ดีกว่า ESP32-CAM ตรงไหน

| | ESP32-CAM | ESP32-S3-CAM |
|---|---|---|
| CPU | 240MHz | 240MHz รุ่นใหม่ มีชุดคำสั่ง AI |
| PSRAM | 4MB (ช้า) | 8MB OPI (เร็วกว่ามาก) |
| อัปโหลดโค้ด | ต้องใช้ USB-TTL + ต่อ IO0 | เสียบ USB-C ได้เลย |
| ภาพสด | VGA ลื่น | SVGA / HD ลื่น |

## บอร์ดที่รองรับ

เลือกบอร์ดที่บรรทัดบนสุดของโค้ด (เปิดบรรทัดเดียว)

| ตั้งค่าในโค้ด | บอร์ด |
|---|---|
| `#define BOARD_S3_EYE` | ESP32-S3-CAM N16R8 ทั่วไป, Freenove ESP32-S3 WROOM CAM, ESP32-S3-EYE |
| `#define BOARD_XIAO_SENSE` | Seeed Studio XIAO ESP32S3 Sense |

กล้องที่ใช้ได้: OV2640, OV3660, OV5640 (หน้าเว็บจะบอกว่าเป็นรุ่นไหน)

> บอร์ด S3 ราคาถูกบางรุ่นใช้ขาไม่เหมือนกัน ถ้าขึ้น `เปิดกล้องไม่สำเร็จ` ให้ดูผังขาจากร้านที่ซื้อ แล้วแก้ตัวเลข `..._GPIO_NUM` ในโค้ด

## ตั้งค่า Arduino IDE (สำคัญ)

1. ติดตั้ง ESP32 board (Boards Manager → esp32 by Espressif)
2. เลือกบอร์ดและตั้งค่าตามนี้

| | ESP32-S3-CAM / Freenove | XIAO ESP32S3 Sense |
|---|---|---|
| Board | ESP32S3 Dev Module | XIAO_ESP32S3 |
| PSRAM | **OPI PSRAM** | **OPI PSRAM** |
| Flash Size | 16MB (หรือตามบอร์ด) | ค่าเดิม |
| USB CDC On Boot | Enabled | ค่าเดิม |

ถ้าลืมเปิด PSRAM กล้องจะได้ภาพเล็กมาก และ Serial Monitor จะเตือน

## วิธีใช้

1. เปิด `esp32_s3_cam/esp32_s3_cam.ino` เลือกบอร์ด แก้ `WIFI_SSID` กับ `WIFI_PASS`
2. เสียบ USB-C แล้วกดอัปโหลด (ถ้าไม่เข้า กดปุ่ม BOOT ค้าง แล้วกด RESET หนึ่งครั้ง)
3. เปิด Serial Monitor (115200) จะเห็นเลข IP เช่น `http://192.168.1.70`
4. เปิด IP นั้นในมือถือ (Wi-Fi เดียวกัน)

หน้าเว็บมีปุ่ม ถ่ายภาพ · ไฟ LED · กลับหัวภาพ · เลือกความละเอียด 320x240 ถึง 1600x1200

## ปัญหาที่เจอบ่อย

| อาการ | วิธีแก้ |
|---|---|
| `เปิดกล้องไม่สำเร็จ` | เลือกบอร์ดผิด หรือสายแพกล้องหลวม |
| `ไม่เจอ PSRAM` | Tools → PSRAM → OPI PSRAM แล้วอัปโหลดใหม่ |
| Serial Monitor ว่าง | เปิด USB CDC On Boot = Enabled |
| บอร์ดร้อน | ปกติเมื่อเปิดภาพสดนาน ๆ ควรมีช่องระบายอากาศ |

## หมายเหตุ

- ดูได้เฉพาะใน Wi-Fi บ้าน ไม่ได้เปิดออกอินเทอร์เน็ต
- ภาพสดเปิดดูได้ทีละ 1 เครื่อง
- ติดกล้องในบ้านหรือพื้นที่ของตัวเองเท่านั้น และไม่ถ่ายพื้นที่ส่วนตัวของคนอื่น

---
ทดสอบคอมไพล์กับ ESP32 Arduino core 2.0.x ทั้ง ESP32S3 Dev Module และ XIAO_ESP32S3
