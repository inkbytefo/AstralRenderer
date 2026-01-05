# 🌌 Astral Renderer: Roadmap V2 (Future Frontiers)

Astral Renderer'ın temel mimarisi (CSM, Bindless, IBL) artık tam kapasiteyle çalışıyor. Sıradaki adımlar, motoru endüstri standartlarının zirvesine taşıyacak olan ileri düzey teknolojileri hedeflemektedir.

## 🚀 Aşama 1: Hardware Accelerated Ray Tracing (Vulkan Ray Tracing)
**Hedef:** Screen-space tekniklerin (SSAO, SSR) ötesine geçerek gerçek fiziksel ışık simülasyonu sağlamak.
- **Top-Level & Bottom-Level Acceleration Structures (TLAS/BLAS):** Sahneler için BVH yapılarının kurulması.
- **Ray Tracing Pipeline:** Ray generation, miss, closest hit ve any hit shader'larının implementasyonu.
- **RT Shadows & Reflections:** CSM'in ötesinde, her mesafede mükemmel gölgeler ve aydınlatma bazlı yansımalar.
- **Denoiser Entegrasyonu:** Düşük sample count ile temiz görüntüler için NVIDIA Real-Time Denoisers (NRD) entegrasyonu.

## ⚡ Aşama 2: Next-Gen Upscaling & Frame Generation
**Hedef:** 4K çözünürlükte bile ultra yüksek kare hızları (FPS) elde etmek.
- **NVIDIA DLSS 3.5 (Ray Reconstruction):** Işın izleme kalitesini yapay zeka ile artırma.
- **AMD FSR 3.1:** Cross-platform upscaling ve frame generation desteği.
- **Intel XeSS:** Intel GPU'lar için optimize edilmiş AI-driven upscaling.

## 🎭 Aşama 3: Mesh Shaders & Nanite-like Geometry
**Hedef:** Milyarlarca poligonu geleneksel vertex shader limitlerine takılmadan işlemek.
- **Task & Mesh Shaders:** Geleneksel input assembler'ı bypass ederek GPU bazlı geometri culling ve LOD yönetimi.
- **Virtual Geometry:** Sahne karmaşıklığına göre dinamik poligon yoğunluğu.
- **GPU-Driven Culling:** CPU yükünü minimize eden occlusion culling sistemleri.

## 🌈 Aşama 4: Global Illumination (GI) & Advanced VFX
**Hedef:** Tamamen dinamik ve gerçek zamanlı dolaylı aydınlatma.
- **Lumen-like GI:** Işığın yüzeylerden sekerek tüm sahneyi aydınlatması.
- **Volumetric Lighting:** Sis, ışık hüzmeleri (God Rays) ve atmosferik saçılma.
- **Advanced Particle System:** GPU bazlı, fizik etkileşimli parçacık sistemleri.

---
## 🛠️ Mevcut Durum: V1 Mimari Tamamlandı
- [x] Cascaded Shadow Maps (CSM)
- [x] Bindless Texture Indexing
- [x] Full IBL (Image-Based Lighting) Pipeline
- [x] PBR (Physically Based Rendering)
- [x] FXAA & Tonemapping
