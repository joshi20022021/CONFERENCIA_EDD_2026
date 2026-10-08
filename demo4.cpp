
// Bibliotecas

#include <iostream>   // std::cout, std::cin: entrada/salida por consola.
#include <iomanip>    // std::hex, std::setw, std::setfill: formatear hash.
#include <sstream>    // std::ostringstream: construir el hash hexadecimal.
#include <string>     // std::string: manejo de cadenas.
#include <vector>     // std::vector: almacenar bloques de la cadena.
#include <ctime>      // std::time: obtener marca de tiempo.
#include <cstdint>    // uint64_t: entero sin signo de 64 bits.
#include <stdexcept>  // std::runtime_error: lanzar excepciones.
#include <fstream>    // std::ofstream: escribir el archivo .dot.
#include <cstdlib>    // std::system: ejecutar el comando de Graphviz.

#include <openssl/evp.h>
// OpenSSL EVP: API criptográfica de alto nivel para SHA-256.

// Función SHA-256. Recibe un std::string y devuelve su hash SHA-256 en formato hexadecimal (64 caracteres).
// Aca se comprueban los valores de retorno de cada llamada a OpenSSL para detectar errores.
std::string sha256(const std::string& data)
{
    // Creamos el contexto del digest.
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    // Si no se pudo reservar memoria para el contexto, error.
    if (!context)
    {
        throw std::runtime_error(
            "No se pudo crear el contexto SHA-256"
        );
    }

    // Buffer donde se guardará el hash binario.
    unsigned char digest[EVP_MAX_MD_SIZE];

    // Longitud real del hash.
    unsigned int length = 0;

    // Inicializamos SHA-256.
    if (EVP_DigestInit_ex(context, EVP_sha256(), nullptr) != 1)
    {
        EVP_MD_CTX_free(context);
        throw std::runtime_error(
            "Error inicializando SHA-256"
        );
    }

    // Introducimos los datos.
    if (EVP_DigestUpdate(context, data.data(), data.size()) != 1)
    {
        EVP_MD_CTX_free(context);
        throw std::runtime_error(
            "Error procesando los datos"
        );
    }

    // Finalizamos y obtenemos el hash binario.
    if (EVP_DigestFinal_ex(context, digest, &length) != 1)
    {
        EVP_MD_CTX_free(context);
        throw std::runtime_error(
            "Error finalizando SHA-256"
        );
    }

    // Liberamos el contexto.
    EVP_MD_CTX_free(context);

    // Convertimos el hash binario a hexadecimal.
    std::ostringstream result;

    for (unsigned int i = 0; i < length; i++)
    {
        result
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<int>(digest[i]);
    }

    return result.str();
}


// CLASE BLOCK : Representa un bloque individual dentro de la blockchain.
class Block
{
public:
    // Posición del bloque en la cadena.
    size_t index;

    // Marca de tiempo de creación.
    long long timestamp;

    // Datos almacenados (por ejemplo, transacción).
    std::string data;

    // Hash del bloque anterior.
    std::string previousHash;

    // Número ajustable durante la prueba de trabajo.
    uint64_t nonce;

    // Hash actual del bloque.
    std::string hash;

    // Constructor. Inicializa los campos y calcula un hash inicial. 
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
        // Hash inicial con nonce = 0.
        hash = calculateHash();
    }

    // Calcula el hash concatenando los campos importantes. Es const porque no modifica el objeto.
    std::string calculateHash() const
    {
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

    // Proof of Work.  Ajusta el nonce hasta que el hash comience por "difficulty" ceros. 
    void mineBlock(unsigned int difficulty)
    {
        // Target: cadena de "difficulty" ceros.
        std::string target(difficulty, '0');

        // Reiniciamos el nonce.
        nonce = 0;

        while (true)
        {
            // Calculamos el hash con el nonce actual.
            hash = calculateHash();

            // Si cumple la condición, terminamos.
            if (
                hash.substr(0, difficulty)
                == target
            )
            {
                break;
            }

            // Probamos el siguiente nonce.
            nonce++;
        }
    }
};

// CLASE BLOCKCHAIN: Administra la cadena de bloques y permite validarla, alterarla y generar reportes visuales con Graphviz.
class Blockchain
{
private:
    // Vector de bloques.
    std::vector<Block> chain;

    // Dificultad de minado.
    unsigned int difficulty;

    // Acorta un hash para que el reporte Graphviz sea legible. "0000abcd1234...ef901234" -> "0000abcd12...ef901234"
    std::string shortHash(const std::string& value) const
    {
        if (value.length() <= 20)
        {
            return value;
        }

        return value.substr(0, 10)
            + "..."
            + value.substr(value.length() - 10);
    }

    // Escapa caracteres especiales para que el texto pueda colocarse de forma segura dentro de una etiqueta DOT.
    std::string escapeDot(const std::string& text) const
    {
        std::string escaped;

        for (char c : text)
        {
            switch (c)
            {
                case '\\':
                    escaped += "\\\\";
                    break;

                case '"':
                    escaped += "\\\"";
                    break;

                case '\n':
                    escaped += "\\n";
                    break;

                case '\r':
                    break;

                default:
                    escaped += c;
                    break;
            }
        }

        return escaped;
    }

public:
    // Constructor.
    // Crea automáticamente el Genesis Block.
    explicit Blockchain(unsigned int difficulty)
        : difficulty(difficulty)
    {
        // Bloque génesis: índice 0, previousHash "0".
        Block genesis(
            0,
            "Genesis Block",
            "0"
        );

        // Lo minamos con la dificultad actual.
        genesis.mineBlock(difficulty);

        // Lo añadimos a la cadena.
        chain.push_back(genesis);
    }

    // Agrega un nuevo bloque a la cadena.
    void addBlock(const std::string& data)
    {
        // El hash del último bloque será el previousHash
        // del nuevo bloque.
        Block block(
            chain.size(),
            data,
            chain.back().hash
        );

        // Lo minamos.
        block.mineBlock(difficulty);

        // Lo añadimos.
        chain.push_back(block);
    }

    // Simula una alteración/corrupción.
    // Importante: solo modifica DATA, pero deja el hash
    // almacenado intacto. Eso provoca que la validación
    // detecte el error.
    void tamperBlock(
        size_t index,
        const std::string& newData
    )
    {
        if (index < chain.size())
        {
            chain[index].data = newData;
        }
    }

    // Valida toda la blockchain.
    // Comprueba tres cosas por cada bloque:
    //   1) Integridad: hash almacenado == hash recalculado.
    //   2) Proof of Work: el hash cumple la dificultad.
    //   3) Enlace: previousHash apunta al hash anterior.
    bool isValid() const
    {
        std::string target(difficulty, '0');

        for (size_t i = 0; i < chain.size(); i++)
        {
            const Block& current = chain[i];

            // 1. Integridad.
            std::string recalculatedHash =
                current.calculateHash();

            if (current.hash != recalculatedHash)
            {
                std::cout
                    << "\nERROR DE INTEGRIDAD\n"
                    << "El bloque "
                    << i
                    << " fue modificado o corrompido.\n\n"
                    << "Hash almacenado:\n"
                    << current.hash
                    << "\n\n"
                    << "Hash recalculado:\n"
                    << recalculatedHash
                    << "\n";

                return false;
            }

            // 2. Proof of Work.
            if (
                current.hash.substr(0, difficulty)
                != target
            )
            {
                std::cout
                    << "\nERROR DE PROOF OF WORK\n"
                    << "El bloque "
                    << i
                    << " ya no cumple la dificultad.\n";

                return false;
            }

            // 3. Enlace.
            if (i > 0)
            {
                if (
                    current.previousHash
                    != chain[i - 1].hash
                )
                {
                    std::cout
                        << "\nERROR DE ENLACE\n"
                        << "El bloque "
                        << i
                        << " no apunta correctamente al bloque anterior.\n";

                    return false;
                }
            }
        }

        return true;
    }

    // Muestra un bloque específico en consola.
    void printBlock(size_t index) const
    {
        // Ignoramos índices fuera de rango.
        if (index >= chain.size())
        {
            return;
        }

        const Block& block = chain[index];

        std::cout
            << "\nBLOQUE "
            << block.index
            << '\n'
            << "Data: "
            << block.data
            << '\n'
            << "Nonce: "
            << block.nonce
            << '\n'
            << "PreviousHash:\n"
            << block.previousHash
            << '\n'
            << "Hash:\n"
            << block.hash
            << '\n';
    }

    // GENERAR REPORTE GRAPHVIZ Cada bloque incluye el resultado de las tres verificaciones: Integridad, Proof of Work y Enlace.
    //
    // Colores: Verde = bloque válido Rojo  = bloque inválido / modificado / corrupto
    void generateGraphvizReport(
        const std::string& dotFile,
        const std::string& imageFile
    ) const
    {
        // Abrimos el archivo .dot para escritura.
        std::ofstream file(dotFile);

        if (!file.is_open())
        {
            std::cout
                << "No se pudo crear el archivo Graphviz: "
                << dotFile
                << '\n';

            return;
        }

        // Grafico DOT
        file << "digraph Blockchain {\n";
        file << "rankdir=LR;\n";
        file << "graph [bgcolor=\"#14001f\", pad=\"0.5\", nodesep=\"0.6\", ranksep=\"0.8\"];\n";
        file << "node [shape=box, style=\"rounded,filled\", fontname=\"Arial\", fontcolor=\"white\", color=\"white\"];\n";
        file << "edge [fontname=\"Arial\", fontcolor=\"white\", color=\"white\", penwidth=2];\n\n";

        std::string target(difficulty, '0');

        // Bandera global de validez.
        bool chainValid = true;

        // Crear un nodo por cada bloque
        for (size_t i = 0; i < chain.size(); i++)
        {
            const Block& current = chain[i];

            // Recalculamos el hash.
            std::string recalculatedHash =
                current.calculateHash();

            // Verificamos integridad.
            bool hashValid =
                current.hash == recalculatedHash;

            // Verificamos Proof of Work.
            bool powValid =
                current.hash.substr(0, difficulty)
                == target;

            // Verificamos enlace (excepto el génesis).
            bool linkValid = true;

            if (i > 0)
            {
                linkValid =
                    current.previousHash
                    == chain[i - 1].hash;
            }

            // Un bloque es válido si pasa las tres pruebas.
            bool blockValid =
                hashValid
                && powValid
                && linkValid;

            // Si algún bloque falla, la cadena entera no es válida.
            if (!blockValid)
            {
                chainValid = false;
            }

            // verde si es válido, rojo si no.
            std::string blockColor =
                blockValid
                ? "#2E7D32"
                : "#C62828";

            // Empezamos a construir el nodo.
            file
                << "B"
                << i
                << " [fillcolor=\""
                << blockColor
                << "\", label=\"";

            // Título del bloque.
            if (i == 0)
            {
                file << "GENESIS BLOCK";
            }
            else
            {
                file
                    << "BLOQUE "
                    << i;
            }

            file << "\\n\\n";

            // Data (escapada).
            file
                << "Data: "
                << escapeDot(current.data)
                << "\\n";

            // Nonce.
            file
                << "Nonce: "
                << current.nonce
                << "\\n\\n";

            // Hash almacenado (abreviado).
            file
                << "Hash almacenado:\\n"
                << shortHash(current.hash)
                << "\\n";

            // Hash recalculado (abreviado).
            file
                << "Hash recalculado:\\n"
                << shortHash(recalculatedHash)
                << "\\n\\n";

            // Resultado de cada verificación.
            file
                << "Integridad: "
                << (hashValid ? "OK" : "ERROR")
                << "\\n";

            file
                << "Proof of Work: "
                << (powValid ? "OK" : "ERROR")
                << "\\n";

            file
                << "Enlace: "
                << (linkValid ? "OK" : "ERROR");

            // Cerramos el nodo.
            file << "\"];\n";
        }

        // Crear las flechas entre bloques
        for (size_t i = 1; i < chain.size(); i++)
        {
            // Comprobamos si el enlace es válido.
            bool linkValid =
                chain[i].previousHash
                == chain[i - 1].hash;

            // Verde si el enlace es correcto, rojo si no.
            std::string edgeColor =
                linkValid
                ? "#66BB6A"
                : "#EF5350";

            file
                << "B"
                << (i - 1)
                << " -> B"
                << i
                << " [color=\""
                << edgeColor
                << "\", label=\"previousHash\"];\n";
        }

        // Nodo con resultado general
        file
            << "resultado [shape=note, style=\"filled\", fontcolor=\"white\", fillcolor=\""
            << (chainValid ? "#1B5E20" : "#B71C1C")
            << "\", label=\""
            << (chainValid
                ? "BLOCKCHAIN VALIDA"
                : "BLOCKCHAIN INVALIDA\\nSe detecto modificacion o corrupcion")
            << "\"];\n";

        // Conectamos el último bloque con el resultado.
        if (!chain.empty())
        {
            file
                << "B"
                << (chain.size() - 1)
                << " -> resultado [style=dashed, color=\"white\"];\n";
        }

        // Cerramos el grafo.
        file << "}\n";
        file.close();

        // Ejecutar Graphviz automáticamente.
        std::string command =
            "dot -Tpng \""
            + dotFile
            + "\" -o \""
            + imageFile
            + "\"";

        int result =
            std::system(command.c_str());

        if (result == 0)
        {
            std::cout
                << "\nReporte Graphviz generado correctamente:\n"
                << " - "
                << dotFile
                << '\n'
                << " - "
                << imageFile
                << '\n';
        }
        else
        {
            // Si falla, probablemente Graphviz no está instalado
            // o "dot" no está en el PATH.
            std::cout
                << "\nSe genero el archivo DOT, pero no se pudo generar el PNG.\n"
                << "Verifica Graphviz con: dot -V\n";
        }
    }
};

// ============================================================
// PROGRAMA PRINCIPAL - DEMO 4
// ============================================================

int main()
{
    std::cout
        << "====================================\n"
        << " DEMO 4 - ALTERAR BLOCKCHAIN\n"
        << "====================================\n";


    // --------------------------------------------------------
    // 1. Construimos una blockchain valida.
    // --------------------------------------------------------
    //
    // Utilizamos dificultad 4:
    //
    // El hash debe comenzar con:
    //
    // 0000...
    //
    Blockchain blockchain(4);


    // --------------------------------------------------------
    // BLOQUE 1
    // --------------------------------------------------------
    //
    // Primera transaccion del ejemplo utilizado
    // en la presentacion.
    //
    blockchain.addBlock(
        "Jens envia Q100 a Alejandra"
    );


    // --------------------------------------------------------
    // BLOQUE 2
    // --------------------------------------------------------

    blockchain.addBlock(
        "Alejandra envia Q50 a Ana"
    );


    // --------------------------------------------------------
    // BLOQUE 3
    // --------------------------------------------------------

    blockchain.addBlock(
        "Ana envia Q25 a Luis"
    );


    // ========================================================
    // 2. VALIDAMOS ANTES DE LA MODIFICACION
    // ========================================================

    std::cout
        << "\n\n========== ANTES DEL ATAQUE ==========\n";


    // Mostramos específicamente el Bloque 1,
    // que será el bloque que vamos a modificar.
    blockchain.printBlock(1);


    // Validamos la blockchain completa.
    bool validBefore =
        blockchain.isValid();


    std::cout
        << "\nBlockchain valida: "
        << (validBefore ? "SI" : "NO")
        << '\n';


    // --------------------------------------------------------
    // REPORTE GRAPHVIZ ANTES DEL ATAQUE
    // --------------------------------------------------------

    blockchain.generateGraphvizReport(
        "reporte_antes.dot",
        "reporte_antes.png"
    );


    // Pausa para poder explicar el estado válido
    // antes de realizar la modificación.
    std::cout
        << "\nPresiona ENTER para modificar el bloque 1...";

    std::cin.get();


    // ========================================================
    // 3. SIMULAMOS UNA ALTERACION
    // ========================================================
    //
    // Original:
    //
    // Jens envia Q100 a Alejandra
    //
    // Modificado:
    //
    // Jens envia Q9000 a Alejandra
    //
    // IMPORTANTE:
    //
    // Solamente modificamos DATA.
    //
    // El hash almacenado y el nonce permanecen iguales.
    //
    // Por eso la validacion detectara posteriormente
    // que el hash almacenado ya no corresponde
    // con los datos actuales.
    //

    std::cout
        << "\n\n============== ATAQUE ==============\n"
        << "Se modifica el Bloque 1:\n"
        << "Jens -> Alejandra Q100\n"
        << "por:\n"
        << "Jens -> Alejandra Q9000\n";


    blockchain.tamperBlock(
        1,
        "Jens envia Q9000 a Alejandra"
    );


    // ========================================================
    // 4. VALIDAMOS DESPUES DE LA MODIFICACION
    // ========================================================

    std::cout
        << "\n\n========= DESPUES DEL ATAQUE =========\n";


    // Mostramos nuevamente el Bloque 1.
    //
    // Ahora veremos Q9000 en DATA,
    // pero el hash almacenado sigue siendo
    // el que correspondía a Q100.
    //
    blockchain.printBlock(1);


    // isValid() vuelve a calcular SHA-256
    // utilizando los datos actuales.
    //
    // El resultado será diferente del hash almacenado.
    //
    bool validAfter =
        blockchain.isValid();


    std::cout
        << "\nResultado final:\n"
        << "Blockchain valida: "
        << (validAfter ? "SI" : "NO")
        << '\n';


    // --------------------------------------------------------
    // REPORTE GRAPHVIZ DESPUES DEL ATAQUE
    // --------------------------------------------------------

    blockchain.generateGraphvizReport(
        "reporte_despues.dot",
        "reporte_despues.png"
    );


    // ========================================================
    // 5. MOSTRAMOS LOS REPORTES GENERADOS
    // ========================================================

    std::cout
        << "\n====================================\n"
        << " REPORTES GENERADOS\n"
        << "====================================\n"

        << "Antes del ataque:\n"
        << "  reporte_antes.dot\n"
        << "  reporte_antes.png\n\n"

        << "Despues del ataque:\n"
        << "  reporte_despues.dot\n"
        << "  reporte_despues.png\n";


    return 0;
}