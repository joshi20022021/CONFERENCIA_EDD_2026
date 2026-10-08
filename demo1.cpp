// bibliotecas 

#include <iostream>   // Para std::cout: mostrar datos por pantalla.
#include <iomanip>    // Para std::hex, std::setw y std::setfill: formatear hexadecimal.
#include <sstream>    // Para std::ostringstream: construir el hash como string.
#include <string>     // Para std::string: manejar cadenas de texto.
#include <stdexcept>  // Para std::runtime_error: lanzar errores.
#include <openssl/evp.h>
// OpenSSL EVP: API de alto nivel para operaciones criptográficas. Aquí se usa para calcular SHA-256.

// Función SHA-256
// Recibe un std::string y devuelve su hash SHA-256 en formato
// hexadecimal (una cadena de 64 caracteres).
std::string sha256(const std::string& data) {

    // Creamos un contexto de digest.
    // EVP_MD_CTX_new() reserva memoria para el contexto.
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    // Si no se pudo crear el contexto, lanzamos una excepción.
    if (!context) {
        throw std::runtime_error(
            "No se pudo crear el contexto SHA-256"
        );
    }

    // Buffer donde se almacenará el hash binario.
    // EVP_MAX_MD_SIZE es el tamaño máximo que puede tener un digest
    // en OpenSSL. Para SHA-256 serán 32 bytes.
    unsigned char digest[EVP_MAX_MD_SIZE];

    // Variable donde OpenSSL escribirá la longitud real del hash.
    unsigned int length = 0;

    // Inicializamos el contexto para usar SHA-256.
    // EVP_sha256() devuelve el algoritmo SHA-256.
    // El tercer parámetro es nullptr porque no usamos motor externo.
    EVP_DigestInit_ex(
        context,
        EVP_sha256(),
        nullptr
    );

    // Procesamos los datos de entrada.
    // data.data() devuelve el puntero a los bytes del string.
    // data.size() indica cuántos bytes debe procesar.
    EVP_DigestUpdate(
        context,
        data.data(),
        data.size()
    );

    // Finalizamos el cálculo.
    // digest recibe el hash binario.
    // length recibe cuántos bytes se escribieron.
    EVP_DigestFinal_ex(
        context,
        digest,
        &length
    );

    // Liberamos el contexto para evitar fugas de memoria.
    EVP_MD_CTX_free(context);

    // Convertimos el hash binario a una cadena hexadecimal. Usamos un ostringstream para ir concatenando.
    std::ostringstream result;

    // Recorremos cada byte del digest.
    for (unsigned int i = 0; i < length; i++) {

        // std::hex: muestra en base hexadecimal.
        // std::setw(2): cada byte debe ocupar al menos 2 dígitos.
        // std::setfill('0'): rellena con '0' si hace falta.
        // static_cast<int>: evita que se interprete como carácter.
        result
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    // Devolvemos el string hexadecimal resultante.
    return result.str();
}

// Función principal / DEMO 1
int main() {

    // Dos textos que solo difieren en la primera letra:
    std::string texto1 = "Estructura";
    std::string texto2 = "estructura";

    // Mostramos un encabezado por pantalla.
    std::cout << " DEMO 1 - SHA-256 Y EFECTO AVALANCHA\n";

    // Primer texto
    std::cout << "Texto 1:\n";
    std::cout << texto1 << "\n\n";

    std::cout << "SHA-256:\n";
    std::cout << sha256(texto1) << "\n\n";

    std::cout << "------------------------------------\n\n";

    // Segundo texto
    std::cout << "Texto 2:\n";
    std::cout << texto2 << "\n\n";

    std::cout << "SHA-256:\n";
    std::cout << sha256(texto2) << "\n\n";

    // un cambio mínimo en la entrada produce un hash completamente distinto en la salida.
    std::cout << "Solo cambiamos B por b.\n";
    std::cout << "Sin embargo, el hash cambio completamente.\n";

    return 0;
}