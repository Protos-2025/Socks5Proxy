# Socks 5 Proxy - Grupo 01

## 1. Indice.

- [Socks 5 Proxy - Grupo 01](#socks-5-proxy---grupo-01)
  - [1. Indice.](#1-indice)
  - [2. Descripción.](#2-descripción)
  - [3. Problemas encontrados durante el diseño y la implementación.](#3-problemas-encontrados-durante-el-diseño-y-la-implementación)
  - [4. Limitaciones de la aplicación.](#4-limitaciones-de-la-aplicación)
  - [5. Posibles extensiones.](#5-posibles-extensiones)
  - [6. Conclusiones.](#6-conclusiones)
  - [7. Ejemplos de prueba.](#7-ejemplos-de-prueba)
  - [8. Guía de instalación](#8-guía-de-instalación)
  - [9. Instrucciones para la configuración.](#9-instrucciones-para-la-configuración)
  - [10. Ejemplos de configuración y monitoreo.](#10-ejemplos-de-configuración-y-monitoreo)
  - [11. Documento de diseño del proyecto](#11-documento-de-diseño-del-proyecto)

## 2. Descripción.

El presente detalla el desarrollo de un proxy para el protocolo SOCKSv5\[[RFC1928](https://datatracker.ietf.org/doc/html/rfc1928)\].
El protocolo implementa autenticacion siguiendo los lineamientos de [RFC1929](https://datatracker.ietf.org/doc/html/rfc1929).

## 3. Problemas encontrados durante el diseño y la implementación.

Algunas dificultades encontradas durante el desarrollo del proyecto incluyen:

* La falta de tareas paralelizables en etapas tempranas del desarrollo, lo que dificultó la distribución del trabajo entre los miembros del equipo. Para solventar esto, [se opto por probar ciertos estados de forma sintetica mediante tests](../tests/copy_test.c) utilizando la libreria libcheck. En particular, se implementaron tests para el estado de copia de datos entre el cliente y el servidor de origen, utilizando mocks de las funciones send y recv para emular la transferencia de datos sin necesidad de establecer conexiones reales y/o interoperar con el sistema operativo. Esto permitió validar el correcto funcionamiento de la lógica de copia de datos y asegurar que los buffers se manejaban adecuadamente.
* El mantener la calidad de codigo y consistencia durante el desarrollo. Un reto que afrontamos de manera temprana con dos herramientas:
  * Pre-commit hooks: Se implementaron hooks pre-commit que formattearan el codigo automaticamente utilizando clang-format. Esto aseguro que todo el codigo comprometido siguiera un estilo consistente, facilitando la lectura y mantenimiento del mismo.
  * Analisis estatico de codigo: Se utilizo PVS-Studio para realizar analisis estaticos de codigo de manera automatica en cada push y pull request a la rama main. Esto ayudo a identificar posibles errores, vulnerabilidades y problemas de calidad en el codigo antes de que fueran integrados.
* Complejidad para evaluar la performance del proxy bajo diferentes condiciones de carga. Utilizamos la herramienta Apache JMeter para crear escenarios de prueba que simularan múltiples conexiones concurrentes. Asi mismo, se utilizo bpftrace para monitorear cualquier uso de syscall bloquantes en el servidor. Para facilitar la ejecucion de estas pruebas, se opto por correr estas en Github Actions, permitiendo asi la automatizacion de las mismas y la obtencion de resultados consistentes.
  ![actions.jpeg](./img/actions.jpeg)
  Si bien le ejecucion de pruebas de performance en Github Actions presento ciertas limitaciones debido a la falta de control sobre el entorno de ejecucion, observamos que las fluctuaciones provenientes de estos [estaban entre 10 y 20%](https://github.com/marketplace/actions/continuous-benchmark-with-pr-comments?utm_source=chatgpt.com#stability-of-virtual-environment) lo cual decidimos tolerar a costa de estandarizar y automatizar la ejecucion de las pruebas. Estas pruebas se corrieron sobre cada pull request y push a la rama main

## 4. Limitaciones de la aplicación.

## 5. Posibles extensiones.

## 6. Conclusiones.

## 7. Ejemplos de prueba.

## 8. Guía de instalación

Para compilar y correr el proxy SOCKSv5, se deben seguir los siguientes pasos:

```sh
# Compilar el proyecto
make clean all
```

```sh
# Correr el servidor
./bin/server
```

## 9. Instrucciones para la configuración.

## 10. Ejemplos de configuración y monitoreo.

## 11. Documento de diseño del proyecto
