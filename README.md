# TinyTask Pro

Gelişmiş Windows makro kaydedici ve oynatıcı.

TinyTask Pro; klavye ve fare olaylarını kaydedebilen, bunları tekrar oynatabilen, hız kontrolü, loop sistemi, kayıt dosyası desteği ve global hotkey özellikleri bulunan hafif bir otomasyon aracıdır.

---

## Özellikler

- Klavye kayıt sistemi
- Fare hareketi kayıt sistemi
- Fare tıklama kayıt sistemi
- Mouse wheel desteği
- Gerçek zamanlı olay listesi
- Hız ayarı (%25 - %400)
- Loop modu
- Tekrar sayısı belirleme
- Duraklat / devam ettir sistemi
- Olay silme sistemi
- `.ttk` kayıt formatı
- Kaydet / yükle desteği
- Global hotkey sistemi
- Multi-monitor desteği
- DPI aware pencere sistemi
- Thread tabanlı oynatma sistemi

---

## Kısayollar

| Tuş | İşlev |
|---|---|
| F8 | Kaydı başlat / durdur |
| F9 | Oynat |
| F10 | Duraklat / devam et |
| F11 | Durdur |

---

## Derleme

### MinGW g++

```bash
g++ -std=c++17 -mwindows -o tinytask_pro.exe tinytask_pro.cpp -lcomctl32 -lcomdlg32
```

---

## Kullanılan Teknolojiler

- C++17
- WinAPI
- Windows Hooks
- SendInput API
- Common Controls API
- Multi-threading (`std::thread`)
- Atomic operations
- Binary file serialization

---

## Desteklenen Olaylar

### Klavye
- Key down
- Key up
- Extended keys
- Scan code desteği

### Fare
- Mouse move
- Left click
- Right click
- Middle click
- X buttons
- Mouse wheel

---

## Dosya Formatı

TinyTask Pro özel `.ttk` formatı kullanır.

Dosya içinde:
- olay tipi
- gecikme süresi
- sanal tuş kodu
- scan code
- koordinatlar
- mouse flag bilgileri

ikili (binary) biçimde saklanır.

---

## Arayüz

Program aşağıdaki sistemleri içerir:

- Olay listesi
- Hız kontrol kaydırıcısı
- Durum göstergesi
- Tekrar sayısı sistemi
- Loop checkbox
- Kayıt yönetim düğmeleri

---

## Güvenlik Notu

Bu proje:
- düşük seviyeli klavye hookları
- düşük seviyeli fare hookları
- giriş simülasyonu

kullandığı için bazı antivirüs yazılımları tarafından yanlış pozitif olarak algılanabilir.

---

## Geliştirici

GitHub: DEha-BUba
