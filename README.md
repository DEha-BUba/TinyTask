# TinyTask (WinAPI) Mini Clone

Bu proje, saf WinAPI ile basit bir TinyTask benzeri kayit/oynat uygulamasidir.

## Ozellikler

- Temel pencere ve Win32 mesaj dongusu
- Kayit Baslat/Durdur dugmesi
- Oynat dugmesi
- Duraklat/Devam Et ve Durdur dugmeleri
- Global klavye ve fare olaylarini `SetWindowsHookEx` ile kaydetme
- `SendInput` ile kaydedilen olaylari oynatma
- Olaylar arasi gecikme kaydi ve playback sirasinda gecikmeye uyma
- Loop secenegi (checkbox)
- Kaydedilen olaylari ListBox'ta gosterme
- Hiz ayari (%25 - %400) icin Trackbar (slider)
- Durum etiketi (Bekliyor/Kayit/Oynatiyor/Duraklatildi)
- GUI donmamasi icin playback thread kullanimi
- Global hotkey'ler:
	- F8: Kayit Baslat/Durdur
	- F9: Oynat
	- F10: Duraklat/Devam Et
	- F11: Oynatma Durdur

## Derleme (MinGW g++)

PowerShell icinde proje klasorunde:

g++ -std=c++17 -Wall -Wextra -municode -mwindows .\main.cpp -o .\TinyTask.exe -lcomctl32

## Calistirma

PowerShell:

.\TinyTask.exe

## Notlar

- Dusuk seviyeli hook kullandigi icin uygulama aktifken global klavye/fare olaylarini kaydeder.
- Kaydi durdurduktan sonra olay listesi ListBox'a doldurulur.
- Playback sirasinda Loop seciliyse olaylar tekrar eder.
- F8/F9/F10/F11 tuslari kayda dahil edilmez, kontrol tusu olarak ayrilir.
