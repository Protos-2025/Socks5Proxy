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
        name: Maximo Wehncke
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

El siguiente RFC describe el protocolo [[XXX]] y su correcta implementación para llevar a cabo un cliente-servidor de monitoreo y configuración remota de un proxy agnostico a los protocolos que el mismo acepte.

--- middle

Introduccion
============

El objetivo de este protocolo es permitir el monitoreo y configuracion de un servidor proxy sin interrumpir la operacion del mismo.

La conexión entre el cliente y el servidor se realiza a través de TCP utilizando alguno de los metodos de autenticación aqui definidos.

## Estructura de un pedido

### Autenticacion

El cliente de este protocolo debera comenzar la comunicacion con un pedido con la siguiente estructura:

~~~~~~~~~~
+-------+-------+------------+-------+------------+
|  VER  | ULEN  |  USERNAME  | PLEN  |  PASSWORD  |
+-------+-------+------------+-------+------------+
|   1   |   1   |  1 to 255  |   1   |  1 to 255  |
+-------+-------+------------+-------+------------+

~~~~~~~~~~
{: #clientreq title="Pedido de autenticacion de un cliente" alt="Pedido de autenticacion de un cliente" }

Donde el servidor respondera al pedido con un mensaje de la siguiente estructura:

~~~~~~~~~~

+-------+-----------+
|  VER  |   STATUS  |
+-------+-----------+
|   1   |     1     |
+-------+-----------+

~~~~~~~~~~
{: #clientreq title="Respuesta del servidor a pedido de autenticacion" alt="Respuesta del servidor a pedido de autenticacion" }

Siendo VER la version del protocolo, y STATUS un byte que indica el estado de la autenticacion:
- 0x00: Autenticacion exitosa
- 0x01: Autenticacion fallida
# - 0x02: Error de protocolo        (devuelve 0x00 si es succses 0x01 si es failure, no dice nada mas, yo sacaria estos)
# - 0x03: Error interno del servidor
# - 0x04: Demasiados intentos fallidos de autenticacion

Los estados 0x5 a 0x0F se reservan para futuras definiciones.

Los estados 0x10 a 0xFF se reservan para definiciones especificas de cada implementacion del protocolo.

El servidor debe responder con un estado 0x00 o distinto de 0x00 segun corresponda, donde un STATUS distinto de cero indicara un error de autenticacion. El estado 0x01 podra ser enviado en lugar de otro estado distinto de cero si no desea revelar informacion sobre el motivo del fallo de autenticacion.

El usuario permanecera autenticado hasta que la conexion sea cerrada por el cliente o el servidor.

### Pedidos de informacion

Un cliente autenticado podra enviar pedidos de informacion o configuracion al servidor utilizando la siguiente estructura:

~~~~~~~~~~

+-------+----------+----------+-----------+------------+
|  VER  | RESERVED |  METHOD  |   NBODY   |    BODY    |
+-------+----------+----------+-----------+------------+
|   1   |    0x0   |     2    |     2     |    NBODY   |
+-------+----------+----------+-----------+------------+

~~~~~~~~~~
{: #clientreq title="Pedido de un cliente" alt="Pedido de un cliente" }

~~~~~~~~~~

+-------+-----------+-------------+------------+
|  VER  |   STATUS  |    NBODY    |    BODY    |
+-------+-----------+-------------+------------+
|   1   |     1     |      2      |    NBODY   |
+-------+-----------+-------------+------------+

~~~~~~~~~~
{: #clientreq title="Respuesta del servidor a un pedido" alt="Respuesta del servidor a un pedido" }

#### VER

El campo VER indica la version del protocolo. Actualmente, solo se soporta la version 0x01.

#### STATUS

El campo STATUS indica el estado de la respuesta del servidor al pedido realizado. Los estados posibles son:

#### RESERVED

El campo RESERVED es un byte reservado para uso futuro y debe ser seteado en 0x00.

#### METHOD

El campo METHOD indica el tipo de pedido que se quiere realizar. Los metodos soportados son:
- 0x00: RESERVADO
- 0x01: Obtener cantidad de usuarios conectados actualmente
- 0x02: Obtener lista de usuarios conectados actualmente
- 0x03: Obtener la cantidad de conexiones historicas
- 0x04: Obtener bytes transferidos historicamente
- 0x05-0xF0: RESERVADO
- 0xF3: Reiniciar la cantidad de conexiones historicas
- 0xF4: Reiniciar la cantidad de bytes transferidos historicamente

Los valores de METHOD 0x10 - 0x20 se reservan a definicion de cada implementacion del protocolo.

#### NBODY

El campo NBODY es un entero de 2 bytes que indica la longitud del campo BODY en bytes.

#### BODY

El campo BODY contiene la informacion adicional necesaria para completar el pedido, dependiendo del metodo seleccionado. Si el metodo no requiere informacion adicional, este campo debera estar vacio y NBODY sera 0x0000.

Si BODY contiene mas bytes que los indicados en NBODY (o mas de 65.535), los bytes adicionales seran ignorados. Si BODY contiene menos bytes que los indicados en NBODY, el servidor debera responder con un error y cerrar la conexion.

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
