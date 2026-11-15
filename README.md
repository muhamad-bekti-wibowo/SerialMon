# SerialMon

Serial monitor untuk Windows (C++ / Qt 6), pengganti PuTTY / Serial Monitor Arduino.

- Pilih port dari dropdown (nama seperti di Device Manager), baud bebas, data/parity/stop/flow, DTR/RTS
- Auto-reconnect saat board di-reset atau kabel dicabut
- Terminal dengan timestamp, tampilan HEX, warna error/warning, kirim teks/HEX, riwayat
- Filter + highlight (teks atau regex)
- Plotter real-time: angka dari baris yang cocok filter jadi kanal grafik (mis. `adc=123,456`), ekspor CSV
- Tema terang/gelap bergaya mbektiw.my.id

Backend serial memakai Win32 API langsung (tanpa QtSerialPort).

## Build

Butuh Qt 6 (MSVC 2022 64-bit) dan Visual Studio. Sesuaikan path `QT` dan `vcvars64.bat` di `build.bat`, lalu:

```
build.bat
```

Hasil: `build\SerialMonitor.exe`. Coba tanpa hardware: `SerialMonitor.exe --demo`.

## Lisensi font

Plus Jakarta Sans dan JetBrains Mono (di `assets/fonts`) berlisensi SIL Open Font License 1.1.
