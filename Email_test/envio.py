import RPi.GPIO as GPIO
import time
import serial
import random
import threading
import smtplib
from email.message import EmailMessage
from datetime import datetime

# Archivo de texto con el contenido del correo
textfile = "mensaje.txt"
apagacion = "mensaje2.txt"
text = "activacion.txt"

# Dirección del remitente y destinatario
you = "ramirezsanchezm489@gmail.com"
me = "nelsonramirezn9@gmail.com"

def correo():
    try:
        # Obtener la fecha y hora actual
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Leer el contenido del archivo y agregar la fecha y hora de envío
        with open(textfile, "r", encoding="utf-8") as fp:
            content = fp.read()

        # Crear el mensaje de correo
        msg = EmailMessage()
        msg.set_content(f"{content}\n\nAlarma parada el: {now}")
        msg['Subject'] = f'The contents of {textfile}'
        msg['From'] = me
        msg['To'] = you

        # Conectar al servidor SMTP y enviar el correo
        with smtplib.SMTP('smtp.gmail.com', 587) as s:
            s.starttls()  # Habilita cifrado TLS
            s.login(me, "lbfz miuj xnjh pisa")  # Verifica que esta contraseña sea válida
            s.send_message(msg)

        print(f"Reporte enviado el {now}")

    except Exception as e:
        print("Error al enviar correo:", e)

def correo2():
    try:
        # Obtener la fecha y hora actual
        now = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        # Leer el contenido del archivo y agregar la fecha y hora de envío
        with open(apagacion, "r", encoding="utf-8") as fp:
            content = fp.read()

        # Crear el mensaje de correo
        msg = EmailMessage()
        msg.set_content(f"{content}\n\nAlarma parada el: {now}")
        msg['Subject'] = f'The contents of {apagacion}'
        msg['From'] = me
        msg['To'] = you

        # Conectar al servidor SMTP y enviar el correo
        with smtplib.SMTP('smtp.gmail.com', 587) as s:
            s.starttls()  # Habilita cifrado TLS
            s.login(me, "lbfz miuj xnjh pisa")  # Verifica que esta contraseña sea válida
            s.send_message(msg)

        print(f"Reporte enviado el {now}")

    except Exception as e:
        print("Error al enviar correo:", e)
TRIG1 = 22  
ECHO1 = 26
TRIG2 = 20  
ECHO2 = 21

IN1 = 5
IN2 = 6
ENA = 13
button1 = 16

#Bandera para habilitar o deshabilitar la bandera de temperatura
enviar_temp = True

GPIO.setmode(GPIO.BCM)  
GPIO.setup(TRIG1, GPIO.OUT)  
GPIO.setup(ECHO1, GPIO.IN)
GPIO.setup(TRIG2, GPIO.OUT)  
GPIO.setup(ECHO2, GPIO.IN)
GPIO.setup(IN1, GPIO.OUT)
GPIO.setup(IN2, GPIO.OUT)
GPIO.setup(ENA, GPIO.OUT)
GPIO.setup(button1, GPIO.IN, pull_up_down=GPIO.PUD_UP)

# Configuración del UART
ser = serial.Serial('/dev/ttyACM0', 9600, timeout=1)
ser.reset_input_buffer()
time.sleep(2)
print("UART conectado en /dev/ttyACM0")
#Lectura Temperatura
archivo = "temp.txt"

def leer_dato_temperatura(file_path):
    try:
        with open(file_path, "r") as archivo:
            linea = archivo.readline().strip()  # Leemos la primera línea y eliminamos espacios
            indicador_str, valor_str = linea.split(",")  # Separamos por la coma
            indicador = int(indicador_str)
            valor = float(valor_str)
            return indicador, valor
    except Exception as e:
        print("Error al leer el archivo:", e)
        return None, None


def read_interval():
    with open(text, "r") as file:
        interval = float(file.readline().strip())
        return max(0.1, interval)
        
# Inicializar PWM
pwm = GPIO.PWM(ENA, 1000)
pwm.start(0)
GPIO.output(IN1, GPIO.LOW)
GPIO.output(IN2, GPIO.HIGH)

# Función para medir distancia
def medir_distancia():
    GPIO.output(TRIG1, GPIO.LOW) 
    time.sleep(0.1)  
    GPIO.output(TRIG1, GPIO.HIGH) 
    time.sleep(0.00001)  
    GPIO.output(TRIG1, GPIO.LOW)  

    # Medir el tiempo de duración del pulso
    while GPIO.input(ECHO1) == GPIO.LOW:
        inicio = time.time() 

    while GPIO.input(ECHO1) == GPIO.HIGH:
        fin = time.time()  

    duracion = fin - inicio  # Duración del pulso
    distancia = duracion * 34300  # Calcular la distancia en cm
    return int(distancia)

def medir_distancia2():
    GPIO.output(TRIG2, GPIO.LOW) 
    time.sleep(0.1)  
    GPIO.output(TRIG2, GPIO.HIGH) 
    time.sleep(0.00001)  
    GPIO.output(TRIG2, GPIO.LOW)  

    while GPIO.input(ECHO2) == GPIO.LOW:
        inicio2 = time.time() 

    while GPIO.input(ECHO2) == GPIO.HIGH:
        fin2 = time.time()  

    duracion2 = fin2 - inicio2
    distancia2 = duracion2 * 34300
    return int(distancia2)

def enviar_temperatura():
    while enviar_temp:
        #temperatura = random.randint(-5, 45)
        indicador, temperatura2 = leer_dato_temperatura("temp.txt")
        #1=F,0=C
        if indicador==1:
            print(f"Temperatura generada: {temperatura2} °F")
        else:
            print(f"Temperatura generada: {temperatura2} °C")
        #print(f"Temperatura generada: {temperatura2} °C")
        if temperatura2 < 20:
            ser.write("C".encode('utf-8'))
            print("Comando enviado: C")
                
        else:
            ser.write("D".encode('utf-8'))
            print("Comando enviado: D")

        # Intentar leer respuesta de la TIVA después de enviar el comando
        respuesta = ''
        timeout = time.time() + 3
        while time.time() < timeout:
            if ser.in_waiting > 0:
                respuesta = ser.readline().decode('utf-8').strip()
                if respuesta == "Temporizador detenido después de 10 segundos":
                        correo()
                if respuesta:
                    print("Respuesta de la TIVA:", respuesta)
                    break
            time.sleep(0.1)

        if not respuesta:
            print("No se recibió respuesta de la TIVA")

        time.sleep(1.5)

#Parte del codigo para abrir 2 hilos al mismo tiempo(ejecutando el envio de temperatura en segundo plano)
temp_thread = threading.Thread(target=enviar_temperatura, daemon=True)
temp_thread.start()

# Programa principal
try:
    while True:
        distancia = medir_distancia()
        distancia2= medir_distancia2()
        print(f"Distancia medida: {distancia2} cm")
        if GPIO.input(button1) == GPIO.LOW:
            ser.write("A\n".encode('utf-8'))
            enviar_temp = False
            time.sleep(0.1)
            print("Boton")
            enviar_temp = True

            # Leer respuesta de la TIVA
            respuesta = ''
            timeout = time.time() + 2  # Timeout de 2 segundos
            while time.time() < timeout:
                if ser.in_waiting > 0:  # Verifica si hay datos en el buffer
                    respuesta = ser.readline().decode('utf-8').strip()
                    print("Respuesta de la TIVA:", respuesta)
                    
                    # Asegurarse de que la respuesta no esté vacía
                    if respuesta == "Temporizador detenido después de 10 segundos":
                        correo()
                    if respuesta:
                        break  # Rompe el bucle y hace la siguiente medición
                time.sleep(0.1)  # Esperar un poco antes de verificar nuevamente
            
            # Si no se recibe respuesta en el tiempo esperado
            if not respuesta:
                print("No se recibió respuesta de la TIVA")
        # Si la distancia es menor a 8 cm, enviar el comando 'A'
        if distancia < 8:
            pwm.ChangeDutyCycle(100)
            enviar_temp = False
            time.sleep(0.1)
            enviar_temp = True    
        else:
            pwm.ChangeDutyCycle(0)
        if distancia2 < 8:
            ser.write("B\n".encode('utf-8'))
            # Leer respuesta de la TIVA
            respuesta = ''
            timeout = time.time() + 2  # Timeout de 2 segundos
            while time.time() < timeout:
                if ser.in_waiting > 0:  # Verifica si hay datos en el buffer
                    respuesta = ser.readline().decode('utf-8').strip()
                    print("Respuesta de la TIVA:", respuesta)
                    
                    if respuesta == "Temporizador detenido después de 10 segundos":
                        correo()# Asegurarse de que la respuesta no esté vacía
                    if respuesta:
                        break
                time.sleep(0.1)  # Esperar un poco antes de verificar nuevamente
            
            # Si no se recibe respuesta en el tiempo esperado
            if not respuesta:
                print("No se recibió respuesta de la TIVA")
        if read_interval() == 1:
            ser.write("F\n".encode('utf-8'))
            correo2()
            print("Apagar alarma: F")
        if read_interval() == 0:
            correo()
        

except KeyboardInterrupt:
    print("Medición interrumpida por el usuario")

finally:
    GPIO.cleanup()  # Limpiar los pines GPIO
    ser.close()  # Cerrar la conexión serial
