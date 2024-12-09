#include <WiFi.h>
#include <HTTPClient.h>

// Credenciales del modo cliente (red Wi-Fi con internet)
const char* ssid_sta = "Guya"; // Cambia por el nombre de tu red Wi-Fi
const char* password_sta = "73786124j"; // Cambia por la contraseña de tu red Wi-Fi

// Credenciales del punto de acceso (AP)
const char* ssid_ap = "ESP32_Asistencia";
const char* password_ap = "12345678";

// Dirección del script de Google Apps
const char* serverName = "https://script.google.com/macros/s/AKfycbzQbgpnloc9fT9xZZGcvUbsxUixLMT_guUxpp-tSy82-khXrDyj_dnfOBkDknruYkT3dg/exec";

// Configuración del servidor web
WiFiServer server(80);

// Página HTML para el formulario
const char* htmlPage = R"=====( 
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Registro de Asistencia</title>
    <style>
        body {
            margin: 0;
            font-family: Arial, sans-serif;
            background-color: #d7f3f0;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
        }
        .container {
            background: white;
            width: 90%;
            max-width: 400px;
            border-radius: 10px;
            overflow: hidden;
            box-shadow: 0px 4px 10px rgba(0, 0, 0, 0.2);
        }
        .header {
            position: relative;
            height: 150px;
            background-color: #00695c;
        }
        .header img {
            width: 100%;
            height: 100%;
            object-fit: cover;
        }
        .form-container {
            padding: 20px;
        }
        .form-container h2 {
            margin: 0;
            font-size: 24px;
            color: #333;
        }
        .form-container p {
            margin: 10px 0;
            font-size: 14px;
            color: #777;
        }
        .form-container input {
            width: 100%;
            padding: 10px;
            margin: 10px 0;
            border: 1px solid #ccc;
            border-radius: 5px;
            font-size: 16px;
        }
        .form-container button {
            width: 100%;
            padding: 10px;
            background-color: #00695c;
            color: white;
            font-size: 16px;
            border: none;
            border-radius: 5px;
            cursor: pointer;
        }
        .form-container button:hover {
            background-color: #004d40;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <img src="link de la imagen"> <!-- Reemplaza con el enlace de tu imagen -->
        </div>
        <div class="form-container">
            <h2>Registro de Asistencia</h2>
            <p>Complete los campos para registrar su asistencia:</p>
            <form action="/send" method="GET">
                <input type="text" name="nombre" placeholder="Nombre Completo" required>
                <input type="text" name="codigo" placeholder="Código de Estudiante" required>
                <input type="email" name="correo" placeholder="Correo Institucional" required>
                <button type="submit">Enviar</button>
            </form>
        </div>
    </div>
</body>
</html>
)=====";

void setup() {
  Serial.begin(115200);

  // Iniciar el modo punto de acceso (AP)
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("Punto de acceso iniciado");
  Serial.print("Nombre de la red: ");
  Serial.println(ssid_ap);
  Serial.print("Contraseña: ");
  Serial.println(password_ap);
  Serial.print("Dirección IP AP: ");
  Serial.println(WiFi.softAPIP());

  // Conectar al Wi-Fi como cliente
  WiFi.begin(ssid_sta, password_sta);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  Serial.println("Conexión WiFi establecida");
  Serial.print("Dirección IP STA: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = client.readStringUntil('\r');
    client.flush();

    if (request.indexOf("GET / ") >= 0 || request.indexOf("GET / HTTP/1.1") >= 0) {
      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-type:text/html\r\n");
      client.print("Connection: close\r\n\r\n");
      client.print(htmlPage);
    } else if (request.indexOf("GET /send") >= 0) {
      String nombre = getValue(request, "nombre");
      String codigo = getValue(request, "codigo");
      String correo = getValue(request, "correo");

      Serial.println("Datos recibidos:");
      Serial.println("Nombre: " + nombre);
      Serial.println("Código: " + codigo);
      Serial.println("Correo: " + correo);

      sendToGoogleSheet(nombre, codigo, correo);

      client.print("HTTP/1.1 200 OK\r\n");
      client.print("Content-type:text/plain\r\n");
      client.print("Connection: close\r\n\r\n");
      client.print("Datos enviados correctamente");
    }

    client.stop();
  }
}

String getValue(String data, String name) {
  int startIndex = data.indexOf(name + "=");
  if (startIndex == -1) return "";
  int endIndex = data.indexOf("&", startIndex);
  if (endIndex == -1) endIndex = data.indexOf(" ", startIndex);
  String value = data.substring(startIndex + name.length() + 1, endIndex);
  value.replace("+", " ");
  return value;
}

void sendToGoogleSheet(String nombre, String codigo, String correo) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String url = String(serverName) + "?nombre=" + urlEncode(nombre) + "&codigo=" + urlEncode(codigo) + "&correo=" + urlEncode(correo);

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Respuesta de Google Sheets: " + response);
    } else {
      Serial.println("Error al enviar datos: " + String(httpResponseCode));
    }

    http.end();
  } else {
    Serial.println("No conectado a WiFi");
  }
}

String urlEncode(String str) {
  String encoded = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (c == ' ') {
      encoded += '+';
    } else if (isalnum(c)) {
      encoded += c;
    } else {
      encoded += '%';
      encoded += String(c, HEX);
    }
  }
  return encoded;
}



