<div align="center">
<h1> SISTEMAS EMBEBIDOS</h1>
</div>
<div align="center">
TRABAJO FINAL 2024: ALIMENTADOR A DIST
</div>

### CONCEPTO
El concepto nace desde el beneficio de poder brindarle comida a cualquier mascota sin necesidad de estar en casa o de olvidarse, que el mismo programa le dé el alimento a una determinada hora.

<hr>

### CIRCUITO
El circuito cuenta con los siguiente elementos:
<div align="center">
  <table style="width: 100%; text-align: center;">
    <tr>
      <td style="width: 33%;">Nombre</td>
      <td style="width: 33%;">Cantidad</td>
      <td style="width: 33%;">Componente</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Mecanismo de apertura</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Servomotor SG90</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Pantalla informativa</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">LCD 16 X 2 (I2C)</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Placa programable</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Esp8266</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Mecanismo sonoro</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Piezo (Buzzer) 5v</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Mecanismo vibratorio</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Motor de vibración</td>
    </tr>
    <tr>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Fuente de alimentación</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">1</td>
      <td colspan="1" style="width: 100%; padding-top: 50px;">Fuente de 3,3v – 5v para protoboard</td>
    </tr>
  </table>
</div>

<br>
<hr>

### DIAGRAMA
<br>
 <div align="center">
   
   <img>  ![image](https://github.com/user-attachments/assets/82f380b7-8971-44f1-b74f-63b5e123ffc9)</img>

</div>

<br>
<hr>

## FUNCIONALIDADES LOGRADAS
-	<b>	Conexión con internet y la app llamada Blynk (Interfaz gráfica)
-	<b> Conexión con servidor NTP para obtener el día y la hora al momento
-	<b> Dispensar porción de alimento a distancia mediante uso de un celular (Conexión y Servomotor)
-	<b>	Dispensado de alimento a distancia a las 10:00 AM y 20:00 PM de no haberse dispensado manualmente (Conexión a un servidor NTP y Servomotor)
-	<b> Emisión de un sonido a elección entre los 3 predefinidos al momento de dispensar la comida (Buzzer y conexión)
-	<b> Mostrar por pantalla y en la aplicación la última vez que dispensó alimento (Conexión y Display LCD)
-	<b> Vibración al momento de dispensar para acomodar los granos dentro de la tolva (Motor de vibración y conexión)

<br>

## VALIDACIONES
- <b> El sistema valida cuantas veces dispenso en el día al momento de dispensar y dispensar automáticamente para no sobrepasarse de las 2 comidas diarias.
- <b> El sistema sincroniza las variables guardadas en la aplicación con las del ESP8266 para que no haya variaciones.

NOTA: Se descarto el uso de la memoria EEPROM ya que en el ESP8266 la misma tiene ciclos de escritura/lectura limitados

<br>

## RIESGOS
### CORTE DE ENERGÍA
-	<b> El sistema está capacitado para validar al momento de iniciar la conexión si entre las 10:00 AM y las 14:00 PM no fue dispensado automáticamente, dispensa en ese momento.
        (Lo mismo en el horario nocturno, realizado para prevenir problemas con la dispensación automática y lo cortes)

<br>

### CORTE DE RED
-	<b> El sistema esta capacitado para manejar una reconexión ante cualquier percance con la red.

<br>

### AMBAS
-	<b> El sistema ante un corte de energía, permite aumentar el número de veces alimentado para así poder darle manualmente sin preocuparse. (Este número se sincroniza al momento que vuelve la energía/conexión)

<br>
<hr>

## CAPTURAS DEL PROGRESO Y RESULTADO
<div style="display: flex; justify-content: space-between;">

  <img src="https://github.com/user-attachments/assets/1be5fe19-cdcd-4169-af42-1f528d92ed7e" width="45%" style="margin: 0 10px;">
  <img src="https://github.com/user-attachments/assets/24122a83-de0a-4138-b110-d2b91d7c5972" width="45%" style="margin: 0 10px;">

</div>

<div style="display: flex; justify-content: space-between;">

  <img src="https://github.com/user-attachments/assets/7388f4cc-4b63-4b03-a380-022d5487719e" width="45%" style="margin: 0 10px;">
  <img src="https://github.com/user-attachments/assets/176f686f-c6bb-405f-8f34-25724fda49da" width="45%" style="margin: 0 10px;">

</div>

<div style="display: flex; justify-content: space-between;">

  <img src="https://github.com/user-attachments/assets/3af86305-1dda-4f41-8611-5631f959172a" width="45%" style="margin: 0 10px;">
  <img src="https://github.com/user-attachments/assets/dc40631e-4fa4-44fa-b272-c84d2ad227f3" width="45%" style="margin: 0 10px;">

</div>

