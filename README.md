# Follow Person: Seguimiento de Personas en 3D con ROS 2 y Lifecycle Nodes

[![ROS 2](https://img.shields.io/badge/ROS_2-Humble%20%2F%20Iron-blue.svg)](https://docs.ros.org/)
[![C++](https://img.shields.io/badge/Language-C%2B%2B17-00599C.svg)](https://isocpp.org/)
[![Build Tool: colcon](https://img.shields.io/badge/Build-colcon-orange.svg)](https://colcon.readthedocs.io/)
[![Architecture: Managed Nodes](https://img.shields.io/badge/ROS2-LifecycleNode-success.svg)](https://design.ros2.org/articles/node_lifecycle.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

Aplicación para **ROS 2** diseñada para la detección en tres dimensiones y el seguimiento continuo de una persona a una distancia de consigna fija ($1.0\text{ m}$), manteniéndose permanentemente orientado hacia el objetivo. 

El proyecto integra un pipeline de percepción 3D que transforma detecciones a transformadas geométricas (**TF2**), una arquitectura de control basada en nodos gestionados (**Lifecycle Nodes**) y lazo cerrado mediante controladores **PID** duales (lineal y angular).

---

## 📌 Tabla de Contenidos
- [Estructura del Repositorio](#-estructura-del-repositorio)
- [Arquitectura del Sistema](#-arquitectura-del-sistema)
- [Pipeline de Percepción 3D (`detection_2_tf_node`)](#-pipeline-de-percepción-3d-detection_2_tf_node)
- [Nodo de Control Gestionado (`follow_person_node`)](#-nodo-de-control-gestionado-follow_person_node)
  - [Ciclo de Vida (Managed Node)](#1-ciclo-de-vida-managed-node)
  - [Máquina de Estados Finitos](#2-máquina-de-estados-finitos)
  - [Control PID Dual](#3-control-pid-dual-pidcontroller)
- [Parámetros de Configuración](#-parámetros-de-configuración)
- [Compilación y Construcción](#-compilación-y-construcción)
- [Guía de Ejecución y Gestión de Ciclo de Vida](#-guía-de-ejecución-y-gestión-de-ciclo-de-vida)

---

## 📂 Estructura del Repositorio

```text
follow_person/
├── CMakeLists.txt
├── package.xml
├── README.md
├── config/
│   └── params.yaml                 # Parámetros PID, umbrales y frames TF
├── include/follow_person/
│   ├── detection_2_tf_node.hpp     # Nodo puente: Detection3DArray -> TF broadcast
│   ├── follow_person_node.hpp      # Nodo Lifecycle principal y FSM
│   └── PIDController.hpp           # Implementación genérica del controlador PID
├── launch/
│   └── follow_person.launch.py     # Lanzador coordinado con carga de parámetros
└── src/
    ├── follow_person/
    │   ├── detection_2_tf_node.cpp
    │   ├── follow_person_node.cpp
    │   └── PIDController.cpp
    ├── detection_2_tf_main.cpp     # Entry point para detection_2_tf_node
    └── follow_person_main.cpp      # Entry point para follow_person_node
```

---

## 📐 Arquitectura del Sistema

```
 ┌────────────────────────────────────┐
 │  Detector 3D (PointCloud / Depth)  │
 └─────────────────┬──────────────────┘
                   │ vision_msgs/msg/Detection3DArray
                   ▼
 ┌────────────────────────────────────┐
 │       detection_2_tf_node          │
 └─────────────────┬──────────────────┘
                   │ TF Broadcast (target_frame: "person_detected")
                   ▼
 ┌────────────────────────────────────┐
 │        TF2 Tree Buffer             │
 └─────────────────┬──────────────────┘
                   │ LookupTransform ("base_link" -> "person_detected")
                   ▼
 ┌────────────────────────────────────┐
 │   follow_person_node (Lifecycle)   │
 │   - FSM (Búsqueda / Seguimiento)   │
 │   - PID Lineal (distancia a 1.0 m) │
 │   - PID Angular (orientación a 0)  │
 └─────────────────┬──────────────────┘
                   │ /cmd_vel (geometry_msgs/msg/Twist)
                   ▼
 ┌────────────────────────────────────┐
 │          Robot Mobile Base         │
 └────────────────────────────────────┘
```

---

## 👁️ Pipeline de Percepción 3D (`detection_2_tf_node`)

El subsistema de percepción desacopla el algoritmo de detección del control cinemático:

1. Se suscribe al tópico de detecciones tridimensionales emitido por el detector (basado en `sensor_msgs/msg/PointCloud2` o mapas de profundidad).
2. Procesa la posición central de la caja delimitadora (*bounding box*) del mensaje `vision_msgs/msg/Detection3DArray`.
3. Emite dinámicamente un marco de coordenadas TF2 (`person_detected` o configurable) referenciado con respecto al sistema de coordenadas de la cámara o al marco `odom`/`map`.

---

## ⚙️ Nodo de Control Gestionado (`follow_person_node`)

### 1. Ciclo de Vida (Managed Node)
El nodo de control implementa la interfaz `rclcpp_lifecycle::LifecycleNode`, garantizando una inicialización y parada controladas:

```
 [Unconfigured] ──configure()──► [Inactive] ──activate()──► [Active]
       ▲                             │                         │
       │                             ▼                         ▼
  cleanup()                      shutdown()               deactivate()
```

- **Unconfigured / Inactive:** El nodo arranca desactivado por defecto; inicializa estructuras y parámetros pero no emite comandos a los actuadores.
- **Active:** Al recibir la transición de activación, habilita los temporizadores de control, el cliente TF2 y el publicador `/cmd_vel`.
- **Deactivate:** Detiene inmediatamente al robot enviando velocidades nulas y congela el cómputo de lazos PID.

### 2. Máquina de Estados Finitos
Durante el estado `Active`, el nodo opera bajo dos modos principales:

```
        ┌──────────────────────────────────────────────┐
        │                                              │
        ▼                                              │
 ┌──────────────┐          Persona detectada    ┌──────────────┐
 │   BÚSQUEDA   ├──────────────────────────────►│ SEGUIMIENTO  │
 │ (Spin / Scan)│                               │ (PID Activo) │
 └──────────────┘◄──────────────────────────────┴──────────────┘
                         Persona perdida
                       (Timeout sin TF válida)
```

- **Modo BÚSQUEDA:** Si no hay un marco TF válido de la persona en el buffer (o expira por timeout), el robot rota sobre su eje vertical a una velocidad angular controlada para barrer el entorno visual hasta reenganchar al usuario.
- **Modo SEGUIMIENTO:** Al disponer de la TF actualizada, se calcula la posición relativa $(x, y)$ respecto a `base_link` y se alimentan los lazos PID.

### 3. Control PID Dual (`PIDController`)
El cálculo de velocidad diferencial se divide en dos lazos independientes:

* **Control de Distancia (Velocidad Lineal $v_x$):**
  $$\text{error}_d = \sqrt{x^2 + y^2} - d_{\text{objetivo}} \quad \text{donde } d_{\text{objetivo}} = 1.0\text{ m}$$
  $$v_x = K_{p,v} \cdot \text{error}_d + K_{i,v} \int \text{error}_d \, dt + K_{d,v} \frac{d(\text{error}_d)}{dt}$$

* **Control de Orientación (Velocidad Angular $\omega_z$):**
  $$\text{error}_\theta = \text{atan2}(y, x)$$
  $$\omega_z = K_{p,\omega} \cdot \text{error}_\theta + K_{i,\omega} \int \text{error}_\theta \, dt + K_{d,\omega} \frac{d(\text{error}_\theta)}{dt}$$

---

## 🛠️ Parámetros de Configuración (`config/params.yaml`)

| Parámetro | Tipo | Descripción | Valor por Defecto |
| :--- | :--- | :--- | :--- |
| `target_distance` | `double` | Distancia deseada a mantener respecto a la persona | `1.0` m |
| `target_frame` | `string` | Nombre del frame TF emitido por el detector | `person_detected` |
| `base_frame` | `string` | Frame de referencia de la base del robot | `base_link` |
| `search_angular_vel` | `double` | Velocidad angular en fase de búsqueda | `0.4` rad/s |
| `pid_linear.kp`, `.ki`, `.kd` | `double` | Ganancias Proporcional, Integral y Derivativa lineal | Calibradas en YAML |
| `pid_angular.kp`, `.ki`, `.kd` | `double` | Ganancias Proporcional, Integral y Derivativa angular | Calibradas en YAML |
| `max_linear_vel` | `double` | Saturación superior de velocidad lineal | `0.5` m/s |
| `max_angular_vel` | `double` | Saturación superior de velocidad angular | `1.0` rad/s |

---

## 🔨 Compilación y Construcción

1. Configura el entorno base de ROS 2:
   ```bash
   source /opt/ros/$ROS_DISTRO/setup.bash
   ```

2. Sitúate en la raíz del workspace de colcon y compila el paquete:
   ```bash
   cd ~/ros2_ws
   colcon build --symlink-install --packages-select follow_person
   ```

3. Carga el entorno resultante en tu terminal:
   ```bash
   source install/setup.bash
   ```

---

## 🚀 Guía de Ejecución y Gestión de Ciclo de Vida

### 1. Lanzamiento Unificado
Ejecuta el launcher principal, el cual levantará el convertidor de detecciones a TF y el nodo de control en estado latente:
```bash
ros2 launch follow_person follow_person.launch.py
```

### 2. Gestión del Lifecycle Node
El nodo `follow_person_node` arranca sin configurar. Desde una segunda terminal, interactúa con su ciclo de vida mediante la CLI de ROS 2:

* **Paso 1: Configurar el nodo (Unconfigured -> Inactive):**
  ```bash
  ros2 lifecycle set /follow_person_node configure
  ```

* **Paso 2: Activar el seguimiento (Inactive -> Active):**
  ```bash
  ros2 lifecycle set /follow_person_node activate
  ```
  *(En este instante el robot comenzará a buscar o a mantener la distancia de 1 metro).*

* **Paso 3: Pausar/Desactivar el seguimiento (Active -> Inactive):**
  ```bash
  ros2 lifecycle set /follow_person_node deactivate
  ```

* **Comprobar estado actual del ciclo de vida:**
  ```bash
  ros2 lifecycle get /follow_person_node
  ```