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

# Estructura de un pedido

El cliente de este protocolo debera comenzar la comunicacion con un pedido con la siguiente estructura:

~~~~~~~~~~

+-------+-----------+----------+----------+---------+-----------+
|  VER  |   AUTH    | RESERVED |  METHOD  |   LEN   |    BODY   |
+-------+-----------+----------+----------+---------+-----------+
|   1   |    XXX    |    0x0   |     2    |    2    |    LEN    |
+-------+-----------+----------+----------+---------+-----------+



~~~~~~~~~~
{: #clientreq title="Pedido de un cliente" alt="Pedido de un cliente" }


## VER

El campo VER indica la version del protocolo. Actualmente, solo se soporta la version 0x01.

## AUTH XXX

## RESERVED

El campo RESERVED es un byte reservado para uso futuro y debe ser seteado en 0x00.

## METHOD

El campo METHOD indica el tipo de pedido que se quiere realizar. Los metodos soportados son:
- 0x00: Healthcheck
- 0x01: Obtener estadisticas
- 0x02: Resetear estadisticas
- 0x03: Cambiar configuracion

Los valores de METHOD 0x10 - 0x20 se reservan a definicion de cada implementacion del protocolo.

## LEN

El campo LEN es un entero de 2 bytes que indica la longitud del campo BODY en bytes.

## BODY

El campo BODY contiene la informacion adicional necesaria para completar el pedido, dependiendo del metodo seleccionado. Si el metodo no requiere informacion adicional, este campo debera estar vacio y LEN sera 0x0000.

Si BODY contiene mas bytes que los indicados en LEN (o mas de 65.535), los bytes adicionales seran ignorados. Si BODY contiene menos bytes que los indicados en LEN, el servidor debera responder con un error y cerrar la conexion.

# Figures

~~~~~~~~~~

+-------+-----------+----------+----------+---------+-----------+
|  VER  |   AUTH    | RESERVED |  METHOD  |   LEN   |    BODY   |
+-------+-----------+----------+----------+---------+-----------+
|   1   |    XXX    |    0x0   |     2    |    2    |    LEN    |
+-------+-----------+----------+----------+---------+-----------+

~~~~~~~~~~
{: #clientreq title="Pedido de un cliente" alt="Pedido de un cliente" }


# References {#refstyle}

The IETF documents referred to here are.

# Security Considerations

Debido a la falta de encriptación del protocolo, no se aconseja su uso en entornos donde se pueda realizar un ataque de sniffing. 

# IANA Considerations

Seriously?


--- back

# Lorem ipsum

Lorem ipsum dolor sit amet, consectetur adipiscing elit. Curabitur nibh mi, mollis varius imperdiet id, venenatis ut nisi. Phasellus mauris urna, ultrices at massa id, faucibus malesuada nisi.
