# BluFi 配网固件烧录指南

本 README 说明如何为工程生成并烧录 BluFi 配网子固件（blufi_app.bin）。

---

## 1. 生成 blufi_app.bin

进入子工程目录并执行编译：

```bash
cd third_party/blufi_app
idf.py build
cd ../..
```

编译成功后，固件位于
third_party/blufi_app/build/blufi_app.bin

## 2. 将生成的 blufi_app.bin 烧录至 ota_2 分区（偏移地址 0xE20000）：

```bash
esptool.py --chip esp32s3 \
           --port /dev/ttyUSB0 \
           write_flash 0xE20000 \
           third_party/blufi_app/build/blufi_app.bin
```