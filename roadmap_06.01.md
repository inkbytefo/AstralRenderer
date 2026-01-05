# 🌌 Astral Renderer: Advanced Core Roadmap (06.01.2026)

Bu döküman, Ray Tracing öncesi motorun temel performans ve kalite limitlerini zorlayacak 3 ana geliştirme aşamasını kapsar.

## 🚀 Aşama 1: GPU-Driven Rendering & Indirect Draw
**Hedef:** CPU draw call darboğazını ortadan kaldırmak ve milyonlarca poligonu GPU bazlı yönetmek.
### **Teknik Detaylar:**
- **Indirect Buffer:** Tüm çizim komutlarını (`VkDrawIndexedIndirectCommand`) içeren bir GPU buffer'ı oluşturulması.
- **GPU Frustum Culling:** Compute shader ile her mesh'in kamera bakış alanında olup olmadığının kontrol edilmesi.
- **Occlusion Culling:** Önceki kareden gelen derinlik verisi (Hi-Z buffer) ile görünmeyen objelerin elenmesi.
- **Single Dispatch:** Tüm sahnenin tek bir `vkCmdDrawIndexedIndirect` komutu ile çizilmesi.

## 🌀 Aşama 2: Temporal Anti-Aliasing (TAA) & Motion Vectors
**Hedef:** Kenar yumuşatma kalitesini artırmak ve Ray Tracing denoiser'ları için zamansal veri altyapısı kurmak.
### **Teknik Detaylar:**
- **Halton Jittering:** Her karede projeksiyon matrisinin piksel altı seviyede (sub-pixel) kaydırılması.
- **Motion Vector Pass:** Her pikselin bir önceki karedeki konumunu hesaplayan özel bir render pass.
- **History Accumulation:** Geçmiş karelerin ağırlıklı ortalamasını alarak titremeyi (shimmering) önleme.
- **Catmull-Rom Filtering:** Reprojection sırasında keskinliği korumak için ileri düzey filtreleme.

## � Aşama 3: Clustered Forward Rendering
**Hedef:** Performans kaybı yaşamadan sahnede yüzlerce dinamik ışık desteği sağlamak.
### **Teknik Detaylar:**
- **Light Clustering:** Görüş alanının (frustum) 3D gridlere (clusters) bölünmesi.
- **GPU Light Culling:** Her cluster içerisine etki eden ışıkların listesinin GPU'da hesaplanması.
- **Z-Binning:** Derinliğe göre ışık arama işleminin optimize edilmesi.
- **Bitmasking:** Shader içerisinde hızlı ışık erişimi için bitwise operasyonlar.

---
## ✅ Mevcut Durum: Aşama 1 Tamamlandı
- [x] Indirect Draw mimarisi için buffer yapılarının kurulması.
- [x] Compute shader culling mantığının tasarlanması.
- [x] GPU-Driven Rendering entegrasyonu (Frustum Culling dahil).
- [ ] Aşama 2: TAA & Motion Vectors (Başlanıyor...)
