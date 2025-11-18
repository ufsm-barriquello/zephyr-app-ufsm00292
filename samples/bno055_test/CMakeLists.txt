# Versão mínima do CMake
cmake_minimum_required(VERSION 3.20.0)

# Procura o pacote do Zephyr
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Cria o projeto
project(bno055_test)

# Adiciona o código fonte (o seu main.c de teste)
target_sources(app PRIVATE src/main.c)
