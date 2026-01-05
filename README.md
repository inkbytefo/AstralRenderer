# Astral Renderer

**Astral Renderer**, modern Vulkan 1.3 API'si kullanılarak geliştirilen, yüksek performanslı ve fiziksel tabanlı (PBR) bir grafik motorudur. Bu proje, modern GPU özelliklerini (Bindless Descriptors, Dynamic Rendering, Sync2) kullanarak bare-metal performansı hedeflemektedir.

## 🚀 Mevcut Özellikler

- **Vulkan 1.3 Core:** Dynamic Rendering, Timeline Semaphores ve Synchronization 2 desteği.
- **PBR Rendering:** Cook-Torrance BRDF tabanlı fiziksel ışıklandırma.
- **IBL (Image Based Lighting):** HDR gökyüzü haritaları üzerinden Diffuse Irradiance ve Prefiltered Specular yansımalar.
- **Render Graph:** Kaynak yönetimini ve pass bağımlılıklarını optimize eden esnek render graph mimarisi.
- **Bindless Descriptors:** Tek bir global descriptor set üzerinden sınırsız texture ve buffer erişimi.
- **Post-Processing:**
  - **SSAO:** Screen Space Ambient Occlusion.
  - **Bloom:** HDR parlama efekti (Threshold + Dual Blur).
  - **FXAA:** Fast Approximate Anti-Aliasing (Optimize edilmiş ve hizalanmış).
  - **Tonemapping:** ACES Filmic Curve.
- **Shadow Mapping:** 4-Cascade Cascaded Shadow Maps (CSM) ile geniş alan gölgelendirmesi.
- **UI:** ImGui entegrasyonu ile gerçek zamanlı parametre kontrolü.

## 🛠️ Son Yapılan İyileştirmeler (6 Ocak 2026)

- **Shader Senkronizasyonu:** `SceneData` yapısı tüm shader'larda (`pbr`, `skybox`, `ssao`, `fxaa`) standart hale getirildi ve hizalama sorunları giderildi.
- **FXAA Artifact Fix:** FXAA shader'ındaki dikey çizgi ve kayma sorunları, push constant hizalaması (8-byte alignment) ve `textureLod` kullanımı ile çözüldü.
- **Model Görünürlük Düzeltmesi:** glTF model yükleme ve ölçeklendirme mantığı iyileştirildi, sahne başlangıç kamerası optimize edildi.
- **Build Sistemi:** Clean-build süreçleri otomatikleştirildi ve Release modunda stabilite sağlandı.

## 📋 Gelecek Görevler (Roadmap)

1.  **Ray Tracing (DXR/Vulkan RayTracing):** Donanım hızlandırmalı ışın izleme ile gerçekçi yansımalar ve gölgeler.
2.  **Mesh Shaders:** Geometri işleme hattını modernize ederek sahne karmaşıklığını artırma.
3.  **DirectStorage:** Asset yükleme sürelerini GPU üzerinden minimize etme.
4.  **Multi-GPU Desteği:** Birden fazla grafik kartı üzerinden render iş yükünü dağıtma.
5.  **Animation System:** glTF skinning ve morph targets desteği.

## 📦 Kurulum

### Gereksinimler
- Vulkan SDK 1.3+
- C++23 destekli derleyici (MSVC 2022+ önerilir)
- CMake 3.20+

### Derleme
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

## 📜 Lisans
Bu proje MIT lisansı altında korunmaktadır.
