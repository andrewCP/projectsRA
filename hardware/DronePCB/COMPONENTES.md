# Lista de componentes — PCB E1 Mini (ESP32)

Componentes de la placa con su designador en la PCB y un proveedor de referencia (Colombia o LCSC/JLCPCB).
Los designadores coinciden con el esquemático y la PCB del archivo `.epro` de esta carpeta.

## Placa principal

| Designador | Componente | Proveedor |
|---|---|---|
| — | ESP32 DevKit 30 pines (ESP32-WROOM-32D) | [JA-Bots](https://ja-bots.com/producto/tarjeta-desarrollo-esp32-wroom-32d-30-pines/) |
| MPU1 | IMU MPU6050 (opción I2C) | [Electronilab](https://electronilab.co/tienda/mpu6050-acelerometro-y-giroscopio-i2c/) |
| MPU1 | IMU MPU9250 (opción 9 ejes) | [JA-Bots](https://ja-bots.com/producto/sensor-inercial-mpu-9250-9-ejes-giroscopio-acelerometro-magnetometro/) |
| H1, H2 | Regleta hembra 2,54 mm (zócalo de la ESP32, 2 × 15 pines) | [Electronilab](https://electronilab.co/tienda/regleta-hembra-2-54mm-40-pines/) |
| — | Receptor ELRS nano | [JA-Bots](https://ja-bots.com/producto/modulo-receptor-elrs-nano/) |
| H3, H4 | Regleta macho 2,54 mm a 90° | [Electronilab](https://electronilab.co/tienda/regleta-conector-macho-2-54mm-40-x-1-pines-inclinado-90/) |
| H5, H6 | Regleta macho 2,54 mm (expansión: un sensor I2C y otro SPI, a elección del docente) | [Electronilab](https://electronilab.co/tienda/regleta-conector-macho-2-54mm-40-x-1-pines/) |
| C9 | Capacitor cerámico 100 nF 50 V, 0603 | [Vistrónica](https://www.vistronica.com/componentes-pasivos/capacitores/capacitor-ceramico-100-nf-50v-yageo-smd-0603-x10-detail.html) |
| R11 | Resistencia 10 kΩ, 0603 (divisor de batería) | [Electronilab](https://electronilab.co/tienda/resistencia-smd-0603-10k-ohm/) |
| R10 | Resistencia 33 kΩ, 0603 (divisor de batería) | [Didácticas Electrónicas](https://www.didacticaselectronicas.com/shop/rs-33k-0603-resistencia-33k-0603-smd-100mw-14693) |

## Regulación de tensión

| Designador | Componente | Proveedor |
|---|---|---|
| U1 | Regulador LDO 3,3 V ME6118A33PG, SOT-89 | [LCSC C236721](https://www.lcsc.com/product-detail/C236721.html) |
| C6, C8 | Capacitor cerámico 100 nF 50 V, 0603 | [Vistrónica](https://www.vistronica.com/componentes-pasivos/capacitores/capacitor-ceramico-100-nf-50v-yageo-smd-0603-x10-detail.html) |
| C5, C7 | Capacitor cerámico 22 µF 16 V, 0805 | [Vistrónica](https://www.vistronica.com/componentes-pasivos/capacitores/capacitor-ceramico-22-uf-16v-samsung-smd-0805-x10-detail.html) |
| C10, C11 | Capacitor electrolítico 470 µF 10 V, SMD 6,3 mm | [JLCPCB C335982](https://jlcpcb.com/partdetail/ROQANG-RVT1A471M0607/C335982) · alternativa [Vistrónica 470 µF 16 V](https://www.vistronica.com/componentes-pasivos/capacitores/capacitor-electrolitico-de-aluminio-470-uf-16v-8x10-2-mm-smd-detail.html) |
| R9 | Resistencia 1 kΩ, 0603 | [Electronilab](https://electronilab.co/tienda/resistencia-smd-0603-1k-ohm/) |
| LED1 | LED 0805 rojo (indicador de encendido) | [Sigma Electrónica](https://www.sigmaelectronica.net/producto/led-0805-rojo/) |
| U2 | Conector JST PH 2,0 mm, 2 pines, macho a 90° (batería) | [Didácticas Electrónicas](https://www.didacticaselectronicas.com/shop/jst-ph2-0-2p-m-a-conector-macho-para-impreso-jst-pitch-2-0mm-de-2-pines-en-angulo-20079) |

## Accionamiento de motores

Los cuatro canales son idénticos. Cada uno tiene un MOSFET que conmuta el motor, su diodo de rueda libre, el desacople y un LED indicador.

| Componente | Motor 1 | Motor 2 | Motor 3 | Motor 4 | Proveedor |
|---|---|---|---|---|---|
| Conector JST 1,25 mm, 3 pines | U7 | U8 | U9 | U10 | [Parsek](https://parsek.com.co/cable-conector-jst-125-mm-macho-hembra) |
| Resistencia de compuerta 47 Ω, 0603 | R1 | R3 | R5 | R7 | [Electrónica Plug and Play](https://www.electronicaplugandplay.com/componentes-pasivos/product/457-resistencia-47-0603) |
| Resistencia pull-down 10 kΩ, 0603 | R2 | R4 | R6 | R8 | [Electronilab](https://electronilab.co/tienda/resistencia-smd-0603-10k-ohm/) |
| MOSFET N AO3416, SOT-23 | Q1 | Q2 | Q3 | Q4 | [LCSC C43307](https://www.lcsc.com/product-detail/C43307.html) |
| Diodo Schottky B5819W, SOD-123 | D1 | D2 | D3 | D4 | [JA-Bots](https://ja-bots.com/producto/diodo-schottky-b5819w-smd-1a-40v-baja-caida-y-alta-eficiencia/) |
| Capacitor cerámico 100 nF 50 V, 0603 | C1 | C2 | C3 | C4 | [Vistrónica](https://www.vistronica.com/componentes-pasivos/capacitores/capacitor-ceramico-100-nf-50v-yageo-smd-0603-x10-detail.html) |
| Resistencia de LED 1 kΩ, 0603 | R12 | R13 | R14 | R15 | [Electronilab](https://electronilab.co/tienda/resistencia-smd-0603-1k-ohm/) |
| LED 0805 rojo | LED2 | LED3 | LED4 | LED5 | [Sigma Electrónica](https://www.sigmaelectronica.net/producto/led-0805-rojo/) |
| Conector JST 1,25 mm, 2 pines (motor) | U3 | U4 | U5 | U6 | [LCSC C7430463](https://www.lcsc.com/product-detail/C7430463.html) |

> Nota: el archivo `BOM_Drones.csv` usa los valores del esquemático (R10 = 33 kΩ, R11 = 10 kΩ, R2/R4/R6/R8 = 10 kΩ).
> Los códigos LCSC heredados del diseño original que no correspondían al valor se dejaron vacíos.
