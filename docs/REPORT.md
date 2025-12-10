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
  - [9.1 Variables](#91-variables)
  - [10. Ejemplos de configuración y monitoreo.](#10-ejemplos-de-configuración-y-monitoreo)
  - [11. Documento de diseño del proyecto](#11-documento-de-diseño-del-proyecto)
  - [12. Extras](#12-extras)
    - [12.1 Matriz de Benchmarks](#121-matriz-de-benchmarks)
    - [12.2 bpftrace y monitoreo de syscalls bloqueantes](#122-bpftrace-y-monitoreo-de-syscalls-bloqueantes)
    - [12.3 Complilación y pruebas con Docker](#123-complilación-y-pruebas-con-docker)

## 2. Descripción.

El presente detalla el desarrollo de un proxy para el protocolo SOCKSv5\[[RFC1928](https://datatracker.ietf.org/doc/html/rfc1928)\].
El protocolo implementa autenticacion siguiendo los lineamientos de [RFC1929](https://datatracker.ietf.org/doc/html/rfc1929).

## 3. Problemas encontrados durante el diseño y la implementación.

Algunas dificultades encontradas durante el desarrollo del proyecto incluyen:

* La falta de tareas paralelizables en etapas tempranas del desarrollo, lo que dificultó la distribución del trabajo entre los miembros del equipo. Para solventar esto, [se opto por probar ciertos estados de forma sintetica mediante tests](../tests/copy_test.c) utilizando la libreria libcheck. En particular, se implementaron tests para el estado de copia de datos entre el cliente y el servidor de origen, utilizando mocks de las funciones send y recv para emular la transferencia de datos sin necesidad de establecer conexiones reales y/o interoperar con el sistema operativo. Esto permitió validar el correcto funcionamiento de la lógica de copia de datos y asegurar que los buffers se manejaban adecuadamente.
* El mantener la calidad de codigo y consistencia durante el desarrollo. Un reto que afrontamos de manera temprana con dos herramientas:
  * Pre-commit hooks: Se implementaron hooks pre-commit que formattearan el codigo automaticamente utilizando [clang-format](../.clang-format) y [clang-tidy](../.clang-tidy). Esto aseguro que todo el codigo comprometido siguiera un estilo consistente, facilitando la lectura y mantenimiento del mismo.
  * Analisis estatico de codigo: Se utilizo PVS-Studio para realizar analisis estaticos de codigo de manera automatica en cada push y pull request a la rama main. Esto ayudo a identificar posibles errores, vulnerabilidades y problemas de calidad en el codigo antes de que fueran integrados.
* Complejidad para evaluar la performance del proxy bajo diferentes condiciones de carga. Utilizamos la herramienta [Apache JMeter](https://jmeter.apache.org/) para crear escenarios de prueba que simularan múltiples conexiones concurrentes. Asi mismo, se utilizo bpftrace para monitorear cualquier uso de syscall bloquantes en el servidor. Para facilitar la ejecucion de estas pruebas, se opto por correr estas en Github Actions, permitiendo asi la automatizacion de las mismas y la obtencion de resultados consistentes.
  ![actions.jpeg](./img/actions.jpeg)
  Si bien le ejecucion de pruebas de performance en Github Actions presento ciertas limitaciones debido a la falta de control sobre el entorno de ejecucion, vimos que las fluctuaciones provenientes de estos [estaban entre 10 y 20%](https://github.com/marketplace/actions/continuous-benchmark-with-pr-comments?utm_source=chatgpt.com#stability-of-virtual-environment) lo cual decidimos tolerar a costa de estandarizar y automatizar la ejecucion de las pruebas. Estas pruebas se corrieron sobre cada pull request y push a la rama main

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

## 9.1 Variables

Se pueden modificar algunas constantes fácilmente definiendo variables de entorno. Estas se van a tomar de un archivo `.env` en la raíz del proyecto. Se puede usar el [`.env.sample`](./.env.sample) provisto como punto de partida.

Algunas de las variables disponibles para su modificacion incluyen:

| Variable             | Descripción                                                                                                                                     | Valor por Defecto |
|----------------------|-------------------------------------------------------------------------------------------------------------------------------------------------|-------------------|
| `DEBUG`              | Habilita flags de compilación para debug (-g, -fsanitize=address, etc)                                                                          | `true`            |
| `TRACE`              | Habilita el monitoreo con `bpftrace` para syscalls bloqueantes al ejecutar server.sh (el entrypoint del servicio `server` en docker)            | `false`           |
| `BUFFER_SIZE`        | Tamaño del buffer usado para leer/escribir datos entre sockets                                                                                  | `1024`            |
| `MAX_LOG_QUEUE_SIZE` | Cantidad máxima de mensajes de log que pueden estar en cola antes de descartar logs                                                             | `100`             |
| `MAX_LOG_SIZE`       | Tamaño máximo (en bytes) de un mensaje de log individual. Los mensajes más largos se van a truncar con puntos suspensivos.                      | `1024`            |
| `LOGGER_MIN_LEVEL`   | Nivel mínimo de log a registrar. Valores posibles: `LOGGER_TRACE`, `LOGGER_DEBUG`, `LOGGER_INFO`, `LOGGER_WARN`, `LOGGER_ERROR`, `LOGGER_FATAL` | `LOGGER_INFO`     |

Todas las variables del servidor y sus valores por defecto se pueden encontrar en [defines.h](./src/server/include/defines.h).

Ademas, la variable `MOCK_ETC_HOST` puede ser definida para establecer un archivo alternativo al `/etc/hosts` del sistema host, el cual sera montado en el contenedor Docker del servidor proxy. Esto es util para pruebas de integracion y performance, ya que permite controlar la resolucion DNS.

## 10. Ejemplos de configuración y monitoreo.

## 11. Documento de diseño del proyecto

El proyecto cuenta con los siguientes modulos principales:
- [src/shared/](../src/shared/): Contiene codigo compartido entre el cliente y el servidor, incluyendo definiciones de estructuras de datos y funciones utilitarias.
- [src/server/](../src/server/): Implementa la logica del servidor proxy SOCKSv5, incluyendo el manejo de conexiones entrantes, autenticacion y transferencia de datos.
- [src/client/](../src/client/): Implementa el cliente de [nuestro protocolo](./PROTOCOL.md)
- [tests/](../tests/): Contiene tests unitarios y de integracion para validar el correcto funcionamiento del proxy.
- [docs/](../docs/): Documentacion del proyecto, incluyendo este reporte y el documento de diseño.
- [scripts/](../scripts/): Scripts auxiliares para la compilacion, ejecucion de pruebas y otras tareas relacionadas con el desarrollo. (Utilizados principalmente como entrypoints de contenedores Docker).
- [.github/workflows/](../.github/workflows/): Configuraciones de Github Actions para CI y pruebas automatizadas.

## 12. Extras

### 12.1 Matriz de Benchmarks

Para poder probar el proxy bajo condiciones de carga y encontrar la mejor configuracion de parametros, se utilizo [Apache JMeter](https://jmeter.apache.org/) para crear escenarios de prueba que simularan multiples conexiones concurrentes. Los scripts de JMeter se encuentran en la carpeta [bench/jmeter](./bench/jmeter). Estas pruebas de carga se corrieron contra una matriz variada de parametros (compilando con mayor y menor tamaño de buffer, mayor y menor capacidad de selectores, etc) y los resultados se evaluaron de acuerdo a su porcentaje de error en respuesta a las conexiones y el tiempo de respuesta (p99 y media). La ejecucion de estas pruebas se automatizo utilizando Github Actions, permitiendo asi la obtencion de resultados consistentes y la comparacion entre distintas configuraciones.

![img/bench-matrix.jpeg](./img/bench-matrix.jpeg)

Los resultados de estas pruebas se pueden encontrar en la seccion de "Actions" del repositorio, en el workflow titulado "Matrix Benchmarking". Cada corrida genera un reporte detallado con las metricas obtenidas para cada configuracion probada.

### 12.2 bpftrace y monitoreo de syscalls bloqueantes

Para asegurarnos de no bloquearnos inecesariamente en syscalls de lectura y escritura, se utilizo `bpftrace` para monitorear las syscalls bloqueantes durante la ejecucion del servidor proxy.

El monitoreo con `bpftrace` se puede habilitar facilmente al correr el servidor utilizando la variable de entorno `TRACE=true`. Esto hace que el entrypoint del contenedor Docker para el servidor ejecute el script de `bpftrace` junto con el servidor proxy, registrando cualquier syscall bloqueante que ocurra durante su ejecucion. Al correr en docker, estandarizamos el entorno y facilitamos la ejecucion del monitoreo sin necesidad de instalar `bpftrace` directamente en el sistema host.

![img/bpftrace-monitoring.jpeg](./img/bpftrace-monitoring.jpeg)

Esta herramienta nos ayudo a identificar y solucionar problemas relacionados con bloqueos innecesarios, mejorando asi la performance y capacidad de respuesta del proxy bajo condiciones de carga.

### 12.3 Complilación y pruebas con Docker

Si bien el proyecto puede compilarse y correrse directamente en un entorno local con `make`, se proveen contenedores Docker para facilitar la compilacion, ejecucion y pruebas del proxy SOCKSv5 en un entorno estandarizado.

El uso de docker nos permitio contar con entornos estandarizados para la compilacion y pruebas, y facilito el uso de herramientas adicionales como `bpftrace` para monitoreo y `Apache JMeter` para pruebas de carga.

A su vez, el sistema de contenedores y red de docker nos permitio facilmente asignar direcciones IPv4 e IPv6 fijas a los contenedores, controlar el DNS, y modificar el archivo /etc/host para simular la resolucion DNS, facilitando la ejecucion de pruebas de integracion y performance entre el cliente, el servidor proxy y servidores de prueba.

Durante las pruebas de performance, se utilizo un archivo `/etc/hosts` personalizado para controlar la resolucion DNS de los servidores de prueba. Este archivo se monta en el contenedor del servidor proxy utilizando la variable de entorno `MOCK_ETC_HOST`, permitiendo asi simular diferentes escenarios de resolucion DNS sin afectar el sistema host. En particular, el dominio `fakegoogle.com` se mapea a las direcciones IPv4 e IPv6 del contenedor `nginx-test-server`, permitiendo asi evaluar la capacidad del proxy para FQDNs y resolucion DNS sin depender de las condiciones externas de la red.
