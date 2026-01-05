# Astral Renderer - Geliştirme Yol Haritası (PLAN.md)

Bu döküman, Astral Renderer projesinin mevcut durumunu analiz eder ve gelecekteki geliştirme adımlarını öncelik sırasına göre listeler.

## 1. Mevcut Durum Analizi (V0.7 - Lighting & Post-Processing)
*   **Tamamlananlar:**
    *   Vulkan 1.3 tabanlı dinamik rendering altyapısı.
    *   glTF 2.0 Model yükleyici (fastgltf).
    *   Bindless Descriptor Set mimarisi (Image, Buffer, Storage Image).
    *   PBR (Cook-Torrance BRDF) Shader entegrasyonu.
    *   IBL (Image Based Lighting) - Irradiance, Prefiltered Map ve BRDF LUT üretimi.
    *   Skybox rendering ve HDR texture desteği.
    *   Fly Camera sistemi (WASD + Mouse).
    *   **Yeni:** Dinamik Işık Yönetimi (Point & Directional Lights).
    *   **Yeni:** Shadow Mapping & PCF Filtering.
    *   **Yeni:** Post-Processing Pipeline (Bloom, ACES Tonemapping, FXAA).
    *   **Yeni:** SSAO (Screen Space Ambient Occlusion) entegrasyonu.
    *   **Yeni:** Cascaded Shadow Maps (CSM) - 4 Cascade desteği.
    *   **Yeni:** ImGui Material & Light Inspector UI.

## 2. Kalite Değerlendirmesi
*   **Güçlü Yönler:** Modern Vulkan özellikleri, bindless tasarım, dinamik ışıklandırma ve gölge desteği, kullanıcı dostu editör arayüzü.
*   **İyileştirmeler:** Bloom algoritması fiziksel doğruluğa (luminance-based extraction & soft thresholding) güncellendi. SSAO kernel boyutu performans için 32 örneğe optimize edildi. Shadow bias ve PCF filtreleme optimize edildi.

---

## 3. Geliştirme Yol Haritası

### **AŞAMA 1: Sahne Yönetimi ve Optimizasyon (Sıradaki Adım)**
1.  **Frustum Culling (Compute-based):**
    *   Görünmeyen objelerin GPU tarafında elenmesi.
2.  **Indirect Drawing:**
    *   Draw call sayısını minimize etmek için `vkCmdDrawIndexedIndirect` kullanımı.
3.  **Mesh Shaders (Opsiyonel):**
    *   Vulkan mesh shader extension desteği ile geometri işlemede yeni nesil yaklaşım.
