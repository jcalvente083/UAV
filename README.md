<div align="center">

# 🚁 ⇄ 🛰️ UAV — Detección en el edge y estación de tierra

**Sistema completo de dron + estación de tierra: el dron detecta objetos con IA a bordo y envía en tiempo real la telemetría, el vídeo y las detecciones a tierra.**

![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt 6](https://img.shields.io/badge/Qt-6.11-41CD52?logo=qt&logoColor=white)
![YOLOv8n](https://img.shields.io/badge/YOLOv8n-ONNX%20Runtime-111F68?logo=onnx&logoColor=white)
![OpenCV](https://img.shields.io/badge/OpenCV-4.x-5C3EE8?logo=opencv&logoColor=white)
![MQTT](https://img.shields.io/badge/UDP%20%C2%B7%20TCP%20%C2%B7%20MQTT-telemetr%C3%ADa-660066?logo=eclipsemosquitto&logoColor=white)
![Docker](https://img.shields.io/badge/Docker-amd64%20%7C%20arm64-2496ED?logo=docker&logoColor=white)
![Arquitectura](https://img.shields.io/badge/arquitectura-hexagonal-orange)

</div>

---

## ✨ Qué es

El repositorio contiene **dos sistemas independientes**. Los dos usan **arquitectura hexagonal** y se comunican mediante un contrato de red común:

| | Proyecto | Qué hace | Tecnología |
|---|---|---|---|
| 🚁 | **[UAV_Onboard](UAV_Onboard)** — *Sistema A* | Software de a bordo: captura vídeo, detecta objetos con **YOLOv8n** en CPU y envía la telemetría y el vídeo a tierra. No tiene interfaz; se configura con `config.json` | C++17 · ONNX Runtime · OpenCV · libmosquitto · Docker |
| 🛰️ | **[UAV_GCS](UAV_GCS)** — *Sistema B* | Estación de tierra: recibe la telemetría y el vídeo y los muestra en un panel con las cajas de detección superpuestas | C++17 · Qt 6 Widgets · libmosquitto |

```mermaid
flowchart LR
    subgraph A["🚁 UAV_Onboard"]
        direction TB
        V["🎥 Vídeo"] --> Y["🧠 YOLOv8n"]
        Y --> T["📡 Telemetría JSON<br/>+ detecciones"]
        S["🔋 Telemetría simulada"] --> T
        V --> J["🗜️ JPEG ≤ 480p"]
    end

    B[("📮 Broker MQTT<br/>:1883")]

    subgraph G["🛰️ UAV_GCS"]
        direction TB
        R["Receptor de telemetría<br/>UDP · TCP · MQTT"] --> D["🖥️ Panel<br/>vídeo + cajas + telemetría"]
        RV["Receptor de vídeo"] --> D
    end

    T -- "UDP :5000" --> R
    T -- "TCP :5000" --> R
    T -- "MQTT" --> B -- "uav/telemetry" --> R
    J -- "UDP :5001" --> RV
```

---

## 🔌 Contrato de red

| Canal | Transporte | Puerto / topic | Formato |
|---|---|---|---|
| 📡 Telemetría | **UDP**, **TCP** o **MQTT**, a elegir | `5000`, o broker `1883` con topic `uav/telemetry` | JSON con batería, velocidad, altura, GPS y detecciones |
| 🎥 Vídeo | siempre **UDP** | `5001` | 1 datagrama = 1 frame JPEG |

El formato completo (framing TCP, claves JSON, coordenadas normalizadas…) está documentado en los README de cada proyecto.

---

## ⚡ Demo rápida (UDP, todo en el mismo PC)

> Antes, compila los dos proyectos siguiendo sus README: [UAV_GCS → Puesta en marcha](UAV_GCS/README.md#-puesta-en-marcha) y [UAV_Onboard → Puesta en marcha](UAV_Onboard/README.md#-puesta-en-marcha).

**1.** Lanza el dron. El `config.json` por defecto ya apunta a `127.0.0.1` por UDP:

```powershell
cd UAV_Onboard\out\build\x64-Release
.\UAV_Onboard.exe
```

**2.** Lanza la estación de tierra:

```powershell
.\UAV_GCS\out\build\x64-Release\UAV_GCS.exe
```

**3.** Deja `127.0.0.1:5000` y **UDP**, y pulsa **"Conectar al Dron"**. 🎉 Se abre el panel con el vídeo, las cajas de detección y la telemetría en directo.

> 🔀 Para probar **TCP**, **MQTT** o el dron dentro de **Docker** (con recursos limitados o emulando ARM64), consulta [la demo completa de UAV_Onboard](UAV_Onboard/README.md#-demo-completa-dron--estación-de-tierra).

---

## 📁 Estructura del repositorio

```
UAV/
├── UAV_Onboard/    🚁 Sistema A: dron (detección + envío)     → README propio
└── UAV_GCS/        🛰️ Sistema B: estación de tierra (Qt)      → README propio
```

---

<div align="center">

**Autor:** Jesús David Calvente Zapata

</div>
