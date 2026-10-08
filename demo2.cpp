// Bibliotecas

#include <iostream>   // std::cout, std::cin: entrada/salida por consola.
#include <iomanip>    // std::hex, std::setw, std::setfill: formatear el hash.
#include <sstream>    // std::ostringstream: construir el hash hexadecimal.
#include <string>     // std::string: manejo de cadenas.
#include <cstdint>    // uint64_t: entero sin signo de 64 bits.
#include <chrono>     // Medición de tiempo de ejecución.
#include <stdexcept>  // std::runtime_error: lanzar excepciones.

#include <openssl/evp.h>
// OpenSSL EVP: API criptográfica de alto nivel. Se usa para calcular SHA-256.

// Función SHA-256 Calcula el hash SHA-256 del texto recibido y lo devuelve como una cadena hexadecimal de 64 caracteres.
std::string sha256(const std::string& data) {

    // Creamos un contexto de digest.
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    // Si no se pudo reservar memoria para el contexto, error.
    if (!context) {
        throw std::runtime_error(
            "No se pudo crear el contexto SHA-256"
        );
    }

    // Buffer donde se guardará el hash binario.
    // Para SHA-256 serán 32 bytes.
    unsigned char digest[EVP_MAX_MD_SIZE];

    // Longitud real del hash generado.
    unsigned int length = 0;

    // Inicializamos el contexto con el algoritmo SHA-256.
    EVP_DigestInit_ex(
        context,
        EVP_sha256(),
        nullptr
    );

    // Introducimos los datos en el contexto.
    EVP_DigestUpdate(
        context,
        data.data(),
        data.size()
    );

    // Finalizamos el cálculo y obtenemos el hash binario.
    EVP_DigestFinal_ex(
        context,
        digest,
        &length
    );

    // Liberamos el contexto.
    EVP_MD_CTX_free(context);

    // Convertimos el hash binario a hexadecimal.
    std::ostringstream result;

    for (unsigned int i = 0; i < length; i++) {

        // std::hex: base hexadecimal.
        // std::setw(2): mínimo 2 dígitos por byte.
        // std::setfill('0'): rellena con ceros a la izquierda.
        // static_cast<int>: evita que se trate como carácter.
        result
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return result.str();
}

// Función proofOfWork
// Simula una prueba de trabajo (Proof of Work) tipo blockchain.
// Recibe:
//   - data: texto base del bloque.
//   - difficulty: número de ceros hexadecimales iniciales
//     que debe tener el hash para considerarse válido.
//
// Busca un "nonce" tal que:
//   SHA256(data + "|difficulty=" + difficulty + "|nonce=" + nonce)
// empiece por "difficulty" ceros.
void proofOfWork(
    const std::string& data,
    unsigned int difficulty
) {

    // Si difficulty = 4, target = "0000".
    // El hash debe empezar por "0000".
    std::string target(
        difficulty,
        '0'
    );

    // nonce: número que vamos a ir probando.
    // attempts: contador de intentos realizados.
    uint64_t nonce = 0;
    uint64_t attempts = 0;

    // Aquí se guardará el hash encontrado.
    std::string hash;

    // Iniciamos la medición de tiempo.
    auto start =
        std::chrono::high_resolution_clock::now();

    // probamos nonces hasta encontrar un hash que cumpla la condición.
    while (true) {

        // Construimos el contenido que se va a hashear. Incluimos difficulty y nonce para que cada intento produzca una entrada distinta.
        std::string content =
            data +
            "|difficulty=" +
            std::to_string(difficulty) +
            "|nonce=" +
            std::to_string(nonce);

        // Calculamos SHA-256 del contenido.
        hash = sha256(content);

        // Contamos el intento.
        attempts++;

        // Comprobamos si los primeros "difficulty" caracteres
        // del hash coinciden con el target (todos ceros).
        if (
            hash.substr(0, difficulty)
            == target
        ) {
            // Si coinciden, hemos encontrado un nonce válido.
            break;
        }

        // Si no, probamos el siguiente nonce.
        nonce++;
    }

    // Detenemos la medición de tiempo.
    auto end =
        std::chrono::high_resolution_clock::now();

    // Calculamos el tiempo transcurrido en milisegundos.
    auto elapsed =
        std::chrono::duration_cast<
            std::chrono::milliseconds
        >(end - start);

    // Mostramos los resultados.
    std::cout
        << "\nDificultad: "
        << difficulty
        << '\n';

    std::cout
        << "Target educativo: "
        << target
        << '\n';

    std::cout
        << "Nonce encontrado: "
        << nonce
        << '\n';

    std::cout
        << "Intentos: "
        << attempts
        << '\n';

    std::cout
        << "Tiempo: "
        << elapsed.count()
        << " ms\n";

    std::cout
        << "Hash:\n"
        << hash
        << "\n";
}

int main() {

    // Texto base que simula el contenido de un bloque.
    std::string data =
        "Bloque de prueba";

    std::cout << " DEMO 2 - PROOF OF WORK\n";

    // Primera prueba: dificultad 2.
    // El hash debe empezar por "00".
    proofOfWork(
        data,
        2
    );

    // Esperamos a que el usuario pulse ENTER.
    std::cout
        << "\nPresiona ENTER para dificultad 3...";

    std::cin.get();

    // Segunda prueba: dificultad 3.
    // El hash debe empezar por "000".
    proofOfWork(
        data,
        3
    );

    // Esperamos de nuevo.
    std::cout
        << "\nPresiona ENTER para dificultad 4...";

    std::cin.get();

    // Tercera prueba: dificultad 4.
    // El hash debe empezar por "0000".
    proofOfWork(
        data,
        4
    );

    return 0;
}