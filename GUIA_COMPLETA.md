# Guia Completa: Sistema de Analisis de Extractos de Inversion

## Tabla de Contenido

1. [Que es un extracto de inversion?](#1-que-es-un-extracto-de-inversion)
2. [Glosario de terminos financieros](#2-glosario-de-terminos-financieros)
3. [Estructura del PDF real](#3-estructura-del-pdf-real)
4. [La aplicacion: pantalla por pantalla](#4-la-aplicacion-pantalla-por-pantalla)
5. [Las graficas: como leerlas](#5-las-graficas-como-leerlas)
6. [El analisis estadistico: que significa cada numero](#6-el-analisis-estadistico-que-significa-cada-numero)
7. [Los archivos de salida (CSV y JSON)](#7-los-archivos-de-salida-csv-y-json)
8. [Ejemplo con datos reales](#8-ejemplo-con-datos-reales)

---

## 1. Que es un extracto de inversion?

Imagina que tienes dinero en un banco, y el banco te manda un resumen mensual de tu cuenta. Un **extracto de inversion** es lo mismo, pero para inversiones.

La empresa **Acciones & Valores S.A.** es una comisionista de bolsa colombiana. Ellos manejan el dinero de sus clientes y lo invierten en diferentes productos financieros. Cada mes (o periodo), le envian al cliente un PDF con el resumen de:

- Cuanto dinero tiene invertido en total
- En que productos esta invertido (CDTs, fondos, efectivo)
- Cuanto gano o perdio en el periodo
- Las tasas de interes que le estan pagando

Este programa lee ese PDF automaticamente (usando OCR para "leer" el texto de las imagenes) y organiza toda esa informacion en tablas, graficas y estadisticas.

---

## 2. Glosario de terminos financieros

### Terminos generales

| Termino | Que significa |
|---------|--------------|
| **Portafolio** | El conjunto total de todas tus inversiones. Como una "cartera" que contiene todo tu dinero invertido en diferentes productos. |
| **Activos** | Todo lo que tienes (tu dinero invertido). Si tienes $6.386 millones en total, esos son tus activos. |
| **Pasivos** | Lo que debes. En este caso generalmente es $0 porque no le debes nada a la comisionista. |
| **NIT** | Numero de Identificacion Tributaria. Es como la cedula pero para efectos de impuestos. |
| **Comisionista de bolsa** | Empresa autorizada para comprar y vender inversiones en nombre de sus clientes. Acciones & Valores es una de estas. |
| **Asesor** | La persona de la comisionista asignada a tu cuenta para aconsejarte. |

### Tipos de inversion en el extracto

| Termino | Que significa |
|---------|--------------|
| **Renta Fija** | Inversiones donde sabes de antemano cuanto te van a pagar. Ejemplo: un CDT donde el banco te dice "te pago 10% anual". Es como prestarle dinero a alguien con la promesa de que te devuelve mas. |
| **FIC (Fondo de Inversion Colectiva)** | Un "pozo" de dinero donde muchas personas ponen su plata junta, y un administrador profesional la invierte. Las ganancias se reparten entre todos segun cuanto pusiste. |
| **Saldos en Efectivo** | Dinero que esta en tu cuenta pero NO esta invertido. Es como tener plata en la cuenta corriente del banco sin hacer nada con ella. |
| **CDT (Certificado de Deposito a Termino)** | Le prestas dinero al banco por un tiempo fijo (ejemplo: 1 ano), y el banco te paga un interes. No puedes sacar el dinero antes de que se cumpla el plazo. |

### La tabla de Renta Fija (columna por columna)

| Columna | Que significa | Ejemplo |
|---------|--------------|---------|
| **Nemotecnico** | El "nombre corto" o codigo del instrumento. Identifica que CDT o titulo especifico es. | `CDTDTF0123` |
| **Fecha Emision** | Cuando se creo el CDT. El dia que el banco dijo "te presto este certificado". | `15/03/2025` |
| **Fecha Vencimiento** | Cuando se termina el CDT y te devuelven tu dinero + intereses. | `15/03/2026` |
| **Fecha Compra** | Cuando TU compraste ese CDT (puede ser diferente a la emision si lo compraste de segunda mano). | `20/03/2025` |
| **Tasa Facial** | El interes que PROMETE pagar el CDT, tal como dice en el papel. Es fija y no cambia. Ejemplo: si dice 9.76%, te pagan 9.76% anual sobre lo invertido. | `9.7618%` |
| **Periodicidad** | Cada cuanto te pagan los intereses. Puede ser mensual, trimestral, semestral o al vencimiento. | `TV` (trimestre vencido) |
| **Valor Nominal** | Cuanto dinero invertiste originalmente en ese CDT. | `$1.000.000.000` |
| **Tasa Negociacion** | El interes real al que se COMPRO el CDT. Si lo compraste mas barato o mas caro que el valor original, esta tasa refleja eso. | `10.09%` |
| **Tasa Valoracion** | El interes que el mercado dice que vale ESE CDT HOY. Cambia todos los dias segun la oferta y demanda del mercado. Es como el "precio actual" del CDT en terminos de interes. | `11.6688%` |
| **Valor Mercado** | Cuanto vale tu CDT HOY si lo vendieras. No es lo mismo que el nominal porque las tasas del mercado cambian. | `$1.095.267.330` |

#### Relacion entre las 3 tasas (ejemplo simple)

Piensa en un CDT como comprar un bono:
- **Tasa Facial (9.76%)**: Lo que dice el papel que te pagan. Es fija.
- **Tasa Negociacion (10.09%)**: Lo que realmente pactaste al comprarlo. Si compraste "con descuento", esta tasa es mas alta que la facial.
- **Tasa Valoracion (11.67%)**: Lo que el mercado dice que vale hoy. Si esta tasa es mas alta que la de negociacion, significa que los intereses del mercado subieron y tu CDT vale un poco menos (porque tu CDT paga menos que lo que el mercado ofrece hoy). Si es mas baja, tu CDT vale mas.

### Terminos del Fondo (FIC)

| Termino | Que significa |
|---------|--------------|
| **Saldo Anterior** | Cuanto dinero tenias en el fondo al INICIO del periodo. | 
| **Adiciones** | Dinero nuevo que metiste al fondo durante el periodo. |
| **Retiros** | Dinero que sacaste del fondo durante el periodo. |
| **Rendimientos** | Las ganancias (o perdidas) que genero tu dinero en el fondo. Es lo que te "gano" el fondo. |
| **Nuevo Saldo** | Cuanto tienes al FINAL del periodo. Debe ser = Saldo Anterior + Adiciones - Retiros + Rendimientos. |
| **Unidades** | Los fondos dividen el dinero total en "pedazos" llamados unidades. Cuantas unidades tienes determina que porcion del fondo es tuya. |
| **Valor Unidad** | Cuanto vale cada unidad hoy. Si el fondo gana dinero, el valor de cada unidad sube. |

### Rentabilidades historicas del fondo

| Termino | Que significa |
|---------|--------------|
| **Rentabilidad Mensual** | Cuanto gano (en porcentaje) el fondo en el ULTIMO MES. Ejemplo: 4.46% significa que si tenias $100, ahora tienes $104.46. |
| **Rentabilidad Semestral** | Cuanto gano en los ultimos 6 meses. |
| **Ano Corrido** | Cuanto gano desde el 1 de enero hasta hoy. |
| **Ultimo Ano** | Cuanto gano en los ultimos 12 meses completos. |
| **Ultimos 2/3 anos** | Promedio anual de los ultimos 2 o 3 anos. |
| **E.A.** | "Efectivo Anual". Significa que la tasa ya esta calculada para un ano completo. Si dice "11.66% E.A.", quiere decir que en un ano tu dinero creceria 11.66%. |

---

## 3. Estructura del PDF real

El PDF de Acciones & Valores tiene 4 paginas y esta protegido con contrasena:

### Pagina 1 - Resumen del Portafolio (vertical)
Es como la "portada" del extracto. Contiene:
- Datos del cliente (nombre, NIT, direccion, telefono)
- Datos del asesor
- Tabla resumen: cuanto tienes en cada tipo de inversion
- **Total Portafolio**: la suma de todo

Los valores aparecen con formato colombiano: `$4.382.067.330,00`
- Los puntos (`.`) separan miles: `4.382.067.330` = cuatro mil trescientos ochenta y dos millones
- La coma (`,`) separa decimales: `,00` = cero centavos

### Pagina 2 - Informacion del Defensor (vertical)
Pagina legal/informativa sobre el defensor del consumidor financiero. **No tiene datos utiles** - el programa la ignora.

### Pagina 3 - Detalle de Inversiones (HORIZONTAL/landscape)
Esta pagina esta en formato horizontal (mas ancha que alta). Contiene las tablas con bordes reales:
- **Saldos en Efectivo**: cuanto hay en cuentas de efectivo
- **Renta Fija**: los 4 CDTs con todas sus columnas (nemotecnico, fechas, tasas, valores)
- **Resumen FIC**: resumen del fondo

### Pagina 4 - Extracto del Fondo (vertical)
Detalle del Fondo de Inversion Colectiva:
- Datos del inversionista
- Movimientos del periodo (saldo anterior, adiciones, retiros, rendimientos, nuevo saldo)
- Rentabilidades historicas (mensual, semestral, etc.)

---

## 4. La aplicacion: pantalla por pantalla

### Barra superior
- **Seleccionar PDF**: Abre un dialogo para elegir el archivo PDF del extracto
- **Procesar**: Inicia el analisis (la app pedira la contrasena si el PDF esta protegido)
- **Exportar CSV**: Guarda los datos en archivos CSV (que puedes abrir en Excel)
- **Barra de progreso**: Muestra el avance del procesamiento
- **Log**: Panel inferior que muestra mensajes de lo que esta haciendo el programa

### Tab 1: Resumen

Muestra los datos de la Pagina 1 del PDF en formato tabla simple:

| Campo | Que ves |
|-------|---------|
| Fecha Extracto | La fecha del periodo del extracto |
| Nombre Cliente | El nombre del titular de la cuenta |
| NIT | Identificacion tributaria |
| Renta Fija | Total invertido en CDTs y titulos de renta fija |
| FIC | Total invertido en fondos de inversion colectiva |
| Saldos en Efectivo | Dinero no invertido |
| Total Activos | Suma de todo lo anterior |
| Total Pasivos | Lo que se debe (generalmente $0) |
| **TOTAL PORTAFOLIO** | **El valor total de todas las inversiones** |

### Tab 2: Renta Fija

Tabla con los instrumentos individuales de renta fija (los CDTs). Cada fila es un CDT diferente con sus 10 columnas (ver glosario arriba).

**Como leer esta tabla**: Cada fila es un CDT que posee el cliente. Los valores importantes son:
- **Nemotecnico**: identifica cual CDT es
- **Tasa Valoracion**: a cuanto lo valora el mercado hoy
- **Valor Mercado**: cuanto vale en pesos hoy

### Tab 3: Fondos

Datos del Fondo de Inversion Colectiva. Muestra:
- Informacion del fondo y del inversionista
- Los movimientos del periodo
- Las rentabilidades historicas

**La formula clave**: `Nuevo Saldo = Saldo Anterior + Adiciones - Retiros + Rendimientos`

### Tab 4: Graficas

Panel dividido en dos partes:
- **Lista izquierda**: nombres de las 5 graficas generadas (ver seccion 5)
- **Panel derecho**: la grafica seleccionada

### Tab 5: Analisis

Resumen estadistico con:
- Composicion del portafolio (que porcentaje hay en cada tipo de inversion)
- Estadisticas de las tasas de interes de los CDTs
- Alertas (situaciones que podrian requerir atencion)

---

## 5. Las graficas: como leerlas

El programa genera 5 graficas en formato PNG:

### Grafica 1: Composicion del Portafolio (%)

**Que muestra**: Barras horizontales que muestran que porcentaje del portafolio esta en cada tipo de inversion.

**Como leerla**: 
- Si "Renta Fija" muestra 68.6%, significa que el 68.6% del dinero total esta en CDTs
- Si "FIC" muestra 31.4%, ese porcentaje esta en fondos
- Lo ideal es tener diversificacion (no todo en un solo tipo)

**Ejemplo**:
```
Renta Fija   ████████████████████████████  68.6%
FIC          ████████████               31.4%
Efectivo     █                           0.0%
```

### Grafica 2: Distribucion Renta Fija por Nemotecnico

**Que muestra**: Barras verticales comparando el valor de mercado de cada CDT.

**Como leerla**:
- Cada barra es un CDT diferente
- La altura de la barra indica cuanto vale ese CDT
- Te permite ver de un vistazo cual CDT tiene mas dinero invertido

### Grafica 3: Rentabilidades FIC (%)

**Que muestra**: Barras con las rentabilidades historicas del fondo.

**Como leerla**:
- Cada barra es un periodo diferente (mensual, semestral, etc.)
- Barras mas altas = mejor rendimiento en ese periodo
- Si una barra esta por debajo de cero, el fondo perdio dinero en ese periodo
- Es util para ver la tendencia: si las barras recientes son mas altas, el fondo esta mejorando

### Grafica 4: Evolucion del Portafolio

**Que muestra**: Una linea con area rellena que muestra como ha cambiado el valor total del portafolio a lo largo del tiempo.

**Como leerla**:
- El eje horizontal es el tiempo (cada punto es un mes)
- El eje vertical es el valor total en pesos
- Si la linea sube, el portafolio esta creciendo
- Si la linea baja, el portafolio perdio valor

**Datos historicos**: Los primeros 12 puntos vienen del archivo `historico_inicial.csv` (datos de ejemplo de marzo 2025 a febrero 2026). El ultimo punto es el extracto real que procesaste.

### Grafica 5: Tabla de Estadisticas

**Que muestra**: Una tabla renderizada como imagen con el resumen estadistico de las tasas y las alertas.

**Como leerla**: Ver seccion 6 para el detalle de cada metrica.

---

## 6. El analisis estadistico: que significa cada numero

### Composicion porcentual

Simplemente cuanto porcentaje del total representa cada tipo de inversion.

**Formula**: `Porcentaje = (Valor del activo / Total Portafolio) * 100`

**Ejemplo real**:
- Renta Fija: $4.382.067.330 / $6.386.676.308 = **68.6%**
- FIC: $2.004.608.978 / $6.386.676.308 = **31.4%**

### Estadisticas de tasas

Para los CDTs, el programa calcula estas estadisticas sobre las tasas de interes:

| Metrica | Que significa | Para que sirve |
|---------|--------------|----------------|
| **Media** | El promedio de todas las tasas. Suma todas y divide entre cuantas hay. | Ver en general a que tasa estan pagando los CDTs. |
| **Desviacion Estandar (Desv.Est)** | Que tan "dispersas" estan las tasas respecto al promedio. Si es baja, todas las tasas son parecidas. Si es alta, hay mucha variacion. | Detectar si tienes CDTs con tasas muy diferentes entre si. |
| **Mediana** | El valor del medio cuando ordenas todas las tasas de menor a mayor. No se afecta por valores extremos. | Complementa la media; si son muy diferentes, hay valores atipicos. |
| **Min** | La tasa mas baja de todos los CDTs. | Identificar el CDT que menos paga. |
| **Max** | La tasa mas alta de todos los CDTs. | Identificar el CDT que mas paga. |
| **N** | Cuantos CDTs hay. | Contexto. |

**Ejemplo con tasas de valoracion reales**:
- Tasas: 11.67%, 10.64%, 12.41%, 11.36%
- Media: (11.67 + 10.64 + 12.41 + 11.36) / 4 = **11.52%**
- Min: **10.64%**, Max: **12.41%**
- La desviacion estandar seria ~0.74%, lo que indica que las tasas son bastante parecidas entre si

### Alertas

El programa genera alertas automaticas para ayudar a identificar situaciones importantes:

| Tipo | Color | Significado |
|------|-------|-------------|
| **INFO** | Gris | Informacion general, no necesariamente un problema |
| **WARNING** | Naranja | Algo que podria necesitar atencion |
| **CRITICAL** | Rojo | Situacion que requiere atencion inmediata |

**Alertas comunes**:
- `Alta concentracion en X: 80%+` — Mas del 80% del dinero esta en un solo tipo de inversion. Poca diversificacion.
- `Tasa de valoracion atipica en CDT_X` — Un CDT tiene una tasa muy diferente al resto. Podria ser una oportunidad o un riesgo.
- `Rendimiento negativo en fondo` — El fondo perdio dinero. Puede ser temporal pero hay que estar atento.
- `Tasa facial baja en CDT_X: <5%` — Un CDT paga menos del 5%, que podria ser bajo dependiendo del contexto.
- `Crecimiento del portafolio: X%` — Cuanto crecio el portafolio en total durante el periodo historico.

### Outliers (valores atipicos)

El programa detecta outliers usando la regla de **k-sigma**: si un valor esta a mas de 2 desviaciones estandar del promedio, se considera atipico.

**Ejemplo simple**: Si las tasas de valoracion son 11.67, 10.64, 12.41, 11.36 (promedio ~11.52, desviacion ~0.74):
- Un valor seria atipico si esta por debajo de 11.52 - 2*0.74 = 10.04 o por encima de 11.52 + 2*0.74 = 13.00
- Ninguna de las tasas del ejemplo es atipica

---

## 7. Los archivos de salida (CSV y JSON)

### Archivos CSV (se abren con Excel)

El programa exporta 4 archivos CSV al directorio que elijas:

| Archivo | Contenido |
|---------|-----------|
| `extracto.csv` | Datos del resumen: cliente, activos, totales |
| `renta_fija.csv` | Tabla completa de instrumentos de renta fija con las 10 columnas |
| `fondos.csv` | Datos del fondo: movimientos y rentabilidades |
| `analisis.csv` | Estadisticas, composicion y alertas |

**Para abrir en Excel**: Doble clic en el archivo .csv, o en Excel ir a Datos > Desde texto/CSV. Los valores usan punto como decimal (formato internacional).

### Archivo JSON

Se guarda automaticamente en `output/json/extracto.json`. Contiene TODOS los datos extraidos en formato estructurado. Es util si quieres procesar los datos con otro programa (Python, JavaScript, etc.).

**Estructura del JSON**:
```
{
  "resumen": {
    "nombre_cliente": "...",
    "total_portafolio": 6386676308.68,
    "activos": { "Renta Fija": 4382067330.0, "FIC": 2004608978.68 },
    ...
  },
  "renta_fija": [
    { "nemotecnico": "CDT...", "tasa_valoracion": 11.668834, ... },
    ...
  ],
  "fondos": [
    { "saldo_anterior": 1997903323.18, "rendimientos": 6705655.50, ... }
  ]
}
```

### Graficas PNG

Se guardan en `output/graphs/`:
- `01_composicion_portafolio.png`
- `02_distribucion_renta_fija.png`
- `03_rentabilidades_fic.png`
- `04_evolucion_saldo.png`
- `05_tabla_estadisticas.png`

---

## 8. Ejemplo con datos reales

Basado en los datos del extracto real:

### Resumen del portafolio

| | Valor |
|---|---|
| Total Renta Fija | $4.382.067.330 |
| Total FIC | $2.004.608.978,68 |
| Saldos en Efectivo | $0 |
| **Total Portafolio** | **$6.386.676.308,68** |

Esto significa que el cliente tiene ~6.387 millones de pesos (colombianos) invertidos, de los cuales:
- **68.6%** esta en CDTs (renta fija) — inversion conservadora, rendimiento predecible
- **31.4%** esta en un fondo (FIC) — puede tener mejor rendimiento pero mas riesgo
- **0%** en efectivo — todo el dinero esta invertido

### Los 4 CDTs

| CDT | Tasa Facial | Tasa Valoracion | Valor Mercado |
|-----|------------|----------------|---------------|
| CDT 1 | 9.76% | 11.67% | ~$1.095M |
| CDT 2 | 9.13% | 10.64% | ~$1.095M |
| CDT 3 | 10.12% | 12.41% | ~$1.095M |
| CDT 4 | 9.44% | 11.36% | ~$1.095M |

**Observacion**: Las tasas de valoracion (lo que el mercado dice hoy) son mas altas que las tasas faciales (lo que promete el papel). Esto significa que las tasas del mercado subieron desde que se compraron estos CDTs.

### Movimientos del fondo

| Movimiento | Valor |
|-----------|-------|
| Saldo anterior | $1.997.903.323,18 |
| Adiciones | $0 |
| Retiros | $0 |
| Rendimientos | $6.705.655,50 |
| **Nuevo saldo** | **$2.004.608.978,68** |

**Lectura**: El cliente no metio ni saco dinero del fondo este mes. El fondo genero ~$6.7 millones de ganancias, subiendo el saldo de ~$1.998M a ~$2.005M.

**Verificacion**: 1.997.903.323,18 + 0 - 0 + 6.705.655,50 = 2.004.608.978,68 ✓

---

> **Nota**: Si ves valores que no coinciden con los del PDF, revisa la consola/log del programa. Los mensajes de diagnostico muestran exactamente que texto leyo el OCR y como lo interpreto. Los errores mas comunes son por mala lectura del OCR en imagenes de baja calidad.
