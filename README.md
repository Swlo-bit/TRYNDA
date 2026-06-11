# Robot Manipulador Mecatrónico de 4 GDL con Programación de Trayectorias

<img width="1062" height="783" alt="Ensamble 1 TRYNDA" src="https://github.com/user-attachments/assets/38f893af-e7ad-41b7-9cb9-6912a2148100" />


## Descripción

Este proyecto corresponde al desarrollo de un robot manipulador de 4 grados de libertad más un gripper, realizado como proyecto integrador de la carrera de Mecatrónica.

El sistema reúne conocimientos adquiridos durante la formación académica en áreas como electrónica, automatización, control, programación, comunicaciones industriales, diseño mecánico, manufactura e impresión 3D.

El robot permite el control manual de cada articulación, la programación de posiciones mediante enseñanza por puntos, el posicionamiento por ángulos y la reproducción automática de trayectorias previamente almacenadas.

Aunque el sistema es completamente funcional, constituye una plataforma abierta que puede ser mejorada en futuras versiones mediante algoritmos avanzados de cinemática, planificación de trayectorias, visión artificial y sistemas de control más sofisticados.


## Características Principales

* Control de 4 grados de libertad más gripper.
* Homing automático mediante finales de carrera.
* Retroalimentación absoluta mediante encoders AS5600.
* Programación por puntos (Teach & Repeat).
* Programación por ángulos.
* Control manual mediante joystick.
* Comunicación inalámbrica ESP-NOW.
* Comunicación I²C entre controladores.
* Paro de emergencia local y remoto.
* Protección del gripper mediante monitoreo de corriente.
* Estructura fabricada principalmente mediante impresión 3D.

  
## Arquitectura de Comunicación y Sensado

Uno de los principales desafíos del proyecto fue la utilización de cuatro encoders magnéticos AS5600 para obtener la posición absoluta de cada articulación.

El AS5600 utiliza comunicación I²C y posee una dirección fija de fábrica (0x36), por lo que múltiples sensores no pueden coexistir en el mismo bus I²C sin generar conflictos de direccionamiento.

Para solucionar esta limitación se implementó una arquitectura distribuida:

* Cada articulación cuenta con un controlador dedicado.
* Cada Arduino Nano posee dos buses I²C independientes:

  * I²C hardware nativo para la comunicación con el sistema principal.
  * I²C por software (SoftwareWire) dedicado exclusivamente a su encoder AS5600.

De esta manera cada controlador puede comunicarse con su propio encoder utilizando una línea I²C aislada, evitando conflictos por direcciones duplicadas.

### Distribución de Comunicaciones

**ESP32 Maestro**

* Interfaz de usuario.
* Comunicación inalámbrica mediante ESP-NOW.

**ESP32 Esclavo**

* Coordinador general del robot.
* Comunicación ESP-NOW con el maestro.
* Comunicación I²C con los Arduino Nano.

**Arduino Nano Hombro**

* I²C hardware: comunicación con ESP32.
* I²C software: encoder AS5600.

**Arduino Nano Codo**

* I²C hardware: comunicación con ESP32.
* I²C software: encoder AS5600.

**Arduino Nano Muñeca**

* I²C hardware: comunicación con ESP32.
* I²C software: encoder AS5600.

Gracias a esta arquitectura fue posible utilizar múltiples encoders AS5600 simultáneamente sin necesidad de multiplexores I²C adicionales.
Esta solución permitió evitar el uso de multiplexores I²C externos, simplificando el hardware y distribuyendo la carga de procesamiento entre varios microcontroladores.


## Arquitectura del Sistema

El sistema está dividido en varios controladores distribuidos:

### ESP32 Maestro

Responsable de:

* Interfaz de usuario.
* Pantalla OLED.
* Teclado matricial.
* Joystick.
* Selección de modos de operación.
* Envío de comandos mediante ESP-NOW.

### ESP32 Esclavo M0

Responsable de:

* Coordinación general del robot.
* Control del eje Base (M1).
* Control del gripper.
* Gestión de trayectorias.
* Supervisión de seguridad.
* Comunicación con los Arduino Nano.

### Arduino Nano M1 (Hombro)

Responsable del control cerrado del eje hombro utilizando:

* Motor paso a paso.
* Encoder AS5600.
* Final de carrera para homing.

### Arduino Nano M2 (Codo)

Responsable del control cerrado del eje codo utilizando:

* Motor paso a paso.
* Encoder AS5600.
* Final de carrera para homing.

### Arduino Nano M3 (Muñeca)

Responsable del control cerrado del eje muñeca utilizando:

* Motor paso a paso.
* Encoder AS5600.
* Final de carrera para homing.


## Modos de Operación

El robot incorpora tres modos principales de programación y control, permitiendo tanto la manipulación manual como la ejecución automática de trayectorias.

### 1. Programación por Ángulos Incrementales

En este modo el operador selecciona una articulación específica y posteriormente introduce un valor angular mediante el teclado matricial.

El valor ingresado no corresponde a una posición absoluta, sino a un incremento respecto a la posición actual de la articulación.

Por ejemplo:

* Posición actual: 45°
* Valor ingresado: +20°
* Nueva posición objetivo: 65°

O bien:

* Posición actual: 45°
* Valor ingresado: -10°
* Nueva posición objetivo: 35°

Cada controlador calcula internamente la nueva posición objetivo sumando o restando el incremento al ángulo absoluto medido por su encoder AS5600.


### 2. Control Manual por Joystick

Este modo permite mover individualmente cualquiera de las articulaciones utilizando el joystick integrado en el control maestro.

El operador selecciona el eje deseado y puede realizar movimientos manuales en ambos sentidos para posicionar el robot de forma rápida e intuitiva.

Este modo es utilizado principalmente para:

* Posicionar manualmente el manipulador.
* Realizar pruebas de funcionamiento.
* Enseñar trayectorias.
* Ajustar posiciones antes de almacenarlas.


### 3. Programación por Puntos

Este modo permite crear trayectorias mediante enseñanza directa.

El usuario puede:

* Liberar una articulación específica.
* Liberar todas las articulaciones simultáneamente.
* Posicionar el robot manualmente.
* Posicionar el robot mediante joystick.
* Fijar nuevamente las articulaciones.
* Almacenar la posición actual como un punto de trayectoria.

Cada vez que se guarda un punto, el sistema registra simultáneamente:

* Posición de la base.
* Posición del hombro.
* Posición del codo.
* Posición de la muñeca.
* Posición del gripper.

Es decir, cada punto representa el estado completo del robot en un instante determinado.

Los puntos almacenados pueden reproducirse posteriormente de manera automática, permitiendo ejecutar secuencias repetitivas sin necesidad de reprogramación.

Actualmente el sistema permite almacenar hasta 20 puntos por rutina, valor que puede ampliarse en futuras versiones.


## Sistema de Seguridad

El robot incorpora múltiples mecanismos de seguridad:

* Paro de emergencia físico.
* Paro de emergencia remoto.
* Límites mecánicos por software.
* Homing obligatorio antes de operación.
* Supervisión de corriente en el gripper mediante INA219.
* Detección de fallos de comunicación con encoders AS5600.


## Hardware Utilizado

### Controladores

* 2 ESP32
* 3 Arduino Nano

### Sensores

* 4 Encoders magnéticos AS5600
* 1 Sensor de corriente INA219
* Finales de carrera mecánicos

### Actuadores

* 1  NEMA 23 (M1)
* 1  NEMA 17 (M0)
* 2  NEMA 17 5:1 (M2 & M3)
* 1  MG996R (Gripper o M5)

### Electrónica de Potencia

* 3 Driver TB6600
* Driver DM556
* Fuente 12V 10A
* 2 Convertidores LM2596

### Mecánica

* Piezas impresas en 3D
* Rodamientos
* Insertos roscados
* Pernos y tuercas
* Acoples mecánicos


## Fabricación

La estructura mecánica fue diseñada principalmente para fabricación mediante impresión 3D.

Debido a las altas cargas transmitidas por una de las articulaciones, fue necesario fabricar una pieza mecánica metálica para la unión del motor correspondiente, ya que la versión impresa no soportaba los esfuerzos generados durante la operación.


## Posibles Mejoras Futuras
* Implementación de cinemática inversa.
* Planificación automática de trayectorias.
* Interpolación lineal y circular.
* Integración con ROS.
* Sistema de visión artificial.
* Detección y seguimiento de objetos.
* Interfaz gráfica para programación.
* Monitoreo remoto mediante WiFi.
* Control avanzado de movimiento.


## Autores

* Sebastian A. Saico
* Saul Andrés Sánchez Barbecho
  * Email: saulsanchezbarbecho@gmail.com
* Jonnathan A. Valdez
* Edwin J. Suqui

Tecnología Superior en Mecatrónica

TECAZUAY – Ecuador
<img width="575" height="562" alt="TRYNDA real" src="https://github.com/user-attachments/assets/34f5c46e-22c9-4a82-93b3-126874cfc13c" />
<img width="899" height="1599" alt="TRYNDA real 2" src="https://github.com/user-attachments/assets/ad195d50-d67a-4407-a31a-6321e3fb672c" />

