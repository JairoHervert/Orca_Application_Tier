# Orca — Application Tier (Backend)

Este repositorio contiene la **capa de aplicación (backend)** del sistema **Orca**. Su responsabilidad principal es **exponer la API (HTTPS)** y **orquestar la lógica de negocio** para proteger repositorios/archivos de código, controlando el acceso y trabajando con el almacenamiento local y la base de datos.

---

## ¿Qué es Orca?

**Orca** es un sistema orientado a **proteger archivos de código fuente** (y, en general, contenido de repositorios) mediante mecanismos criptográficos, permitiendo:

- **Cifrar y descifrar repositorios** para proteger su contenido.
- **Delimitar el acceso** a un conjunto de **N desarrolladores** (control de acceso), de modo que solo usuarios autorizados puedan obtener llaves y/o acceder al contenido.
- **Administrar usuarios, roles y estatus**, así como flujos de enrolamiento y concesión de acceso.
- Proveer una API HTTPS para que los clientes puedan interactuar con el sistema de forma segura.

> **Nota**: Esta capa es el **backend (servidor)**. La interfaz de usuario vive en el **cliente CLI** (Presentation Tier).

---

## Cliente CLI (Presentation Tier)

Este backend trabaja a la par del cliente CLI (capa de presentación), disponible en:

- **Orca Presentation Tier (CLI)**: https://github.com/JairoHervert/Orca_Presentation_Tier

---

## Tecnologías y dependencias

- **C++17**
- **OpenSSL** (`-lssl -lcrypto`) para HTTPS/TLS y criptografía en la capa de transporte.
- **Crypto++** (`-lcryptopp`) para primitivas criptográficas utilizadas por el servidor.
- **Cliente MySQL/MariaDB** (headers y libs para conectar a la base de datos)
- **dotenv-cpp** (carga de variables de entorno desde `.env`):  
  https://github.com/laserpants/dotenv-cpp
- **nlohmann/json** (JSON):  
  https://json.nlohmann.me/

> **Nota**: Las dependencias son las mismas que las del frontend/cliente CLI.

---

## Compilación en Linux

### Requisitos

Asegúrate de contar con:
- `g++` con soporte **C++17**
- OpenSSL (headers + libs)
- Crypto++ (headers + libs)
- Cliente MySQL/MariaDB (headers + libs)
- SOCI (C++ Database Access Library)

En Debian/Ubuntu suele ser algo como:
```bash
sudo apt-get install build-essential libssl-dev libcrypto++-dev libmariadb-dev libsoci-dev libsoci-mysql4
```

(En Fedora o CentOS, los nombres de paquetes pueden variar: `mariadb-devel`, `soci-devel`, etc.)

### Comando de compilación

```bash
g++ src/main.cpp src/infrastructure/config/ConfigEnv.cpp src/interfaces/HttpApi.cpp \
  -I../third_party -I/usr/include/mysql \
  -o main \
  -lssl -lcrypto -lsoci_core -lsoci_mysql -lmariadb -lcryptopp
```

---

## Generar Certificados SSL

Antes de ejecutar el backend, necesitas generar los certificados SSL para HTTPS. El proyecto incluye un script para facilitar esta tarea:

```bash
bash src/scripts/generate-certs.sh
```

Este script generará automáticamente:
- `server.crt` - Certificado SSL
- `server.key` - Llave privada SSL

> **Nota**: Los certificados generados son autofirmados y solo deben usarse para desarrollo. En producción, utiliza certificados válidos emitidos por una Autoridad Certificadora (CA) confiable.

---

## Configuración

### Variables de entorno (.env)

Crea un archivo `.env` en la raíz del proyecto con el siguiente contenido:

```env
SERVER_HOST=0.0.0.0
SERVER_PORT=8443

SSL_CERT_PATH=server.crt
SSL_KEY_PATH=server.key

DB_HOST=0.0.0.0
DB_PORT=3306
DB_NAME=orca
DB_USER=root
DB_PASSWORD=2357

# Directorios de trabajo para repositorios
REPOSITORIES_ROOT=./repos_locales
REPOSITORIES_CIPHER=./repos_cipher
REPOSITORIES_WORKSPACE=./repos_work
```

#### Descripción de variables

**Servidor HTTPS:**
- `SERVER_HOST`: Host/IP donde escucha el servidor HTTPS (por defecto `0.0.0.0` para todas las interfaces)
- `SERVER_PORT`: Puerto donde escucha el servidor HTTPS (por defecto `8443`)

**Certificados SSL/TLS:**
- `SSL_CERT_PATH`: Ruta al certificado SSL (para HTTPS)
- `SSL_KEY_PATH`: Ruta a la llave privada SSL

**Base de datos:**
- `DB_HOST`: Host del servidor MySQL/MariaDB
- `DB_PORT`: Puerto del servidor de base de datos (por defecto `3306`)
- `DB_NAME`: Nombre de la base de datos
- `DB_USER`: Usuario de la base de datos
- `DB_PASSWORD`: Contraseña del usuario de la base de datos

**Repositorios (para desarrollo/testing local):**
- `REPOSITORIES_ROOT`: Directorio donde se almacenan repositorios en texto plano
- `REPOSITORIES_CIPHER`: Directorio donde se almacenan repositorios cifrados
- `REPOSITORIES_WORKSPACE`: Directorio de trabajo temporal para operaciones de repositorio

> **Importante**: En ambientes productivos, utiliza rutas absolutas y asegúrate de que los directorios tengan los permisos adecuados.

---

## Ejecución

1. **Genera los certificados SSL** (si aún no lo has hecho):
   ```bash
   bash src/scripts/generate-certs.sh
   ```

2. Verifica que exista tu archivo `.env` en la raíz del proyecto.

3. Asegúrate de que la base de datos esté accesible y que las rutas de repositorios existan (o que el programa las cree automáticamente).

4. Ejecuta el backend:

```bash
./main
```

El servidor debería iniciar y escuchar en `https://0.0.0.0:8443` (o el puerto que hayas configurado).

---

## Integración con el Cliente (Presentation Tier)

El cliente CLI (capa de presentación) se encarga de la interacción con el usuario y consume la API publicada por este backend. 

**Flujo típico:**
1. El usuario ejecuta comandos desde el CLI (ej: `./orca create_user`, `./orca encrypt_repo`)
2. El CLI envía peticiones HTTPS al backend
3. El backend procesa la lógica de negocio, interactúa con la base de datos y el sistema de archivos
4. El backend responde al CLI con los resultados
5. El CLI presenta los resultados al usuario

Para el flujo completo (cliente ↔ servidor), consulta:
- **Orca Presentation Tier**: https://github.com/JairoHervert/Orca_Presentation_Tier

---

## Estructura del proyecto

```
.
├── src/
│   ├── main.cpp                # Punto de entrada del backend
│   ├── infrastructure/
│   │   └── config/
│   │       └── ConfigEnv.cpp   # Configuración de variables de entorno
│   ├── interfaces/
│   │   └── HttpApi.cpp         # API REST / Endpoints HTTPS
│   ├── scripts/
│   │   └── generate-certs.sh   # Script para generar certificados SSL
│   └── ...
├── ../third_party/             # Librerías de terceros (headers)
├── build/                      # Artefactos de compilación (gitignored)
├── repos_locales/              # Repositorios en texto plano (gitignored)
├── repos_cipher/               # Repositorios cifrados (gitignored)
├── repos_work/                 # Workspace temporal (gitignored)
├── .env                        # Variables de entorno (gitignored)
├── server.crt                  # Certificado SSL (gitignored en producción)
├── server.key                  # Llave privada SSL (gitignored)
└── README.md
```

---

## Seguridad

### Consideraciones importantes:

- **NUNCA** subas el archivo `.env` a repositorios públicos
- **NUNCA** subas llaves privadas (`server.key`) a repositorios
- Utiliza variables de entorno o gestores de secretos en producción
- Asegúrate de que los directorios de repositorios cifrados tengan permisos restrictivos
- Utiliza certificados SSL válidos en producción (no autofirmados)
- Implementa rate limiting y validación de entrada en todos los endpoints
- Mantén actualizadas las dependencias de seguridad (OpenSSL, Crypto++, etc.)

---
