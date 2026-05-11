#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <iomanip>
#include <ctime>
#include <sstream>
using namespace std;

// ============================================================
//  ESTRUCTURAS DE DATOS
// ============================================================

struct Libro {
    int id;
    string titulo;
    string autor;
    string genero;
    int paginas;
    double valoracion_promedio;
    int num_valoraciones;
    int popularidad; // contador global de prestamos
};

struct Arista {
    int destino;
    int peso; // veces que se pidio el libro destino despues del origen
};

struct Usuario {
    int id;
    string nombre;
    vector<int> historial; // ids de libros prestados en orden cronologico
};

// ============================================================
//  GRAFO DIRIGIDO Y PONDERADO (lista de adyacencia)
// ============================================================

class GrafoLibros {
public:
    unordered_map<int, Libro> libros;            // id -> Libro
    unordered_map<int, vector<Arista>> adyacencia; // id -> lista de aristas

    void agregarLibro(const Libro& l) {
        libros[l.id] = l;
        if (adyacencia.find(l.id) == adyacencia.end())
            adyacencia[l.id] = {};
    }

    // Registra que despues de leer 'origen' se leyo 'destino'
    void agregarOActualizarArista(int origen, int destino) {
        auto& vec = adyacencia[origen];
        for (auto& a : vec) {
            if (a.destino == destino) {
                a.peso++;
                return;
            }
        }
        Arista nueva; nueva.destino = destino; nueva.peso = 1;
        vec.push_back(nueva);
    }

    // Actualiza popularidad y valoracion de un libro
    void registrarPrestamo(int id) {
        if (libros.count(id)) libros[id].popularidad++;
    }

    void agregarValoracion(int id, double valor) {
        if (!libros.count(id)) return;
        Libro& l = libros[id];
        l.valoracion_promedio =
            (l.valoracion_promedio * l.num_valoraciones + valor)
            / (l.num_valoraciones + 1);
        l.num_valoraciones++;
    }

    // BFS hasta profundidad maxima, devuelve candidatos con puntaje
    // Puntaje = peso_arista * 3 + valoracion_promedio * 2 - distancia * 1.5
    vector<pair<int,double>> recomendar(int libroInicial,
                                        const unordered_set<int>& yaLeidos,
                                        int profMax = 3,
                                        int topN  = 5)
    {
        if (!libros.count(libroInicial)) return {};

        // BFS
        // cola: (id_libro, distancia)
        queue<pair<int,int>> cola;
        unordered_map<int,int>    visitado;   // id -> distancia
        unordered_map<int,double> puntaje;
        unordered_map<int,double> pesoBorde;  // mejor peso de arista hacia este nodo

        cola.push(make_pair(libroInicial, 0));
        visitado[libroInicial] = 0;

        while (!cola.empty()) {
            int actual = cola.front().first;
            int dist   = cola.front().second;
            cola.pop();

            if (dist >= profMax) continue;

            for (int ai = 0; ai < (int)adyacencia[actual].size(); ai++) {
                const Arista& a = adyacencia[actual][ai];
                int vecino = a.destino;
                if (!visitado.count(vecino)) {
                    visitado[vecino] = dist + 1;
                    pesoBorde[vecino] = a.peso;
                    cola.push(make_pair(vecino, dist + 1));
                } else {
                    pesoBorde[vecino] = max(pesoBorde[vecino], (double)a.peso);
                }
            }
        }

        // Calcular puntaje para cada candidato
        vector<pair<int,double>> candidatos;
        for (unordered_map<int,int>::iterator it = visitado.begin(); it != visitado.end(); ++it) {
            int id   = it->first;
            int dist = it->second;
            if (id == libroInicial) continue;
            if (yaLeidos.count(id))  continue;
            if (!libros.count(id))   continue;

            double p = pesoBorde[id] * 3.0
                     + libros[id].valoracion_promedio * 2.0
                     - dist * 1.5;
            candidatos.push_back(make_pair(id, p));
        }

        // Ordenar de mayor a menor puntaje
        sort(candidatos.begin(), candidatos.end(),
             [](const pair<int,double>& a, const pair<int,double>& b){
                 return a.second > b.second;
             });

        // Si faltan, completar con los mas populares no leidos
        if ((int)candidatos.size() < topN) {
            vector<pair<int,int>> populares;
            for (unordered_map<int,Libro>::iterator it = libros.begin(); it != libros.end(); ++it) {
                int id = it->first;
                if (yaLeidos.count(id)) continue;
                if (id == libroInicial) continue;
                bool yaEsta = false;
                for (int ci = 0; ci < (int)candidatos.size(); ci++)
                    if (candidatos[ci].first == id) { yaEsta = true; break; }
                if (!yaEsta)
                    populares.push_back(make_pair(id, it->second.popularidad));
            }
            sort(populares.begin(), populares.end(),
                 [](const pair<int,int>& a, const pair<int,int>& b){
                     return a.second > b.second;
                 });
            for (int pi = 0; pi < (int)populares.size(); pi++) {
                if ((int)candidatos.size() >= topN) break;
                candidatos.push_back(make_pair(populares[pi].first, (double)populares[pi].second * 0.5));
            }
        }

        if ((int)candidatos.size() > topN)
            candidatos.resize(topN);

        return candidatos;
    }
};

// ============================================================
//  BASE DE DATOS EN MEMORIA
// ============================================================

GrafoLibros grafo;
unordered_map<int, Usuario> usuarios; // id -> Usuario
int proximoIdUsuario = 1;

// ============================================================
//  DATOS INICIALES (50 libros)
// ============================================================

void cargarLibrosIniciales() {
    // Formato: id, titulo, autor, genero, paginas, valoracion, num_val, popularidad
    vector<Libro> datos = {
        {1,  "Cien anos de soledad",     "Gabriel Garcia Marquez", "Realismo magico", 417, 4.8, 120, 95},
        {2,  "El amor en los tiempos del colera", "Gabriel Garcia Marquez", "Romance",   348, 4.6,  98, 80},
        {3,  "Don Quijote de la Mancha", "Miguel de Cervantes",    "Clasico",         863, 4.7, 200, 110},
        {4,  "1984",                     "George Orwell",          "Distopia",        328, 4.9, 300, 200},
        {5,  "Un mundo feliz",           "Aldous Huxley",          "Distopia",        311, 4.5, 180, 150},
        {6,  "El senor de los anillos",  "J.R.R. Tolkien",         "Fantasia",        1178,4.9, 400, 320},
        {7,  "Harry Potter y la piedra filosofal", "J.K. Rowling", "Fantasia",        309, 4.8, 500, 400},
        {8,  "El nombre del viento",     "Patrick Rothfuss",       "Fantasia",        662, 4.7, 250, 210},
        {9,  "Dune",                     "Frank Herbert",          "Ciencia ficcion", 412, 4.8, 220, 185},
        {10, "Fundacion",                "Isaac Asimov",           "Ciencia ficcion", 244, 4.6, 190, 160},
        {11, "El marciano",              "Andy Weir",              "Ciencia ficcion", 369, 4.7, 175, 145},
        {12, "Fahrenheit 451",           "Ray Bradbury",           "Distopia",        158, 4.5, 165, 130},
        {13, "Matar un ruisenor",        "Harper Lee",             "Drama",           281, 4.8, 280, 230},
        {14, "El gran Gatsby",           "F. Scott Fitzgerald",    "Drama",           180, 4.4, 210, 170},
        {15, "Orgullo y prejuicio",      "Jane Austen",            "Romance",         432, 4.7, 310, 260},
        {16, "Cumbres borrascosas",      "Emily Bronte",           "Romance",         342, 4.3, 195, 155},
        {17, "Ana Karenina",             "Leon Tolstoi",           "Drama",           964, 4.6, 170, 140},
        {18, "Crimen y castigo",         "Fiodor Dostoievski",     "Drama",           551, 4.7, 200, 165},
        {19, "El proceso",               "Franz Kafka",            "Absurdismo",      160, 4.4, 145, 115},
        {20, "La metamorfosis",          "Franz Kafka",            "Absurdismo",       55, 4.5, 185, 150},
        {21, "En busca del tiempo perdido","Marcel Proust",        "Clasico",        3031, 4.3, 100,  75},
        {22, "Ulises",                   "James Joyce",            "Modernismo",      730, 4.2,  90,  65},
        {23, "El alquimista",            "Paulo Coelho",           "Filosofia",       197, 4.5, 380, 310},
        {24, "Siddhartha",               "Hermann Hesse",          "Filosofia",       152, 4.6, 270, 220},
        {25, "El lobo estepario",        "Hermann Hesse",          "Filosofia",       237, 4.4, 190, 155},
        {26, "Perfume",                  "Patrick Suskind",        "Suspenso",        263, 4.6, 230, 190},
        {27, "El codigo Da Vinci",       "Dan Brown",              "Thriller",        454, 4.1, 420, 350},
        {28, "Angeles y demonios",       "Dan Brown",              "Thriller",        620, 4.0, 300, 245},
        {29, "El silencio de los inocentes","Thomas Harris",       "Thriller",        338, 4.7, 260, 215},
        {30, "Sherlock Holmes: Estudio en escarlata","Arthur Conan Doyle","Misterio", 126, 4.6, 290, 240},
        {31, "Asesinato en el Orient Express","Agatha Christie",   "Misterio",        256, 4.7, 340, 280},
        {32, "Y no quedo ninguno",       "Agatha Christie",        "Misterio",        264, 4.8, 360, 300},
        {33, "La sombra del viento",     "Carlos Ruiz Zafon",      "Misterio",        567, 4.8, 310, 260},
        {34, "El juego del angel",       "Carlos Ruiz Zafon",      "Misterio",        598, 4.6, 240, 200},
        {35, "Rayuela",                  "Julio Cortazar",         "Experimental",    635, 4.5, 175, 140},
        {36, "Ficciones",               "Jorge Luis Borges",       "Cuentos",         174, 4.8, 265, 220},
        {37, "El Hobbit",               "J.R.R. Tolkien",          "Fantasia",        310, 4.8, 450, 370},
        {38, "Las cronicas de Narnia",  "C.S. Lewis",              "Fantasia",        767, 4.6, 320, 265},
        {39, "Eragon",                  "Christopher Paolini",     "Fantasia",        503, 4.3, 280, 230},
        {40, "El juego de Ender",       "Orson Scott Card",        "Ciencia ficcion", 226, 4.7, 300, 250},
        {41, "Neuromancer",             "William Gibson",          "Ciencia ficcion", 271, 4.5, 180, 145},
        {42, "La guerra de los mundos", "H.G. Wells",              "Ciencia ficcion", 192, 4.4, 200, 165},
        {43, "El retrato de Dorian Gray","Oscar Wilde",            "Clasico",         254, 4.7, 290, 240},
        {44, "Drácula",                 "Bram Stoker",             "Terror",          418, 4.6, 270, 220},
        {45, "Frankenstein",            "Mary Shelley",            "Terror",          288, 4.5, 250, 205},
        {46, "It",                      "Stephen King",            "Terror",         1138, 4.6, 330, 275},
        {47, "El resplandor",           "Stephen King",            "Terror",          447, 4.7, 310, 255},
        {48, "Sapiens",                 "Yuval Noah Harari",       "Historia",        443, 4.8, 400, 340},
        {49, "Homo Deus",               "Yuval Noah Harari",       "Historia",        450, 4.6, 280, 230},
        {50, "El gen egoista",          "Richard Dawkins",         "Ciencia",         360, 4.6, 220, 180}
    };

    for (auto& l : datos)
        grafo.agregarLibro(l);

    // Aristas iniciales (secuencias de lectura frecuentes)
    // Formato: agregarOActualizarArista(origen, destino) repetido segun peso
    auto conectar = [&](int o, int d, int veces){
        for (int i = 0; i < veces; i++)
            grafo.agregarOActualizarArista(o, d);
    };
    // Realismo magico -> Romance
    conectar(1, 2, 8); conectar(2, 15, 5); conectar(1, 35, 6);
    // Distopia
    conectar(4, 5, 12); conectar(5, 12, 9); conectar(12, 4, 7);
    conectar(4, 48, 6); conectar(5, 48, 5);
    // Fantasia
    conectar(6, 37, 15); conectar(37, 7, 14); conectar(7, 8, 10);
    conectar(8, 6, 8);   conectar(37, 38, 9); conectar(38, 39, 6);
    conectar(6, 8, 7);   conectar(7, 38, 8);  conectar(39, 8, 5);
    // Ciencia ficcion
    conectar(9, 10, 11); conectar(10, 40, 9); conectar(40, 11, 8);
    conectar(9, 40, 7);  conectar(10, 42, 6); conectar(11, 41, 5);
    conectar(41, 9, 4);  conectar(42, 10, 5);
    // Thriller / Misterio
    conectar(27, 28, 13); conectar(28, 27, 10); conectar(30, 31, 9);
    conectar(31, 32, 11); conectar(32, 33, 8); conectar(33, 34, 9);
    conectar(29, 31, 7);  conectar(27, 33, 6);
    // Terror
    conectar(44, 45, 8); conectar(45, 46, 7); conectar(46, 47, 10);
    conectar(47, 44, 6); conectar(26, 44, 5);
    // Filosofia
    conectar(23, 24, 9); conectar(24, 25, 7); conectar(25, 23, 5);
    conectar(23, 36, 6); conectar(24, 18, 5);
    // Drama clasico
    conectar(13, 14, 7); conectar(15, 16, 8); conectar(17, 18, 9);
    conectar(18, 19, 6); conectar(19, 20, 8); conectar(43, 14, 5);
    // Historia / Ciencia
    conectar(48, 49, 12); conectar(49, 50, 8); conectar(50, 48, 6);
}

// ============================================================
//  UTILIDADES DE PRESENTACION
// ============================================================

void limpiarPantalla() {
    cout << "\n" << string(60, '=') << "\n";
}

void pausar() {
    cout << "\nPresione Enter para continuar...";
    cin.ignore();
    cin.get();
}

void mostrarLibro(const Libro& l) {
    cout << left
         << setw(4)  << l.id
         << setw(42) << l.titulo.substr(0, 40)
         << setw(22) << l.autor.substr(0, 20)
         << setw(18) << l.genero
         << setw(6)  << l.paginas
         << fixed << setprecision(1) << setw(5) << l.valoracion_promedio
         << "\n";
}

void cabeceraCatalogo() {
    cout << left
         << setw(4)  << "ID"
         << setw(42) << "Titulo"
         << setw(22) << "Autor"
         << setw(18) << "Genero"
         << setw(6)  << "Pags"
         << setw(5)  << "Val."
         << "\n"
         << string(97, '-') << "\n";
}

// ============================================================
//  OPCIONES DE MENU
// ============================================================

void verCatalogo() {
    limpiarPantalla();
    cout << "            CATALOGO DE LIBROS\n";
    cout << string(60, '=') << "\n";
    cabeceraCatalogo();

    // 1. Extraer todos los libros a un vector temporal
    vector<Libro> listaLibros;
    for (auto it = grafo.libros.begin(); it != grafo.libros.end(); ++it) {
        listaLibros.push_back(it->second);
    }

    // 2. Ordenar el vector por ID de menor a mayor
    sort(listaLibros.begin(), listaLibros.end(), [](const Libro& a, const Libro& b) {
        return a.id < b.id;
    });

    // 3. Imprimir los libros ya ordenados
    for (int i = 0; i < (int)listaLibros.size(); i++) {
        mostrarLibro(listaLibros[i]);
    }

    pausar();
}

void registrarUsuario() {
    limpiarPantalla();
    cout << "        REGISTRO DE NUEVO USUARIO\n";
    cout << string(60, '=') << "\n";
    cin.ignore();
    cout << "Nombre del usuario: ";
    string nombre;
    getline(cin, nombre);
    Usuario u;
    u.id = proximoIdUsuario++;
    u.nombre = nombre;
    usuarios[u.id] = u;
    cout << "\nUsuario registrado con ID: " << u.id << "\n";
    pausar();
}

void registrarPrestamo() {
    limpiarPantalla();
    cout << "          REGISTRO DE PRESTAMO\n";
    cout << string(60, '=') << "\n";

    if (usuarios.empty()) {
        cout << "No hay usuarios registrados. Registre un usuario primero.\n";
        pausar();
        return;
    }

    int idUsuario;
    cout << "ID del usuario: ";
    cin >> idUsuario;
    if (!usuarios.count(idUsuario)) {
        cout << "Usuario no encontrado.\n";
        pausar();
        return;
    }

    int idLibro;
    cout << "ID del libro a prestar: ";
    cin >> idLibro;
    if (!grafo.libros.count(idLibro)) {
        cout << "Libro no encontrado.\n";
        pausar();
        return;
    }

    Usuario& u = usuarios[idUsuario];

    // Si hay un libro anterior en el historial, agregar arista
    if (!u.historial.empty()) {
        int anterior = u.historial.back();
        grafo.agregarOActualizarArista(anterior, idLibro);
    }
    u.historial.push_back(idLibro);
    grafo.registrarPrestamo(idLibro);

    double valoracion = 0;
    cout << "Ingrese valoracion del libro (1.0 - 5.0, 0 para omitir): ";
    cin >> valoracion;
    if (valoracion >= 1.0 && valoracion <= 5.0)
        grafo.agregarValoracion(idLibro, valoracion);

    cout << "\nPrestamo registrado: \""
         << grafo.libros[idLibro].titulo
         << "\" -> Usuario: " << u.nombre << "\n";
    pausar();
}

void solicitarRecomendaciones() {
    limpiarPantalla();
    cout << "        RECOMENDACIONES PERSONALIZADAS\n";
    cout << string(60, '=') << "\n";

    if (usuarios.empty()) {
        cout << "No hay usuarios registrados.\n";
        pausar();
        return;
    }

    int idUsuario;
    cout << "ID del usuario: ";
    cin >> idUsuario;
    if (!usuarios.count(idUsuario)) {
        cout << "Usuario no encontrado.\n";
        pausar();
        return;
    }

    Usuario& u = usuarios[idUsuario];

    int libroBase = -1;
    if (!u.historial.empty()) {
        libroBase = u.historial.back();
        cout << "Ultimo libro prestado: \""
             << grafo.libros[libroBase].titulo << "\" (ID " << libroBase << ")\n";
        cout << "Usar este libro como base? (1=Si / ingrese otro ID): ";
        int opcion;
        cin >> opcion;
        if (opcion != 1) libroBase = opcion;
    } else {
        cout << "El usuario no tiene historial. Ingrese ID de un libro base: ";
        cin >> libroBase;
    }

    if (!grafo.libros.count(libroBase)) {
        cout << "Libro no encontrado.\n";
        pausar();
        return;
    }

    unordered_set<int> yaLeidos(u.historial.begin(), u.historial.end());
    auto recomendaciones = grafo.recomendar(libroBase, yaLeidos);

    if (recomendaciones.empty()) {
        cout << "\nNo se encontraron recomendaciones.\n";
        pausar();
        return;
    }

    cout << "\n" << string(60, '-') << "\n";
    cout << "TOP 5 RECOMENDACIONES para \"" << u.nombre << "\":\n";
    cout << string(60, '-') << "\n";

    int rank = 1;
    for (int ri = 0; ri < (int)recomendaciones.size(); ri++) {
        int    id     = recomendaciones[ri].first;
        double puntaje = recomendaciones[ri].second;
        const Libro& l = grafo.libros[id];

        // Determinar motivo
        string motivo;
        // Verificar si hay arista directa o cercana desde libroBase
        bool porSecuencia = false;
        for (auto& a : grafo.adyacencia[libroBase])
            if (a.destino == id) { porSecuencia = true; break; }
        if (porSecuencia)
            motivo = "Secuencia de lectura previa";
        else if (l.genero == grafo.libros[libroBase].genero)
            motivo = "Coincidencia de genero literario";
        else if (l.autor == grafo.libros[libroBase].autor)
            motivo = "Mismo autor";
        else if (l.popularidad > 200)
            motivo = "Popular en el catalogo";
        else
            motivo = "Gustos literarios similares";

        cout << rank++ << ". " << l.titulo << "\n"
             << "   Autor:    " << l.autor  << "\n"
             << "   Genero:   " << l.genero << "\n"
             << "   Paginas:  " << l.paginas << "\n"
             << "   Valoracion: " << fixed << setprecision(1) << l.valoracion_promedio << "/5.0\n"
             << "   Motivo:   " << motivo << "\n"
             << "   Puntaje:  " << fixed << setprecision(2) << puntaje << "\n"
             << string(40, '.') << "\n";
    }
    pausar();
}

void verHistorialUsuario() {
    limpiarPantalla();
    cout << "         HISTORIAL DE USUARIO\n";
    cout << string(60, '=') << "\n";

    int idUsuario;
    cout << "ID del usuario: ";
    cin >> idUsuario;
    if (!usuarios.count(idUsuario)) {
        cout << "Usuario no encontrado.\n";
        pausar();
        return;
    }
    const Usuario& u = usuarios[idUsuario];
    cout << "Usuario: " << u.nombre << "\n";
    cout << "Libros prestados (" << u.historial.size() << "):\n\n";

    if (u.historial.empty()) {
        cout << "Sin prestamos registrados.\n";
    } else {
        for (int i = 0; i < (int)u.historial.size(); i++) {
            int id = u.historial[i];
            cout << " " << i+1 << ". [ID " << id << "] "
                 << grafo.libros[id].titulo << "\n";
        }
    }
    pausar();
}

void verUsuarios() {
    limpiarPantalla();
    cout << "         LISTA DE USUARIOS\n";
    cout << string(60, '=') << "\n";
    if (usuarios.empty()) {
        cout << "No hay usuarios registrados.\n";
    } else {
        cout << left << setw(6) << "ID" << setw(30) << "Nombre" << "Prestamos\n";
        cout << string(45, '-') << "\n";
        for (auto it = usuarios.begin(); it != usuarios.end(); ++it)
            cout << setw(6) << it->first << setw(30) << it->second.nombre << it->second.historial.size() << "\n";
    }
    pausar();
}

// ============================================================
//  MENU PRINCIPAL
// ============================================================

void menuPrincipal() {
    while (true) {
        limpiarPantalla();
        cout << "   SISTEMA DE GESTION DE BIBLIOTECA\n";
        cout << string(60, '=') << "\n";
        cout << "  1. Ver catalogo de libros\n";
        cout << "  2. Registrar nuevo usuario\n";
        cout << "  3. Registrar prestamo\n";
        cout << "  4. Solicitar recomendaciones\n";
        cout << "  5. Ver historial de usuario\n";
        cout << "  6. Ver lista de usuarios\n";
        cout << "  0. Salir\n";
        cout << string(60, '-') << "\n";
        cout << "Seleccione una opcion: ";

        int opcion;
        if (!(cin >> opcion)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }

        switch (opcion) {
            case 1: verCatalogo();             break;
            case 2: registrarUsuario();        break;
            case 3: registrarPrestamo();       break;
            case 4: solicitarRecomendaciones();break;
            case 5: verHistorialUsuario();     break;
            case 6: verUsuarios();             break;
            case 0:
                cout << "\nSaliendo del sistema. Hasta luego.\n";
                return;
            default:
                cout << "Opcion invalida.\n";
                break;
        }
    }
}

// ============================================================
//  MAIN
// ============================================================

int main() {
    cargarLibrosIniciales();
    menuPrincipal();
    return 0;
}