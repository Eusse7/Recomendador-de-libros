# Recomendador-de-libros
Sistema de recomendacion de libros
# Recomendador de Libros basado en Grafos 

Este repositorio contiene el código fuente de un **Sistema de Gestión y Recomendación para Bibliotecas**, desarrollado en C++. El proyecto está enfocado en la aplicación práctica de estructuras de datos avanzadas, modelando el comportamiento de los lectores a través de la teoría de grafos.

El sistema representa el catálogo de libros como los nodos de un **grafo dirigido y ponderado** (implementado mediante listas de adyacencia). Las conexiones (aristas) entre los libros se crean dinámicamente basándose en los historiales de copréstamo de los usuarios, permitiendo rastrear las trayectorias secuenciales de lectura.

##  Características Principales

* **Motor de Recomendaciones Inteligente:** Utiliza una versión adaptada del algoritmo de **Búsqueda en Anchura (BFS)** para explorar la red de libros. Calcula un *score* dinámico combinando el peso de las aristas (frecuencia de lectura secuencial), la calificación promedio de la obra y la afinidad de género del lector.
* **Gestión de Catálogo y Usuarios:** Permite visualizar los libros ordenados por ID, registrar nuevos perfiles de usuario y gestionar préstamos.
* **Sistema de Valoración:** Actualización dinámica de la popularidad y el rating de los libros mediante un promedio acumulativo.
* **Interfaz de Línea de Comandos (CLI):** Menú interactivo y guiado para facilitar la interacción del usuario final.

##  Tecnologías y Estructuras de Datos
* **Lenguaje:** C++ (Standard Library)
* **Estructuras core:** `std::unordered_map` (Tablas Hash), `std::vector`, `std::queue`.
* **Algoritmia:** Búsqueda en grafos (BFS), ordenamiento (`std::sort`), lambdas.

##  Ejecución
El proyecto es un archivo nativo `.cpp` que no requiere dependencias externas. Puede compilarse usando GCC/MinGW con el siguiente comando:
`g++ sistema_biblioteca.cpp -o biblioteca`
