# Astral Renderer - Roadmap (06.01.2026)

Bu doküman, Astral Renderer'ın görsel kalite ve performansını artırmak için belirlenen 3 aşamalı geliştirme planını detaylandırır.

---

## 🚀 Aşama 1: Cascaded Shadow Maps (CSM) & PCF
**Hedef:** Geniş sahnelerde yüksek kaliteli ve yumuşak gölgeler elde etmek.

### **Teknik Detaylar:**
- **Frustum Splitting:** Kamera bakış alanını (frustum) 4 farklı derinlik bölgesine (cascade) bölmek.
- **Light Projection Matrix:** Her bir cascade için ışık uzayında (light space) sıkı bir bounding box hesaplamak.
- **Array Textures:** Tüm cascade'leri tek bir `VkImage` (layer count = 4) içinde tutarak shader'da tek bir descriptor ile erişmek.
- **PCF (Percentage Closer Filtering):** 3x3 veya 5x5 kernel kullanarak gölge kenarlarındaki aliasing'i (tırtıklanmayı) gidermek.
- **Depth Bias:** Cascade geçişlerindeki "shadow acne" sorununu önlemek için dinamik bias hesaplaması.

---

## 🚀 Aşama 2: Tam Bindless Texture Sistemi
**Hedef:** CPU yükünü azaltmak ve materyal yönetimini modernize etmek.

### **Teknik Detaylar:**
- **Global Descriptor Set:** Tüm texture'ları içeren devasa bir `VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER` array'i tanımlamak.
- **Dynamic Indexing:** Shader içerisinde materyal verisinden gelen index ile doğrudan ilgili texture'a erişmek.
- **Resource Management:** Texture yükleme sırasında global bir manager üzerinden index tahsis etmek.
- **Performance:** Draw call sayısını azaltmak ve bind-less rendering (SetBindless) mimarisine tam geçiş.

---

## 🚀 Aşama 3: IBL Pre-processing Pipeline
**Hedef:** PBR materyaller için fiziksel tabanlı çevresel aydınlatma.

### **Teknik Detaylar:**
- **Equirectangular to Cubemap:** HDR gökyüzü haritalarını cubemap formatına dönüştüren compute shader.
- **Irradiance Map:** Difüz aydınlatma için düşük çözünürlüklü, konvolüsyon uygulanmış cubemap üretimi.
- **Prefiltered Map:** Speküler yansımalar ve pürüzlülük (roughness) seviyeleri için mip-mapped cubemap üretimi.
- **BRDF LUT:** Fresnel ve geometri terimleri için önceden hesaplanmış Look-Up Table üretimi.

---

## ✅ Mevcut Durum: IBL Pipeline & Tüm Geliştirmeler Tamamlandı
- [x] Frustum split matematiği (C++)
- [x] Shadow map array texture oluşturma
- [x] PCF entegrasyonu
- [x] Bindless Texture Indexing entegrasyonu
- [x] Kamera hareket hızı optimizasyonu (25.0f)
- [x] Equirectangular to Cubemap compute shader
- [x] Irradiance map convolution
- [x] Prefiltered map generation
- [x] BRDF LUT generation
- [x] Compute shader senkronizasyon bariyerleri (Vulkan)

---
## 🌟 Final Durumu
Astral Renderer artık modern grafik tekniklerini (CSM, Bindless, IBL) tam kapasiteyle desteklemektedir. 
1. **Gelişmiş Gölgeler:** CSM ile her mesafede keskin ve stabil gölgeler.
2. **Performans:** Bindless mimari ile düşük CPU overhead.
3. **Görsel Kalite:** HDR tabanlı tam fiziksel tabanlı aydınlatma (PBR + IBL).
