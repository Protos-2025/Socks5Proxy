---
title: Protocolo Agnostico para Monitoreo de Proxies

docname: pdc-itba-01-2025b
cat: exp

author:
      -
        ins: T. Pietravallo
        name: Tomás Pietravallo
        org: Protocolos de Comunicación, ITBA
        street:
          - San Martín 202
        city: Buenos Aires
        code: C1004AAF
        country: AR
        phone: +54 0810-222-4822
        email: tpietravallo@itba.edu.ar
        uri: https://itba.edu.ar
      -
        ins: M. Wehncke
        name: Máximo Wehncke
        org: Protocolos de Comunicación, ITBA
        street:
          - San Martín 202
        city: Buenos Aires
        code: C1004AAF
        country: AR
        phone: +54 0810-222-4822
        email: mwehncke@itba.edu.ar
        uri: https://itba.edu.ar
      -
        ins: L. Chiossone
        name: Lorenzo Chiossone
        org: Protocolos de Comunicación, ITBA
        street:
          - San Martín 202
        city: Buenos Aires
        code: C1004AAF
        country: AR
        phone: +54 0810-222-4822
        email: lchiossone@itba.edu.ar
        uri: https://itba.edu.ar
      -
        ins: L. Oliveto
        name: Lucia Oliveto
        org: Protocolos de Comunicación, ITBA
        street:
          - San Martín 202
        city: Buenos Aires
        code: C1004AAF
        country: AR
        phone: +54 0810-222-4822
        email: lchiossone@itba.edu.ar
        uri: https://itba.edu.ar

normative:
  RFC2119:

informative:
  REST:
    title: Architectural Styles and the Design of Network-based Software Architectures
    author:
        ins: R. Fielding
        name: Roy Fielding
        org: University of California, Irvine
    date: 2000

--- note Lorem Ipsum

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur nibh mi, mollis varius imperdiet id, venenatis ut nisi. Phasellus mauris urna, ultrices at massa id, faucibus malesuada nisi.

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur nibh mi, mollis varius imperdiet id, venenatis ut nisi. Phasellus mauris urna, ultrices at massa id, faucibus malesuada nisi.


--- abstract

El siguiente RFC describe el protocolo PAM y su correcta implementación para llevar a cabo un cliente-servidor de monitoreo y configuración remota de un proxy agnóstico a los protocolos que el mismo acepte.

--- middle

Introducción
============

El objetivo de este protocolo es permitir el monitoreo y configuración de un servidor proxy sin interrumpir la operación del mismo.

La conexión entre el cliente y el servidor se realiza a través de TCP utilizando alguno de los métodos de autenticación aquí definidos.

## Estructura de un pedido

### Autenticación

El cliente de este protocolo deberá comenzar la comunicación con un pedido con la siguiente estructura:

~~~~~~~~~~
+-------+-----------+-----------+------------+------------+
|  VER  |   NUSER   |   NPASS   |  USERNAME  |  PASSWORD  |
+-------+-----------+-----------+------------+------------+
|   1   |     1     |     1     |    NUSER   |    NPASS   |
+-------+-----------+-----------+------------+------------+
~~~~~~~~~~
{: #clientreq title="Pedido de autenticación de un cliente" alt="Pedido de autenticación de un cliente" }

Donde el servidor responderá al pedido con un mensaje de la siguiente estructura:

~~~~~~~~~~

+-------+-----------+
|  VER  |   STATUS  |
+-------+-----------+
|   1   |     1     |
+-------+-----------+

~~~~~~~~~~
{: #clientreq title="Respuesta del servidor a pedido de autenticación" alt="Respuesta del servidor a pedido de autenticación" }

Siendo VER la versión del protocolo, y STATUS un byte que indica el estado de la autenticación:

- 0x00: Autenticación exitosa
- 0x01: Autenticación fallida

Los estados 0x5 a 0x0F se reservan para futuras definiciones.

Los estados 0x10 a 0xFF se reservan para definiciones específicas de cada implementación del protocolo.

El servidor debe responder con un estado 0x00 o distinto de 0x00 según corresponda, donde un STATUS distinto de cero indicará un error de autenticación. El estado 0x01 podrá ser enviado en lugar de otro estado distinto de cero si no desea revelar información sobre el motivo del fallo de autenticación.

El usuario permanecerá autenticado hasta que la conexión sea cerrada por el cliente o el servidor.

### Pedidos de información

Un cliente autenticado podrá enviar pedidos de información o configuración al servidor utilizando la siguiente estructura:

~~~~~~~~~~

+-------+----------+----------+-----------+------------+
|  VER  | RESERVED |  METHOD  |   NBODY   |    BODY    |
+-------+----------+----------+-----------+------------+
|   1   |    0x0   |     2    |     2     |    NBODY   |
+-------+----------+----------+-----------+------------+

~~~~~~~~~~
{: #clientreq title="Pedido de un cliente" alt="Pedido de un cliente" }



#### VER

El campo VER indica la versión del protocolo. Actualmente, solo se soporta la versión 0x01.

#### STATUS

El campo STATUS indica el estado de la respuesta del servidor al pedido realizado. Los estados posibles son:

#### RESERVED

El campo RESERVED es un byte reservado para uso futuro y debe ser seteado en 0x00.

#### METHOD

El campo METHOD indica el tipo de pedido que se quiere realizar. Los métodos soportados son:
- **0x00: RESERVADO**
- **0x01: Obtener lista de usuarios conectados actualmente**
- **0x02: Añadir usuario**
- **0x03: Remover usuario**
- **0x04: Cambiar contraseña**
- **0x05: Cambiar rol**
- **0x06: Métricas**

- **0x07-0xF0: RESERVADO**

Los valores de METHOD 0x10 - 0x20 se reservan a definición de cada implementación del protocolo.

#### NBODY

El campo NBODY es un entero de 2 bytes que indica la longitud del campo BODY en bytes.

#### BODY

El campo BODY contiene la información adicional necesaria para completar el pedido, dependiendo del método seleccionado. Si el método no requiere información adicional, este campo deberá estar vacío y NBODY será 0x0000.

Si BODY contiene más bytes que los indicados en NBODY (o más de 65.535), los bytes adicionales serán ignorados. Si BODY contiene menos bytes que los indicados en NBODY, el servidor deberá responder con un error y cerrar la conexión.

El contenido de BODY debera ser, para cada método:

- **0x01: Obtener lista de usuarios conectados actualmente**:

Vacío.


- **0x02: Añadir usuario**

~~~~~~~~~~

+-------+------------+-------+------------+-------+
| ULEN  |  USERNAME  | PLEN  |  PASSWORD  |  ROL  |
+-------+------------+-------+------------+-------+
|   1   |    ULEN    |   1   |    PLEN    |   1   |
+-------+------------+-------+------------+-------+

~~~~~~~~~~
{: #clientreq title="Formato de BODY para el pedido del método 0x02" alt="Respuesta del servidor a un pedido" }

- **0x03: Remover usuario**

~~~~~~~~~~
+-------+------------+
| ULEN  |  USERNAME  | 
+-------+------------+
|   1   |    ULEN    | 
+-------+------------+
~~~~~~~~~~
{: #clientreq title="Formato de BODY para el pedido del método 0x03" alt="Respuesta del servidor a un pedido" }

- **0x04: Cambiar contraseña**

~~~~~~~~~~

+-------+------------+-----------+--------------+
| ULEN  |  USERNAME  | NEW_PLEN  | NEW_PASSWORD |
+-------+------------+-----------+--------------+
|   1   |    ULEN    |     1     |   NEW_PLEN   |
+-------+------------+-----------+--------------+

~~~~~~~~~~
{: #clientreq title="Formato de BODY para el pedido del método 0x04" alt="Respuesta del servidor a un pedido" }
- **0x05: Cambiar rol**

~~~~~~~~~~

+-------+------------+-----------+
| ULEN  |  USERNAME  | NEW_ROL   |
+-------+------------+-----------+
|   1   |    ULEN    |     1     |
+-------+------------+-----------+

~~~~~~~~~~
{: #clientreq title="Formato de BODY para el pedido del método 0x05" alt="Respuesta del servidor a un pedido" }
- **0x06: Métricas**

Vacío.

### Respuesta del servidor

El servidor responde al pedido con un mensaje de la siguiente estructura


~~~~~~~~~~


+-------+-----------+-------------+------------+
|  VER  |   STATUS  |    NBODY    |    BODY    |
+-------+-----------+-------------+------------+
|   1   |     1     |      2      |    NBODY   |
+-------+-----------+-------------+------------+


~~~~~~~~~~
{: #clientreq title="Respuesta del servidor a un pedido" alt="Respuesta del servidor a un pedido" }

#### VER

El campo VER indica la versión del protocolo. Actualmente, solo se soporta la versión 0x01.

#### STATUS

El campo STATUS indica el estado de la respuesta del servidor al pedido realizado. Los estados posibles son:
- 0x00: Pedido exitoso
- 0x01: Pedido fallido
- 0x02: No autorizado
- 0x03: Usuario ya existente
- 0x04: Usuario contraseña incorrecta
- 0x05: Usuario credenciales muy largas
- 0x06: Usuario incorrecto


#### NBODY

El campo NBODY es un entero de 2 bytes que indica la longitud del campo BODY en bytes.

#### BODY
El campo BODY contiene la información que se desea enviarle a el usuario. 


# Figures

Figuras...

# References {#refstyle}

The IETF documents referred to here are.

# Security Considerations

Debido a la falta de encriptación del protocolo, no se aconseja su uso en entornos donde se pueda realizar un ataque de sniffing. 

# IANA Considerations

Seriously?


--- back

# Lorem ipsum

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur nibh mi, mollis varius imperdiet id, venenatis ut nisi. Phasellus mauris urna, ultrices at massa id, faucibus malesuada nisi.

