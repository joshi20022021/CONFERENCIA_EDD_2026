// Bibliotecas

#include <iostream>   // std::cout, std::cin: entrada/salida por consola.
#include <iomanip>    // std::hex, std::setw, std::setfill: formatear el hash.
#include <sstream>    // std::ostringstream: construir el hash hexadecimal.
#include <string>     // std::string: manejo de cadenas.
#include <vector>     // std::vector: almacenar los bloques de la cadena.
#include <ctime>      // std::time: obtener la marca de tiempo.
#include <cstdint>    // uint64_t: entero sin signo de 64 bits.
#include <stdexcept>  // std::runtime_error: lanzar excepciones.

#include <openssl/evp.h>
// OpenSSL EVP: API criptográfica de alto nivel. Se usa para calcular SHA-256.

// Función SHA-256. Recibe un std::string y devuelve su hash SHA-256 en formato hexadecimal (64 caracteres).

std::string sha256(const std::string& data) {

    // Creamos un contexto de digest.
    EVP_MD_CTX* context =
        EVP_MD_CTX_new();

    // Si no se pudo reservar memoria para el contexto, error.
    if (!context) {
        throw std::runtime_error(
            "No se pudo crear el contexto SHA-256"
        );
    }

    // Buffer donde se almacenará el hash binario.
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

    // Liberamos el contexto para evitar fugas de memoria.
    EVP_MD_CTX_free(context);

    // Convertimos el hash binario a hexadecimal.
    std::ostringstream result;

    for (unsigned int i = 0; i < length; i++) {

        // std::hex: base hexadecimal.
        // std::setw(2): mínimo 2 dígitos por byte.
        // std::setfill('0'): rellena con ceros a la izquierda.
        // static_cast<int>: evita que se interprete como carácter.
        result
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return result.str();
}

// CLASE BLOCK: Representa un bloque individual dentro de la blockchain.
class Block {

public:

    // Índice o posición del bloque en la cadena.
    size_t index;

    // Marca de tiempo de creación del bloque.
    long long timestamp;

    // Datos que almacena el bloque (por ejemplo, una transacción).
    std::string data;

    // Hash del bloque anterior.
    std::string previousHash;

    // Número que se ajusta durante la prueba de trabajo.
    uint64_t nonce;

    // Hash actual del bloque.
    std::string hash;

    // Constructor.
    // Inicializa los campos y calcula el hash inicial con nonce = 0.
    Block(
        size_t index,
        const std::string& data,
        const std::string& previousHash
    )
        : index(index),
          timestamp(
              static_cast<long long>(
                  std::time(nullptr)
              )
          ),
          data(data),
          previousHash(previousHash),
          nonce(0)
    {
        // Calculamos el hash inicial antes de minar.
        hash =
            calculateHash();
    }

    // Calcula la huella del bloque. Concatena todos los campos relevantes y aplica SHA-256. Es const porque no modifica el objeto.
    std::string calculateHash() const {

        std::string content =

            std::to_string(index)
            + "|"

            + std::to_string(timestamp)
            + "|"

            + data
            + "|"

            + previousHash
            + "|"

            + std::to_string(nonce);

        return sha256(content);
    }
    // Proof of Work. Incrementa el nonce hasta que el hash empiece por "difficulty" ceros.
    void mineBlock(
        unsigned int difficulty
    ) {

        // Target: cadena formada por "difficulty" ceros.
        std::string target(
            difficulty,
            '0'
        );

        // Contador de intentos adicionales.
        uint64_t attempts = 0;

        // Mientras el hash no cumpla la condición.
        while (
            hash.substr(
                0,
                difficulty
            )
            != target
        ) {

            // Probamos el siguiente nonce.
            nonce++;

            // Contamos el intento.
            attempts++;

            // Recalculamos el hash con el nuevo nonce.
            hash =
                calculateHash();
        }

        // Mostramos información del bloque minado.
        std::cout
            << "\nBloque "
            << index
            << " minado\n";

        std::cout
            << "Nonce: "
            << nonce
            << '\n';

        std::cout
            << "Intentos: "
            << attempts
            << '\n';

        std::cout
            << "Hash:\n"
            << hash
            << "\n";
    }
};

// CLASE BLOCKCHAIN
// Administra una cadena de bloques.
class Blockchain {

private:

    // Vector que almacena todos los bloques.
    std::vector<Block> chain;

    // Dificultad de minado (número de ceros iniciales).
    unsigned int difficulty;

public:

    // Constructor: Crea la blockchain y el bloque génesis.
    explicit Blockchain(
        unsigned int difficulty
    )
        : difficulty(difficulty)
    {
        std::cout
            << "\nCreando Genesis Block...\n";

        // El bloque génesis tiene índice 0, datos fijos
        // y previousHash "0".
        Block genesis(
            0,
            "Genesis Block",
            "0"
        );

        // Se mina el bloque génesis.
        genesis.mineBlock(
            difficulty
        );

        // Se añade a la cadena.
        chain.push_back(
            genesis
        );
    }

    // Agregar un nuevo bloque a la cadena.
    void addBlock(
        const std::string& data
    ) {

        // El hash del último bloque será el previousHash
        // del nuevo bloque.
        std::string previousHash =
            chain.back().hash;

        // Creamos el bloque con el índice correspondiente.
        Block block(
            chain.size(),
            data,
            previousHash
        );

        // Lo minamos con la dificultad actual.
        block.mineBlock(
            difficulty
        );

        // Lo añadimos a la cadena.
        chain.push_back(
            block
        );
    }

    // Mostrar todos los bloques por pantalla.
    void print() const {

        std::cout << " BLOCKCHAIN COMPLETA\n";

        // Recorremos cada bloque de la cadena.
        for (
            const Block& block :
            chain
        ) {

            std::cout
                << "\nBLOQUE "
                << block.index
                << "\n";

            std::cout
                << "Data:\n"
                << block.data
                << "\n\n";

            std::cout
                << "PreviousHash:\n"
                << block.previousHash
                << "\n\n";

            std::cout
                << "Nonce:\n"
                << block.nonce
                << "\n\n";

            std::cout
                << "Hash:\n"
                << block.hash
                << "\n";

            std::cout
                << "------------------------------------\n";
        }
    }

    // Validar integridad y enlaces de la blockchain. Devuelve true si todo es correcto.
    bool isValid() const {

        // Target según la dificultad.
        std::string target(
            difficulty,
            '0'
        );

        // Recorremos todos los bloques.
        for (
            size_t i = 0;
            i < chain.size();
            i++
        ) {

            const Block& current =
                chain[i];

            // 1. Integridad: El hash guardado debe coincidir con el recalculado.
            if (
                current.hash
                !=
                current.calculateHash()
            ) {
                return false;
            }

            // 2. Proof of Work: El hash debe empezar por la cantidad de ceros indicada por la dificultad.
            if (
                current.hash.substr(
                    0,
                    difficulty
                )
                != target
            ) {
                return false;
            }

            // 3. Enlace: El previousHash debe coincidir con el hash del bloque anterior. Se omite para el génesis.
            if (i > 0) {

                if (
                    current.previousHash
                    !=
                    chain[i - 1].hash
                ) {
                    return false;
                }
            }
        }

        // Si pasó todas las comprobaciones, es válida.
        return true;
    }
};

// DEMO 3 CONSTRUIR UNA BLOCKCHAIN COMPLETA

int main() {

    std::cout
        << "====================================\n";

    std::cout
        << " DEMO 3 - CONSTRUIR BLOCKCHAIN\n";

    std::cout
        << "====================================\n";


    // 1. CREAMOS LA BLOCKCHAIN Utilizamos dificultad 4.
    Blockchain blockchain(
        4
    );


    // 2. AGREGAMOS BLOQUE 1 Primera transacción 

    std::cout
        << "\nAgregando Bloque 1...\n";


    blockchain.addBlock(
        "Jens envia Q100 a Alejandra"
    );


    // 3. AGREGAMOS BLOQUE 2: Segunda transacción

    std::cout
        << "\nAgregando Bloque 2...\n";


    blockchain.addBlock(
        "Alejandra envia Q50 a Ana"
    );


    // 4. AGREGAMOS BLOQUE 3: Tercera transacción:

    std::cout
        << "\nAgregando Bloque 3...\n";


    blockchain.addBlock(
        "Ana envia Q25 a Luis"
    );


    // 5. MOSTRAMOS TODA LA BLOCKCHAIN

    blockchain.print();

    // 6. VALIDAMOS LA BLOCKCHAIN
    // 1. Integridad
    //    hash almacenado == hash recalculado
    // 2. Proof of Work
    //    hash comienza con "0000"
    // 3. Enlace
    //    previousHash == hash del bloque anterior

    bool valid =
        blockchain.isValid();


    std::cout
        << "\nBlockchain valida: "
        << (
            valid
            ? "SI"
            : "NO"
        )
        << '\n';


    return 0;
}