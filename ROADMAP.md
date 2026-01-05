# Astral Renderer Project Roadmap

Bu döküman, Astral Renderer projesinin gelişim sürecini, tamamlanan ve planlanan görevleri profesyonel bir şekilde takip etmek için oluşturulmuştur.

## Durum Göstergeleri
- [ ] Beklemede (Pending)
- [/] Devam Ediyor (In Progress)
- [x] Tamamlandı (Completed)

---

## 2026 Q1: Modern Rendering Core (Mevcut Durum: %100 Tamamlandı)
- [x] **Vulkan 1.3 Core:** Dynamic Rendering, Sync2, Bindless architecture.
- [x] **PBR Pipeline:** Cook-Torrance BRDF, glTF 2.0 support.
- [x] **IBL:** HDR skybox, Irradiance & Prefiltered importance sampling.
- [x] **Shadow Mapping:** Directional shadows, PCF filtering.
- [x] **Post-Processing V1:** HDR, Bloom, ACES Tonemapping, FXAA.
- [x] **UI Entegrasyonu:** ImGui ile Material ve Light Inspector.
- [x] **SSAO:** Screen Space Ambient Occlusion ile mikro detaylar.
- [x] **Cascaded Shadow Maps (CSM):** Geniş sahneler için gelişmiş gölge yönetimi.

## 2026 Q2: Advanced Graphics & Performance
- [/] **GPU-Driven Rendering:** Indirect draws, GPU frustum culling. (Altyapı hazırlandı)
- [ ] **TAA (Temporal Anti-Aliasing):** Kenar yumuşatma ve hareket netliği.
- [ ] **Variable Rate Shading (VRS):** Performans için dinamik piksel yoğunluğu.
- [ ] **Mesh Shaders:** Yeni nesil geometri işleme hattı.
- [ ] **Order Independent Transparency (OIT):** Karmaşık cam ve partikül sistemleri.
- [ ] **Clustered Forward Rendering:** Yüzlerce dinamik ışık desteği.

## 2026 Q3: Ray Tracing & Global Illumination
- [ ] **Hybrid Ray Tracing:** RT Shadows ve RT Reflections entegrasyonu.
- [ ] **Probe-Based GI:** Dinamik sahneler için gerçek zamanlı küresel aydınlatma.
- [ ] **Denoiser:** RT gürültüsünü temizlemek için Spatio-temporal filtreler.

## 2026 Q4: Platform & VR
- [ ] **OpenXR Integration:** VR desteği (Quest 3 / PCVR).
- [ ] **Multi-GPU Support:** Cihazlar arası yük dengeleme.
- [ ] **DirectStorage:** Sahne yükleme sürelerini milisaniyelere indirme.

---

**Son Güncelleme:** 2026-01-05
