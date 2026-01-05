# Astral Renderer Architecture Documentation

**Date:** 2026-01-05  
**Project:** Astral Renderer  
**Version:** 1.0.1-Alpha

## 1. Giriş
Astral Renderer, yüksek performanslı, modern ve ölçeklenebilir bir Vulkan tabanlı grafik motorudur. Bu renderer, bağımsız bir kütüphane olarak tasarlanmış olup, ileride bir oyun motoruna entegre edilecek şekilde modüler bir yapıya sahiptir. 2D, 3D ve VR desteği sunmayı hedefler.

## 2. Temel Tasarım Prensipleri
- **Vulkan 1.3+ Standardı:** En güncel Vulkan özelliklerini (Dynamic Rendering, Synchronization 2, Maintenance 4) kullanır.
- **Bindless Rendering:** Descriptor set güncellemelerini minimize etmek için bindless tasarım yaklaşımı benimsenmiştir. Global bir descriptor set (Set #0) üzerinden tüm doku ve buffer'lara indeks ile erişim sağlanır.
- **GPU-Driven Pipeline:** Geometri işleme, culling ve resource metadata yönetimi GPU tarafında yapılarak CPU bottleneck'leri minimize edilir.
- **Multi-Threaded Recording:** Command buffer kayıt işlemleri ikincil (secondary) buffer'lar aracılığıyla paralel thread'lerde yürütülür.
- **Memory Aliasing & Optimization:** RenderGraph üzerinden transient kaynaklar için agresif bellek alias kullanımı ve `VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT` ile VRAM tasarrufu sağlanır.
- **Modülerlik:** Renderer, platform bağımsızdır ve çekirdek (Core), Kaynak Yönetimi (Resource Management) ve Pipeline katmanlarına ayrılmıştır.

## 3. Mimari Katmanlar

### 3.1. Core (Çekirdek) Katmanı
Vulkan API'sinin düşük seviyeli soyutlamalarını içerir.
- **Context:** `VkInstance`, `VkSurfaceKHR`, `VkPhysicalDevice` ve `VkDevice` yönetimini sağlar.
- **Queue Manager:** Graphics, Compute, Transfer ve Present kuyruklarını yönetir.
- **Memory Manager (VMA):** [Vulkan Memory Allocator](https://github.com/GPUOpen-Libraries-And-SDKs/VulkanMemoryAllocator) entegrasyonu ile akıllı bellek yönetimi.
- **Device Abstraction:** Cihaz özelliklerini (Features), limitlerini ve uzantılarını (Extensions) yönetir.

### 3.2. Resource Management (Kaynak Yönetimi)
GPU kaynaklarının yaşam döngüsünü yönetir.
- **Buffer & Image:** Bellek tahsisi, layout geçişleri ve veri transferi.
- **Shader Manager:** HLSL/GLSL shader'ların SPIR-V formatına derlenmesi ve hot-reload desteği.
- **Texture Manager:** Mipmap oluşturma, sıkıştırma ve bindless texture array yönetimi.

### 3.3. Pipeline & Rendering Katmanı
Görüntünün nasıl oluşturulacağını tanımlar.
- **RenderGraph:** Frame bazlı kaynak bağımlılıklarını, senkronizasyonu ve bellek alias yönetimini otomatikleştirir.
- **Dynamic Rendering:** Vulkan 1.3 ile gelen `vkCmdBeginRendering` API'sini kullanarak karmaşık RenderPass/Framebuffer yapısını ortadan kaldırır.
- **Synchronization 2:** Daha okunabilir ve performanslı bariyer (barrier) yönetimi.

### 3.4. Renderer Özellikleri (Renderer Features)
- **2D Renderer:** Sprite batching, font işleme ve 2D efektler.
- **3D Renderer:**
  - **Model Yükleme:** `fastgltf` kütüphanesi ile yüksek performanslı glTF 2.0 (ASCII/Binary) yükleme.
  - **PBR (Physically Based Rendering):** Cook-Torrance BRDF tabanlı, metalik-roughness iş akışını takip eden materyal sistemi.
  - **Bindless Mimari:** Set #0 (Textures), Set #1 (Global Buffers) ve Set #2 (Material Buffers) ayrımı ile sınırsız kaynak erişimi.
  - **Kamera Sistemi:** Quaternion tabanlı fly-camera, yumuşak hareket ve perspektif projeksiyon (Vulkan Y-flip uyumlu).
  - **Forward Rendering:** Dinamik rendering altyapısı ile optimize edilmiş forward pass.
  - **Işıklandırma & IBL:** 
    - Dinamik Point Light (headlamp) desteği.
    - IBL (Image Based Lighting) via EnvironmentManager.
    - Equirectangular to Cubemap compute shader dönüşümü.
    - Skybox rendering pass (RenderGraph entegre).

## 4. Teknoloji Yığını (Tech Stack)
- **Dil:** C++23 (Modern özellikler için).
- **Grafik API:** Vulkan 1.3+.
- **Pencere Yönetimi:** GLFW.
- **Matematik:** GLM.
- **Model Yükleme:** fastgltf.
- **Loglama:** spdlog.

## 5. Klasör Yapısı
```text
Astral_Renderer/
├── assets/             # Shader, model ve texture dosyaları
├── docs/               # Ek dökümantasyon
├── external/           # Üçüncü parti kütüphaneler (submodules)
├── include/            # Public header dosyaları
│   └── astral/
├── src/                # Kaynak kodlar
│   ├── core/           # Vulkan context, device, queue
│   ├── resources/      # Buffer, image, textures
│   ├── renderer/       # RenderGraph, Pipelining
│   └── platform/       # Windowing, input
├── tests/              # Birim ve entegrasyon testleri
├── CMakeLists.txt      # Build konfigürasyonu
└── ARCHITECTURE.md     # Bu dosya
```

## 6. Gelecek Planları
- **Mesh Shaders:** Geometri pipeline optimizasyonu.
- **DirectStorage:** Hızlı varlık yükleme.
- **Global Illumination:** Real-time ray-traced veya probe-based GI.
